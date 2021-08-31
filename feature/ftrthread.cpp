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

#include <gp_Cone.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <GCE2d_MakeSegment.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepLib.hxx>
#include <BRepFill.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeHalfSpace.hxx>
#include <ShapeUpgrade_UnifySameDomain.hxx>
#include <BRepBuilderAPI_Transform.hxx>

#include <osg/Switch>

#include "tools/occtools.h"
#include "tools/idtools.h"
#include "globalutilities.h"
#include "annex/annseershape.h"
#include "annex/anncsysdragger.h"
#include "library/lbrplabel.h"
#include "library/lbrcsysdragger.h"
#include "project/serial/generated/prjsrlthdsthread.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "feature/ftrbooleanoperation.h"
#include "feature/ftrshapecheck.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrprimitive.h"
#include "tools/featuretools.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "feature/ftrthread.h"

using namespace ftr::Thread;

using boost::uuids::uuid;

QIcon Feature::icon = QIcon(":/resources/images/constructionThread.svg");

inline static const prf::Thread& pTh(){return prf::manager().rootPtr->features().thread().get();}

struct Feature::Stow
{
  Feature &feature;
  Primitive primitive;
  
  prm::Parameter diameter{prm::Names::Diameter, pTh().diameter(), prm::Tags::Diameter}; //!< major diameter.
  prm::Parameter pitch{prm::Names::Pitch, pTh().pitch(), prm::Tags::Pitch};
  prm::Parameter length{prm::Names::Length, pTh().length(), prm::Tags::Length};
  prm::Parameter angle{prm::Names::Angle, pTh().angle(), prm::Tags::Angle}; //!< included angle of thread section in degrees.
  prm::Parameter internal{QObject::tr("Internal"), pTh().internal(), Tags::Internal}; //!< boolean to signal internal or external threads.
  prm::Parameter fake{QObject::tr("Fake"), pTh().fake(), Tags::Fake}; //!< true means no helical.
  prm::Parameter leftHanded{QObject::tr("Left Handed"), pTh().leftHanded(), Tags::LeftHanded};

  osg::ref_ptr<lbr::PLabel> diameterLabel{new lbr::PLabel(&diameter)};
  osg::ref_ptr<lbr::PLabel> pitchLabel{new lbr::PLabel(&pitch)};
  osg::ref_ptr<lbr::PLabel> lengthLabel{new lbr::PLabel(&length)};
  osg::ref_ptr<lbr::PLabel> angleLabel{new lbr::PLabel(&angle)};
  osg::ref_ptr<lbr::PLabel> internalLabel{new lbr::PLabel(&internal)};
  osg::ref_ptr<lbr::PLabel> fakeLabel{new lbr::PLabel(&fake)};
  osg::ref_ptr<lbr::PLabel> leftHandedLabel{new lbr::PLabel(&leftHanded)};

  uuid solidId = gu::createRandomId();
  std::vector<uuid> ids;
  
  Stow(Feature& fIn)
  : feature(fIn)
  , primitive(Primitive::Input{fIn, fIn.parameters, fIn.annexes})
  {
    //not going to use any optional stuff from primitive.
    
    auto setupParameter = [&](prm::Parameter &p)
    {
      p.setConstraint(prm::Constraint::buildNonZeroPositive());
      p.connectValue(std::bind(&Feature::setModelDirty, &feature));
      feature.parameters.push_back(&p);
    };
    setupParameter(diameter);
    setupParameter(pitch);
    setupParameter(length);
    
    //angle has a special constraint
    prm::Constraint angleConstraint;
    prm::Boundary lower(0.0, prm::Boundary::End::Open);
    prm::Boundary upper(180.0, prm::Boundary::End::Open);
    prm::Interval interval(lower, upper);
    angleConstraint.intervals.push_back(interval);
    angle.setConstraint(angleConstraint);
    angle.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&angle);
    
