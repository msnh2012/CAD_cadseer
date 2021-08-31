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
#include <string>
#include <map>

#include "globalutilities.h"
#include "tools/idtools.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "library/lbripgroup.h"
#include "library/lbrcsysdragger.h"
#include "feature/ftrboxbuilder.h"
#include "annex/annseershape.h"
#include "annex/anncsysdragger.h"
#include "project/serial/generated/prjsrlbxsbox.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrpick.h"
#include "feature/ftrprimitive.h"
#include "tools/featuretools.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "feature/ftrbox.h"

using namespace ftr::Box;
using boost::uuids::uuid;

//this is probably overkill for primitives such as box as it
//would probably be consistent using offsets. but this should
//be able to be used for all features. so consistency.

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

QIcon Feature::icon = QIcon(":/resources/images/constructionBox.svg");

inline static const prf::Box& pBox(){return prf::manager().rootPtr->features().box().get();}

struct Feature::Stow
{
  Feature &feature;
  Primitive primitive;

  const osg::Quat lipDraggerRotation{osg::Quat(osg::PI_2, osg::Vec3d(0.0, 1.0, 0.0))};
  const osg::Quat wipDraggerRotation{osg::Quat(osg::PI_2, osg::Vec3d(-1.0, 0.0, 0.0))};
  //height rotation is identity
  
  Stow() = delete;
  Stow(Feature &fIn)
  : feature(fIn)
  , primitive(Primitive::Input{fIn, fIn.parameters, fIn.annexes})
  {
    primitive.addLength(pBox().length());
    primitive.addWidth(pBox().width());
    primitive.addHeight(pBox().height());

    primitive.lengthIP->setMatrixDims(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(0.0, 0.0, -1.0)));
    primitive.lengthIP->setRotationAxis(osg::Vec3d(1.0, 0.0, 0.0), osg::Vec3d(0.0, 0.0, 1.0));
    
    //don't have to set matrix dims for width
    primitive.widthIP->setRotationAxis(osg::Vec3d(0.0, 1.0, 0.0), osg::Vec3d(0.0, 0.0, -1.0));
    
