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

#include <assert.h>

#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "globalutilities.h"
#include "tools/idtools.h"
#include "tools/featuretools.h"
#include "library/lbrlineardimension.h"
#include "library/lbripgroup.h"
#include "library/lbrcsysdragger.h"
#include "project/serial/generated/prjsrlcylscylinder.h"
#include "annex/anncsysdragger.h"
#include "feature/ftrcylinderbuilder.h"
#include "feature/ftrinputtype.h"
#include "annex/annseershape.h"
#include "feature/ftrpick.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrprimitive.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "feature/ftrcylinder.h"

using namespace ftr::Cylinder;
using boost::uuids::uuid;

enum class FeatureTag
{
  Root,         //!< compound
  Solid,        //!< solid
  Shell,        //!< shell
  FaceBottom,   //!< bottom of cone
  FaceCylindrical,  //!< conical face
  FaceTop,      //!< might be empty
  WireBottom,   //!< wire on base face
  WireCylindrical,  //!< wire along conical face
  WireTop,      //!< wire along top
  EdgeBottom,   //!< bottom edge.
  EdgeCylindrical,  //!< edge on conical face
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
  {FeatureTag::FaceCylindrical, "FaceCylindrical"},
  {FeatureTag::FaceTop, "FaceTop"},
  {FeatureTag::WireBottom, "WireBottom"},
  {FeatureTag::WireCylindrical, "WireCylindrical"},
  {FeatureTag::WireTop, "WireTop"},
  {FeatureTag::EdgeBottom, "EdgeBottom"},
  {FeatureTag::EdgeCylindrical, "EdgeCylindrical"},
  {FeatureTag::EdgeTop, "EdgeTop"},
  {FeatureTag::VertexBottom, "VertexBottom"},
  {FeatureTag::VertexTop, "VertexTop"}
};

QIcon Feature::icon = QIcon(":/resources/images/constructionCylinder.svg");

inline static const prf::Cylinder& pCyl(){return prf::manager().rootPtr->features().cylinder().get();}

struct Feature::Stow
{
  Feature &feature;
  Primitive primitive;
  
  Stow() = delete;
  Stow(Feature &fIn)
  : feature(fIn)
  , primitive(Primitive::Input{fIn, fIn.parameters, fIn.annexes})
  {
    primitive.addRadius(pCyl().radius());
    primitive.addHeight(pCyl().height());
    
    primitive.radiusIP->setMatrixDims(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(0.0, 1.0, 0.0)));
    primitive.radiusIP->setMatrixDragger(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(-1.0, 0.0, 0.0)));
    primitive.radiusIP->setDimsFlipped(true);
    primitive.radiusIP->setRotationAxis(osg::Vec3d(0.0, 0.0, 1.0), osg::Vec3d(-1.0, 0.0, 0.0));

    primitive.heightIP->setMatrixDims(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(1.0, 0.0, 0.0)));
    primitive.heightIP->setRotationAxis(osg::Vec3d(0.0, 0.0, 1.0), osg::Vec3d(0.0, -1.0, 0.0));
    
    initializeMaps();
  }
  
  void updateIPs()
  {
    primitive.IPsToCsys();
    
    //height of radius dragger
    static const auto rot = osg::Matrixd::rotate(osg::Quat(osg::PI_2, osg::Vec3d(-1.0, 0.0, 0.0)));
    auto trans = osg::Matrixd::translate(0.0, 0.0, primitive.height->getDouble() / 2.0);
    primitive.radiusIP->setMatrixDragger(rot * trans);
    primitive.radiusIP->mainDim->setSqueeze(primitive.height->getDouble() / 2.0);
    primitive.radiusIP->mainDim->setExtensionOffset(primitive.height->getDouble() / 2.0);
    
    primitive.heightIP->mainDim->setSqueeze(primitive.radius->getDouble());
    primitive.heightIP->mainDim->setExtensionOffset(primitive.radius->getDouble());
  }
  
  void initializeMaps()
  {
    //result 
    std::vector<uuid> tempIds; //save ids for later.
    for (unsigned int index = 0; index < 14; ++index)
    {
      uuid tempId = gu::createRandomId();
      tempIds.push_back(tempId);
      primitive.sShape.insertEvolve(gu::createNilId(), tempId);
    }
    
    //helper lamda
    auto insertIntoFeatureMap = [this](const uuid &idIn, FeatureTag featureTagIn)
    {
      primitive.sShape.insertFeatureTag(idIn, featureTagMap.at(featureTagIn));
    };
    
    //first we do the compound that is root. this is not in box maker.
    insertIntoFeatureMap(tempIds.at(0), FeatureTag::Root);
    insertIntoFeatureMap(tempIds.at(1), FeatureTag::Solid);
    insertIntoFeatureMap(tempIds.at(2), FeatureTag::Shell);
    insertIntoFeatureMap(tempIds.at(3), FeatureTag::FaceBottom);
    insertIntoFeatureMap(tempIds.at(4), FeatureTag::FaceCylindrical);
    insertIntoFeatureMap(tempIds.at(5), FeatureTag::FaceTop);
    insertIntoFeatureMap(tempIds.at(6), FeatureTag::WireBottom);
    insertIntoFeatureMap(tempIds.at(7), FeatureTag::WireCylindrical);
    insertIntoFeatureMap(tempIds.at(8), FeatureTag::WireTop);
    insertIntoFeatureMap(tempIds.at(9), FeatureTag::EdgeBottom);
    insertIntoFeatureMap(tempIds.at(10), FeatureTag::EdgeCylindrical);
    insertIntoFeatureMap(tempIds.at(11), FeatureTag::EdgeTop);
    insertIntoFeatureMap(tempIds.at(12), FeatureTag::VertexBottom);
    insertIntoFeatureMap(tempIds.at(13), FeatureTag::VertexTop);
  }

  void updateResult(const CylinderBuilder &cylinderBuilderIn)
  {
    //helper lamda
    auto updateShapeByTag = [this](const TopoDS_Shape &shapeIn, FeatureTag featureTagIn)
    {
      uuid localId = primitive.sShape.featureTagId(featureTagMap.at(featureTagIn));
      primitive.sShape.updateId(shapeIn, localId);
    };
    
    updateShapeByTag(primitive.sShape.getRootOCCTShape(), FeatureTag::Root);
    updateShapeByTag(cylinderBuilderIn.getSolid(), FeatureTag::Solid);
    updateShapeByTag(cylinderBuilderIn.getShell(), FeatureTag::Shell);
    updateShapeByTag(cylinderBuilderIn.getFaceBottom(), FeatureTag::FaceBottom);
    updateShapeByTag(cylinderBuilderIn.getFaceCylindrical(), FeatureTag::FaceCylindrical);
    updateShapeByTag(cylinderBuilderIn.getFaceTop(), FeatureTag::FaceTop);
    updateShapeByTag(cylinderBuilderIn.getWireBottom(), FeatureTag::WireBottom);
    updateShapeByTag(cylinderBuilderIn.getWireCylindrical(), FeatureTag::WireCylindrical);
    updateShapeByTag(cylinderBuilderIn.getWireTop(), FeatureTag::WireTop);
    updateShapeByTag(cylinderBuilderIn.getEdgeBottom(), FeatureTag::EdgeBottom);
    updateShapeByTag(cylinderBuilderIn.getEdgeCylindrical(), FeatureTag::EdgeCylindrical);
    updateShapeByTag(cylinderBuilderIn.getEdgeTop(), FeatureTag::EdgeTop);
    updateShapeByTag(cylinderBuilderIn.getVertexBottom(), FeatureTag::VertexBottom);
    updateShapeByTag(cylinderBuilderIn.getVertexTop(), FeatureTag::VertexTop);
    
    primitive.sShape.setRootShapeId(primitive.sShape.featureTagId(featureTagMap.at(FeatureTag::Root)));
  }
};

Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  name = QObject::tr("Cylinder");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
}

