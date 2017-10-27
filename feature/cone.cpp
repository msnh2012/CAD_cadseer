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

#include <globalutilities.h>
#include <tools/idtools.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <library/lineardimension.h>
#include <library/ipgroup.h>
#include <project/serial/xsdcxxoutput/featurecone.h>
#include <feature/seershape.h>
#include <feature/conebuilder.h>
#include <feature/cone.h>

using namespace ftr;
using boost::uuids::uuid;

enum class FeatureTag
{
  Root,         //!< compound
  Solid,        //!< solid
  Shell,        //!< shell
  FaceBottom,   //!< bottom of cone
  FaceConical,  //!< conical face
  FaceTop,      //!< might be empty
  WireBottom,   //!< wire on base face
  WireConical,  //!< wire along conical face
  WireTop,      //!< wire along top
  EdgeBottom,   //!< bottom edge.
  EdgeConical,  //!< edge on conical face
  EdgeTop,      //!< top edge
  VertexBottom, //!< bottom vertex
  VertexTop     //!< top vertex
};

static const std::map<FeatureTag, std::string> featureTagMap = 
{
  {FeatureTag::Root, "Root"},
  {FeatureTag::Solid, "Solid"},
  {FeatureTag::Shell, "Shell"},
  {FeatureTag::FaceBottom, "FaceBase"},
  {FeatureTag::FaceConical, "FaceConical"},
  {FeatureTag::FaceTop, "FaceTop"},
  {FeatureTag::WireBottom, "WireBottom"},
  {FeatureTag::WireConical, "WireConical"},
  {FeatureTag::WireTop, "WireTop"},
  {FeatureTag::EdgeBottom, "EdgeBottom"},
  {FeatureTag::EdgeConical, "EdgeConical"},
  {FeatureTag::EdgeTop, "EdgeTop"},
  {FeatureTag::VertexBottom, "VertexBottom"},
  {FeatureTag::VertexTop, "VertexTop"}
};

QIcon Cone::icon;

//only complete rotational cone. no partials. because top or bottom radius
//maybe 0.0, faces and wires might be null and edges maybe degenerate.
Cone::Cone() : CSysBase(),
  radius1(prm::Names::Radius1, prf::manager().rootPtr->features().cone().get().radius1()),
  radius2(prm::Names::Radius2, prf::manager().rootPtr->features().cone().get().radius2()),
  height(prm::Names::Height, prf::manager().rootPtr->features().cone().get().height())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionCone.svg");
  
  name = QObject::tr("Cone");
  mainSwitch->setUserValue(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  initializeMaps();
  
  radius1.setConstraint(prm::Constraint::buildZeroPositive());
  radius2.setConstraint(prm::Constraint::buildZeroPositive());
  height.setConstraint(prm::Constraint::buildNonZeroPositive());
  
  parameterVector.push_back(&radius1);
  parameterVector.push_back(&radius2);
  parameterVector.push_back(&height);
  
  radius1.connectValue(boost::bind(&Cone::setModelDirty, this));
  radius2.connectValue(boost::bind(&Cone::setModelDirty, this));
  height.connectValue(boost::bind(&Cone::setModelDirty, this));
  
  setupIPGroup();
}

Cone::~Cone()
{

}

