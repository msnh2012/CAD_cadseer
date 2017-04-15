/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#include <globalutilities.h>
#include <tools/idtools.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <feature/seershape.h>
#include <library/ipgroup.h>
#include <project/serial/xsdcxxoutput/featureoblong.h>
#include <feature/oblongbuilder.h>
#include <feature/oblong.h>

using namespace ftr;
using boost::uuids::uuid;

QIcon Oblong::icon;

//duplicated from box.
enum class FeatureTag
{
  Root,         //!< compound
  Solid,        //!< solid
  Shell,        //!< shell
  FaceXP,       //!< x positive face
  FaceXN,       //!< x negative face
  FaceYP,       //!< y positive face
  FaceYN,       //!< y negative face
  FaceZP,       //!< z positive face
  FaceZN,       //!< z negative face
  WireXP,       //!< x positive wire
  WireXN,       //!< x negative wire
  WireYP,       //!< y positive wire
  WireYN,       //!< y negative wire
  WireZP,       //!< z positive wire
  WireZN,       //!< z negative wire
  EdgeXPYP,     //!< edge at intersection of x positive face and y positive face.
  EdgeXPZP,     //!< edge at intersection of x positive face and z positive face.
  EdgeXPYN,     //!< edge at intersection of x positive face and y negative face.
  EdgeXPZN,     //!< edge at intersection of x positive face and z negative face.
  EdgeXNYN,     //!< edge at intersection of x negative face and y negative face.
  EdgeXNZP,     //!< edge at intersection of x negative face and z positive face.
  EdgeXNYP,     //!< edge at intersection of x negative face and y positive face.
  EdgeXNZN,     //!< edge at intersection of x negative face and z negative face.
  EdgeYPZP,     //!< edge at intersection of y positive face and z positive face.
  EdgeYPZN,     //!< edge at intersection of y positive face and z negative face.
  EdgeYNZP,     //!< edge at intersection of y negative face and z positive face.
  EdgeYNZN,     //!< edge at intersection of y negative face and z negative face.
  VertexXPYPZP, //!< vertex at intersection of faces x+, y+, z+
  VertexXPYNZP, //!< vertex at intersection of faces x+, y-, z+
  VertexXPYNZN, //!< vertex at intersection of faces x+, y-, z-
  VertexXPYPZN, //!< vertex at intersection of faces x+, y+, z-
  VertexXNYNZP, //!< vertex at intersection of faces x-, y-, z+
  VertexXNYPZP, //!< vertex at intersection of faces x-, y+, z+
  VertexXNYPZN, //!< vertex at intersection of faces x-, y+, z-
  VertexXNYNZN  //!< vertex at intersection of faces x-, y-, z-
};

static const std::map<FeatureTag, std::string> featureTagMap = 
{
  {FeatureTag::Root, "Root"},
  {FeatureTag::Solid, "Solid"},
  {FeatureTag::Shell, "Shell"},
  {FeatureTag::FaceXP, "FaceXP"},
  {FeatureTag::FaceXN, "FaceXN"},
  {FeatureTag::FaceYP, "FaceYP"},
  {FeatureTag::FaceYN, "FaceYN"},
  {FeatureTag::FaceZP, "FaceZP"},
  {FeatureTag::FaceZN, "FaceZN"},
  {FeatureTag::WireXP, "WireXP"},
  {FeatureTag::WireXN, "WireXN"},
  {FeatureTag::WireYP, "WireYP"},
  {FeatureTag::WireYN, "WireYN"},
  {FeatureTag::WireZP, "WireZP"},
  {FeatureTag::WireZN, "WireZN"},
  {FeatureTag::EdgeXPYP, "EdgeXPYP"},
  {FeatureTag::EdgeXPZP, "EdgeXPZP"},
  {FeatureTag::EdgeXPYN, "EdgeXPYN"},
  {FeatureTag::EdgeXPZN, "EdgeXPZN"},
  {FeatureTag::EdgeXNYN, "EdgeXNYN"},
  {FeatureTag::EdgeXNZP, "EdgeXNZP"},
  {FeatureTag::EdgeXNYP, "EdgeXNYP"},
  {FeatureTag::EdgeXNZN, "EdgeXNZN"},
  {FeatureTag::EdgeYPZP, "EdgeYPZP"},
  {FeatureTag::EdgeYPZN, "EdgeYPZN"},
  {FeatureTag::EdgeYNZP, "EdgeYNZP"},
  {FeatureTag::EdgeYNZN, "EdgeYNZN"},
  {FeatureTag::VertexXPYPZP, "VertexXPYPZP"},
  {FeatureTag::VertexXPYNZP, "VertexXPYNZP"},
  {FeatureTag::VertexXPYNZN, "VertexXPYNZN"},
  {FeatureTag::VertexXPYPZN, "VertexXPYPZN"},
  {FeatureTag::VertexXNYNZP, "VertexXNYNZP"},
  {FeatureTag::VertexXNYPZP, "VertexXNYPZP"},
  {FeatureTag::VertexXNYPZN, "VertexXNYPZN"},
  {FeatureTag::VertexXNYNZN, "VertexXNYNZN"}
};

