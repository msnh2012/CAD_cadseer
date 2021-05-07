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

#include "globalutilities.h"
#include "tools/idtools.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "library/lbrlineardimension.h"
#include "library/lbripgroup.h"
#include "library/lbrcsysdragger.h"
#include "project/serial/generated/prjsrlcnscone.h"
#include "annex/annseershape.h"
#include "annex/anncsysdragger.h"
#include "tools/featuretools.h"
#include "feature/ftrpick.h"
#include "feature/ftrconebuilder.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrprimitive.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "feature/ftrcone.h"

using namespace ftr::Cone;
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

QIcon Feature::icon;

inline static const prf::Cone& pCone(){return prf::manager().rootPtr->features().cone().get();}

struct Feature::Stow
{
  Feature &feature;
  Primitive primitive;
  
  const osg::Matrixd radiusDimBase{osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(0.0, 1.0, 0.0))};
  const osg::Matrixd radiusDraggerBase{osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(-1.0, 0.0, 0.0))};
  
  Stow() = delete;
  Stow(Feature &fIn)
  : feature(fIn)
  , primitive(Primitive::Input{fIn, fIn.parameters, fIn.annexes})
  {
    primitive.addRadius1(pCone().radius1());
    primitive.addRadius2(pCone().radius2());
    primitive.addHeight(pCone().height());
    
    auto radiusAxisBase = std::make_tuple(osg::Vec3d(0.0, 0.0, 1.0), osg::Vec3d(-1.0, 0.0, 0.0));
    
    primitive.radius1IP->setMatrixDims(radiusDimBase);
    primitive.radius1IP->setMatrixDragger(radiusDraggerBase);
    primitive.radius1IP->setDimsFlipped(true);
    primitive.radius1IP->setRotationAxis(std::get<0>(radiusAxisBase), std::get<1>(radiusAxisBase));
    
    primitive.radius2IP->setMatrixDims(radiusDimBase);
    primitive.radius2IP->setMatrixDragger(radiusDraggerBase);
    primitive.radius2IP->setDimsFlipped(true);
    primitive.radius2IP->setRotationAxis(std::get<0>(radiusAxisBase), std::get<1>(radiusAxisBase));

    primitive.heightIP->setMatrixDims(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(1.0, 0.0, 0.0)));
    primitive.heightIP->setRotationAxis(osg::Vec3d(0.0, 0.0, 1.0), osg::Vec3d(0.0, -1.0, 0.0));
    
    initializeMaps();
  }
  
  void updateIPs()
  {
    primitive.IPsToCsys();
    
    //radius1 on base is in correct location.
    
    //radius2 dragger and dims need to match height
    primitive.radius2IP->setMatrixDims(radiusDimBase * osg::Matrixd::translate(0.0, 0.0, primitive.height->getDouble()));
    primitive.radius2IP->setMatrixDragger(radiusDraggerBase * osg::Matrixd::translate(0.0, 0.0, primitive.height->getDouble()));
    
    //push height dimension of to max radius.
    double offset = std::max(primitive.radius1->getDouble(), primitive.radius2->getDouble());
    primitive.heightIP->mainDim->setSqueeze(offset);
    primitive.heightIP->mainDim->setExtensionOffset(offset);
  }
  
  //the quantity of cone shapes can change so generating maps from first update can lead to missing
  //ids and shapes. So here we will generate the maps with all necessary rows.
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
  
  void updateResult(const ConeBuilder& coneBuilderIn)
  {
    //helper lamda
    auto updateShapeByTag = [this](const TopoDS_Shape &shapeIn, FeatureTag featureTagIn)
    {
      //when a radius is set to zero we get null shapes, so skip
      if (shapeIn.IsNull())
        return;
      uuid localId = primitive.sShape.featureTagId(featureTagMap.at(featureTagIn));
      primitive.sShape.updateId(shapeIn, localId);
    };
    
    updateShapeByTag(primitive.sShape.getRootOCCTShape(), FeatureTag::Root);
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
    
    primitive.sShape.setRootShapeId(primitive.sShape.featureTagId(featureTagMap.at(FeatureTag::Root)));
  }
};

//only complete rotational cone. no partials. because top or bottom radius
//maybe 0.0, faces and wires might be null and edges maybe degenerate.
Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionCone.svg");
  
  name = QObject::tr("Cone");
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
    
    ConeBuilder coneBuilder
    (
      stow->primitive.radius1->getDouble(),
      stow->primitive.radius2->getDouble(),
      stow->primitive.height->getDouble(),
      gu::toOcc(stow->primitive.csys.getMatrix())
    );
    stow->primitive.sShape.setOCCTShape(coneBuilder.getSolid(), getId());
    stow->updateResult(coneBuilder);
    mainTransform->setMatrix(osg::Matrixd::identity());
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in cone update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in cone update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in cone update." << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  stow->updateIPs();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Feature::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::cns::Cone coneOut
  (
    Base::serialOut(),
    stow->primitive.csysType.serialOut(),
    stow->primitive.radius1->serialOut(),
    stow->primitive.radius2->serialOut(),
    stow->primitive.height->serialOut(),
    stow->primitive.csys.serialOut(),
    stow->primitive.csysLinked.serialOut(),
    stow->primitive.csysDragger.serialOut(),
    stow->primitive.sShape.serialOut(),
    stow->primitive.heightIP->serialOut(),
    stow->primitive.radius1IP->serialOut(),
    stow->primitive.radius2IP->serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::cns::cone(stream, coneOut, infoMap);
}

void Feature::serialRead(const prj::srl::cns::Cone& sCone)
{
  Base::serialIn(sCone.base());
  stow->primitive.csysType.serialIn(sCone.csysType());
  stow->primitive.radius1->serialIn(sCone.radius1());
  stow->primitive.radius2->serialIn(sCone.radius2());
  stow->primitive.height->serialIn(sCone.height());
  stow->primitive.csys.serialIn(sCone.csys());
  stow->primitive.csysLinked.serialIn(sCone.csysLinked());
  stow->primitive.csysDragger.serialIn(sCone.csysDragger());
  stow->primitive.sShape.serialIn(sCone.seerShape());
  stow->primitive.heightIP->serialIn(sCone.heightIP());
  stow->primitive.radius1IP->serialIn(sCone.radius1IP());
  stow->primitive.radius2IP->serialIn(sCone.radius2IP());
}