    internal.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&internal);
    
    fake.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&fake);
    
    leftHanded.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&leftHanded);
    
    feature.overlaySwitch->addChild(diameterLabel.get());
    feature.overlaySwitch->addChild(pitchLabel.get());
    feature.overlaySwitch->addChild(lengthLabel.get());
    feature.overlaySwitch->addChild(angleLabel.get());
    feature.overlaySwitch->addChild(internalLabel.get());
    feature.overlaySwitch->addChild(fakeLabel.get());
    feature.overlaySwitch->addChild(leftHandedLabel.get());
  }
  
  void updateIds()
  {
    //the solid is the only shape guarenteed to be consistant. We just use a shape index
    //mapping. This won't guarentee consistant ids, but should eliminate subsequant feature
    //id bloat.
    
    occt::ShapeVector shapes = occt::mapShapes(primitive.sShape.getRootOCCTShape());
    int diff = shapes.size() - ids.size();
    if (diff < 0)
      diff = 0;
    for (int i = 0; i < diff; ++i)
      ids.push_back(gu::createRandomId());
    int si = 0; //shapeIndex.
    for (const auto &s : shapes)
    {
      //don't overwrite the root compound
      if (primitive.sShape.findId(s).is_nil())
      {
        primitive.sShape.updateId(s, ids.at(si));
        if (!primitive.sShape.hasEvolveRecordOut(ids.at(si)))
          primitive.sShape.insertEvolve(gu::createNilId(), ids.at(si));
      }
      si++;
    }
    
    TopoDS_Shape solid = occt::getFirstNonCompound(primitive.sShape.getRootOCCTShape());
    assert(solid.ShapeType() == TopAbs_SOLID);
    primitive.sShape.updateId(solid, solidId);
    if (!primitive.sShape.hasEvolveRecordOut(solidId))
      primitive.sShape.insertEvolve(gu::createNilId(), solidId);
  }

  void updateLabels()
  {
    osg::Matrixd m = primitive.csys.getMatrix();
    double d = diameter.getDouble();
    double l = length.getDouble();
    
    diameterLabel->setMatrix(osg::Matrixd::translate(osg::Vec3d(d / 2.0, 0.0, l * .625) * m));
    pitchLabel->setMatrix(osg::Matrixd::translate(osg::Vec3d(d / 2.0, 0.0, l * .875) * m));
    lengthLabel->setMatrix(osg::Matrixd::translate(osg::Vec3d(0.0, 0.0, l) * m));
    angleLabel->setMatrix(osg::Matrixd::translate(osg::Vec3d(d / 2.0, 0.0, l *.375) * m));
    internalLabel->setMatrix(osg::Matrixd::translate(osg::Vec3d(-d / 2.0, 0.0, l * .875) * m));
    fakeLabel->setMatrix(osg::Matrixd::translate(osg::Vec3d(-d / 2.0, 0.0, l * .625) * m));
    leftHandedLabel->setMatrix(osg::Matrixd::translate(osg::Vec3d(-d / 2.0, 0.0, l *.375) * m));
  }
};

//defaulting to 10mm screw. need to fill in with preferences.
Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  name = QObject::tr("Thread");
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

//builds one revolution
TopoDS_Edge buildOneHelix(double dia, double p)
{
  auto surface = new Geom_CylindricalSurface(gp_Ax3(), dia/2.0);
  gp_Pnt2d p1(0.0, 0.0);
  gp_Pnt2d p2(2.0 * osg::PI, p);
  GCE2d_MakeSegment segmentMaker(p1, p2);
  if (!segmentMaker.IsDone())
    throw std::runtime_error("couldn't build helix");
  auto trimmedCurve = segmentMaker.Value();
  TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(trimmedCurve, surface);
  BRepLib::BuildCurves3d(edge);
  return edge;
}