Oblong::Oblong() :
  CSysBase(),
  length(ParameterNames::Length, prf::manager().rootPtr->features().oblong().get().length()),
  width(ParameterNames::Width, prf::manager().rootPtr->features().oblong().get().width()),
  height(ParameterNames::Height, prf::manager().rootPtr->features().oblong().get().height())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionOblong.svg");
  
  name = QObject::tr("Oblong");
  mainSwitch->setUserValue(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  initializeMaps();
  
  length.setConstraint(ParameterConstraint::buildNonZeroPositive());
  width.setConstraint(ParameterConstraint::buildNonZeroPositive());
  height.setConstraint(ParameterConstraint::buildNonZeroPositive());
  
  parameterVector.push_back(&length);
  parameterVector.push_back(&width);
  parameterVector.push_back(&height);
  
  length.connectValue(boost::bind(&Oblong::setModelDirty, this));
  width.connectValue(boost::bind(&Oblong::setModelDirty, this));
  height.connectValue(boost::bind(&Oblong::setModelDirty, this));
  
  setupIPGroup();
}

Oblong::~Oblong(){}

//duplicate of box.
void Oblong::initializeMaps()
{
  //result 
  std::vector<uuid> tempIds; //save ids for later.
  for (unsigned int index = 0; index < 35; ++index)
  {
    uuid tempId = gu::createRandomId();
    tempIds.push_back(tempId);
    
    EvolveRecord evolveRecord;
    evolveRecord.outId = tempId;
    seerShape->insertEvolve(evolveRecord);
  }
  
  //helper lamda
  auto insertIntoFeatureMap = [this](const uuid &idIn, FeatureTag featureTagIn)
  {
    FeatureTagRecord record;
    record.id = idIn;
    record.tag = featureTagMap.at(featureTagIn);
    seerShape->insertFeatureTag(record);
  };
  
  insertIntoFeatureMap(tempIds.at(0), FeatureTag::Root);
  insertIntoFeatureMap(tempIds.at(1), FeatureTag::Solid);
  insertIntoFeatureMap(tempIds.at(2), FeatureTag::Shell);
  insertIntoFeatureMap(tempIds.at(3), FeatureTag::FaceXP);
  insertIntoFeatureMap(tempIds.at(4), FeatureTag::FaceXN);
  insertIntoFeatureMap(tempIds.at(5), FeatureTag::FaceYP);
  insertIntoFeatureMap(tempIds.at(6), FeatureTag::FaceYN);
  insertIntoFeatureMap(tempIds.at(7), FeatureTag::FaceZP);
  insertIntoFeatureMap(tempIds.at(8), FeatureTag::FaceZN);
  insertIntoFeatureMap(tempIds.at(9), FeatureTag::WireXP);
  insertIntoFeatureMap(tempIds.at(10), FeatureTag::WireXN);
  insertIntoFeatureMap(tempIds.at(11), FeatureTag::WireYP);
  insertIntoFeatureMap(tempIds.at(12), FeatureTag::WireYN);
  insertIntoFeatureMap(tempIds.at(13), FeatureTag::WireZP);
  insertIntoFeatureMap(tempIds.at(14), FeatureTag::WireZN);
  insertIntoFeatureMap(tempIds.at(15), FeatureTag::EdgeXPYP);
  insertIntoFeatureMap(tempIds.at(16), FeatureTag::EdgeXPZP);
  insertIntoFeatureMap(tempIds.at(17), FeatureTag::EdgeXPYN);
  insertIntoFeatureMap(tempIds.at(18), FeatureTag::EdgeXPZN);
  insertIntoFeatureMap(tempIds.at(19), FeatureTag::EdgeXNYN);
  insertIntoFeatureMap(tempIds.at(20), FeatureTag::EdgeXNZP);
  insertIntoFeatureMap(tempIds.at(21), FeatureTag::EdgeXNYP);
  insertIntoFeatureMap(tempIds.at(22), FeatureTag::EdgeXNZN);
  insertIntoFeatureMap(tempIds.at(23), FeatureTag::EdgeYPZP);
  insertIntoFeatureMap(tempIds.at(24), FeatureTag::EdgeYPZN);
  insertIntoFeatureMap(tempIds.at(25), FeatureTag::EdgeYNZP);
  insertIntoFeatureMap(tempIds.at(26), FeatureTag::EdgeYNZN);
  insertIntoFeatureMap(tempIds.at(27), FeatureTag::VertexXPYPZP);
  insertIntoFeatureMap(tempIds.at(28), FeatureTag::VertexXPYNZP);
  insertIntoFeatureMap(tempIds.at(29), FeatureTag::VertexXPYNZN);
  insertIntoFeatureMap(tempIds.at(30), FeatureTag::VertexXPYPZN);
  insertIntoFeatureMap(tempIds.at(31), FeatureTag::VertexXNYNZP);
  insertIntoFeatureMap(tempIds.at(32), FeatureTag::VertexXNYPZP);
  insertIntoFeatureMap(tempIds.at(33), FeatureTag::VertexXNYPZN);
  insertIntoFeatureMap(tempIds.at(34), FeatureTag::VertexXNYNZN);
}

