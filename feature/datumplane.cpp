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
#include <boost/variant/variant.hpp>

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

#include <tools/infotools.h>
#include <globalutilities.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <modelviz/nodemaskdefs.h>
#include <library/ipgroup.h>
#include <library/csysdragger.h>
#include <library/plabel.h>
#include <annex/csysdragger.h>
#include <modelviz/datumplane.h>
#include <annex/seershape.h>
#include <feature/updatepayload.h>
#include <feature/inputtype.h>
#include <project/serial/xsdcxxoutput/featuredatumplane.h>
#include <tools/featuretools.h>
#include <tools/occtools.h>
#include <feature/parameter.h>
#include <feature/datumaxis.h>
#include <feature/datumplane.h>

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
};

//default constructed plane is 2.0 x 2.0
DatumPlane::DatumPlane() :
Base()
, dpType(DPType::Constant)
, cachedSize(1.0)
, picks()
, csys(new prm::Parameter(prm::Names::CSys, osg::Matrixd::identity()))
, flip(new prm::Parameter(QObject::tr("Flip"), false))
, autoSize(new prm::Parameter(QObject::tr("Auto Size"), false))
, size(new prm::Parameter(QObject::tr("Size"), 1.0))
, offset(new prm::Parameter(prm::Names::Offset, 1.0))
, angle(new prm::Parameter(prm::Names::Angle, 45.0))
, csysDragger(new ann::CSysDragger(this, csys.get()))
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
  csys->connectValue(std::bind(&DatumPlane::setModelDirty, this));
  
  flip->connectValue(std::bind(&DatumPlane::setModelDirty, this));
  parameters.push_back(flip.get());
  
  autoSize->connectValue(std::bind(&DatumPlane::setModelDirty, this));
  parameters.push_back(autoSize.get());
  
  size->setConstraint(prm::Constraint::buildNonZeroPositive());
  size->connectValue(std::bind(&DatumPlane::setModelDirty, this));
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
  
  flipLabel->showName = true;
  flipLabel->valueHasChanged();
  flipLabel->constantHasChanged();
  overlaySubSwitch->addChild(flipLabel.get());
  
  autoSizeLabel->showName = true;
  autoSizeLabel->valueHasChanged();
  autoSizeLabel->constantHasChanged();
  overlaySubSwitch->addChild(autoSizeLabel.get());
  
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
  
  angleLabel->showName = true;
  angleLabel->valueHasChanged();
  angleLabel->constantHasChanged();
  overlaySubSwitch->addChild(angleLabel.get());
  
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

