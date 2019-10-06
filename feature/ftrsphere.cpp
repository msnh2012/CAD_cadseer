/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2015  Thomas S. Anderson blobfish.at.gmx.com
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

#include <BRepPrimAPI_MakeSphere.hxx>

#include "globalutilities.h"
#include "tools/idtools.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "library/lbripgroup.h"
#include "library/lbrcsysdragger.h"
#include "project/serial/xsdcxxoutput/featuresphere.h"
#include "annex/anncsysdragger.h"
#include "annex/annseershape.h"
#include "parameter/prmparameter.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrsphere.h"

using namespace ftr;
using namespace boost::uuids;

enum class FeatureTag
{
  Root,         //!< compound
  Solid,        //!< solid
  Shell,        //!< shell
  Face,
  Wire,
  Edge,
  VertexBottom,
  VertexTop
};

static const std::map<FeatureTag, std::string> featureTagMap = 
{
  {FeatureTag::Root, "Root"},
  {FeatureTag::Solid, "Solid"},
  {FeatureTag::Shell, "Shell"},
  {FeatureTag::Face, "Face"},
  {FeatureTag::Wire, "Wire"},
  {FeatureTag::Edge, "Edge"},
  {FeatureTag::VertexBottom, "VertexBottom"},
  {FeatureTag::VertexTop, "VertexTop"}
};

QIcon Sphere::icon;

Sphere::Sphere() :
Base(),
radius(new prm::Parameter(prm::Names::Radius, prf::manager().rootPtr->features().sphere().get().radius())),
csys(new prm::Parameter(prm::Names::CSys, osg::Matrixd::identity())),
csysDragger(new ann::CSysDragger(this, csys.get())),
sShape(new ann::SeerShape())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionSphere.svg");
  
  name = QObject::tr("Sphere");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  initializeMaps();
  
  radius->setConstraint(prm::Constraint::buildNonZeroPositive());
  
  parameters.push_back(radius.get());
  parameters.push_back(csys.get());
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
  annexes.insert(std::make_pair(ann::Type::CSysDragger, csysDragger.get()));
  overlaySwitch->addChild(csysDragger->dragger);
  
  radius->connectValue(std::bind(&Sphere::setModelDirty, this));
  csys->connectValue(std::bind(&Sphere::setModelDirty, this));
  
  setupIPGroup();
}

Sphere::~Sphere()
{

}

void Sphere::setRadius(double radiusIn)
{
  radius->setValue(radiusIn);
}

void Sphere::setCSys(const osg::Matrixd &csysIn)
{
  osg::Matrixd oldSystem = static_cast<osg::Matrixd>(*csys);
  if (!csys->setValue(csysIn))
    return; // already at this csys
    
  //apply the same transformation to dragger, so dragger moves with it.
  osg::Matrixd diffMatrix = osg::Matrixd::inverse(oldSystem) * csysIn;
  csysDragger->draggerUpdate(csysDragger->dragger->getMatrix() * diffMatrix);
}

const prm::Parameter& Sphere::getRadius() const
{
  return *radius;
}

osg::Matrixd Sphere::getCSys() const
{
  return static_cast<osg::Matrixd>(*csys);
}

void Sphere::setupIPGroup()
{
  radiusIP = new lbr::IPGroup(radius.get());
  radiusIP->setMatrixDims(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(0.0, 1.0, 0.0)));
  radiusIP->setMatrixDragger(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(-1.0, 0.0, 0.0)));
  radiusIP->setDimsFlipped(true);
  radiusIP->setRotationAxis(osg::Vec3d(0.0, 0.0, 1.0), osg::Vec3d(-1.0, 0.0, 0.0));
  overlaySwitch->addChild(radiusIP.get());
  csysDragger->dragger->linkToMatrix(radiusIP.get());
  
  updateIPGroup();
}

void Sphere::updateIPGroup()
{
  radiusIP->setMatrix(static_cast<osg::Matrixd>(*csys));
  radiusIP->valueHasChanged();
  radiusIP->constantHasChanged();
}

