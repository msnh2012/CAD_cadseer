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
#include "project/serial/generated/prjsrlsprssphere.h"
#include "annex/anncsysdragger.h"
#include "annex/annseershape.h"
#include "feature/ftrinputtype.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "tools/featuretools.h"
#include "feature/ftrprimitive.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrsphere.h"

using namespace ftr::Sphere;
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

QIcon Feature::icon = QIcon(":/resources/images/constructionSphere.svg");

inline static const prf::Sphere& pSph(){return prf::manager().rootPtr->features().sphere().get();}

struct Feature::Stow
{
  Primitive primitive;
  
  Stow(Feature &fIn)
  : primitive(Primitive::Input{fIn, fIn.parameters, fIn.annexes})
  {
    primitive.addRadius(pSph().radius());
    primitive.radiusIP->setMatrixDims(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(0.0, 1.0, 0.0)));
    primitive.radiusIP->setMatrixDragger(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(-1.0, 0.0, 0.0)));
    primitive.radiusIP->setDimsFlipped(true);
    primitive.radiusIP->setRotationAxis(osg::Vec3d(0.0, 0.0, 1.0), osg::Vec3d(-1.0, 0.0, 0.0));
    
    initializeMaps();
  }
  
  void initializeMaps()
  {
    //result 
    std::vector<uuid> tempIds; //save ids for later.
    for (unsigned int index = 0; index < 8; ++index)
    {
      uuid tempId = gu::createRandomId();
      tempIds.push_back(tempId);
      primitive.sShape.insertEvolve(gu::createNilId(), tempId);
    }
    
    //helper lamda
    auto insertIntoFeatureMap = [&](const uuid &idIn, FeatureTag featureTagIn)
    {
      primitive.sShape.insertFeatureTag(idIn, featureTagMap.at(featureTagIn));
    };
    
    insertIntoFeatureMap(tempIds.at(0), FeatureTag::Root);
    insertIntoFeatureMap(tempIds.at(1), FeatureTag::Solid);
    insertIntoFeatureMap(tempIds.at(2), FeatureTag::Shell);
    insertIntoFeatureMap(tempIds.at(3), FeatureTag::Face);
    insertIntoFeatureMap(tempIds.at(4), FeatureTag::Wire);
    insertIntoFeatureMap(tempIds.at(5), FeatureTag::Edge);
    insertIntoFeatureMap(tempIds.at(6), FeatureTag::VertexBottom);
    insertIntoFeatureMap(tempIds.at(7), FeatureTag::VertexTop);
  }
  
  void updateResult(BRepPrimAPI_MakeSphere &sphereMaker)
  {
    //helper lamda
    auto updateShapeByTag = [&](const TopoDS_Shape &shapeIn, FeatureTag featureTagIn)
    {
      uuid localId = primitive.sShape.featureTagId(featureTagMap.at(featureTagIn));
      primitive.sShape.updateId(shapeIn, localId);
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
};

Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  name = QObject::tr("Sphere");
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
    if (static_cast<Primitive::CSysType>(stow->primitive.csysType.getInt()) == Primitive::Linked)
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
    
    BRepPrimAPI_MakeSphere sphereMaker(gu::toOcc(stow->primitive.csys.getMatrix()), stow->primitive.radius->getDouble());
    sphereMaker.Build();
    assert(sphereMaker.IsDone());
    stow->primitive.sShape.setOCCTShape(sphereMaker.Shape(), getId());
    stow->updateResult(sphereMaker);
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
  stow->primitive.IPsToCsys();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Feature::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::sprs::Sphere sphereOut
  (
    Base::serialOut(),
    stow->primitive.csysType.serialOut(),
    stow->primitive.radius->serialOut(),
    stow->primitive.csys.serialOut(),
    stow->primitive.csysLinked.serialOut(),
    stow->primitive.csysDragger.serialOut(),
    stow->primitive.sShape.serialOut(),
    stow->primitive.radiusIP->serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::sprs::sphere(stream, sphereOut, infoMap);
}

void Feature::serialRead(const prj::srl::sprs::Sphere& sSphereIn)
{
  Base::serialIn(sSphereIn.base());
  stow->primitive.csysType.serialIn(sSphereIn.csysType());
  stow->primitive.radius->serialIn(sSphereIn.radius());
  stow->primitive.csys.serialIn(sSphereIn.csys());
  stow->primitive.csysLinked.serialIn(sSphereIn.csysLinked());
  stow->primitive.csysDragger.serialIn(sSphereIn.csysDragger());
  stow->primitive.sShape.serialIn(sSphereIn.seerShape());
  stow->primitive.radiusIP->serialIn(sSphereIn.radiusIP());
}
