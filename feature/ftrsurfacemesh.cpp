/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2019  Thomas S. Anderson blobfish.at.gmx.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <sstream>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include <BRepClass3d.hxx>
#include <TopoDS.hxx>

#include <osg/Switch>
#include <osg/MatrixTransform>

#include "globalutilities.h"
#include "tools/occtools.h"
#include "tools/idtools.h"
#include "tools/featuretools.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinputtype.h"
#include "annex/annseershape.h"
#include "annex/annsurfacemesh.h"
#include "annex/anncsysdragger.h"
#include "library/lbrcsysdragger.h"
#include "feature/ftrpick.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "mesh/mshocct.h"
#include "mesh/mshmesh.h"
#include "modelviz/mdvsurfacemesh.h"
#include "project/serial/generated/prjsrlsfmssurfacemesh.h"
#include "feature/ftrsurfacemesh.h"

using namespace ftr::SurfaceMesh;
using boost::uuids::uuid;
QIcon Feature::icon = QIcon(":/resources/images/constructionSurfaceMesh.svg");

struct Feature::Stow
{
  Feature &feature;
  
  prm::Parameter meshType{QObject::tr("Mesh Type"), 0, Tags::MeshType};
  prm::Parameter csys{prm::Names::CSys, osg::Matrixd::identity(), prm::Tags::CSys};
  prm::Parameter source{QObject::tr("Source"), ftr::Picks(), Tags::Source};
  prm::Observer csysObserver;
  
  ann::CSysDragger csysDragger;
  std::unique_ptr<ann::SurfaceMesh> mesh{std::make_unique<ann::SurfaceMesh>()};
  
  msh::prm::OCCT occtParameters;
  msh::prm::Netgen netgenParameters;
  msh::prm::GMSH gmshParameters;
  
  Stow() = delete;
  Stow(Feature &fIn)
  :feature(fIn)
  , csysObserver(std::bind(&Feature::setModelDirty, &feature))
  , csysDragger(&feature, &csys)
  {
    QStringList tStrings = //keep in sync with enum in header.
    {
      QObject::tr("Inert")
      , QObject::tr("Occt")
      , QObject::tr("Netgen")
      , QObject::tr("Gmsh")
    };
    meshType.setEnumeration(tStrings);
    meshType.connectValue(std::bind(&Feature::setModelDirty, &feature));
    meshType.connectValue(std::bind(&Stow::prmActiveSync, this));
    feature.parameters.push_back(&meshType);
    
    csys.connect(csysObserver);
    feature.parameters.push_back(&csys);
    
    source.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&source);
    
    feature.annexes.insert(std::make_pair(ann::Type::SurfaceMesh, mesh.get()));
    feature.annexes.insert(std::make_pair(ann::Type::CSysDragger, &csysDragger));
    
    feature.overlaySwitch->addChild(csysDragger.dragger);
      //we don't use lod stuff that is in base by default. so remove
    feature.mainTransform->removeChildren(0, feature.mainTransform->getNumChildren());
  }
  
  void prmActiveSync()
  {
    //shouldn't need to block anything
    switch (static_cast<MeshType>(meshType.getInt()))
    {
      case Inert:{
        csysDragger.draggerUpdate();
        csys.setActive(true);
        source.setValue(ftr::Picks());
        source.setActive(false);
        break;}
      case Occt:{
        csys.setActive(false);
        source.setActive(true);
        break;}
      case Netgen:{
        csys.setActive(false);
        source.setActive(true);
        break;}
      case Gmsh:{
        csys.setActive(false);
        source.setActive(true);
        break;}
    }
  }
};

Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  name = QObject::tr("SurfaceMesh");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
}

Feature::~Feature() = default;

/*! @brief assigns mesh annex to feature
 * 
 * @param mIn pointer to new mesh annex.
 * @details automatically sets mesh type to inert.
 * marks visual dirty for display update.
 * 
 */
void Feature::setMesh(std::unique_ptr<ann::SurfaceMesh> mIn, bool setToInert)
{
  if (setToInert)
    stow->meshType.setValue(static_cast<int>(Inert));
  annexes.erase(ann::Type::SurfaceMesh);
  stow->mesh = std::move(mIn);
  annexes.insert(std::make_pair(ann::Type::SurfaceMesh,stow->mesh.get()));
  setVisualDirty();
}

const msh::prm::OCCT& Feature::getOcctParameters() const
{
  return stow->occtParameters;
}

void Feature::setOcctParameters(const msh::prm::OCCT &prmsIn)
{
  stow->occtParameters = prmsIn;
  stow->meshType.setValue(static_cast<int>(Occt));
  setModelDirty(); //if we are already at occt type then above won't set dirty.
}

const msh::prm::Netgen& Feature::getNetgenParameters() const 
{
  return stow->netgenParameters;
}

void Feature::setNetgenParameters(const msh::prm::Netgen &prmsIn)
{
  stow->netgenParameters = prmsIn;
  stow->meshType.setValue(static_cast<int>(Netgen));
  setModelDirty();
}

