/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2020 Thomas S. Anderson blobfish.at.gmx.com
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

#include <boost/filesystem/path.hpp>

#include <osg/Switch>
#include <osg/MatrixTransform>

#include "globalutilities.h"
#include "annex/annsurfacemesh.h"
#include "library/lbrplabel.h"
#include "parameter/prmparameter.h"
#include "tools/occtools.h"
#include "modelviz/mdvsurfacemesh.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinputtype.h"
#include "project/serial/generated/prjsrlsrmssurfaceremesh.h"
#include "feature/ftrsurfaceremesh.h"

using namespace ftr;
using boost::uuids::uuid;

QIcon SurfaceReMesh::icon = QIcon(":/resources/images/constructionSurfaceReMesh.svg");

SurfaceReMesh::SurfaceReMesh():
Base()
, mesh(std::make_unique<ann::SurfaceMesh>())
, reMeshType(std::make_unique<prm::Parameter>(QObject::tr("ReMesh Type"), 0))
, minEdgeLength(std::make_unique<prm::Parameter>(QObject::tr("Minimum Edge Length"), 1.0))
, maxEdgeLength(std::make_unique<prm::Parameter>(QObject::tr("Maximum Edge Length"), 2.0))
, iterations(std::make_unique<prm::Parameter>(QObject::tr("Iterations"), 3))
, reMeshTypeLabel(new lbr::PLabel(reMeshType.get()))
, minEdgeLengthLabel(new lbr::PLabel(minEdgeLength.get()))
, maxEdgeLengthLabel(new lbr::PLabel(maxEdgeLength.get()))
, iterationsLabel(new lbr::PLabel(iterations.get()))
{
  name = QObject::tr("SurfaceReMesh");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  QStringList tStrings =
  {
    QObject::tr("CGAL")
    , QObject::tr("PMPLIB Uniform")
  };
  reMeshType->setEnumeration(tStrings);
  reMeshType->connectValue(std::bind(&SurfaceReMesh::setModelDirty, this));
  parameters.push_back(reMeshType.get());
  minEdgeLength->connectValue(std::bind(&SurfaceReMesh::setModelDirty, this));
  parameters.push_back(minEdgeLength.get());
  maxEdgeLength->connectValue(std::bind(&SurfaceReMesh::setModelDirty, this));
  parameters.push_back(maxEdgeLength.get());
  iterations->connectValue(std::bind(&SurfaceReMesh::setModelDirty, this));
  parameters.push_back(iterations.get());
  
  annexes.insert(std::make_pair(ann::Type::SurfaceMesh, mesh.get()));
  
  //we don't use lod stuff that is in base by default. so remove
  mainTransform->removeChildren(0, mainTransform->getNumChildren());
  
  overlaySwitch->addChild(reMeshTypeLabel.get());
  overlaySwitch->addChild(minEdgeLengthLabel.get());
  overlaySwitch->addChild(maxEdgeLengthLabel.get());
  overlaySwitch->addChild(iterationsLabel.get());
}

SurfaceReMesh::~SurfaceReMesh(){}

void SurfaceReMesh::updateModel(const UpdatePayload &pIn)
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
    
    auto getFeature = [&](const std::string& tagIn) -> const Base&
    {
      std::vector<const Base*> tfs = pIn.getFeatures(tagIn);
      if (tfs.size() != 1)
        throw std::runtime_error("wrong number of parents");
      return *(tfs.front());
    };
    
    auto getSurfaceMesh = [&](const Base &bfIn) -> const ann::SurfaceMesh&
    {
      if (!bfIn.hasAnnex(ann::Type::SurfaceMesh))
        throw std::runtime_error("parent doesn't have seer shape.");
      const ann::SurfaceMesh &tsm = bfIn.getAnnex<ann::SurfaceMesh>(ann::Type::SurfaceMesh);
      return tsm;
    };
    
    const Base &tbf = getFeature(ftr::InputType::target);
    const ann::SurfaceMesh &tsm = getSurfaceMesh(tbf);
    
    *mesh = tsm;
    
    if (reMeshType->getInt() == 0)
      mesh->remeshCGAL(minEdgeLength->getDouble(), iterations->getInt());
    if (reMeshType->getInt() == 1)
      mesh->remeshPMPUniform(minEdgeLength->getDouble(), iterations->getInt());
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in " << getTypeString() << " update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in " << getTypeString() << " update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in " << getTypeString() << " update." << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void SurfaceReMesh::updateVisual()
{
  mainTransform->removeChildren(0, mainTransform->getNumChildren());
  setVisualClean();
  if (!mesh)
    return;
  osg::Switch *viz = mdv::generate(*mesh);
  if (viz)
    mainTransform->addChild(viz);
  
  const osg::BoundingSphere &bs = mainTransform->getBound();
  if (!bs.valid())
  {
    lastUpdateLog += "Invalid bounding sphere. no label placement\n";
    return;
  }
  
  osg::Matrixd m = mainTransform->getMatrix();
  osg::Vec3d x = gu::getXVector(m);
  osg::Vec3d y = gu::getYVector(m);
  reMeshTypeLabel->setMatrix(m);
  minEdgeLengthLabel->setMatrix(osg::Matrixd::translate(m.getTrans() + x * -bs.radius()));
  maxEdgeLengthLabel->setMatrix(osg::Matrixd::translate(m.getTrans() + x * bs.radius()));
  iterationsLabel->setMatrix(osg::Matrixd::translate(m.getTrans() + y * bs.radius()));
}

void SurfaceReMesh::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::srms::SurfaceReMesh so
  (
    Base::serialOut()
    , mesh->serialOut()
    , reMeshType->serialOut()
    , minEdgeLength->serialOut()
    , maxEdgeLength->serialOut()
    , iterations->serialOut()
    , reMeshTypeLabel->serialOut()
    , minEdgeLengthLabel->serialOut()
    , maxEdgeLengthLabel->serialOut()
    , iterationsLabel->serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::srms::surfaceremesh(stream, so, infoMap);
}

void SurfaceReMesh::serialRead(const prj::srl::srms::SurfaceReMesh &so)
{
  Base::serialIn(so.base());
  mesh->serialIn(so.mesh());
  reMeshType->serialIn(so.reMeshType());
  minEdgeLength->serialIn(so.minEdgeLength());
  maxEdgeLength->serialIn(so.maxEdgeLength());
  iterations->serialIn(so.iterations());
  
  reMeshTypeLabel->serialIn(so.reMeshTypeLabel());
  minEdgeLengthLabel->serialIn(so.minEdgeLengthLabel());
  maxEdgeLengthLabel->serialIn(so.maxEdgeLengthLabel());
  iterationsLabel->serialIn(so.iterationsLabel());
}
