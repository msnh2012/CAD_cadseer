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

#include <tools/idtools.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <library/ipgroup.h>
#include <library/csysdragger.h>
#include <project/serial/xsdcxxoutput/featuresphere.h>
#include <annex/csysdragger.h>
#include <annex/seershape.h>
#include <feature/sphere.h>

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
radius(prm::Names::Radius, prf::manager().rootPtr->features().sphere().get().radius()),
csys(prm::Names::CSys, osg::Matrixd::identity()),
csysDragger(new ann::CSysDragger(this, &csys)),
sShape(new ann::SeerShape())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionSphere.svg");
  
  name = QObject::tr("Sphere");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  initializeMaps();
  
  radius.setConstraint(prm::Constraint::buildNonZeroPositive());
  
  parameterVector.push_back(&radius);
  parameterVector.push_back(&csys);
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
  annexes.insert(std::make_pair(ann::Type::CSysDragger, csysDragger.get()));
  overlaySwitch->addChild(csysDragger->dragger);
  
  radius.connectValue(boost::bind(&Sphere::setModelDirty, this));
  csys.connectValue(boost::bind(&Sphere::setModelDirty, this));
  
  setupIPGroup();
}

Sphere::~Sphere()
{

}

void Sphere::setRadius(const double& radiusIn)
{
  radius.setValue(radiusIn);
}

void Sphere::setCSys(const osg::Matrixd &csysIn)
{
  osg::Matrixd oldSystem = static_cast<osg::Matrixd>(csys);
  if (!csys.setValue(csysIn))
    return; // already at this csys
    
  //apply the same transformation to dragger, so dragger moves with it.
  osg::Matrixd diffMatrix = osg::Matrixd::inverse(oldSystem) * csysIn;
  csysDragger->draggerUpdate(csysDragger->dragger->getMatrix() * diffMatrix);
}

void Sphere::setupIPGroup()
{
  radiusIP = new lbr::IPGroup(&radius);
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
  radiusIP->setMatrix(static_cast<osg::Matrixd>(csys));
  radiusIP->valueHasChanged();
  radiusIP->constantHasChanged();
}

void Sphere::updateModel(const UpdatePayload&)
{
  setFailure();
  lastUpdateLog.clear();
  try
  {
    BRepPrimAPI_MakeSphere sphereMaker(gu::toOcc(static_cast<osg::Matrixd>(csys)), static_cast<double>(radius));
    sphereMaker.Build();
    assert(sphereMaker.IsDone());
    sShape->setOCCTShape(sphereMaker.Shape());
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
    
    ann::EvolveRecord evolveRecord;
    evolveRecord.outId = tempId;
    sShape->insertEvolve(evolveRecord);
  }
  
  //helper lamda
  auto insertIntoFeatureMap = [this](const uuid &idIn, FeatureTag featureTagIn)
  {
    ann::FeatureTagRecord record;
    record.id = idIn;
    record.tag = featureTagMap.at(featureTagIn);
    sShape->insertFeatureTag(record);
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
    sShape->updateShapeIdRecord(shapeIn, localId);
  };
  
  updateShapeByTag(sShape->getRootOCCTShape(), FeatureTag::Root);
  updateShapeByTag(sphereMaker.Shape(), FeatureTag::Solid);
  
  BRepPrim_Sphere &sphereSubMaker = sphereMaker.Sphere();
  
  updateShapeByTag(sphereSubMaker.Shell(), FeatureTag::Shell);
  updateShapeByTag(sphereSubMaker.LateralFace(), FeatureTag::Face);
  updateShapeByTag(sphereSubMaker.LateralWire(), FeatureTag::Wire);
  updateShapeByTag(sphereSubMaker.StartEdge(), FeatureTag::Edge);
  updateShapeByTag(sphereSubMaker.BottomStartVertex(), FeatureTag::VertexBottom);
  updateShapeByTag(sphereSubMaker.TopStartVertex(), FeatureTag::VertexTop);
  
  sShape->setRootShapeId(sShape->featureTagId(featureTagMap.at(FeatureTag::Root)));
}

void Sphere::serialWrite(const QDir &dIn)
{
  prj::srl::FeatureSphere sphereOut
  (
    Base::serialOut(),
    radius.serialOut(),
    csys.serialOut(),
    csysDragger->serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::sphere(stream, sphereOut, infoMap);
}

void Sphere::serialRead(const prj::srl::FeatureSphere& sSphereIn)
{
  Base::serialIn(sSphereIn.featureBase());
  radius.serialIn(sSphereIn.radius());
  csys.serialIn(sSphereIn.csys());
  csysDragger->serialIn(sSphereIn.csysDragger());
  
  updateIPGroup();
}