void Feature::updateModel(const UpdatePayload &pIn)
{
  setFailure();
  lastUpdateLog.clear();
  try
  {
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    auto ct = static_cast<MeshType>(stow->meshType.getInt());
    if (ct == Inert)
    {
      //the csys parameter will have total transformation
      //we only want the increment that was applied
      osg::Matrixd increment = mainTransform->getMatrix();
      if (!increment.isIdentity())
      {
        stow->mesh->transform(increment);
        mainTransform->setMatrix(osg::Matrixd::identity());
      }
    }
    else
    {
      const auto picks = stow->source.getPicks();
      if (picks.empty())
        throw std::runtime_error("No input shape picks");
      tls::Resolver resolver(pIn);
      if (!resolver.resolve(picks.front()))
        throw std::runtime_error("Unable to resolve shape picks");
      auto shapes = resolver.getShapes();
      if (slc::isObjectType(picks.front().selectionType))
      {
        auto csysPrms = resolver.getFeature()->getParameters(prm::Tags::CSys);
        if (!csysPrms.empty())
        {
          prm::ObserverBlocker(stow->csysObserver);
          stow->csys.setValue(csysPrms.front()->getMatrix());
        }
        shapes = resolver.getSeerShape()->useGetNonCompoundChildren();
      }
      if (shapes.empty())
        throw std::runtime_error("No resolved shapes");
      
      TopoDS_Shape temp = shapes.front();
      if (temp.IsNull())
        throw std::runtime_error("occt shape is null");
      TopoDS_Shell stm;
      TopoDS_Face ftm;
      if (temp.ShapeType() == TopAbs_SOLID)
        stm = BRepClass3d::OuterShell(TopoDS::Solid(temp));
      else if (temp.ShapeType() == TopAbs_SHELL)
        stm = TopoDS::Shell(temp);
      else if (temp.ShapeType() == TopAbs_FACE)
        ftm = TopoDS::Face(temp);
      else
        throw std::runtime_error("unsupported shape type");
      
      if (ct == Occt)
      {
        if (!stm.IsNull())
          setMesh(std::make_unique<ann::SurfaceMesh>(msh::srf::generate(stm, stow->occtParameters)), false);
        else
          setMesh(std::make_unique<ann::SurfaceMesh>(msh::srf::generate(ftm, stow->occtParameters)), false);
      }
      else if (ct == Netgen)
      {
#ifndef NETGEN_PRESENT
        throw std::runtime_error("Netgen not available.");
#endif
        stow->netgenParameters.filePath = boost::filesystem::temp_directory_path();
        stow->netgenParameters.filePath /= boost::filesystem::path(gu::idToString(getId()) + ".brep");
        if (!stm.IsNull())
          setMesh(std::make_unique<ann::SurfaceMesh>(msh::srf::generate(stm, stow->netgenParameters)), false);
        else
          setMesh(std::make_unique<ann::SurfaceMesh>(msh::srf::generate(ftm, stow->netgenParameters)), false);
      }
      else if (ct == Gmsh)
      {
#ifndef GMSH_PRESENT
        throw std::runtime_error("Gmsh not available.");
#endif
        stow->gmshParameters.filePath = boost::filesystem::temp_directory_path();
        stow->gmshParameters.filePath /= boost::filesystem::path(gu::idToString(getId()) + ".brep");
        if (!stm.IsNull())
          setMesh(std::make_unique<ann::SurfaceMesh>(msh::srf::generate(stm, stow->gmshParameters)), false);
        else
          setMesh(std::make_unique<ann::SurfaceMesh>(msh::srf::generate(ftm, stow->gmshParameters)), false);
      }
    }
    
    setSuccess();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in surface mesh update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in surface mesh update. " << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Feature::updateVisual()
{
  mainTransform->removeChildren(0, mainTransform->getNumChildren());
  setVisualClean();
  if (!stow->mesh)
    return;
  osg::Switch *viz = mdv::generate(*stow->mesh);
  if (viz)
    mainTransform->addChild(viz);
}

void Feature::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::sfms::SurfaceMesh so
  (
    Base::serialOut()
    , stow->meshType.serialOut()
    , stow->csys.serialOut()
    , stow->source.serialOut()
    , stow->csysDragger.serialOut()
    , stow->mesh->serialOut()
  );
  
  auto ct = static_cast<MeshType>(stow->meshType.getInt());
  switch (ct)
  {
    case Inert: {break;} //no settings
    case Occt: {so.parametersOCCT() = stow->occtParameters.serialOut(); break;}
    case Netgen: {so.parametersNetgen() = stow->netgenParameters.serialOut(); break;}
    case Gmsh: {so.parametersGMSH() = stow->gmshParameters.serialOut(); break;}
  }
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::sfms::surfaceMesh(stream, so, infoMap);
}

void Feature::serialRead(const prj::srl::sfms::SurfaceMesh &smIn)
{
  Base::serialIn(smIn.base());
  stow->meshType.serialIn(smIn.meshType());
  stow->csys.serialIn(smIn.csys());
  stow->source.serialIn(smIn.source());
  stow->csysDragger.serialIn(smIn.csysDragger());
  stow->mesh->serialIn(smIn.surface());
  
  if (smIn.parametersOCCT())
    stow->occtParameters.serialIn(smIn.parametersOCCT().get());
  if (smIn.parametersNetgen())
    stow->netgenParameters.serialIn(smIn.parametersNetgen().get());
  if (smIn.parametersGMSH())
    stow->gmshParameters.serialIn(smIn.parametersGMSH().get());
}