void Oblong::setLength(const double &lengthIn)
{
  if (lengthIn == length)
    return;
  assert(lengthIn > Precision::Confusion());
  setModelDirty();
  length = lengthIn;
}

void Oblong::setWidth(const double &widthIn)
{
  if (widthIn == width)
    return;
  assert(widthIn > Precision::Confusion());
  setModelDirty();
  width = widthIn;
}

void Oblong::setHeight(const double &heightIn)
{
  if (heightIn == height)
    return;
  assert(heightIn > Precision::Confusion());
  setModelDirty();
  height = heightIn;
}

void Oblong::setParameters(const double &lengthIn, const double &widthIn, const double &heightIn)
{
  //dirty is called in setters.
  //asserts called in setters.
  setLength(lengthIn);
  setWidth(widthIn);
  setHeight(heightIn);
}

void Oblong::updateModel(const UpdateMap& mapIn)
{
  setFailure();
  
  CSysBase::updateModel(mapIn);
  
  try
  {
    if (!(length.getValue() > width.getValue()))
      throw std::runtime_error("length must be greater than width");
    
    OblongBuilder oblongMaker(length, width, height, system);
    seerShape->setOCCTShape(oblongMaker.getSolid());
    updateResult(oblongMaker);
    
    setSuccess();
  }
  catch (Standard_Failure)
  {
    Handle_Standard_Failure e = Standard_Failure::Caught();
    std::cout << std::endl << "Error in oblong update. " << e->GetMessageString() << std::endl;
  }
  catch (std::exception &e)
  {
    std::cout << std::endl << "Error in oblong update: " << e.what() << std::endl;
  }
  setModelClean();
  
  updateIPGroup();
}