void Sphere::updateModel(const UpdatePayload&)
{
  setFailure();
  lastUpdateLog.clear();
  sShape->reset();
  try
  {
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    BRepPrimAPI_MakeSphere sphereMaker(gu::toOcc(static_cast<osg::Matrixd>(*csys)), static_cast<double>(*radius));
    sphereMaker.Build();
    assert(sphereMaker.IsDone());
    sShape->setOCCTShape(sphereMaker.Shape(), getId());
    updateResult(sphereMaker);
    mainTransform->setMatrix(osg::Matrixd::identity());
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in sphere update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in sphere update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in sphere update." << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  updateIPGroup();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Sphere::initializeMaps()
{
  //result 
  std::vector<uuid> tempIds; //save ids for later.
  for (unsigned int index = 0; index < 8; ++index)
  {
    uuid tempId = gu::createRandomId();
    tempIds.push_back(tempId);
    sShape->insertEvolve(gu::createNilId(), tempId);
  }
  
  //helper lamda
  auto insertIntoFeatureMap = [this](const uuid &idIn, FeatureTag featureTagIn)
  {
    sShape->insertFeatureTag(idIn, featureTagMap.at(featureTagIn));
  };
  
  insertIntoFeatureMap(tempIds.at(0), FeatureTag::Root);
  insertIntoFeatureMap(tempIds.at(1), FeatureTag::Solid);
  insertIntoFeatureMap(tempIds.at(2), FeatureTag::Shell);
  insertIntoFeatureMap(tempIds.at(3), FeatureTag::Face);
  insertIntoFeatureMap(tempIds.at(4), FeatureTag::Wire);
  insertIntoFeatureMap(tempIds.at(5), FeatureTag::Edge);
  insertIntoFeatureMap(tempIds.at(6), FeatureTag::VertexBottom);
  insertIntoFeatureMap(tempIds.at(7), FeatureTag::VertexTop);
  
//   std::cout << std::endl << std::endl <<
//     "result Container: " << std::endl << resultContainer << std::endl << std::endl <<
//     "feature Container:" << std::endl << featureContainer << std::endl << std::endl <<
//     "evolution Container:" << std::endl << evolutionContainer << std::endl << std::endl;
}

void Sphere::updateResult(BRepPrimAPI_MakeSphere &sphereMaker)
{
  //helper lamda
  auto updateShapeByTag = [this](const TopoDS_Shape &shapeIn, FeatureTag featureTagIn)
  {
    uuid localId = sShape->featureTagId(featureTagMap.at(featureTagIn));
    sShape->updateId(shapeIn, localId);
  };
  
//   updateShapeByTag(sShape->getRootOCCTShape(), FeatureTag::Root);
  updateShapeByTag(sphereMaker.Shape(), FeatureTag::Solid);
  
  BRepPrim_Sphere &sphereSubMaker = sphereMaker.Sphere();
  
  updateShapeByTag(sphereSubMaker.Shell(), FeatureTag::Shell);
  updateShapeByTag(sphereSubMaker.LateralFace(), FeatureTag::Face);
  updateShapeByTag(sphereSubMaker.LateralWire(), FeatureTag::Wire);
  updateShapeByTag(sphereSubMaker.StartEdge(), FeatureTag::Edge);
  updateShapeByTag(sphereSubMaker.BottomStartVertex(), FeatureTag::VertexBottom);
  updateShapeByTag(sphereSubMaker.TopStartVertex(), FeatureTag::VertexTop);
  
//   sShape->setRootShapeId(sShape->featureTagId(featureTagMap.at(FeatureTag::Root)));
}

void Sphere::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::FeatureSphere sphereOut
  (
    Base::serialOut(),
    radius->serialOut(),
    csys->serialOut(),
    csysDragger->serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::sphere(stream, sphereOut, infoMap);
}

void Sphere::serialRead(const prj::srl::FeatureSphere& sSphereIn)
{
  Base::serialIn(sSphereIn.featureBase());
  radius->serialIn(sSphereIn.radius());
  csys->serialIn(sSphereIn.csys());
  csysDragger->serialIn(sSphereIn.csysDragger());
  
  updateIPGroup();
}