void DatumPlane::updateModel(const UpdatePayload &pli)
{
  setFailure();
  lastUpdateLog.clear();
  try
  {
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
      csys->setValueQuiet(nm);
    }
    
    if (static_cast<bool>(*autoSize))
    {
      size->setValueQuiet(cachedSize);
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
  std::vector<const Base*> features = pli.getFeatures(ftr::InputType::create);
  if (features.size() != 1)
    throw std::runtime_error("POffset: Wrong number of 'create' inputs");
  
  osg::Matrixd faceSystem;
  faceSystem = osg::Matrixd::identity();
  if (features.front()->getType() == Type::DatumPlane)
  {
    const DatumPlane *inputPlane = dynamic_cast<const DatumPlane*>(features.front());
    assert(inputPlane);
    faceSystem = inputPlane->getSystem();
    
    // just size this plane to the source plane.
    cachedSize = inputPlane->getSize();
  }
  else //look for seer shape.
  {
    if (picks.empty() || picks.front().id.is_nil())
      throw std::runtime_error("POffset: no pick or nil shape id");
    if (!features.front()->hasAnnex(ann::Type::SeerShape))
      throw std::runtime_error("POffset: Parent feature doesn't have seer shape.");
    const ann::SeerShape &shape = features.front()->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
    
    auto resolvedPicks = tls::resolvePicks(features.front(), picks.front(), pli.shapeHistory);
    if (resolvedPicks.front().resultId.is_nil())
    {
      std::ostringstream stream;
      stream << "POffset: can't find target face id. Skipping id: " << gu::idToString(picks.front().id);
      throw std::runtime_error(stream.str());
    }
    assert(shape.hasId(resolvedPicks.front().resultId));
    if (!shape.hasId(resolvedPicks.front().resultId))
      throw std::runtime_error("POffset: seer shape doesn't have id");
      
    const TopoDS_Shape &faceShape = shape.getOCCTShape(resolvedPicks.front().resultId);
    assert(faceShape.ShapeType() == TopAbs_FACE);
    if (faceShape.ShapeType() != TopAbs_FACE)
      throw std::runtime_error("POffset: shape is not a face");
    
    //get coordinate system
    BRepAdaptor_Surface adaptor(TopoDS::Face(faceShape));
    if (adaptor.GetType() != GeomAbs_Plane)
      throw std::runtime_error("POffset: wrong surface type");
    gp_Ax2 tempSystem = adaptor.Plane().Position().Ax2();
    if (faceShape.Orientation() == TopAbs_REVERSED)
      tempSystem.SetDirection(tempSystem.Direction().Reversed());
    faceSystem = gu::toOsg(tempSystem);
    
    //calculate parameter boundaries and project onto plane.
    gp_Pnt centerPoint, cornerPoint;
    centerRadius(getBoundingBox(faceShape), centerPoint, cornerPoint);
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
  csys->setValueQuiet(faceSystem);
}

void DatumPlane::goUpdatePCenter(const UpdatePayload &pli)
{
  std::vector<const Base*> features = pli.getFeatures(InputType::create);
  if (features.empty() || features.size() > 2)
    throw std::runtime_error("PCenter: Wrong number of 'create' inputs");
  
  boost::optional<osg::Matrixd> face1System, face2System;
  
  if (features.size() == 1)
  {
    //note: can't do a center with only 1 datum plane. so we know this condition must be a shape.
    if (!features.front()->hasAnnex(ann::Type::SeerShape))
      throw std::runtime_error("PCenter: null seer shape");
    const ann::SeerShape &shape = features.front()->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
    
    std::vector<uuid> ids;
    auto resolvedPicks = tls::resolvePicks(features, picks, pli.shapeHistory);
    for (const auto &resolved : resolvedPicks)
    {
      if (resolved.resultId.is_nil())
        continue;
      ids.push_back(resolved.resultId);
    }
    if (ids.size() != 2)
      throw std::runtime_error("PCenter: wrong number of resolved ids");
    assert(shape.hasId(ids.front()) && shape.hasId(ids.back()));
    if ((!shape.hasId(ids.front())) || (!shape.hasId(ids.back())))
      throw std::runtime_error("PCenter: ids not in shape.");
    
    TopoDS_Shape face1 = shape.getOCCTShape(ids.front()); assert(!face1.IsNull());
    TopoDS_Shape face2 = shape.getOCCTShape(ids.back()); assert(!face2.IsNull());
    if (face1.IsNull() || face2.IsNull())
      throw std::runtime_error("PCenter: null faces");
    face1System = getFaceSystem(face1);
    face2System = getFaceSystem(face2);
    
    //calculate size.
    cachedSize = std::max(std::sqrt(getBoundingBox(face1).SquareExtent()), std::sqrt(getBoundingBox(face2).SquareExtent())) / 2.0;
  }
  else if (features.size() == 2)
  {
    //with 2 inputs, either one can be a face or a datum.
    double radius1, radius2;
    
    std::vector<const ftr::Base*> featuresToResolve;
    for (const auto &f : features)
    {
      if (f->getType() == Type::DatumPlane)
      {
        const DatumPlane *inputPlane = dynamic_cast<const DatumPlane*>(f);
        assert(inputPlane);
        if (!face1System)
        {
          face1System = inputPlane->getSystem();
          radius1 = inputPlane->getSize();
        }
        else if (!face2System)
        {
          face2System = inputPlane->getSystem();
          radius2 = inputPlane->getSize();
        }
      }
      else if (f->hasAnnex(ann::Type::SeerShape))
        featuresToResolve.push_back(f);
    }
    
    auto resolvedPicks = tls::resolvePicks(featuresToResolve, picks, pli.shapeHistory);
    for (const auto &resolved : resolvedPicks)
    {
      const ann::SeerShape &shape = resolved.feature->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
      assert(shape.hasId(resolved.resultId));
      if (!shape.hasId(resolved.resultId))
        throw std::runtime_error("PCenter: expected id not found in seershape.");
      TopoDS_Shape face = shape.getOCCTShape(resolved.resultId);
      if (face.IsNull())
        throw std::runtime_error("PCenter: input has null shape");
      if (face.ShapeType() != TopAbs_FACE)
        throw std::runtime_error("PCenter: shape is not of type face");
      if (!face1System)
      {
        face1System = getFaceSystem(face);
        radius1 = std::sqrt(getBoundingBox(face).SquareExtent()) / 2.0;
      }
      else if (!face2System)
      {
        face2System = getFaceSystem(face);
        radius2 = std::sqrt(getBoundingBox(face).SquareExtent()) / 2.0;
      }
    }
    
    cachedSize = std::max(radius1, radius2);
  }
  else
    throw std::runtime_error("PCenter: wrong number of inputs");
  
  if ((!face1System) || (!face2System))
    throw std::runtime_error("PCenter: couldn't get both systems.");
  
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
    
  csys->setValueQuiet(newSystem.get());
}

void DatumPlane::goUpdateAAngleP(const UpdatePayload &pli)
{
  boost::optional<osg::Vec3d> axisOrigin, axisDirection, planeNormal;
  double tempSize = 0.0;
  
  std::vector<const Base*> rfs = pli.getFeatures(DatumPlane::rotationAxis);
  if (rfs.size() != 1)
    throw std::runtime_error("AAngleP: Wrong number of 'rotationAxis' inputs");
  if (rfs.front()->getType() == ftr::Type::DatumAxis)
  {
    const DatumAxis *da = dynamic_cast<const DatumAxis*>(rfs.front());
    axisOrigin = da->getOrigin();
    axisDirection = da->getDirection();
    tempSize = std::max(tempSize, da->getSize() / 2.0);
  }
  else
  {
    assert(rfs.front()->hasAnnex(ann::Type::SeerShape));
    const ann::SeerShape &ss = rfs.front()->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
    std::vector<tls::Resolved> axisResolves;
    for (const auto &p : picks)
    {
      if (p.tag == DatumPlane::rotationAxis)
      {
        axisResolves = tls::resolvePicks(rfs.front(), p, pli.shapeHistory);
        break;
      }
    }
    if (!axisResolves.empty())
    {
      //just use first resolve
      if (axisResolves.front().resultId.is_nil())
        throw std::runtime_error("AAngleP: resolved 'rotationAxis' id is nil");
      assert(ss.hasId(axisResolves.front().resultId));
      const TopoDS_Shape &axisShape = ss.getOCCTShape(axisResolves.front().resultId);
      auto glean = occt::gleanAxis(axisShape);
      if (!glean.second)
        throw std::runtime_error("AAngleP: couldn't glean 'rotationAxis'");
      axisOrigin = gu::toOsg(glean.first.Location());
      axisDirection = gu::toOsg(glean.first.Direction());
      occt::BoundingBox bb(axisShape);
      tempSize = std::max(tempSize, bb.getDiagonal() / 2.0);
    }
    if (axisResolves.size() > 1)
    {
      std::ostringstream s; s << "WARNING: more than one resolved axis in: " << std::endl;
      lastUpdateLog += s.str();
    }
  }
  
  std::vector<const Base*> pfs = pli.getFeatures(DatumPlane::plane1);
  if (pfs.size() != 1)
    throw std::runtime_error("AAngleP: Wrong number of 'plane' inputs");
  if (pfs.front()->getType() == ftr::Type::DatumPlane)
  {
    const DatumPlane *dp = dynamic_cast<const DatumPlane*>(pfs.front());
    planeNormal = gu::getZVector(dp->getSystem());
    tempSize = std::max(tempSize, dp->getSize());
  }
  else
  {
    assert(pfs.front()->hasAnnex(ann::Type::SeerShape));
    const ann::SeerShape &ss = pfs.front()->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
    std::vector<tls::Resolved> planeResolves;
    
    for (const auto &p : picks)
    {
      if (p.tag == DatumPlane::plane1)
      {
        planeResolves = tls::resolvePicks(pfs.front(), p, pli.shapeHistory);
        break;
      }
    }
    if (!planeResolves.empty())
    {
      //just use first resolve
      if (planeResolves.front().resultId.is_nil())
        throw std::runtime_error("AAngleP: resolved 'plane' id is nil");
      assert(ss.hasId(planeResolves.front().resultId));
      const TopoDS_Shape &planeShape = ss.getOCCTShape(planeResolves.front().resultId);
      if (planeShape.ShapeType() != TopAbs_FACE)
        throw std::runtime_error("AAngleP: resolved 'plane' shape is not a face");
      planeNormal = gu::getZVector(getFaceSystem(planeShape));
      occt::BoundingBox bb(planeShape);
      tempSize = std::max(tempSize, bb.getDiagonal() / 2.0);
    }
    if (planeResolves.size() > 1)
    {
      std::ostringstream s; s << "WARNING: more than one resolved plane in: " << std::endl;
      lastUpdateLog += s.str();
    }
  }
  
  if (!axisOrigin || !axisDirection || !planeNormal)
    throw std::runtime_error("Error: unresolved parents");
  if ((1.0 - std::fabs(planeNormal.get() * axisDirection.get())) < std::numeric_limits<float>::epsilon())
    throw std::runtime_error("AAngleP: edge and face are parallel");
  if (tempSize <= 0.0)
    tempSize = 1.0;
  cachedSize = tempSize;
  
  osg::Quat rotation(osg::DegreesToRadians(static_cast<double>(*angle)), axisDirection.get());
  osg::Vec3d outNormal(rotation * planeNormal.get());
  osg::Matrixd csysOut = osg::Matrixd::rotate(osg::Vec3d(0.0, 0.0, 1.0), outNormal);
  csysOut.setTrans(axisOrigin.get());
  csys->setValueQuiet(csysOut);
}

void DatumPlane::goUpdateAverage3P(const UpdatePayload &pli)
{
  auto getParentFeature = [&](const char *tagIn) -> const Base*
  {
    std::vector<const Base*> t1 = pli.getFeatures(tagIn);
    if (t1.size() != 1)
      return nullptr;
    return t1.front();
  };
  
  const Base *p1f = getParentFeature(plane1);
  const Base *p2f = getParentFeature(plane2);
  const Base *p3f = getParentFeature(plane3);
  if (!p1f || !p2f || !p3f)
    throw std::runtime_error("Average3P: invalid inputs");
  
  double tSize = 1.0;
  typedef std::vector<opencascade::handle<Geom_Surface>, NCollection_StdAllocator<opencascade::handle<Geom_Surface>>> GeomVector;
  GeomVector planes;
  osg::Vec3d averageNormal(0.0, 0.0, 0.0);
  
  auto resolveSystem = [&](const Base *fIn, const char *tagIn)
  {
    if (!fIn->hasAnnex(ann::Type::SeerShape))
      return;
    boost::optional<Pick> pick;
    for (const auto &p : picks)
    {
      if (p.tag == tagIn)
        pick = p;
    }
    if (!pick)
      return;
    auto resolves = tls::resolvePicks(fIn, pick.get(), pli.shapeHistory);
    if (resolves.empty())
      return;
    if (resolves.size() > 2)
    {
      std::ostringstream s; s << "WARNING: more than one resolved plane" << std::endl;
      lastUpdateLog += s.str();
    }
    const ann::SeerShape &ss = fIn->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
    assert(ss.hasId(resolves.front().resultId));
    const TopoDS_Shape &shape = ss.getOCCTShape(resolves.front().resultId);
    if (shape.ShapeType() != TopAbs_FACE)
      return;
    
    BRepAdaptor_Surface sa(TopoDS::Face(shape));
    if (sa.GetType() == GeomAbs_Plane)
    {
      planes.push_back(sa.Surface().Surface());
      averageNormal += gu::toOsg(sa.Plane().Axis().Direction());
      if (shape.Orientation() == TopAbs_REVERSED)
        averageNormal *= -1.0;
      occt::BoundingBox bb(shape);
      tSize = std::max(tSize, bb.getDiagonal() / 2.0);
    }
  };
  
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
  
  if (p1f->getType() == ftr::Type::DatumPlane)
    resolveDatum(p1f);
  else
    resolveSystem(p1f, plane1);
  
  if (p2f->getType() == ftr::Type::DatumPlane)
    resolveDatum(p2f);
  else
    resolveSystem(p2f, plane2);
  
  if (p3f->getType() == ftr::Type::DatumPlane)
    resolveDatum(p3f);
  else
    resolveSystem(p3f, plane3);
  
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
  csys->setValueQuiet(ns);
  cachedSize = tSize;
}

void DatumPlane::goUpdateThrough3P(const UpdatePayload &pli)
{
  std::vector<osg::Vec3d> points;
  for (const auto &p : picks)
  {
    std::vector<const Base*> features = pli.getFeatures(p.tag);
    if (features.size() != 1)
      continue;
    std::vector<tls::Resolved> res = tls::resolvePicks(features.front(), p, pli.shapeHistory);
    if (res.empty())
      continue;
    if (res.size() > 1)
    {
      std::ostringstream s; s << "WARNING: more than one resolved point" << std::endl;
      lastUpdateLog += s.str();
    }
    if (!res.front().feature->hasAnnex(ann::Type::SeerShape))
      continue;
    const ann::SeerShape &ss = res.front().feature->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
    assert(ss.hasId(res.front().resultId));
    boost::optional<osg::Vec3d> tp;
    if (res.front().pick.selectionType == slc::Type::StartPoint)
    {
      const TopoDS_Shape &v = ss.getOCCTShape(ss.useGetStartVertex(res.front().resultId));
      assert(v.ShapeType() == TopAbs_VERTEX);
      if (!(v.ShapeType() == TopAbs_VERTEX))
        continue;
      points.push_back(gu::toOsg(BRep_Tool::Pnt(TopoDS::Vertex(v))));
    }
    else if (res.front().pick.selectionType == slc::Type::EndPoint)
    {
      const TopoDS_Shape &v = ss.getOCCTShape(ss.useGetEndVertex(res.front().resultId));
      assert(v.ShapeType() == TopAbs_VERTEX);
      if (v.ShapeType() != TopAbs_VERTEX)
        continue;
      points.push_back(gu::toOsg(BRep_Tool::Pnt(TopoDS::Vertex(v))));
    }
    else if (res.front().pick.selectionType == slc::Type::CenterPoint)
    {
      std::vector<osg::Vec3d> centers = ss.useGetCenterPoint(res.front().resultId);
      if (centers.empty())
        continue;
      points.push_back(centers.front());
    }
    else if (res.front().pick.isParameterType())
    {
      const TopoDS_Shape &shape = ss.getOCCTShape(res.front().resultId);
      assert(shape.ShapeType() == TopAbs_EDGE);
      if (shape.ShapeType() != TopAbs_EDGE)
        continue;
      points.push_back(res.front().pick.getPoint(TopoDS::Edge(shape)));
    }
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
  
  csys->setValueQuiet(ts);
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
  
  //offset parameter has to be placed at original position. notice modifing m.
  osg::Vec3d projection = gu::getZVector(m) * static_cast<double>(*offset) * -1.0;
  m.setTrans(m.getTrans() + projection);
  offsetIP->setMatrix(m);
}

QTextStream& DatumPlane::getInfo(QTextStream &streamIn) const 
{
    Inherited::getInfo(streamIn);
    
    streamIn << "System is: " << endl;
    gu::osgMatrixOut(streamIn, static_cast<osg::Matrixd>(*csys));
    
    return streamIn;
}

//serial support for datum plane needs to be done.
void DatumPlane::serialWrite(const boost::filesystem::path &dIn)
{

  prj::srl::FeatureDatumPlane datumPlaneOut
  (
    Base::serialOut(),
    static_cast<int>(dpType),
    ::ftr::serialOut(picks),
    csys->serialOut(),
    flip->serialOut(),
    autoSize->serialOut(),
    size->serialOut(),
    offset->serialOut(),
    angle->serialOut(),
    csysDragger->serialOut(),
    flipLabel->serialOut(),
    autoSizeLabel->serialOut(),
    angleLabel->serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::datumPlane(stream, datumPlaneOut, infoMap);
}

void DatumPlane::serialRead(const prj::srl::FeatureDatumPlane &dpi)
{
  Base::serialIn(dpi.featureBase());
  dpType = static_cast<DPType>(dpi.dpType());
  picks = ::ftr::serialIn(dpi.picks());
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

  cachedSize = static_cast<double>(*size);
  mainTransform->setMatrix(static_cast<osg::Matrixd>(*csys));
  
  sizeIP->valueHasChanged();
  sizeIP->constantHasChanged();
  offsetIP->valueHasChanged();
  offsetIP->constantHasChanged();
  
  updateOverlayViz();
  updateLabelPositions();
  updateGeometry();
}
