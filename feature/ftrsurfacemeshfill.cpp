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

#include "globalutilities.h"
#include "annex/annsurfacemesh.h"
#include "library/lbrplabel.h"
#include "parameter/prmparameter.h"
#include "tools/occtools.h"
#include "modelviz/mdvsurfacemesh.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinputtype.h"
#include "project/serial/generated/prjsrlsmfssurfacemeshfill.h"
#include "feature/ftrsurfacemeshfill.h"

using namespace ftr;
using boost::uuids::uuid;

QIcon SurfaceMeshFill::icon;

SurfaceMeshFill::SurfaceMeshFill():
Base()
, mesh(std::make_unique<ann::SurfaceMesh>())
, algorithm(std::make_unique<prm::Parameter>(QObject::tr("Algorithm"), 1))
, algorithmLabel(new lbr::PLabel(algorithm.get()))
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionSurfaceMeshFill.svg");
  
  name = QObject::tr("SurfaceMeshFill");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  annexes.insert(std::make_pair(ann::Type::SurfaceMesh, mesh.get()));
  
  QStringList tStrings =
  {
    QObject::tr("CGAL")
    , QObject::tr("PMPLIB")
  };
  algorithm->setEnumeration(tStrings);
  algorithm->connectValue(std::bind(&SurfaceMeshFill::setModelDirty, this));
  parameters.push_back(algorithm.get());
  
  overlaySwitch->addChild(algorithmLabel);
}

SurfaceMeshFill::~SurfaceMeshFill() = default;

void SurfaceMeshFill::updateModel(const UpdatePayload &pIn)
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
    
    if (algorithm->getInt() == 0)
      mesh->fillHolesCGAL();
    if (algorithm->getInt() == 1)
      mesh->fillHolesPMP();;

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

void SurfaceMeshFill::updateVisual()
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
  algorithmLabel->setMatrix(m);
//   osg::Vec3d x = gu::getXVector(m);
//   osg::Vec3d y = gu::getYVector(m);
//   minEdgeLengthLabel->setMatrix(osg::Matrixd::translate(m.getTrans() + x * -bs.radius()));
//   maxEdgeLengthLabel->setMatrix(osg::Matrixd::translate(m.getTrans() + x * bs.radius()));
//   iterationsLabel->setMatrix(osg::Matrixd::translate(m.getTrans() + y * bs.radius()));
}

void SurfaceMeshFill::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::smfs::SurfaceMeshFill so
  (
    Base::serialOut()
    , mesh->serialOut()
    , algorithm->serialOut()
    , algorithmLabel->serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::smfs::surfacemeshfill(stream, so, infoMap);
}

void SurfaceMeshFill::serialRead(const prj::srl::smfs::SurfaceMeshFill &so)
{
  Base::serialIn(so.base());
  mesh->serialIn(so.mesh());
  algorithm->serialIn(so.algorithm());
  algorithmLabel->serialIn(so.algorithmLabel());
}