Feature::~Feature() = default;

/*
void Feature::setCSys(const osg::Matrixd &csysIn)
{
  osg::Matrixd oldSystem = csys->getMatrix();
  if (!csys->setValue(csysIn))
    return; // already at this csys
    
  //apply the same transformation to dragger, so dragger moves with it.
  osg::Matrixd diffMatrix = osg::Matrixd::inverse(oldSystem) * csysIn;
  csysDragger->draggerUpdate(csysDragger->dragger->getMatrix() * diffMatrix);
}
*/

void Feature::updateModel(const UpdatePayload &plIn)
{
  setFailure();
  lastUpdateLog.clear();
  stow->primitive.sShape.reset();
  try
  {
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    //nothing for constant.
    if (static_cast<CSysType>(stow->primitive.csysType.getInt()) == Linked)
    {
      const auto &picks = stow->primitive.csysLinked.getPicks();
      if (picks.empty())
        throw std::runtime_error("No picks for csys link");
      tls::Resolver resolver(plIn);
      if (!resolver.resolve(picks.front()))
        throw std::runtime_error("Couldn't resolve csys link pick");
      auto csysPrms = resolver.getFeature()->getParameters(prm::Tags::CSys);
      if (csysPrms.empty())
        throw std::runtime_error("csys link feature has no csys parameter");
      prm::ObserverBlocker(stow->primitive.csysObserver);
      stow->primitive.csys.setValue(csysPrms.front()->getMatrix());
      stow->primitive.csysDragger.draggerUpdate();
    }
    
    CylinderBuilder cylinderMaker
    (
      stow->primitive.radius->getDouble(),
      stow->primitive.height->getDouble(),
      gu::toOcc(stow->primitive.csys.getMatrix())
    );
    stow->primitive.sShape.setOCCTShape(cylinderMaker.getSolid(), getId());
    stow->updateResult(cylinderMaker);
    mainTransform->setMatrix(osg::Matrixd::identity());
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in cylinder update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in cylinder update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in cylinder update." << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  stow->updateIPs();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Feature::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::cyls::Cylinder cylinderOut
  (
    Base::serialOut(),
    stow->primitive.csysType.serialOut(),
    stow->primitive.radius->serialOut(),
    stow->primitive.height->serialOut(),
    stow->primitive.csys.serialOut(),
    stow->primitive.csysLinked.serialOut(),
    stow->primitive.csysDragger.serialOut(),
    stow->primitive.sShape.serialOut(),
    stow->primitive.heightIP->serialOut(),
    stow->primitive.radiusIP->serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::cyls::cylinder(stream, cylinderOut, infoMap);
}

void Feature::serialRead(const prj::srl::cyls::Cylinder& sCylinderIn)
{
  Base::serialIn(sCylinderIn.base());
  stow->primitive.csysType.serialIn(sCylinderIn.csysType());
  stow->primitive.radius->serialIn(sCylinderIn.radius());
  stow->primitive.height->serialIn(sCylinderIn.height());
  stow->primitive.csys.serialIn(sCylinderIn.csys());
  stow->primitive.csysLinked.serialIn(sCylinderIn.csysLinked());
  stow->primitive.csysDragger.serialIn(sCylinderIn.csysDragger());
  stow->primitive.sShape.serialIn(sCylinderIn.seerShape());
  stow->primitive.heightIP->serialIn(sCylinderIn.heightIP());
  stow->primitive.radiusIP->serialIn(sCylinderIn.radiusIP());
}