//ref:
//https://en.wikipedia.org/wiki/ISO_metric_screw_thread
//tried using BRepBuilderAPI_FastSewing but it didn't work. FYI
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
    
    double d = stow->diameter.getDouble();
    double p = stow->pitch.getDouble();
    double l = stow->length.getDouble();
    double a = osg::DegreesToRadians(stow->angle.getDouble());
    
    double h = p / (2.0 * std::tan(a / 2.0));
    
    TopoDS_Shape proto;
    
    if (!stow->fake.getBool())
    {
      //internal or external
      TopoDS_Edge outside01 = buildOneHelix(d + .125 * h * 2.0, p);
      
      TopoDS_Edge inside01 = buildOneHelix(d - .875 * h * 2.0, p);
      occt::moveShape(inside01, gp_Vec(0.0, 0.0, 1.0), p / 2.0);
      
      TopoDS_Edge outside02 = outside01;
      occt::moveShape(outside02, gp_Vec(0.0, 0.0, 1.0), p);
      
      TopoDS_Edge inside02 = inside01;
      occt::moveShape(inside02, gp_Vec(0.0, 0.0, 1.0), p);
      
      TopoDS_Face face01 = BRepFill::Face(outside01, inside01);
      TopoDS_Face face02 = BRepFill::Face(inside01, outside02);
      TopoDS_Face face03 = BRepFill::Face(outside02, inside02);
      
      BRepBuilderAPI_Sewing sewOp;
      TopoDS_Shape edgeToBlend;
      double blendRadius = 0.0;
      if (stow->internal.getBool())
      {
        //internal threads
        sewOp.Add(face02);
        sewOp.Add(face03);
        sewOp.Perform();
        edgeToBlend = sewOp.ModifiedSubShape(outside02);
        blendRadius = (p / 16.0 * std::tan(a / 2.0)) / std::sin(a / 2.0);
      }
      else
      {
        //external threads
        sewOp.Add(face01);
        sewOp.Add(face02);
        sewOp.Perform();
        edgeToBlend = sewOp.ModifiedSubShape(inside01);
        blendRadius = (p / 8.0 * std::tan(a / 2.0)) / std::sin(a / 2.0);
      }
      
      assert(blendRadius > 0.0);
      assert(!edgeToBlend.IsNull());
      assert(!sewOp.SewedShape().IsNull());
      
      BRepFilletAPI_MakeFillet filletOp(sewOp.SewedShape());
      filletOp.Add(blendRadius, TopoDS::Edge(sewOp.ModifiedSubShape(edgeToBlend)));
      filletOp.Build();
      if (!filletOp.IsDone())
        throw std::runtime_error("fillet operation failed");
      
      proto = occt::getFirstNonCompound(filletOp.Shape());
    }
    else
    {
      //build fake threads. just cone faces no helix, no blends, or flats.
      gp_Cone coneDef(gp_Ax3(), (osg::PI / 2.0 - a / 2.0), (d - .875 * h * 2.0) / 2.0);
      double sectionLength = p / 2.0 / std::sin(a / 2.0);
      TopoDS_Face face01 = BRepBuilderAPI_MakeFace(coneDef, 0.0, 2 * osg::PI, 0.0, sectionLength);
      
      gp_Trsf mirror; mirror.SetMirror(gp_Ax1(gp_Pnt(0.0, 0.0, p / 2.0), gp_Dir(1.0, 0.0, 0.0)));
      TopoDS_Face face02 = TopoDS::Face(BRepBuilderAPI_Transform(face01, mirror, true).Shape());
      
      BRepBuilderAPI_Sewing sewOp;
      sewOp.Add(face01);
      sewOp.Add(face02);
      sewOp.Perform();
      
      proto = occt::getFirstNonCompound(sewOp.SewedShape());
    }
 
    if (proto.IsNull())
      throw std::runtime_error("couldn't build proto shape");
    
    BRepBuilderAPI_Sewing sewOp;
    int instanceCount = static_cast<int>(l / p);
    instanceCount += 4;
    for (int i = 0; i < instanceCount; ++i)
    {
      TopoDS_Shape t = proto;
      occt::moveShape(t, gp_Vec(0.0, 0.0, 1.0), p * i);
      sewOp.Add(t);
    }
    sewOp.Perform();
    proto = occt::getFirstNonCompound(sewOp.SewedShape());
    if (proto.ShapeType() != TopAbs_SHELL)
      throw std::runtime_error("didn't get expected shell out of sew operation");
    occt::moveShape(proto, gp_Vec(0.0, 0.0, -1.0), p * 2.25);
    
    gp_Pnt refPoint(d, 0.0, l / 2.0);
    TopoDS_Solid tool = BRepPrimAPI_MakeHalfSpace(TopoDS::Shell(proto), refPoint);
    TopoDS_Shape out;
    if (!stow->fake.getBool())
    {
      //real threads
      if (stow->internal.getBool())
      {
        //internal
        
        //create solid to work on.
        BRepPrimAPI_MakeCylinder baseMaker(d, l); //plenty big on diameter.
        baseMaker.Build();
        if (!baseMaker.IsDone())
          throw std::runtime_error("couldn't build cylinder");
        
        //cut threads away.
        BooleanOperation threadCut(baseMaker.Shape(), tool, BOPAlgo_CUT);
        threadCut.Build();
        if (!threadCut.IsDone())
          throw std::runtime_error("OCC subtraction failed");
        
        //make a cylinder and union for thread trim. p/4 in ref picture.
        BRepPrimAPI_MakeCylinder flatMaker(d / 2.0 - .625 * h, l);
        flatMaker.Build();
        if (!flatMaker.IsDone())
          throw std::runtime_error("couldn't build cylinder");
        BooleanOperation flatUniter(threadCut.Shape(), flatMaker.Shape(), BOPAlgo_FUSE);
        flatUniter.Build();
        if (!flatUniter.IsDone())
          throw std::runtime_error("OCC union failed");
        
        //this probably overkill.
        ShapeUpgrade_UnifySameDomain usd(flatUniter.Shape());
        usd.History().Nullify(); 
        usd.Build();
        
        out = usd.Shape();
      }
      else
      {
        //external
        BRepPrimAPI_MakeCylinder cylinderMaker(d / 2.0, l);
        cylinderMaker.Build();
        if (!cylinderMaker.IsDone())
          throw std::runtime_error("couldn't build cylinder");
        
        BooleanOperation subtracter(cylinderMaker.Shape(), tool, BOPAlgo_CUT);
        subtracter.Build();
        if (!subtracter.IsDone())
          throw std::runtime_error("OCC subtraction failed");
        out = subtracter.Shape();
      }
    }
    else
    {
      //fake threads.
      BRepPrimAPI_MakeCylinder cylinderMaker(d, l); //plenty big
      cylinderMaker.Build();
      if (!cylinderMaker.IsDone())
        throw std::runtime_error("couldn't build cylinder");
      
      BooleanOperation subtracter(cylinderMaker.Shape(), tool, BOPAlgo_CUT);
      subtracter.Build();
      if (!subtracter.IsDone())
        throw std::runtime_error("OCC subtraction failed");
      out = subtracter.Shape();
    }
    
    //doesn't hurt to mirror the fake threads.
    if (stow->leftHanded.getBool())
    {
      gp_Trsf mirror;
      mirror.SetMirror(gp_Ax2(gp_Pnt(0.0, 0.0, l / 2.0), gp_Dir(0.0, 0.0, 1.0)));
      out = BRepBuilderAPI_Transform(out, mirror, true).Shape();
    }
    
    ShapeCheck check(out);
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    gp_Trsf nt; //new transformation
    nt.SetTransformation(gp_Ax3(gu::toOcc(stow->primitive.csys.getMatrix())));
    nt.Invert();
    TopLoc_Location nl(nt); //new location
    out.Location(nt);
    
    stow->primitive.sShape.setOCCTShape(out, getId());
    stow->updateIds();
    stow->primitive.sShape.ensureNoNils();
    stow->primitive.sShape.ensureNoDuplicates();
        
    mainTransform->setMatrix(osg::Matrixd::identity());
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in thread update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in thread update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in thread update. " << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  stow->updateLabels();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Feature::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::thds::Thread to //thread out.
  (
    Base::serialOut(),
    stow->primitive.csysType.serialOut(),
    stow->diameter.serialOut(),
    stow->pitch.serialOut(),
    stow->length.serialOut(),
    stow->angle.serialOut(),
    stow->internal.serialOut(),
    stow->fake.serialOut(),
    stow->leftHanded.serialOut(),
    stow->primitive.csys.serialOut(),
    stow->primitive.csysLinked.serialOut(),
    stow->primitive.csysDragger.serialOut(),
    stow->primitive.sShape.serialOut(),
    stow->diameterLabel->serialOut(),
    stow->pitchLabel->serialOut(),
    stow->lengthLabel->serialOut(),
    stow->angleLabel->serialOut(),
    stow->internalLabel->serialOut(),
    stow->fakeLabel->serialOut(),
    stow->leftHandedLabel->serialOut(),
    gu::idToString(stow->solidId)
  );
  
  for (const auto &idOut : stow->ids)
    to.ids().push_back(gu::idToString(idOut));
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::thds::thread(stream, to, infoMap);
}