void Oblong::updateResult(const OblongBuilder& oblongMakerIn)
{
  //helper lamda
  auto updateShapeByTag = [this](const TopoDS_Shape &shapeIn, FeatureTag featureTagIn)
  {
    uuid localId = seerShape->featureTagId(featureTagMap.at(featureTagIn));
    seerShape->updateShapeIdRecord(shapeIn, localId);
  };
  
  updateShapeByTag(seerShape->getRootOCCTShape(), FeatureTag::Root);
  updateShapeByTag(oblongMakerIn.getSolid(), FeatureTag::Solid);
  updateShapeByTag(oblongMakerIn.getShell(), FeatureTag::Shell);
  updateShapeByTag(oblongMakerIn.getFaceXP(), FeatureTag::FaceXP);
  updateShapeByTag(oblongMakerIn.getFaceXN(), FeatureTag::FaceXN);
  updateShapeByTag(oblongMakerIn.getFaceYP(), FeatureTag::FaceYP);
  updateShapeByTag(oblongMakerIn.getFaceYN(), FeatureTag::FaceYN);
  updateShapeByTag(oblongMakerIn.getFaceZP(), FeatureTag::FaceZP);
  updateShapeByTag(oblongMakerIn.getFaceZN(), FeatureTag::FaceZN);
  updateShapeByTag(oblongMakerIn.getWireXP(), FeatureTag::WireXP);
  updateShapeByTag(oblongMakerIn.getWireXN(), FeatureTag::WireXN);
  updateShapeByTag(oblongMakerIn.getWireYP(), FeatureTag::WireYP);
  updateShapeByTag(oblongMakerIn.getWireYN(), FeatureTag::WireYN);
  updateShapeByTag(oblongMakerIn.getWireZP(), FeatureTag::WireZP);
  updateShapeByTag(oblongMakerIn.getWireZN(), FeatureTag::WireZN);
  updateShapeByTag(oblongMakerIn.getEdgeXPYP(), FeatureTag::EdgeXPYP);
  updateShapeByTag(oblongMakerIn.getEdgeXPZP(), FeatureTag::EdgeXPZP);
  updateShapeByTag(oblongMakerIn.getEdgeXPYN(), FeatureTag::EdgeXPYN);
  updateShapeByTag(oblongMakerIn.getEdgeXPZN(), FeatureTag::EdgeXPZN);
  updateShapeByTag(oblongMakerIn.getEdgeXNYN(), FeatureTag::EdgeXNYN);
  updateShapeByTag(oblongMakerIn.getEdgeXNZP(), FeatureTag::EdgeXNZP);
  updateShapeByTag(oblongMakerIn.getEdgeXNYP(), FeatureTag::EdgeXNYP);
  updateShapeByTag(oblongMakerIn.getEdgeXNZN(), FeatureTag::EdgeXNZN);
  updateShapeByTag(oblongMakerIn.getEdgeYPZP(), FeatureTag::EdgeYPZP);
  updateShapeByTag(oblongMakerIn.getEdgeYPZN(), FeatureTag::EdgeYPZN);
  updateShapeByTag(oblongMakerIn.getEdgeYNZP(), FeatureTag::EdgeYNZP);
  updateShapeByTag(oblongMakerIn.getEdgeYNZN(), FeatureTag::EdgeYNZN);
  updateShapeByTag(oblongMakerIn.getVertexXPYPZP(), FeatureTag::VertexXPYPZP);
  updateShapeByTag(oblongMakerIn.getVertexXPYNZP(), FeatureTag::VertexXPYNZP);
  updateShapeByTag(oblongMakerIn.getVertexXPYNZN(), FeatureTag::VertexXPYNZN);
  updateShapeByTag(oblongMakerIn.getVertexXPYPZN(), FeatureTag::VertexXPYPZN);
  updateShapeByTag(oblongMakerIn.getVertexXNYNZP(), FeatureTag::VertexXNYNZP);
  updateShapeByTag(oblongMakerIn.getVertexXNYPZP(), FeatureTag::VertexXNYPZP);
  updateShapeByTag(oblongMakerIn.getVertexXNYPZN(), FeatureTag::VertexXNYPZN);
  updateShapeByTag(oblongMakerIn.getVertexXNYNZN(), FeatureTag::VertexXNYNZN);
  
  seerShape->setRootShapeId(seerShape->featureTagId(featureTagMap.at(FeatureTag::Root)));
}

