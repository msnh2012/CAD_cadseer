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
, angle(new prm::Parameter(prm::Names::Angle, 90.0))
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
  csys->connectValue(boost::bind(&DatumPlane::setModelDirty, this));
  
  flip->connectValue(boost::bind(&DatumPlane::setModelDirty, this));
  parameters.push_back(flip.get());
  
  autoSize->connectValue(boost::bind(&DatumPlane::setVisualDirty, this));
  parameters.push_back(autoSize.get());
  
  size->setConstraint(prm::Constraint::buildNonZeroPositive());
  size->connectValue(boost::bind(&DatumPlane::setVisualDirty, this));
  parameters.push_back(size.get());
  
  offset->connectValue(boost::bind(&DatumPlane::setModelDirty, this));
  parameters.push_back(offset.get());
  
  prm::Constraint ac;
  prm::Boundary lower(0.0, prm::Boundary::End::Closed);
  prm::Boundary upper(360.0, prm::Boundary::End::Closed);
  prm::Interval interval(lower, upper);
  ac.intervals.push_back(interval);
  angle->setConstraint(ac);
  angle->connectValue(boost::bind(&DatumPlane::setModelDirty, this));
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
  
  overlaySubSwitch->addChild(csysDragger->dragger);
  
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
    
    if (static_cast<bool>(*flip))
    {
      osg::Matrixd cm = static_cast<osg::Matrixd>(*csys);
      osg::Quat r(osg::PI, gu::getXVector(cm));
      osg::Matrixd nm(cm.getRotate() * r);
      nm.setTrans(cm.getTrans());
      csys->setValueQuiet(nm);
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
  
  if (!(gu::toOcc(normal1).IsParallel(gu::toOcc(normal2), Precision::Angular())))
    throw std::runtime_error("PCenter: planes not parallel");
  
  osg::Vec3d projection = face2System.get().getTrans() - face1System.get().getTrans();
  double mag = projection.length() / 2.0;
  projection.normalize();
  projection *= mag;
  osg::Vec3d freshOrigin = face1System.get().getTrans() + projection;
  
  osg::Matrixd newSystem = face1System.get();
  newSystem.setTrans(freshOrigin);
  csys->setValueQuiet(newSystem);
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
    tempSize = std::max(tempSize, da->getSize());
  }
  else
  {
    assert(rfs.front()->hasAnnex(ann::Type::SeerShape));
    const ann::SeerShape &ss = rfs.front()->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
    std::vector<tls::Resolved> axisResolves = tls::resolvePicks(rfs, picks, pli.shapeHistory);
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
      tempSize = std::max(tempSize, bb.getDiagonal());
    }
    if (axisResolves.size() > 1)
    {
      std::ostringstream s; s << "WARNING: more than one resolved axis in: " << BOOST_CURRENT_FUNCTION << std::endl;
      lastUpdateLog += s.str();
    }
  }
  
  std::vector<const Base*> pfs = pli.getFeatures(DatumPlane::plane);
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
    auto it = picks.begin();
    for (; it != picks.end(); ++it)
    {
      if (it->tag == DatumPlane::plane)
        break;
    }
    if (it == picks.end())
      throw std::runtime_error("Couldn't find plane pick");
    std::vector<tls::Resolved> planeResolves = tls::resolvePicks(pfs.front(), *it, pli.shapeHistory);
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
      tempSize = std::max(tempSize, bb.getDiagonal());
    }
    if (planeResolves.size() > 1)
    {
      std::ostringstream s; s << "WARNING: more than one resolved plane in: " << BOOST_CURRENT_FUNCTION << std::endl;
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

void DatumPlane::updateVisual()
{
  if (static_cast<bool>(*autoSize))
  {
    size->setValueQuiet(cachedSize);
    sizeIP->valueHasChanged();
  }
  
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
  overlaySubSwitch->setChildValue(autoSizeLabel.get(), true);
  overlaySubSwitch->setChildValue(flipLabel.get(), true);
  if (!static_cast<bool>(*autoSize))
    overlaySubSwitch->setChildValue(sizeIP.get(), true);
  if (dpType == DPType::POffset)
    overlaySubSwitch->setChildValue(offsetIP.get(), true);
  if (dpType == DPType::AAngleP)
    overlaySubSwitch->setChildValue(angleLabel.get(), true);
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
