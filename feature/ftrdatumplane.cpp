/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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

#include <limits>

#include <boost/optional/optional.hpp>

#include <QTextStream>

#include <BRepAdaptor_Surface.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopExp.hxx>
#include <gp_Pln.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <BRep_Builder.hxx>
#include <Geom_Plane.hxx>
#include <Geom_Line.hxx>
#include <GeomAPI_IntSS.hxx>
#include <GeomAPI_IntCS.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>

#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/Switch>

#include "tools/infotools.h"
#include "globalutilities.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "modelviz/mdvnodemaskdefs.h"
#include "library/lbripgroup.h"
#include "library/lbrcsysdragger.h"
#include "library/lbrplabel.h"
#include "annex/anncsysdragger.h"
#include "modelviz/mdvdatumplane.h"
#include "annex/annseershape.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinputtype.h"
#include "project/serial/generated/prjsrldtpsdatumplane.h"
#include "tools/featuretools.h"
#include "tools/occtools.h"
#include "parameter/prmparameter.h"
#include "feature/ftrdatumaxis.h"
#include "feature/ftrdatumplane.h"

using namespace ftr;

QIcon DatumPlane::icon;

using boost::uuids::uuid;

static Bnd_Box getBoundingBox(const TopoDS_Shape &shapeIn)
{
  Bnd_Box out;
  BRepBndLib::Add(shapeIn, out);
  return out;
}

static void centerRadius(const Bnd_Box &boxIn, gp_Pnt &centerOut, gp_Pnt &cornerOut)
{
  cornerOut = boxIn.CornerMin();
  
  gp_Vec min(boxIn.CornerMin().Coord());
  gp_Vec max(boxIn.CornerMax().Coord());
  gp_Vec projection(max - min);
  projection = projection.Normalized() * (projection.Magnitude() / 2.0);
  gp_Vec centerVec = min + projection;
  centerOut = gp_Pnt(centerVec.XYZ());
}

//throws std::runtime_error
static osg::Matrixd getFaceSystem(const TopoDS_Shape &faceShape)
{
  assert(faceShape.ShapeType() == TopAbs_FACE);
  BRepAdaptor_Surface adaptor(TopoDS::Face(faceShape));
  if (adaptor.GetType() != GeomAbs_Plane)
    throw std::runtime_error("DatumPlane: wrong surface type");
  gp_Ax2 tempSystem = adaptor.Plane().Position().Ax2();
  if (faceShape.Orientation() == TopAbs_REVERSED)
    tempSystem.SetDirection(tempSystem.Direction().Reversed());
  osg::Matrixd faceSystem = gu::toOsg(tempSystem);
  
  gp_Pnt centerPoint, cornerPoint;
  centerRadius(getBoundingBox(faceShape), centerPoint, cornerPoint);
  double centerDistance = adaptor.Plane().Distance(centerPoint);
  osg::Vec3d workVector = gu::getZVector(faceSystem);
  osg::Vec3d centerVec = gu::toOsg(centerPoint) + (workVector * centerDistance);
  faceSystem.setTrans(centerVec);
  
  return faceSystem;
}

