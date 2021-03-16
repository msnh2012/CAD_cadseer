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
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinputtype.h"
#include "annex/annseershape.h"
#include "annex/annsurfacemesh.h"
#include "annex/anncsysdragger.h"
#include "library/lbrcsysdragger.h"
#include "parameter/prmparameter.h"
#include "mesh/mshocct.h"
#include "mesh/mshmesh.h"
#include "modelviz/mdvsurfacemesh.h"
#include "project/serial/generated/prjsrlsfmssurfacemesh.h"
#include "feature/ftrsurfacemesh.h"

using namespace ftr;
using boost::uuids::uuid;

QIcon SurfaceMesh::icon;

SurfaceMesh::SurfaceMesh():
Base()
, csys(new prm::Parameter(prm::Names::CSys, osg::Matrixd::identity(), prm::Tags::CSys))
, csysDragger(new ann::CSysDragger(this, csys.get()))
, mesh(new ann::SurfaceMesh())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionSurfaceMesh.svg");
  
  name = QObject::tr("SurfaceMesh");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  parameters.push_back(csys.get());
  
  annexes.insert(std::make_pair(ann::Type::SurfaceMesh, mesh.get()));
  annexes.insert(std::make_pair(ann::Type::CSysDragger, csysDragger.get()));
  //update will decide if dragger is added to overlay
  
  //we don't use lod stuff that is in base by default. so remove
  mainTransform->removeChildren(0, mainTransform->getNumChildren());
  
  csys->connectValue(std::bind(&SurfaceMesh::setModelDirty, this));
  csys->connectActive(std::bind(&SurfaceMesh::csysActive, this));
}

SurfaceMesh::~SurfaceMesh(){}

void SurfaceMesh::setMeshType(MeshType t)
{
  meshType = t;
  if (meshType == MeshType::inert)
    csys->setActive(true);
  else
    csys->setActive(false);
  setModelDirty();
}

/*! @brief assigns mesh annex to feature
 * 
 * @param mIn pointer to new mesh annex.
 * @details automatically sets mesh type to inert.
 * marks visual dirty for display update.
 * 
 */
void SurfaceMesh::setMesh(std::unique_ptr<ann::SurfaceMesh> mIn, bool setToInert)
{
  if (setToInert)
    setMeshType(MeshType::inert);
  annexes.erase(ann::Type::SurfaceMesh);
  mesh = std::move(mIn);
  annexes.insert(std::make_pair(ann::Type::SurfaceMesh, mesh.get()));
  setVisualDirty();
}

void SurfaceMesh::setOcctParameters(const msh::prm::OCCT &prmsIn)
{
  occtParameters = prmsIn;
  setMeshType(MeshType::occt);
  setModelDirty();
}

void SurfaceMesh::setNetgenParameters(const msh::prm::Netgen &prmsIn)
{
  netgenParameters = prmsIn;
  setMeshType(MeshType::netgen);
  setModelDirty();
}

void SurfaceMesh::updateModel(const UpdatePayload &pIn)
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
    if (meshType == MeshType::inert)
    {
      //the csys parameter will have total transformation
      //we only want the increment that was applied
      osg::Matrixd increment = mainTransform->getMatrix();
      if (!increment.isIdentity())
      {
        mesh->transform(increment);
        mainTransform->setMatrix(osg::Matrixd::identity());
      }
    }
    else
    {
      std::vector<const Base*> tfs = pIn.getFeatures(InputType::target);
      if (tfs.size() != 1)
        throw std::runtime_error("wrong number of parents");
      if (!tfs.front()->hasAnnex(ann::Type::SeerShape))
        throw std::runtime_error("parent doesn't have seer shape.");
      const ann::SeerShape &tss = tfs.front()->getAnnex<ann::SeerShape>();
      if (tss.isNull())
        throw std::runtime_error("target seer shape is null");
      TopoDS_Shape temp = occt::getFirstNonCompound(tss.getRootOCCTShape());
      if (temp.IsNull())
        throw std::runtime_error("root occt shape of seer shape is null");
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
      
      if (meshType == MeshType::occt)
      {
        if (!stm.IsNull())
          setMesh(std::make_unique<ann::SurfaceMesh>(msh::srf::generate(stm, occtParameters)), false);
        else
          setMesh(std::make_unique<ann::SurfaceMesh>(msh::srf::generate(ftm, occtParameters)), false);
      }
      else if (meshType == MeshType::netgen)
      {
#ifndef NETGEN_PRESENT
        throw std::runtime_error("Netgen not available.");
#endif
        netgenParameters.filePath = boost::filesystem::temp_directory_path();
        netgenParameters.filePath /= boost::filesystem::path(gu::idToString(getId()) + ".brep");
        if (!stm.IsNull())
          setMesh(std::make_unique<ann::SurfaceMesh>(msh::srf::generate(stm, netgenParameters)), false);
        else
          setMesh(std::make_unique<ann::SurfaceMesh>(msh::srf::generate(ftm, netgenParameters)), false);
      }
      else if (meshType == MeshType::gmsh)
      {
#ifndef GMSH_PRESENT
        throw std::runtime_error("Gmsh not available.");
#endif
        gmshParameters.filePath = boost::filesystem::temp_directory_path();
        gmshParameters.filePath /= boost::filesystem::path(gu::idToString(getId()) + ".brep");
        if (!stm.IsNull())
          setMesh(std::make_unique<ann::SurfaceMesh>(msh::srf::generate(stm, gmshParameters)), false);
        else
          setMesh(std::make_unique<ann::SurfaceMesh>(msh::srf::generate(ftm, gmshParameters)), false);
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

void SurfaceMesh::updateVisual()
{
  mainTransform->removeChildren(0, mainTransform->getNumChildren());
  setVisualClean();
  if (!mesh)
    return;
  osg::Switch *viz = mdv::generate(*mesh);
  if (viz)
    mainTransform->addChild(viz);
}

void SurfaceMesh::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::sfms::SurfaceMesh so
  (
    Base::serialOut()
    , mesh->serialOut()
    , csys->serialOut()
    , csysDragger->serialOut()
    , occtParameters.serialOut()
    , netgenParameters.serialOut()
    , gmshParameters.serialOut()
    , static_cast<int>(meshType)
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::sfms::surfaceMesh(stream, so, infoMap);
}

void SurfaceMesh::serialRead(const prj::srl::sfms::SurfaceMesh &smIn)
{
  Base::serialIn(smIn.base());
  mesh->serialIn(smIn.surface());
  csys->serialIn(smIn.csys());
  csysDragger->serialIn(smIn.csysDragger());
  occtParameters.serialIn(smIn.parametersOCCT());
  netgenParameters.serialIn(smIn.parametersNetgen());
  gmshParameters.serialIn(smIn.parametersGMSH());
  meshType = static_cast<MeshType>(smIn.meshType());
  
  csysActive();
}

void SurfaceMesh::csysActive()
{
  if (csys->isActive())
    overlaySwitch->addChild(csysDragger->dragger);
  else
    overlaySwitch->removeChild(csysDragger->dragger);
}