void Feature::serialRead(const prj::srl::thds::Thread &ti)
{
  Base::serialIn(ti.base());
  stow->primitive.csysType.serialIn(ti.csysType());
  stow->diameter.serialIn(ti.diameter());
  stow->pitch.serialIn(ti.pitch());
  stow->length.serialIn(ti.length());
  stow->angle.serialIn(ti.angle());
  stow->internal.serialIn(ti.internal());
  stow->fake.serialIn(ti.fake());
  stow->leftHanded.serialIn(ti.leftHanded());
  stow->primitive.csys.serialIn(ti.csys());
  stow->primitive.csysLinked.serialIn(ti.csysLinked());
  stow->primitive.csysDragger.serialIn(ti.csysDragger());
  stow->primitive.sShape.serialIn(ti.seerShape());
  stow->diameterLabel->serialIn(ti.diameterLabel());
  stow->pitchLabel->serialIn(ti.pitchLabel());
  stow->lengthLabel->serialIn(ti.lengthLabel());
  stow->angleLabel->serialIn(ti.angleLabel());
  stow->internalLabel->serialIn(ti.internalLabel());
  stow->fakeLabel->serialIn(ti.fakeLabel());
  stow->leftHandedLabel->serialIn(ti.leftHandedLabel());
  stow->solidId = gu::stringToId(ti.solidId());
  
  for (const auto &idIn : ti.ids())
    stow->ids.push_back(gu::stringToId(idIn));
}