//default constructed plane is 2.0 x 2.0
DatumPlane::DatumPlane() :
Base()
, dpType(DPType::Constant)
, cachedSize(1.0)
, picks()
, csys(std::make_unique<prm::Parameter>(prm::Names::CSys, osg::Matrixd::identity()))
, flip(std::make_unique<prm::Parameter>(QObject::tr("Flip"), false))
, autoSize(std::make_unique<prm::Parameter>(prm::Names::AutoSize, false, prm::Tags::AutoSize))
, size(std::make_unique<prm::Parameter>(prm::Names::Size, 1.0, prm::Tags::Size))
, offset(std::make_unique<prm::Parameter>(prm::Names::Offset, 1.0, prm::Tags::Offset))
, angle(std::make_unique<prm::Parameter>(prm::Names::Angle, 45.0, prm::Tags::Angle))
, prmObserver(std::make_unique<prm::Observer>(std::bind(&DatumPlane::setModelDirty, this)))
, csysDragger(std::make_unique<ann::CSysDragger>(this, csys.get()))
, flipLabel(new lbr::PLabel(flip.get()))
, autoSizeLabel(new lbr::PLabel(autoSize.get()))
, sizeIP(new lbr::IPGroup(size.get()))
, offsetIP(new lbr::IPGroup(offset.get()))
, angleLabel(new lbr::PLabel(angle.get()))
, overlaySubSwitch(new osg::Switch())
, display(new mdv::DatumPlane())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionDatumPlane.svg");
  
  name = QObject::tr("Datum Plane");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  parameters.push_back(csys.get());
  csys->connect(*prmObserver);
  
  flip->connectValue(std::bind(&DatumPlane::setModelDirty, this));
  parameters.push_back(flip.get());
  
  autoSize->connectValue(std::bind(&DatumPlane::setModelDirty, this));
  parameters.push_back(autoSize.get());
  
  size->setConstraint(prm::Constraint::buildNonZeroPositive());
  size->connect(*prmObserver);
  parameters.push_back(size.get());
  
  offset->connectValue(std::bind(&DatumPlane::setModelDirty, this));
  parameters.push_back(offset.get());
  
  prm::Constraint ac;
  prm::Boundary lower(0.0, prm::Boundary::End::Closed);
  prm::Boundary upper(360.0, prm::Boundary::End::Closed);
  prm::Interval interval(lower, upper);
  ac.intervals.push_back(interval);
  angle->setConstraint(ac);
  angle->connectValue(std::bind(&DatumPlane::setModelDirty, this));
  parameters.push_back(angle.get());
  
  overlaySubSwitch->addChild(flipLabel.get());
  overlaySubSwitch->addChild(autoSizeLabel.get());
  overlaySubSwitch->addChild(angleLabel.get());
  
  sizeIP->setMatrixDims(osg::Matrixd::rotate(osg::PI, osg::Vec3d(0.0, 0.0, 1.0)));
  sizeIP->noAutoRotateDragger();
  sizeIP->setRotationAxis(osg::Vec3d(0.0, 1.0, 0.0), osg::Vec3d(0.0, 0.0, 1.0));
  sizeIP->setMatrixDragger(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(1.0, 0.0, 0.0)));
  sizeIP->valueHasChanged();
  sizeIP->constantHasChanged();
  overlaySubSwitch->addChild(sizeIP.get());
  
  offsetIP->setMatrixDims(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(1.0, 0.0, 0.0)));
  offsetIP->noAutoRotateDragger();
  offsetIP->setRotationAxis(osg::Vec3d(0.0, 0.0, 1.0), osg::Vec3d(0.0, 1.0, 0.0));
  offsetIP->valueHasChanged();
  offsetIP->constantHasChanged();
  overlaySubSwitch->addChild(offsetIP.get());
  
  annexes.insert(std::make_pair(ann::Type::CSysDragger, csysDragger.get()));
  overlaySubSwitch->addChild(csysDragger->dragger.get());
  
  mainTransform->addChild(display.get());
  overlaySwitch->addChild(overlaySubSwitch.get());
  updateGeometry();
}

DatumPlane::~DatumPlane(){}

void DatumPlane::setSize(double sIn)
{
  //should we set autoSize off?
  size->setValue(sIn);
}

double DatumPlane::getSize() const
{
  return static_cast<double>(*size);
}

void DatumPlane::setSystem(const osg::Matrixd &sysIn)
{
  csys->setValue(sysIn);
}

osg::Matrixd DatumPlane::getSystem() const
{
  return static_cast<osg::Matrixd>(*csys);
}

void DatumPlane::setPicks(const Picks &psIn)
{
  picks = psIn;
  setModelDirty();
}

void DatumPlane::setDPType(DPType tIn)
{
  if (dpType == tIn)
    return;
  dpType = tIn;
  updateOverlayViz();
  setModelDirty();
}