void Cone::setupIPGroup()
{
  heightIP = new lbr::IPGroup(&height);
  heightIP->setMatrixDims(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(1.0, 0.0, 0.0)));
  heightIP->setRotationAxis(osg::Vec3d(0.0, 0.0, 1.0), osg::Vec3d(0.0, -1.0, 0.0));
  overlaySwitch->addChild(heightIP.get());
  dragger->linkToMatrix(heightIP.get());
  
  radius1IP = new lbr::IPGroup(&radius1);
  radius1IP->setMatrixDims(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(0.0, 1.0, 0.0)));
  radius1IP->setMatrixDragger(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(-1.0, 0.0, 0.0)));
  radius1IP->setDimsFlipped(true);
  radius1IP->setRotationAxis(osg::Vec3d(0.0, 0.0, 1.0), osg::Vec3d(-1.0, 0.0, 0.0));
  overlaySwitch->addChild(radius1IP.get());
  dragger->linkToMatrix(radius1IP.get());
  
  radius2IP = new lbr::IPGroup(&radius2);
  radius2IP->setMatrixDims(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(0.0, 1.0, 0.0)));
  radius2IP->setMatrixDragger(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(-1.0, 0.0, 0.0)));
  radius2IP->setDimsFlipped(true);
  radius2IP->setRotationAxis(osg::Vec3d(0.0, 0.0, 1.0), osg::Vec3d(-1.0, 0.0, 0.0));
  overlaySwitch->addChild(radius2IP.get());
  dragger->linkToMatrix(radius2IP.get());
  
  updateIPGroup();
}

void Cone::updateIPGroup()
{
  //height of radius2 dragger
  osg::Matrixd freshMatrix;
  freshMatrix.setRotate(osg::Quat(osg::PI_2, osg::Vec3d(-1.0, 0.0, 0.0)));
  freshMatrix.setTrans(osg::Vec3d (0.0, 0.0, static_cast<double>(height)));
  radius2IP->setMatrixDragger(freshMatrix);
  radius2IP->mainDim->setSqueeze(static_cast<double>(height));
  radius2IP->mainDim->setExtensionOffset(static_cast<double>(height));
  
  heightIP->setMatrix(gu::toOsg(system));
  radius1IP->setMatrix(gu::toOsg(system));
  radius2IP->setMatrix(gu::toOsg(system));
  
  heightIP->mainDim->setSqueeze(static_cast<double>(radius1));
  heightIP->mainDim->setExtensionOffset(static_cast<double>(radius1));
  
  heightIP->valueHasChanged();
  heightIP->constantHasChanged();
  radius1IP->valueHasChanged();
  radius1IP->constantHasChanged();
  radius2IP->valueHasChanged();
  radius2IP->constantHasChanged();
}

void Cone::setRadius1(const double& radius1In)
{
  radius1.setValue(radius1In);
}

void Cone::setRadius2(const double& radius2In)
{
  radius2.setValue(radius2In);
}

void Cone::setHeight(const double& heightIn)
{
  height.setValue(heightIn);
}

void Cone::setParameters(const double& radius1In, const double& radius2In, const double& heightIn)
{
  setRadius1(radius1In);
  setRadius2(radius2In);
  setHeight(heightIn);
}

void Cone::getParameters(double& radius1Out, double& radius2Out, double& heightOut) const
{
  radius1Out = static_cast<double>(radius1);
  radius2Out = static_cast<double>(radius2);
  heightOut = static_cast<double>(height);
}

void Cone::updateModel(const UpdatePayload &payloadIn)
{
  setFailure();
  
  CSysBase::updateModel(payloadIn);
  
  try
  {
    ConeBuilder coneBuilder(static_cast<double>(radius1), static_cast<double>(radius2), static_cast<double>(height), system);
    seerShape->setOCCTShape(coneBuilder.getSolid());
    updateResult(coneBuilder);
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::cout << std::endl << "Error in cone update. " << e.GetMessageString() << std::endl;
  }
  setModelClean();
  updateIPGroup();
}