void Oblong::setupIPGroup()
{
  lengthIP = new lbr::IPGroup(&length);
  lengthIP->setMatrixDims(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(0.0, 0.0, -1.0)));
  lengthIP->noAutoRotateDragger();
  lengthIP->setRotationAxis(osg::Vec3d(1.0, 0.0, 0.0), osg::Vec3d(0.0, 0.0, 1.0));
  lengthIP->valueHasChanged();
  lengthIP->constantHasChanged();
  overlaySwitch->addChild(lengthIP.get());
  dragger->linkToMatrix(lengthIP.get());
  
  widthIP = new lbr::IPGroup(&width);
  //   widthIP->setMatrixDims(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(0.0, 0.0, -1.0)));
  widthIP->noAutoRotateDragger();
  widthIP->setRotationAxis(osg::Vec3d(0.0, 1.0, 0.0), osg::Vec3d(0.0, 0.0, -1.0));
  widthIP->valueHasChanged();
  widthIP->constantHasChanged();
  overlaySwitch->addChild(widthIP.get());
  dragger->linkToMatrix(widthIP.get());
  
  heightIP = new lbr::IPGroup(&height);
  heightIP->setMatrixDims(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(1.0, 0.0, 0.0)));
  heightIP->noAutoRotateDragger();
  heightIP->setRotationAxis(osg::Vec3d(0.0, 0.0, 1.0), osg::Vec3d(0.0, 1.0, 0.0));
  heightIP->valueHasChanged();
  heightIP->constantHasChanged();
  overlaySwitch->addChild(heightIP.get());
  dragger->linkToMatrix(heightIP.get());
  
  updateIPGroup();
}

void Oblong::updateIPGroup()
{
  lengthIP->setMatrix(gu::toOsg(system));
  widthIP->setMatrix(gu::toOsg(system));
  heightIP->setMatrix(gu::toOsg(system));
  
  osg::Matrix lMatrix;
  lMatrix.setRotate(osg::Quat(osg::PI_2, osg::Vec3d(0.0, 1.0, 0.0)));
  lMatrix.setTrans(osg::Vec3d(0.0, width / 2.0, height / 2.0));
  lengthIP->setMatrixDragger(lMatrix);
  
  osg::Matrix wMatrix;
  wMatrix.setRotate(osg::Quat(osg::PI_2, osg::Vec3d(-1.0, 0.0, 0.0)));
  wMatrix.setTrans(osg::Vec3d(length / 2.0, 0.0, height / 2.0));
  widthIP->setMatrixDragger(wMatrix);
  
  osg::Matrix hMatrix;
  //no need to rotate
  hMatrix.setTrans(osg::Vec3d(length / 2.0, width / 2.0, 0.0));
  heightIP->setMatrixDragger(hMatrix);
}

void Oblong::serialWrite(const QDir &dIn)
{
  prj::srl::FeatureOblong oblongOut
  (
    CSysBase::serialOut(),
    length.serialOut(),
    width.serialOut(),
    height.serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::oblong(stream, oblongOut, infoMap);
}

void Oblong::serialRead(const prj::srl::FeatureOblong &sOblong)
{
  CSysBase::serialIn(sOblong.featureCSysBase());
  length.serialIn(sOblong.length());
  width.serialIn(sOblong.width());
  height.serialIn(sOblong.height());
}