void DatumPlane::setAutoSize(bool vIn)
{
  autoSize->setValue(vIn);
}

prm::Parameter* DatumPlane::getAutoSizeParameter()
{
  return autoSize.get();
}

prm::Parameter* DatumPlane::getSizeParameter()
{
  return size.get();
}

prm::Parameter* DatumPlane::getOffsetParameter()
{
  return offset.get();
}

prm::Parameter* DatumPlane::getAngleParameter()
{
  return angle.get();
}

void DatumPlane::updateModel(const UpdatePayload &pli)
{
  setFailure();
  lastUpdateLog.clear();
  try
  {
    prm::ObserverBlocker block(*prmObserver);
    
    if (dpType == DPType::Constant)
      goUpdateConstant();
    else if (dpType == DPType::POffset)
      goUpdatePOffset(pli);
    else if (dpType == DPType::PCenter)
      goUpdatePCenter(pli);
    else if (dpType == DPType::AAngleP)
      goUpdateAAngleP(pli);
    else if (dpType == DPType::Average3P)
      goUpdateAverage3P(pli);
    else if (dpType == DPType::Through3P)
      goUpdateThrough3P(pli);
    
    if (static_cast<bool>(*flip))
    {
      osg::Matrixd cm = static_cast<osg::Matrixd>(*csys);
      osg::Quat r(osg::PI, gu::getXVector(cm));
      osg::Matrixd nm(cm.getRotate() * r);
      nm.setTrans(cm.getTrans());
      csys->setValue(nm);
    }
    
    if (static_cast<bool>(*autoSize))
    {
      size->setValue(cachedSize);
      sizeIP->valueHasChanged();
    }
    
    mainTransform->setMatrix(static_cast<osg::Matrixd>(*csys));
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in datum plane update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in datum plane update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in datum plane update." << std::endl;
    lastUpdateLog += s.str();
  }
  
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void DatumPlane::goUpdateConstant()
{
  
}

void DatumPlane::goUpdatePOffset(const UpdatePayload &pli)
{
  if (picks.size() != 1)
    throw std::runtime_error("POffset: Wrong number of picks");
  
  tls::Resolver pr(pli);
  if (!pr.resolve(picks.front()))
    throw std::runtime_error("POffset: Pick resolution failed");
  
  osg::Matrixd faceSystem;
  faceSystem = osg::Matrixd::identity();
  if (pr.getFeature()->getType() == ftr::Type::DatumPlane)
  {
    const DatumPlane *inputPlane = dynamic_cast<const DatumPlane*>(pr.getFeature());
    assert(inputPlane);
    cachedSize = inputPlane->getSize();
    faceSystem = inputPlane->getSystem();
  }
  else
  {
    auto rShapes = pr.getShapes();
    if (rShapes.empty())
      throw std::runtime_error("POffset: Resolved shapes are empty");
    if (rShapes.size() > 1)
    {
      std::ostringstream s; s << "WARNING: POffset: Multiple resolve shapes. Using first" << std::endl;
      lastUpdateLog += s.str();
    }
    if (rShapes.front().ShapeType() != TopAbs_FACE)
      throw std::runtime_error("POffset: Resolved shape is not a face.");
    
    //get coordinate system
    //TODO GeomLib_IsPlanarSurface
    BRepAdaptor_Surface adaptor(TopoDS::Face(rShapes.front()));
    if (adaptor.GetType() != GeomAbs_Plane)
      throw std::runtime_error("POffset: wrong surface type");
    gp_Ax2 tempSystem = adaptor.Plane().Position().Ax2();
    if (rShapes.front().Orientation() == TopAbs_REVERSED)
      tempSystem.SetDirection(tempSystem.Direction().Reversed());
    faceSystem = gu::toOsg(tempSystem);
    
    //calculate parameter boundaries and project onto plane.
    gp_Pnt centerPoint, cornerPoint;
    centerRadius(getBoundingBox(rShapes.front()), centerPoint, cornerPoint);
    double centerDistance = adaptor.Plane().Distance(centerPoint);
    double cornerDistance = adaptor.Plane().Distance(cornerPoint);
    osg::Vec3d workVector = gu::getZVector(faceSystem);
    osg::Vec3d centerVec = gu::toOsg(centerPoint) + (workVector * centerDistance);
    osg::Vec3d cornerVec = gu::toOsg(cornerPoint) + (workVector * cornerDistance);
    faceSystem.setTrans(centerVec);
    
    osg::Vec3d offsetVec = centerVec * osg::Matrixd::inverse(faceSystem);
    double tempRadius = (centerVec - cornerVec).length();
    cachedSize = offsetVec.x() + tempRadius;
  }
  osg::Vec3d normal = gu::getZVector(faceSystem) * static_cast<double>(*offset);
  faceSystem.setTrans(faceSystem.getTrans() + normal);
  csys->setValue(faceSystem);
}

void DatumPlane::goUpdatePCenter(const UpdatePayload &pli)
{
  if (picks.size() != 2)
    throw std::runtime_error("Wrong number of picks for center datum");
  tls::Resolver r0(pli);
  if (!r0.resolve(picks.front()))
    throw std::runtime_error("Couldn't resolve pick 0 for center datum");
  tls::Resolver r1(pli);
  if (!r1.resolve(picks.back()))
    throw std::runtime_error("Couldn't resolve pick 1 for center datum");
  
  double tempSize = std::numeric_limits<double>::min();
  
  auto getResolvedSystem = [&](const tls::Resolver &r) -> boost::optional<osg::Matrixd>
  {
    if (r.getFeature()->getType() == ftr::Type::DatumPlane)
    {
      const DatumPlane *inputPlane = dynamic_cast<const DatumPlane*>(r.getFeature());
      assert(inputPlane);
      tempSize = std::max(tempSize, inputPlane->getSize());
      return inputPlane->getSystem();
    }
    else
    {
      auto rShapes = r.getShapes();
      if (rShapes.empty())
        throw std::runtime_error("Resolved shapes are empty for center datum");
      if (rShapes.size() > 1)
      {
        std::ostringstream s; s << "WARNING: Multiple resolve shapes for center datum. Using first" << std::endl;
        lastUpdateLog += s.str();
      }
      if (rShapes.front().ShapeType() != TopAbs_FACE)
        throw std::runtime_error("Resolved shape is not a face for center datum.");
      
      tempSize = std::max(tempSize, std::sqrt(getBoundingBox(rShapes.front()).SquareExtent()) / 2.0);
      return getFaceSystem(rShapes.front());
    }
    return boost::none;
  };
  
  boost::optional<osg::Matrixd> face1System = getResolvedSystem(r0);
  boost::optional<osg::Matrixd> face2System = getResolvedSystem(r1);
  if (!face1System || !face2System)
    throw std::runtime_error("Invalid face systems");

  osg::Vec3d normal1 = gu::getZVector(face1System.get());
  osg::Vec3d normal2 = gu::getZVector(face2System.get());
  boost::optional<osg::Matrixd> newSystem;
  
  if ((gu::toOcc(normal1).IsParallel(gu::toOcc(normal2), Precision::Angular())))
  {
    osg::Vec3d projection = face2System.get().getTrans() - face1System.get().getTrans();
    double mag = projection.length() / 2.0;
    projection.normalize();
    projection *= mag;
    osg::Vec3d freshOrigin = face1System.get().getTrans() + projection;
    
    newSystem = face1System.get();
    newSystem.get().setTrans(freshOrigin);
  }
  else //try to make a bisector
  {
    gp_Pnt o1 = gu::toOcc(face1System.get().getTrans()).XYZ();
    gp_Dir d1 = gu::toOcc(normal1);
    opencascade::handle<Geom_Surface> surface1 = new Geom_Plane(o1, d1);
    gp_Pnt o2 = gu::toOcc(face2System.get().getTrans()).XYZ();
    gp_Dir d2 = gu::toOcc(normal2);
    opencascade::handle<Geom_Surface> surface2 = new Geom_Plane(o2, d2);
    
    GeomAPI_IntSS intersector(surface1, surface2, Precision::Confusion());
    if (!intersector.IsDone() || (intersector.NbLines() != 1))
      throw std::runtime_error("Couldn't intersect planes");
    const opencascade::handle<Geom_Curve> &c = intersector.Line(1);
    const opencascade::handle<Geom_Line> &l = dynamic_cast<Geom_Line*>(c.get());
    if (l.IsNull())
      throw std::runtime_error("Couldn't cast intersection results to line");
    
    osg::Vec3d ld = gu::toOsg(gp_Vec(l->Position().Direction().XYZ()));
    osg::Vec3d lo = gu::toOsg(l->Position().Location());
    osg::Vec3d py = normal1 + normal2;
    py.normalize();
    osg::Vec3d pn = ld ^ py;
    pn.normalize();
    
    //I am not smart enough for the quats.
    osg::Matrixd ts
    (
      ld.x(), ld.y(), ld.z(), 0.0,
      py.x(), py.y(), py.z(), 0.0,
      pn.x(), pn.y(), pn.z(), 0.0,
      0.0, 0.0, 0.0, 1.0
    );
    //project center of parent geometry onto line and move half of tSize.
    boost::optional<osg::Vec3d> p1, p2;
    GeomAPI_ProjectPointOnCurve proj1(gp_Pnt(gu::toOcc(face1System.get().getTrans()).XYZ()), c); 
    if (proj1.NbPoints() > 0)
      p1 = gu::toOsg(proj1.NearestPoint());
    GeomAPI_ProjectPointOnCurve proj2(gp_Pnt(gu::toOcc(face2System.get().getTrans()).XYZ()), c); 
    if (proj2.NbPoints() > 0)
      p2 = gu::toOsg(proj2.NearestPoint());
    if (p1 && p2)
    {
      osg::Vec3d projection = p2.get() - p1.get();
      double mag = projection.length() / 2.0;
      projection.normalize();
      projection *= mag;
      ts.setTrans(p1.get() + projection);
    }
    else
    {
      std::ostringstream s; s << "WARNING: couldn't project face origin onto intersection of planes" << std::endl;
      lastUpdateLog += s.str();
      ts.setTrans(lo);
    }
    newSystem = ts;
  }
  
  if (!newSystem)
    throw std::runtime_error("PCenter: couldn't derive center datum");
  cachedSize = tempSize;
  csys->setValue(newSystem.get());
}

void DatumPlane::goUpdateAAngleP(const UpdatePayload &pli)
{
  if (picks.size() != 2)
    throw std::runtime_error("AAngleP: Wrong number of picks");
  
  boost::optional<osg::Vec3d> axisOrigin, axisDirection, planeNormal;
  double tempSize = 0.0;
  tls::Resolver pr(pli);
  for (const auto &p : picks)
  {
    if (!pr.resolve(p))
      throw std::runtime_error("AAngleP: Failed to resolve pick");
    if (p.tag == plane)
    {
      if (slc::isObjectType(p.selectionType) && pr.getFeature()->getType() == ftr::Type::DatumPlane)
      {
        const DatumPlane *dp = dynamic_cast<const DatumPlane*>(pr.getFeature());
        planeNormal = gu::getZVector(dp->getSystem());
        tempSize = std::max(tempSize, dp->getSize());
      }
      else //with a shape pick, resolver verifies we something
      {
        auto rShapes = pr.getShapes();
        if (rShapes.size() > 1)
        {
          std::ostringstream s; s << "WARNING: AAngleP: more than one resolved plane. Using first" << std::endl;
          lastUpdateLog += s.str();
        }
        if (rShapes.front().ShapeType() != TopAbs_FACE)
          throw std::runtime_error("AAngleP: resolved 'plane' shape is not a face");
        planeNormal = gu::getZVector(getFaceSystem(rShapes.front()));
        occt::BoundingBox bb(rShapes.front());
        tempSize = std::max(tempSize, bb.getDiagonal() / 2.0);
      }
    }
    else if (p.tag == axis)
    {
      if (slc::isObjectType(p.selectionType) && pr.getFeature()->getType() == ftr::Type::DatumAxis)
      {
        const DatumAxis *da = dynamic_cast<const DatumAxis*>(pr.getFeature());
        axisOrigin = da->getOrigin();
        axisDirection = da->getDirection();
        tempSize = std::max(tempSize, da->getSize() / 2.0);
      }
      else
      {
        auto rShapes = pr.getShapes();
        if (rShapes.size() > 1)
        {
          std::ostringstream s; s << "WARNING: AAngleP: more than one resolved axis. Using first" << std::endl;
          lastUpdateLog += s.str();
        }
        const TopoDS_Shape &axisShape = rShapes.front();
        auto glean = occt::gleanAxis(axisShape);
        if (!glean.second)
          throw std::runtime_error("AAngleP: couldn't glean 'axis'");
        axisOrigin = gu::toOsg(glean.first.Location());
        axisDirection = gu::toOsg(glean.first.Direction());
        occt::BoundingBox bb(axisShape);
        tempSize = std::max(tempSize, bb.getDiagonal() / 2.0);
      }
    }
  }
  
  if (!axisOrigin || !axisDirection || !planeNormal)
    throw std::runtime_error("AAngleP: missing definition");
  if ((1.0 - std::fabs(planeNormal.get() * axisDirection.get())) < std::numeric_limits<float>::epsilon())
    throw std::runtime_error("AAngleP: edge and face are parallel");
  if (tempSize <= 0.0)
    tempSize = 1.0;
  cachedSize = tempSize;
  
  osg::Quat rotation(osg::DegreesToRadians(static_cast<double>(*angle)), axisDirection.get());
  osg::Vec3d outNormal(rotation * planeNormal.get());
  osg::Matrixd csysOut = osg::Matrixd::rotate(osg::Vec3d(0.0, 0.0, 1.0), outNormal);
  csysOut.setTrans(axisOrigin.get());
  csys->setValue(csysOut);
}

void DatumPlane::goUpdateAverage3P(const UpdatePayload &pli)
{
  if (picks.size() != 3)
    throw std::runtime_error("Average3P: Wrong number of picks");
  
  typedef std::vector<opencascade::handle<Geom_Surface>, NCollection_StdAllocator<opencascade::handle<Geom_Surface>>> GeomVector;
  GeomVector planes;
  osg::Vec3d averageNormal(0.0, 0.0, 0.0);
  double tSize = 1.0;
  auto resolveDatum = [&](const Base *fIn)
  {
    const DatumPlane *dp = dynamic_cast<const DatumPlane*>(fIn);
    assert(dp);
    
    osg::Matrixd dpSys = dp->getSystem();
    averageNormal += gu::getZVector(dpSys);
    gp_Pnt o = gu::toOcc(dpSys.getTrans()).XYZ();
    gp_Dir d = gu::toOcc(gu::getZVector(dpSys));
    planes.push_back(new Geom_Plane(o, d));
    tSize = std::max(tSize, dp->getSize());
  };
  auto resolveFace = [&](const TopoDS_Shape &sIn)
  {
    assert(sIn.ShapeType() == TopAbs_FACE);
    BRepAdaptor_Surface sa(TopoDS::Face(sIn));
    if (sa.GetType() == GeomAbs_Plane)
    {
      planes.push_back(sa.Surface().Surface());
      osg::Vec3d t = gu::toOsg(sa.Plane().Axis().Direction());
      if (sIn.Orientation() == TopAbs_REVERSED)
        t *= -1.0;
      averageNormal += t;
      occt::BoundingBox bb(sIn);
      tSize = std::max(tSize, bb.getDiagonal() / 2.0);
    }
  };
  
  tls::Resolver pr(pli);
  for (const auto &p : picks)
  {
    if (!pr.resolve(p))
      throw std::runtime_error("Average3P: Failed to resolve pick");
    if (pr.getFeature()->getType() == ftr::Type::DatumPlane)
      resolveDatum(pr.getFeature());
    else
    {
      auto rShapes = pr.getShapes();
      if (rShapes.empty())
        throw std::runtime_error("Average3P: No resolved shapes");
      if (rShapes.size() > 1)
      {
        std::ostringstream s; s << "WARNING: Average3P: more than one resolved shape. Using first" << std::endl;
        lastUpdateLog += s.str();
      }
      if (rShapes.front().ShapeType() != TopAbs_FACE)
        throw std::runtime_error("Average3P: Shape is not a face");
      resolveFace(rShapes.front());
    }
  }
  
  if (planes.size() != 3)
    throw std::runtime_error("Average3P: Invalid plane count");
  
  GeomAPI_IntSS intersector(planes.at(0), planes.at(1), Precision::Confusion());
  if (!intersector.IsDone() || (intersector.NbLines() != 1))
    throw std::runtime_error("Average3P: Couldn't intersect planes");
  const opencascade::handle<Geom_Curve> &c = intersector.Line(1);
  const opencascade::handle<Geom_Line> &l = dynamic_cast<Geom_Line*>(c.get());
  if (l.IsNull())
    throw std::runtime_error("Average3P: Couldn't cast intersection results to line");
  
  GeomAPI_IntCS ci(c, planes.at(2));
  if (!ci.IsDone() || (ci.NbPoints() != 1))
    throw std::runtime_error("Average3P: Couldn't intersect line and plane");
  osg::Vec3d to = gu::toOsg(ci.Point(1));
  averageNormal.normalize();
  osg::Matrixd ns = osg::Matrixd::rotate(osg::Vec3d(0.0, 0.0, 1.0), averageNormal);
  ns.setTrans(to);
  csys->setValue(ns);
  cachedSize = tSize;
}

void DatumPlane::goUpdateThrough3P(const UpdatePayload &pli)
{
  if (picks.size() != 3)
    throw std::runtime_error("Through3P: Wrong number of picks");
  
  tls::Resolver pr(pli);
  std::vector<osg::Vec3d> points;
  for (const auto &p : picks)
  {
    if (!pr.resolve(p))
      throw std::runtime_error("Through3P: Pick resolution failed");
    auto tps = pr.getPoints();
    if (tps.empty())
      throw std::runtime_error("Through3P: No resolved points");
    if (tps.size() > 1)
    {
      std::ostringstream s; s << "WARNING: Through3P: more than one resolved point. Using first" << std::endl;
      lastUpdateLog += s.str();
    }
    points.push_back(tps.front());
  }
  
  if (points.size() != 3)
    throw std::runtime_error("Through3P: couldn't get 3 points");
  
  osg::Vec3d v0 = points.at(1) - points.at(0);
  osg::Vec3d v1 = points.at(2) - points.at(0);
  
  osg::Vec3d v0n(v0);
  v0n.normalize();
  osg::Vec3d v1n(v1);
  v1n.normalize();
  if ((1.0 - std::fabs(v0n * v1n)) < std::numeric_limits<float>::epsilon())
    throw std::runtime_error("Through3P: points are collinear");
  
  osg::Vec3d z = v0 ^ v1;
  z.normalize();
  osg::Matrixd ts = osg::Matrixd::rotate(osg::Vec3d(0.0, 0.0, 1.0), z);
  
  osg::Vec3d center = (points.at(0) + points.at(1) + points.at(2)) / 3.0;
  ts.setTrans(center);
  
  csys->setValue(ts);
  cachedSize = std::max(std::max(v0.length(), v1.length()), (v1 - v0).length()) / 2.0;
}

void DatumPlane::updateVisual()
{
  updateGeometry();
  updateOverlayViz();
  updateLabelPositions();
  setVisualClean();
}

void DatumPlane::updateGeometry()
{
  double r = static_cast<double>(*size);
  display->setParameters(-r, r, -r, r);
}

void DatumPlane::updateOverlayViz()
{
  //just turn everything off and turn on what is needed.
  overlaySubSwitch->setAllChildrenOff();
  if (dpType != DPType::Constant)
    overlaySubSwitch->setChildValue(autoSizeLabel.get(), true);
  overlaySubSwitch->setChildValue(flipLabel.get(), true);
  if (!static_cast<bool>(*autoSize))
    overlaySubSwitch->setChildValue(sizeIP.get(), true);
  if (dpType == DPType::POffset)
    overlaySubSwitch->setChildValue(offsetIP.get(), true);
  if (dpType == DPType::AAngleP)
    overlaySubSwitch->setChildValue(angleLabel.get(), true);
  if (dpType == DPType::Constant)
    overlaySubSwitch->setChildValue(csysDragger->dragger.get(), true);
}

void DatumPlane::updateLabelPositions()
{
  double s = static_cast<double>(*size);
  osg::Matrixd m = static_cast<osg::Matrixd>(*csys);
  
  autoSizeLabel->setMatrix(osg::Matrixd::translate(osg::Vec3d(0.0, s, 0.0) * m));
  sizeIP->setMatrix(m);
  flipLabel->setMatrix(osg::Matrixd::translate(osg::Vec3d(s, 0.0, 0.0) * m));
  angleLabel->setMatrix(osg::Matrixd::translate(m.getTrans()));
  
  //offset parameter has to be placed at original position. notice modifying m.
  osg::Vec3d projection = gu::getZVector(m) * static_cast<double>(*offset) * -1.0;
  m.setTrans(m.getTrans() + projection);
  offsetIP->setMatrix(m);
}

QTextStream& DatumPlane::getInfo(QTextStream &streamIn) const 
{
    Inherited::getInfo(streamIn);
    
    streamIn << "System is: " << Qt::endl;
    gu::osgMatrixOut(streamIn, static_cast<osg::Matrixd>(*csys));
    
    return streamIn;
}

//serial support for datum plane needs to be done.
void DatumPlane::serialWrite(const boost::filesystem::path &dIn)
{

  prj::srl::dtps::DatumPlane datumPlaneOut
  (
    Base::serialOut(),
    static_cast<int>(dpType),
    csys->serialOut(),
    flip->serialOut(),
    autoSize->serialOut(),
    size->serialOut(),
    offset->serialOut(),
    angle->serialOut(),
    csysDragger->serialOut(),
    flipLabel->serialOut(),
    autoSizeLabel->serialOut(),
    angleLabel->serialOut(),
    sizeIP->serialOut(),
    offsetIP->serialOut()
  );
  for (const auto &p : picks)
    datumPlaneOut.picks().push_back(p);
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::dtps::datumPlane(stream, datumPlaneOut, infoMap);
}

void DatumPlane::serialRead(const prj::srl::dtps::DatumPlane &dpi)
{
  Base::serialIn(dpi.base());
  dpType = static_cast<DPType>(dpi.dpType());
  csys->serialIn(dpi.csys());
  flip->serialIn(dpi.flip());
  autoSize->serialIn(dpi.autoSize());
  size->serialIn(dpi.size());
  offset->serialIn(dpi.offset());
  angle->serialIn(dpi.angle());
  csysDragger->serialIn(dpi.csysDragger());
  flipLabel->serialIn(dpi.flipLabel());
  autoSizeLabel->serialIn(dpi.autoSizeLabel());
  angleLabel->serialIn(dpi.angleLabel());
  for (const auto &p : dpi.picks())
    picks.emplace_back(p);
  sizeIP->serialIn(dpi.sizeIP());
  offsetIP->serialIn(dpi.offsetIP());

  cachedSize = static_cast<double>(*size);
  mainTransform->setMatrix(static_cast<osg::Matrixd>(*csys));
  
  updateOverlayViz();
  updateLabelPositions();
  updateGeometry();
}
