/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2018  Thomas S. Anderson blobfish.at.gmx.com
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

#include <gp_Ax3.hxx>
#include <BRepPrimAPI_MakeTorus.hxx>

#include <osg/Switch>

#include "globalutilities.h"
#include "tools/idtools.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "library/lbrlineardimension.h"
#include "library/lbripgroup.h"
#include "library/lbrplabel.h"
#include "library/lbrcsysdragger.h"
#include "annex/annseershape.h"
#include "annex/anncsysdragger.h"
#include "project/serial/generated/prjsrltrsstorus.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "tools/featuretools.h"
#include "feature/ftrprimitive.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrtorus.h"

using boost::uuids::uuid;
using namespace ftr::Torus;
QIcon Feature::icon = QIcon(":/resources/images/constructionTorus.svg");

inline static const prf::Torus& pTor(){return prf::manager().rootPtr->features().torus().get();}

struct Feature::Stow
{
  Feature &feature;
  Primitive primitive;
  prm::Parameter seam{QObject::tr("Seam Angle"), 0.0, prm::Tags::Angle};
  osg::ref_ptr<lbr::PLabel> seamLabel{new lbr::PLabel(&seam)};
  std::vector<uuid> offsetIds;
  
  Stow(Feature &fIn)
  :feature(fIn)
  , primitive(Primitive::Input{fIn, fIn.parameters, fIn.annexes})
  {
    primitive.addRadius1(pTor().radius1());
    primitive.addRadius2(pTor().radius2());
    //constraints on radius in primitives are non zero positive. overwrite.
    primitive.radius1->setConstraint(prm::Constraint::buildZeroPositive());
    primitive.radius2->setConstraint(prm::Constraint::buildZeroPositive());
    
    prm::Constraint sc;
    prm::Boundary lower(0.0, prm::Boundary::End::Closed);
    prm::Boundary upper(360.0, prm::Boundary::End::Open);
    prm::Interval interval(lower, upper);
    sc.intervals.push_back(interval);
    seam.setConstraint(sc);
    seam.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&seam);
    
    feature.overlaySwitch->addChild(seamLabel.get());
    primitive.csysDragger.dragger->linkToMatrix(seamLabel.get());
    
    primitive.radius1IP->setMatrixDims(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(0.0, 0.0, -1.0)));
    primitive.radius1IP->setMatrixDragger(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(0.0, 1.0, 0.0)));
    primitive.radius1IP->setRotationAxis(osg::Vec3d(1.0, 0.0, 0.0), osg::Vec3d(0.0, 0.0, 1.0));
    
    primitive.radius2IP->setRotationAxis(osg::Vec3d(0.0, 1.0, 0.0), osg::Vec3d(1.0, 0.0, 1.0));
    
    initializeMaps();
  }
  
  void updateIPs()
  {
    primitive.IPsToCsys();
    static const auto rotate = osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(-1.0, 0.0, 0.0));
    auto translate = osg::Matrixd::translate(0.0, primitive.radius1->getDouble(), 0.0);
    primitive.radius2IP->setMatrixDragger(rotate * translate);
    primitive.radius2IP->setMatrixDims(osg::Matrix::translate(0.0, primitive.radius1->getDouble(), 0.0));
    
    //seamLabel positioning. label is on the -x side to avoid dragger. Thus the negs below.
    auto r1 = primitive.radius1->getDouble();
    auto r2 = primitive.radius2->getDouble();
    auto a = osg::DegreesToRadians(seam.getDouble());
    seamLabel->setMatrix(osg::Matrixd::translate(
      osg::Vec3d(-r2, 0.0, 0.0) //start location.
      * osg::Matrixd::rotate(a, osg::Vec3d(0.0, 0.0, -1.0)) //rotate for seam angle
      * osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(1.0, 0.0, 0.0)) //rotate into xz plane
      * osg::Matrixd::translate(-r1, 0.0, 0.0) //translate out to major radius
      * primitive.csys.getMatrix() //tranform to the csys
    ));
  }
  
  void initializeMaps()
  {
    //result 
    for (unsigned int index = 0; index < 8; ++index)
    {
      uuid tempId = gu::createRandomId();
      offsetIds.push_back(tempId);
      primitive.sShape.insertEvolve(gu::createNilId(), tempId);
    }
  }
  
  void updateResult()
  {
    auto sv = primitive.sShape.getAllShapes();
    assert(sv.size() == 8);
    std::size_t i = 0;
    for (const auto &s : sv)
    {
      if (primitive.sShape.findId(s).is_nil())
        primitive.sShape.updateId(s, offsetIds.at(i));
      i++; 
    }
  //   sShape->setRootShapeId(offsetIds.at(0));
  }
};


Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  name = QObject::tr("Torus");
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
    
    if (!(stow->primitive.radius1->getDouble() > stow->primitive.radius2->getDouble()))
      throw std::runtime_error("radius1 must be bigger than radius2");
    
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
    
    double sa = osg::DegreesToRadians(stow->seam.getDouble());
    BRepPrimAPI_MakeTorus tMaker(stow->primitive.radius1->getDouble(), stow->primitive.radius2->getDouble(), sa, sa + osg::PI * 2.0);
    tMaker.Build();
    assert(tMaker.IsDone());
    if (!tMaker.IsDone())
      throw std::runtime_error("BRepPrimAPI_MakeTorus failed");
    
    TopoDS_Shape out = tMaker.Shape();
    gp_Trsf nt; //new transformation
    nt.SetTransformation(gp_Ax3(gu::toOcc(stow->primitive.csys.getMatrix())));
    nt.Invert();
    TopLoc_Location nl(nt); //new location
    out.Location(nt);
    stow->primitive.sShape.setOCCTShape(out, getId());
    
    stow->updateResult();
    
    mainTransform->setMatrix(osg::Matrixd::identity());
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in torus update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in torus update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in torus update. " << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  stow->updateIPs();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Feature::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::trss::Torus to //torus out.
  (
    Base::serialOut(),
    stow->primitive.csysType.serialOut(),
    stow->primitive.radius1->serialOut(),
    stow->primitive.radius2->serialOut(),
    stow->seam.serialOut(),
    stow->primitive.csys.serialOut(),
    stow->primitive.csysLinked.serialOut(),
    stow->primitive.csysDragger.serialOut(),
    stow->primitive.sShape.serialOut(),
    stow->primitive.radius1IP->serialOut(),
    stow->primitive.radius2IP->serialOut(),
    stow->seamLabel->serialOut()
  );
  
  for (const auto &idOut : stow->offsetIds)
    to.offsetIds().push_back(gu::idToString(idOut));
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::trss::torus(stream, to, infoMap);
}

void Feature::serialRead(const prj::srl::trss::Torus &ti)
{
  Base::serialIn(ti.base());
  stow->primitive.csysType.serialIn(ti.csysType());
  stow->primitive.radius1->serialIn(ti.radius1());
  stow->primitive.radius2->serialIn(ti.radius2());
  stow->seam.serialIn(ti.seam());
  stow->primitive.csys.serialIn(ti.csys());
  stow->primitive.csysLinked.serialIn(ti.csysLinked());
  stow->primitive.csysDragger.serialIn(ti.csysDragger());
  stow->primitive.sShape.serialIn(ti.seerShape());
  stow->primitive.radius1IP->serialIn(ti.radius1IP());
  stow->primitive.radius2IP->serialIn(ti.radius2IP());
  stow->seamLabel->serialIn(ti.seamLabel());
  
  for (const auto &idIn : ti.offsetIds())
    stow->offsetIds.push_back(gu::stringToId(idIn));
}
