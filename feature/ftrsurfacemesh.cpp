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
#include "modelviz/mdvsurfacemesh.h"
#include "project/serial/xsdcxxoutput/featuresurfacemesh.h"
#include "feature/ftrsurfacemesh.h"


using namespace ftr;
using boost::uuids::uuid;

QIcon SurfaceMesh::icon;

SurfaceMesh::SurfaceMesh():
Base()
, mesh(new ann::SurfaceMesh())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionSurfaceMesh.svg");
  
  name = QObject::tr("SurfaceMesh");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  annexes.insert(std::make_pair(ann::Type::SurfaceMesh, mesh.get()));
  
  //we don't use lod stuff that is in base by default. so remove
  mainTransform->removeChildren(0, mainTransform->getNumChildren());
}

SurfaceMesh::~SurfaceMesh(){}

void SurfaceMesh::setMeshType(MeshType t)
{
  meshType = t;
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
    meshType = MeshType::inert;
  annexes.erase(ann::Type::SurfaceMesh);
  mesh = std::move(mIn);
  annexes.insert(std::make_pair(ann::Type::SurfaceMesh, mesh.get()));
  setVisualDirty();
}

void SurfaceMesh::setOcctParameters(const msh::prm::OCCT &prmsIn)
{
  occtParameters = prmsIn;
  meshType = MeshType::occt;
  setModelDirty();
}

void SurfaceMesh::setNetgenParameters(const msh::prm::Netgen &prmsIn)
{
  netgenParameters = prmsIn;
  meshType = MeshType::netgen;
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
    if (meshType != MeshType::inert)
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
          setMesh(ann::SurfaceMesh::generate(stm, occtParameters), false);
        else
          setMesh(ann::SurfaceMesh::generate(ftm, occtParameters), false);
      }
      else if (meshType == MeshType::netgen)
      {
#ifndef NETGEN_PRESENT
        throw std::runtime_error("Netgen not available.");
#endif
        netgenParameters.filePath = boost::filesystem::temp_directory_path();
        netgenParameters.filePath /= boost::filesystem::path(gu::idToString(getId()) + ".brep");
        if (!stm.IsNull())
          setMesh(ann::SurfaceMesh::generate(stm, netgenParameters), false);
        else
          setMesh(ann::SurfaceMesh::generate(ftm, netgenParameters), false);
      }
      else if (meshType == MeshType::gmsh)
      {
#ifndef GMSH_PRESENT
        throw std::runtime_error("Gmsh not available.");
#endif
        gmshParameters.filePath = boost::filesystem::temp_directory_path();
        gmshParameters.filePath /= boost::filesystem::path(gu::idToString(getId()) + ".brep");
        if (!stm.IsNull())
          setMesh(ann::SurfaceMesh::generate(stm, gmshParameters), false);
        else
          setMesh(ann::SurfaceMesh::generate(ftm, gmshParameters), false);
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
  prj::srl::FeatureSurfaceMesh so
  (
    Base::serialOut()
    , mesh->serialOut()
    , occtParameters.serialOut()
    , netgenParameters.serialOut()
    , gmshParameters.serialOut()
    , static_cast<int>(meshType)
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::surfaceMesh(stream, so, infoMap);
}

void SurfaceMesh::serialRead(const prj::srl::FeatureSurfaceMesh &smIn)
{
  Base::serialIn(smIn.featureBase());
  mesh->serialIn(smIn.surface());
  occtParameters.serialIn(smIn.parametersOCCT());
  netgenParameters.serialIn(smIn.parametersNetgen());
  gmshParameters.serialIn(smIn.parametersGMSH());
  meshType = static_cast<MeshType>(smIn.meshType());
}