void Cone::updateResult(const ConeBuilder& coneBuilderIn)
{
  //helper lamda
  auto updateShapeByTag = [this](const TopoDS_Shape &shapeIn, FeatureTag featureTagIn)
  {
    //when a radius is set to zero we get null shapes, so skip
    if (shapeIn.IsNull())
      return;
    uuid localId = seerShape->featureTagId(featureTagMap.at(featureTagIn));
    seerShape->updateShapeIdRecord(shapeIn, localId);
  };
  
  updateShapeByTag(seerShape->getRootOCCTShape(), FeatureTag::Root);
  updateShapeByTag(coneBuilderIn.getSolid(), FeatureTag::Solid);
  updateShapeByTag(coneBuilderIn.getShell(), FeatureTag::Shell);
  updateShapeByTag(coneBuilderIn.getFaceBottom(), FeatureTag::FaceBottom);
  updateShapeByTag(coneBuilderIn.getFaceConical(), FeatureTag::FaceConical);
  updateShapeByTag(coneBuilderIn.getFaceTop(), FeatureTag::FaceTop);
  updateShapeByTag(coneBuilderIn.getWireBottom(), FeatureTag::WireBottom);
  updateShapeByTag(coneBuilderIn.getWireConical(), FeatureTag::WireConical);
  updateShapeByTag(coneBuilderIn.getWireTop(), FeatureTag::WireTop);
  updateShapeByTag(coneBuilderIn.getEdgeBottom(), FeatureTag::EdgeBottom);
  updateShapeByTag(coneBuilderIn.getEdgeConical(), FeatureTag::EdgeConical);
  updateShapeByTag(coneBuilderIn.getEdgeTop(), FeatureTag::EdgeTop);
  updateShapeByTag(coneBuilderIn.getVertexBottom(), FeatureTag::VertexBottom);
  updateShapeByTag(coneBuilderIn.getVertexTop(), FeatureTag::VertexTop);
  
  seerShape->setRootShapeId(seerShape->featureTagId(featureTagMap.at(FeatureTag::Root)));
}

//the quantity of cone shapes can change so generating maps from first update can lead to missing
//ids and shapes. So here we will generate the maps with all necessary rows.
void Cone::initializeMaps()
{
  //result 
  std::vector<uuid> tempIds; //save ids for later.
  for (unsigned int index = 0; index < 14; ++index)
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
  
  //first we do the compound that is root. this is not in box maker.
  insertIntoFeatureMap(tempIds.at(0), FeatureTag::Root);
  insertIntoFeatureMap(tempIds.at(1), FeatureTag::Solid);
  insertIntoFeatureMap(tempIds.at(2), FeatureTag::Shell);
  insertIntoFeatureMap(tempIds.at(3), FeatureTag::FaceBottom);
  insertIntoFeatureMap(tempIds.at(4), FeatureTag::FaceConical);
  insertIntoFeatureMap(tempIds.at(5), FeatureTag::FaceTop);
  insertIntoFeatureMap(tempIds.at(6), FeatureTag::WireBottom);
  insertIntoFeatureMap(tempIds.at(7), FeatureTag::WireConical);
  insertIntoFeatureMap(tempIds.at(8), FeatureTag::WireTop);
  insertIntoFeatureMap(tempIds.at(9), FeatureTag::EdgeBottom);
  insertIntoFeatureMap(tempIds.at(10), FeatureTag::EdgeConical);
  insertIntoFeatureMap(tempIds.at(11), FeatureTag::EdgeTop);
  insertIntoFeatureMap(tempIds.at(12), FeatureTag::VertexBottom);
  insertIntoFeatureMap(tempIds.at(13), FeatureTag::VertexTop);
  
  
//   std::cout << std::endl << std::endl <<
//     "result Container: " << std::endl << resultContainer << std::endl << std::endl <<
//     "feature Container:" << std::endl << featureContainer << std::endl << std::endl <<
//     "evolution Container:" << std::endl << evolutionContainer << std::endl << std::endl;
}

void Cone::serialWrite(const QDir &dIn)
{
  prj::srl::FeatureCone coneOut
  (
    CSysBase::serialOut(),
    radius1.serialOut(),
    radius2.serialOut(),
    height.serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::cone(stream, coneOut, infoMap);
}

void Cone::serialRead(const prj::srl::FeatureCone& sCone)
{
  CSysBase::serialIn(sCone.featureCSysBase());
  radius1.serialIn(sCone.radius1());
  radius2.serialIn(sCone.radius2());
  height.serialIn(sCone.height());
  
  updateIPGroup();
}