    primitive.heightIP->setMatrixDims(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(1.0, 0.0, 0.0)));
    primitive.heightIP->setRotationAxis(osg::Vec3d(0.0, 0.0, 1.0), osg::Vec3d(0.0, 1.0, 0.0));
    
    initializeMaps();
  }
  
  void updateIPs()
  {
    primitive.IPsToCsys();
    
    using osgM = osg::Matrixd;
    double l = primitive.length->getDouble() / 2.0;
    double w = primitive.width->getDouble() / 2.0;
    double h = primitive.height->getDouble() / 2.0;
    
    primitive.lengthIP->setMatrixDragger(osgM::rotate(lipDraggerRotation) * osgM::translate(osg::Vec3d(0.0, w, h)));
    primitive.widthIP->setMatrixDragger(osgM::rotate(wipDraggerRotation) * osgM::translate(osg::Vec3d(l, 0.0, h)));
    primitive.heightIP->setMatrixDragger(osgM::translate(osg::Vec3d(l, w, 0.0)));
  }
  
  void initializeMaps()
  {
    //result 
    std::vector<uuid> tempIds; //save ids for later.
    for (unsigned int index = 0; index < 35; ++index)
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
  //   std::cout << std::endl << std::endl <<
  //     "result Container: " << std::endl << resultContainer << std::endl << std::endl <<
  //     "feature Container:" << std::endl << featureContainer << std::endl << std::endl <<
  //     "evolution Container:" << std::endl << evolutionContainer << std::endl << std::endl;
  }
  
  void updateResult(const BoxBuilder& boxMakerIn)
  {
    //helper lamda
    auto updateShapeByTag = [&](const TopoDS_Shape &shapeIn, FeatureTag featureTagIn)
    {
      uuid localId = primitive.sShape.featureTagId(featureTagMap.at(featureTagIn));
      primitive.sShape.updateId(shapeIn, localId);
    };
    
  //   updateShapeByTag(sShape->getRootOCCTShape(), FeatureTag::Root);
    updateShapeByTag(boxMakerIn.getSolid(), FeatureTag::Solid);
    updateShapeByTag(boxMakerIn.getShell(), FeatureTag::Shell);
    updateShapeByTag(boxMakerIn.getFaceXP(), FeatureTag::FaceXP);
    updateShapeByTag(boxMakerIn.getFaceXN(), FeatureTag::FaceXN);
    updateShapeByTag(boxMakerIn.getFaceYP(), FeatureTag::FaceYP);
    updateShapeByTag(boxMakerIn.getFaceYN(), FeatureTag::FaceYN);
    updateShapeByTag(boxMakerIn.getFaceZP(), FeatureTag::FaceZP);
    updateShapeByTag(boxMakerIn.getFaceZN(), FeatureTag::FaceZN);
    updateShapeByTag(boxMakerIn.getWireXP(), FeatureTag::WireXP);
    updateShapeByTag(boxMakerIn.getWireXN(), FeatureTag::WireXN);
    updateShapeByTag(boxMakerIn.getWireYP(), FeatureTag::WireYP);
    updateShapeByTag(boxMakerIn.getWireYN(), FeatureTag::WireYN);
    updateShapeByTag(boxMakerIn.getWireZP(), FeatureTag::WireZP);
    updateShapeByTag(boxMakerIn.getWireZN(), FeatureTag::WireZN);
    updateShapeByTag(boxMakerIn.getEdgeXPYP(), FeatureTag::EdgeXPYP);
    updateShapeByTag(boxMakerIn.getEdgeXPZP(), FeatureTag::EdgeXPZP);
    updateShapeByTag(boxMakerIn.getEdgeXPYN(), FeatureTag::EdgeXPYN);
    updateShapeByTag(boxMakerIn.getEdgeXPZN(), FeatureTag::EdgeXPZN);
    updateShapeByTag(boxMakerIn.getEdgeXNYN(), FeatureTag::EdgeXNYN);
    updateShapeByTag(boxMakerIn.getEdgeXNZP(), FeatureTag::EdgeXNZP);
    updateShapeByTag(boxMakerIn.getEdgeXNYP(), FeatureTag::EdgeXNYP);
    updateShapeByTag(boxMakerIn.getEdgeXNZN(), FeatureTag::EdgeXNZN);
    updateShapeByTag(boxMakerIn.getEdgeYPZP(), FeatureTag::EdgeYPZP);
    updateShapeByTag(boxMakerIn.getEdgeYPZN(), FeatureTag::EdgeYPZN);
    updateShapeByTag(boxMakerIn.getEdgeYNZP(), FeatureTag::EdgeYNZP);
    updateShapeByTag(boxMakerIn.getEdgeYNZN(), FeatureTag::EdgeYNZN);
    updateShapeByTag(boxMakerIn.getVertexXPYPZP(), FeatureTag::VertexXPYPZP);
    updateShapeByTag(boxMakerIn.getVertexXPYNZP(), FeatureTag::VertexXPYNZP);
    updateShapeByTag(boxMakerIn.getVertexXPYNZN(), FeatureTag::VertexXPYNZN);
    updateShapeByTag(boxMakerIn.getVertexXPYPZN(), FeatureTag::VertexXPYPZN);
    updateShapeByTag(boxMakerIn.getVertexXNYNZP(), FeatureTag::VertexXNYNZP);
    updateShapeByTag(boxMakerIn.getVertexXNYPZP(), FeatureTag::VertexXNYPZP);
    updateShapeByTag(boxMakerIn.getVertexXNYPZN(), FeatureTag::VertexXNYPZN);
    updateShapeByTag(boxMakerIn.getVertexXNYNZN(), FeatureTag::VertexXNYNZN);
  //   sShape->setRootShapeId(sShape->featureTagId(featureTagMap.at(FeatureTag::Root)));
  //   std::cout << std::endl << "update result:" << std::endl << resultContainer << std::endl;
  }
};


Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  name = QObject::tr("Box");
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
    
    BoxBuilder boxMaker
    (
      stow->primitive.length->getDouble(),
      stow->primitive.width->getDouble(),
      stow->primitive.height->getDouble(),
      gu::toOcc(stow->primitive.csys.getMatrix())
    );
    stow->primitive.sShape.setOCCTShape(boxMaker.getSolid(), getId());
    stow->updateResult(boxMaker);
    mainTransform->setMatrix(osg::Matrixd::identity());
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in box update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in box update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in box update." << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  stow->updateIPs();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Feature::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::bxs::Box boxOut
  (
    Base::serialOut(),
    stow->primitive.csysType.serialOut(),
    stow->primitive.length->serialOut(),
    stow->primitive.width->serialOut(),
    stow->primitive.height->serialOut(),
    stow->primitive.csys.serialOut(),
    stow->primitive.csysLinked.serialOut(),
    stow->primitive.csysDragger.serialOut(),
    stow->primitive.sShape.serialOut(),
    stow->primitive.lengthIP->serialOut(),
    stow->primitive.widthIP->serialOut(),
    stow->primitive.heightIP->serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::bxs::box(stream, boxOut, infoMap);
}

void Feature::serialRead(const prj::srl::bxs::Box& sBox)
{
  Base::serialIn(sBox.base());
  stow->primitive.csysType.serialIn(sBox.csysType());
  stow->primitive.length->serialIn(sBox.length());
  stow->primitive.width->serialIn(sBox.width());
  stow->primitive.height->serialIn(sBox.height());
  stow->primitive.csys.serialIn(sBox.csys());
  stow->primitive.csysLinked.serialIn(sBox.csysLinked());
  stow->primitive.sShape.serialIn(sBox.seerShape());
  stow->primitive.csysDragger.serialIn(sBox.csysDragger());
  stow->primitive.lengthIP->serialIn(sBox.lengthIP());
  stow->primitive.widthIP->serialIn(sBox.widthIP());
  stow->primitive.heightIP->serialIn(sBox.heightIP());
}
