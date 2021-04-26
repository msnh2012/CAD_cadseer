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
#include "tools/tlsosgtools.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "feature/ftrdatumaxis.h"
#include "feature/ftrdatumplane.h"

using namespace ftr::DatumPlane;

QIcon Feature::icon;

using boost::uuids::uuid;

//throws std::runtime_error
static std::pair<osg::Matrixd, double> getFaceSystem(const TopoDS_Shape &faceShape)
{
  assert(faceShape.ShapeType() == TopAbs_FACE);
  BRepAdaptor_Surface adaptor(TopoDS::Face(faceShape));
  if (adaptor.GetType() != GeomAbs_Plane)
    throw std::runtime_error("DatumPlane: wrong surface type");
  gp_Ax2 tempSystem = adaptor.Plane().Position().Ax2();
  if (faceShape.Orientation() == TopAbs_REVERSED)
    tempSystem.SetDirection(tempSystem.Direction().Reversed());
  osg::Matrixd faceSystem = gu::toOsg(tempSystem);
  
  //get center
  double uMin = adaptor.FirstUParameter();
  double uMax = adaptor.LastUParameter();
  double uRange = uMax - uMin;
  double vMin = adaptor.FirstVParameter();
  double vMax = adaptor.LastVParameter();
  double vRange = vMax - vMin;
  
  double uMid = uRange / 2.0 + uMin;
  double vMid = vRange / 2.0 + vMin;
  gp_Pnt centerPoint = adaptor.Value(uMid, vMid);
  faceSystem.setTrans(gu::toOsg(centerPoint));
  
  //get size.
  double size = osg::Vec2(uRange, vRange).length() / 2.0;
  return std::make_pair(faceSystem, size);
}

struct Feature::Stow
{
  Feature &feature;
  
  prm::Parameter dpType; //!< datum plane type
  prm::Parameter csys; //!< coordinate system constant dragger mode.
  prm::Parameter flip; //!< bool. reverse normal
  prm::Parameter autoSize; //!< bool. auto calculate radius.
  prm::Parameter size; //!< double. distance to edges.
  prm::Parameter offset; //!< double. distance for Offset
  prm::Parameter angle; //!< double. angle for rotation
  prm::Parameter linkPicks;
  prm::Parameter offsetPicks;
  prm::Parameter centerPicks;
  prm::Parameter axisAnglePicks;
  prm::Parameter averagePicks;
  prm::Parameter pointsPicks;
  prm::Observer dirtyObserver;
  prm::Observer syncObserver;
  ann::CSysDragger csysDragger; //!< for constant type
  osg::ref_ptr<lbr::PLabel> flipLabel;
  osg::ref_ptr<lbr::PLabel> autoSizeLabel;
  osg::ref_ptr<lbr::IPGroup> sizeIP; //!< for POffset
  osg::ref_ptr<lbr::IPGroup> offsetIP; //!< for POffset
  osg::ref_ptr<lbr::PLabel> angleLabel;
  osg::ref_ptr<mdv::DatumPlane> display;
  double cachedSize; //!< for auto calc.
  
  Stow() = delete;
  Stow(Feature &featureIn)
  : feature(featureIn)
  , dpType(QObject::tr("Type"), 0, PrmTags::datumPlaneType)
  , csys(prm::Names::CSys, osg::Matrixd::identity(), prm::Tags::CSys)
  , flip(QObject::tr("Flip"), false)
  , autoSize(prm::Names::AutoSize, false, prm::Tags::AutoSize)
  , size(prm::Names::Size, 1.0, prm::Tags::Size)
  , offset(prm::Names::Offset, 1.0, prm::Tags::Offset)
  , angle(prm::Names::Angle, 45.0, prm::Tags::Angle)
  , linkPicks(prm::Names::CSysLinked, Picks(), prm::Tags::CSysLinked)
  , offsetPicks(QObject::tr("Offset Picks"), Picks(), PrmTags::offsetPicks)
  , centerPicks(QObject::tr("Center Picks"), Picks(), PrmTags::centerPicks)
  , axisAnglePicks(QObject::tr("Axis Angle Picks"), Picks(), PrmTags::axisAnglePicks)
  , averagePicks(QObject::tr("Average Picks"), Picks(), PrmTags::averagePicks)
  , pointsPicks(QObject::tr("Point Picks"), Picks(), PrmTags::pointsPicks)
  , dirtyObserver(std::bind(&Feature::setModelDirty, &feature))
  , syncObserver(std::bind(&Stow::prmActiveSync, this))
  , csysDragger(&feature, &csys)
  , flipLabel(new lbr::PLabel(&flip))
  , autoSizeLabel(new lbr::PLabel(&autoSize))
  , sizeIP(new lbr::IPGroup(&size))
  , offsetIP(new lbr::IPGroup(&offset))
  , angleLabel(new lbr::PLabel(&angle))
  , display(new mdv::DatumPlane())
  , cachedSize(1.0)
  {
    QStringList tStrings = //keep in sync with enum in header.
    {
      QObject::tr("Constant")
      , QObject::tr("Link")
      , QObject::tr("Offset")
      , QObject::tr("Center Bisect")
      , QObject::tr("Axis Angle")
      , QObject::tr("Average")
      , QObject::tr("Points")
    };
    dpType.setEnumeration(tStrings);
    prm::Constraint tConstraint = prm::Constraint::buildUnit();
    tConstraint.intervals.front().upper.value = static_cast<double>(tStrings.size() - 1);
    dpType.setConstraint(tConstraint);
    dpType.connect(dirtyObserver);
    dpType.connect(syncObserver);
    feature.parameters.push_back(&dpType);
    
    csys.connect(dirtyObserver);
    feature.parameters.push_back(&csys);
    
    flip.connect(dirtyObserver);
    feature.parameters.push_back(&flip);
    
    autoSize.connect(dirtyObserver);
    autoSize.connect(syncObserver);
    feature.parameters.push_back(&autoSize);
    
    size.setConstraint(prm::Constraint::buildNonZeroPositive());
    size.connect(dirtyObserver);
    feature.parameters.push_back(&size);
    
    offset.connect(dirtyObserver);
    feature.parameters.push_back(&offset);
    
    prm::Constraint ac;
    prm::Boundary lower(0.0, prm::Boundary::End::Closed);
    prm::Boundary upper(360.0, prm::Boundary::End::Closed);
    prm::Interval interval(lower, upper);
    ac.intervals.push_back(interval);
    angle.setConstraint(ac);
    angle.connect(dirtyObserver);
    feature.parameters.push_back(&angle);
    
    auto goPickInit = [&](prm::Parameter &prm)
    {
      prm.setExpressionLinkable(false);
      prm.connect(dirtyObserver);
      feature.parameters.push_back(&prm);
    };
    goPickInit(linkPicks);
    goPickInit(offsetPicks);
    goPickInit(centerPicks);
    goPickInit(axisAnglePicks);
    goPickInit(averagePicks);
    goPickInit(pointsPicks);

    feature.overlaySwitch->addChild(flipLabel.get());
    feature.overlaySwitch->addChild(autoSizeLabel.get());
    feature.overlaySwitch->addChild(angleLabel.get());
    
    sizeIP->setMatrixDims(osg::Matrixd::rotate(osg::PI, osg::Vec3d(0.0, 0.0, 1.0)));
    sizeIP->noAutoRotateDragger();
    sizeIP->setRotationAxis(osg::Vec3d(0.0, 1.0, 0.0), osg::Vec3d(0.0, 0.0, 1.0));
    sizeIP->setMatrixDragger(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(1.0, 0.0, 0.0)));
    feature.overlaySwitch->addChild(sizeIP.get());
    
    offsetIP->setMatrixDims(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(1.0, 0.0, 0.0)));
    offsetIP->noAutoRotateDragger();
    offsetIP->setRotationAxis(osg::Vec3d(0.0, 0.0, 1.0), osg::Vec3d(0.0, 1.0, 0.0));
    feature.overlaySwitch->addChild(offsetIP.get());
    
    feature.annexes.insert(std::make_pair(ann::Type::CSysDragger, &csysDragger));
    feature.overlaySwitch->addChild(csysDragger.dragger.get());
    
    feature.mainTransform->addChild(display.get());
  }
  Stow(const Stow&) = delete;
  Stow(const Stow&&) = delete;
  
  void updateGeometry()
  {
    double r = size.getDouble();
    display->setParameters(-r, r, -r, r);
  }
  void prmActiveSync()
  {
    //we set autosize off for constant and link. If user changes to 
    //others where autosize is applicable, they will have to turn it back on.
    prm::ObserverBlocker block(dirtyObserver); //callers responsible for dirtyModel.
    prm::ObserverBlocker block2(syncObserver); //don't recurse.
    
    switch (dpType.getInt())
    {
      case static_cast<int>(DPType::Constant):
        csysDragger.draggerUpdate();
        csys.setActive(true);
        autoSize.setValue(false);
        autoSize.setActive(false);
        size.setActive(true);
        offset.setActive(false);
        angle.setActive(false);
        linkPicks.setActive(false);
        offsetPicks.setActive(false);
        centerPicks.setActive(false);
        axisAnglePicks.setActive(false);
        averagePicks.setActive(false);
        pointsPicks.setActive(false);
        break;
      case static_cast<int>(DPType::Linked):
        csys.setActive(false);
        autoSize.setValue(false);
        autoSize.setActive(false);
        size.setActive(true);
        offset.setActive(false);
        angle.setActive(false);
        linkPicks.setActive(true);
        offsetPicks.setActive(false);
        centerPicks.setActive(false);
        axisAnglePicks.setActive(false);
        averagePicks.setActive(false);
        pointsPicks.setActive(false);
        break;
      case static_cast<int>(DPType::Offset):
        csys.setActive(false);
        autoSize.setActive(true);
        if (autoSize.getBool())
          size.setActive(false);
        else
          size.setActive(true);
        offset.setActive(true);
        angle.setActive(false);
        linkPicks.setActive(false);
        offsetPicks.setActive(true);
        centerPicks.setActive(false);
        axisAnglePicks.setActive(false);
        averagePicks.setActive(false);
        pointsPicks.setActive(false);
        break;
      case static_cast<int>(DPType::Center):
        csys.setActive(false);
        autoSize.setActive(true);
        if (autoSize.getBool())
          size.setActive(false);
        else
          size.setActive(true);
        offset.setActive(false);
        angle.setActive(false);
        linkPicks.setActive(false);
        offsetPicks.setActive(false);
        centerPicks.setActive(true);
        axisAnglePicks.setActive(false);
        averagePicks.setActive(false);
        pointsPicks.setActive(false);
        break;
      case static_cast<int>(DPType::AxisAngle):
        csys.setActive(false);
        autoSize.setActive(true);
        if (autoSize.getBool())
          size.setActive(false);
        else
          size.setActive(true);
        offset.setActive(false);
        angle.setActive(true);
        linkPicks.setActive(false);
        offsetPicks.setActive(false);
        centerPicks.setActive(false);
        axisAnglePicks.setActive(true);
        averagePicks.setActive(false);
        pointsPicks.setActive(false);
        break;
      case static_cast<int>(DPType::Average):
        csys.setActive(false);
        autoSize.setActive(true);
        if (autoSize.getBool())
          size.setActive(false);
        else
          size.setActive(true);
        offset.setActive(false);
        angle.setActive(false);
        linkPicks.setActive(false);
        offsetPicks.setActive(false);
        centerPicks.setActive(false);
        axisAnglePicks.setActive(false);
        averagePicks.setActive(true);
        pointsPicks.setActive(false);
        break;
      case static_cast<int>(DPType::Points):
        csys.setActive(false);
        autoSize.setActive(true);
        if (autoSize.getBool())
          size.setActive(false);
        else
          size.setActive(true);
        offset.setActive(false);
        angle.setActive(false);
        linkPicks.setActive(false);
        offsetPicks.setActive(false);
        centerPicks.setActive(false);
        axisAnglePicks.setActive(false);
        averagePicks.setActive(false);
        pointsPicks.setActive(true);
        break;
      default:
        throw std::runtime_error("Unrecognized Datum Plane Type");
        break;
    }
  }
  
  void updateLabelPositions()
  {
    double s = size.getDouble();
    osg::Matrixd m = csys.getMatrix();
    
    if (flip.getBool())
    {
      osg::Quat r(osg::PI, gu::getXVector(m));
      osg::Matrixd nm(m.getRotate() * r);
      nm.setTrans(m.getTrans());
      m = nm;
    }
    
    autoSizeLabel->setMatrix(osg::Matrixd::translate(osg::Vec3d(0.0, s, 0.0) * m));
    sizeIP->setMatrix(m);
    flipLabel->setMatrix(osg::Matrixd::translate(osg::Vec3d(s, 0.0, 0.0) * m));
    angleLabel->setMatrix(osg::Matrixd::translate(m.getTrans()));
    
    //offset parameter has to be placed at original position. notice modifying m.
    osg::Vec3d projection = gu::getZVector(m) * offset.getDouble() * -1.0;
    m.setTrans(m.getTrans() + projection);
    offsetIP->setMatrix(m);
  }
  
  //throws for no system. no size is just a warning.
  std::tuple<osg::Matrixd, double> getSystemAndSize(const tls::Resolver &pr)
  {
    osg::Matrixd matrixOut;
    double sizeOut;
    
    if (slc::isObjectType(pr.getPick().selectionType))
    {
      auto csysPrms = pr.getFeature()->getParameters(prm::Tags::CSys);
      if (csysPrms.empty())
        throw std::runtime_error("Couldn't get csys parameter from object pick");
      matrixOut = csysPrms.front()->getMatrix();
      
      auto sizePrms = pr.getFeature()->getParameters(prm::Tags::Size);
      if (sizePrms.empty())
      {
        std::ostringstream s;
        s << "WARNING: Couldn't get size parameter from object pick" << std::endl;
        feature.lastUpdateLog += s.str();
        sizeOut = cachedSize;
      }
      else
      {
        sizeOut = sizePrms.front()->getDouble();
      }
    }
    else
    {
      auto rShapes = pr.getShapes();
      if (rShapes.empty())
        throw std::runtime_error("Resolved shapes are empty");
      if (rShapes.size() > 1)
      {
        std::ostringstream s; s << "WARNING: Multiple resolve shapes. Using first" << std::endl;
        feature.lastUpdateLog += s.str();
      }
      if (rShapes.front().ShapeType() != TopAbs_FACE)
        throw std::runtime_error("Resolved shape is not a face.");
      
      std::tie(matrixOut, sizeOut) = getFaceSystem(rShapes.front());
    }
    return std::make_tuple(matrixOut, sizeOut);
  }
  
  void goUpdateConstant()
  {
    
  }
  
  void goUpdateLink(const UpdatePayload &pli)
  {
    auto linkedFeatures = pli.getFeatures(InputType::linkCSys);
    if (linkedFeatures.empty())
      throw std::runtime_error("No Features for linked csys");
    auto systemParameters = linkedFeatures.front()->getParameters(prm::Tags::CSys);
    if (systemParameters.empty())
      throw std::runtime_error("Feature for csys link, doesn't have csys parameter");
    
    osg::Matrixd newSys = systemParameters.front()->getMatrix();

    prm::ObserverBlocker block(dirtyObserver);
    csys.setValue(newSys);
  }

  void goUpdatePOffset(const UpdatePayload &pli)
  {
    const auto &tPicks = offsetPicks.getPicks();
    if (tPicks.size() != 1)
      throw std::runtime_error("POffset: Wrong number of picks");
    
    tls::Resolver pr(pli);
    if (!pr.resolve(tPicks.front()))
      throw std::runtime_error("POffset: Pick resolution failed");
    
    osg::Matrixd faceSystem = osg::Matrixd::identity();
    std::tie(faceSystem, cachedSize) = getSystemAndSize(pr);
    osg::Vec3d normal = gu::getZVector(faceSystem) * offset.getDouble();
    faceSystem.setTrans(faceSystem.getTrans() + normal);
    csys.setValue(faceSystem);
  }

  void goUpdatePCenter(const UpdatePayload &pli)
  {
    const auto &tPicks = centerPicks.getPicks();
    if (tPicks.size() != 2)
      throw std::runtime_error("Wrong number of picks for center datum");
    tls::Resolver r0(pli);
    if (!r0.resolve(tPicks.front()))
      throw std::runtime_error("Couldn't resolve pick 0 for center datum");
    tls::Resolver r1(pli);
    if (!r1.resolve(tPicks.back()))
      throw std::runtime_error("Couldn't resolve pick 1 for center datum");
    
    osg::Matrixd face1System, face2System;
    double size1, size2;
    std::tie(face1System, size1) = getSystemAndSize(r0);
    std::tie(face2System, size2) = getSystemAndSize(r1);
    cachedSize = std::max(size1, size2);

    osg::Vec3d normal1 = gu::getZVector(face1System);
    osg::Vec3d normal2 = gu::getZVector(face2System);
    boost::optional<osg::Matrixd> newSystem;
    
    if ((gu::toOcc(normal1).IsParallel(gu::toOcc(normal2), Precision::Angular())))
    {
      osg::Vec3d projection = face2System.getTrans() - face1System.getTrans();
      double mag = projection.length() / 2.0;
      projection.normalize();
      projection *= mag;
      osg::Vec3d freshOrigin = face1System.getTrans() + projection;
      
      newSystem = face1System;
      newSystem.get().setTrans(freshOrigin);
    }
    else //try to make a bisector
    {
      gp_Pnt o1 = gu::toOcc(face1System.getTrans()).XYZ();
      gp_Dir d1 = gu::toOcc(normal1);
      opencascade::handle<Geom_Surface> surface1 = new Geom_Plane(o1, d1);
      gp_Pnt o2 = gu::toOcc(face2System.getTrans()).XYZ();
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
      GeomAPI_ProjectPointOnCurve proj1(gp_Pnt(gu::toOcc(face1System.getTrans()).XYZ()), c); 
      if (proj1.NbPoints() > 0)
        p1 = gu::toOsg(proj1.NearestPoint());
      GeomAPI_ProjectPointOnCurve proj2(gp_Pnt(gu::toOcc(face2System.getTrans()).XYZ()), c); 
      if (proj2.NbPoints() > 0)
        p2 = gu::toOsg(proj2.NearestPoint());
      if (p1 && p2)
      {
        osg::Vec3d projection = p2.get() - p1.get();
        ts.setTrans(p1.get() + projection * 0.5);
      }
      else
      {
        std::ostringstream s; s << "WARNING: couldn't project face origin onto intersection of planes" << std::endl;
        feature.lastUpdateLog += s.str();
        ts.setTrans(lo);
      }
      newSystem = ts;
    }
    
    if (!newSystem)
      throw std::runtime_error("PCenter: couldn't derive center datum");
    
    csys.setValue(newSystem.get());
  }

  void goUpdateAAngleP(const UpdatePayload &pli)
  {
    const auto &tPicks = axisAnglePicks.getPicks();
    if (tPicks.size() != 2)
      throw std::runtime_error("AAngleP: Wrong number of picks");
    
    boost::optional<osg::Vec3d> axisOrigin, axisDirection, planeNormal;
    double tempSize = 0.0;
    tls::Resolver pr(pli);
    for (const auto &p : tPicks)
    {
      if (!pr.resolve(p))
        throw std::runtime_error("AAngleP: Failed to resolve pick");
      
      if (p.tag == InputTags::plane)
      {
        osg::Matrixd localSystem;
        double localSize;
        std::tie(localSystem, localSize) = getSystemAndSize(pr);
        planeNormal = gu::getZVector(localSystem);
        tempSize = std::max(tempSize, localSize);
      }
      else if (p.tag == InputTags::axis)
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
            feature.lastUpdateLog += s.str();
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
    
    osg::Quat rotation(osg::DegreesToRadians(angle.getDouble()), axisDirection.get());
    osg::Vec3d outNormal(rotation * planeNormal.get());
    
    std::array<std::optional<osg::Vec3d>, 4> axesOut =
    {
      axisDirection.get()
      , std::nullopt
      , outNormal
      , axisOrigin.get()
    };
    auto oOut = tls::matrixFromAxes(axesOut);
    if (!oOut)
      throw std::runtime_error("AAngleP: couldn't build CSys");
    csys.setValue(*oOut);
  }

  void goUpdateAverage3P(const UpdatePayload &pli)
  {
    const auto &tPicks = averagePicks.getPicks();
    if (tPicks.size() != 3)
      throw std::runtime_error("Average3P: Wrong number of picks");
    
    typedef std::vector<opencascade::handle<Geom_Surface>, NCollection_StdAllocator<opencascade::handle<Geom_Surface>>> GeomVector;
    GeomVector planes;
    osg::Vec3d averageNormal(0.0, 0.0, 0.0);
    double tSize = 1.0;
    auto resolveDatum = [&](const Base *fIn)
    {
      const Feature *dp = dynamic_cast<const Feature*>(fIn);
      assert(dp);
      
      osg::Matrixd dpSys = dp->getParameters(prm::Tags::CSys).front()->getMatrix();
      averageNormal += gu::getZVector(dpSys);
      gp_Pnt o = gu::toOcc(dpSys.getTrans()).XYZ();
      gp_Dir d = gu::toOcc(gu::getZVector(dpSys));
      planes.push_back(new Geom_Plane(o, d));
      tSize = std::max(tSize, dp->getParameters(prm::Tags::Size).front()->getDouble());
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
    for (const auto &p : tPicks)
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
          feature.lastUpdateLog += s.str();
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
    csys.setValue(ns);
    cachedSize = tSize;
  }

  void goUpdateThrough3P(const UpdatePayload &pli)
  {
    const auto &tPicks = pointsPicks.getPicks();
    if (tPicks.size() != 3)
      throw std::runtime_error("Through3P: Wrong number of picks");
    
    tls::Resolver pr(pli);
    std::vector<osg::Vec3d> points;
    for (const auto &p : tPicks)
    {
      if (!pr.resolve(p))
        throw std::runtime_error("Through3P: Pick resolution failed");
      auto tps = pr.getPoints();
      if (tps.empty())
        throw std::runtime_error("Through3P: No resolved points");
      if (tps.size() > 1)
      {
        std::ostringstream s; s << "WARNING: Through3P: more than one resolved point. Using first" << std::endl;
        feature.lastUpdateLog += s.str();
      }
      points.push_back(tps.front());
    }
    
    if (points.size() != 3)
      throw std::runtime_error("Through3P: couldn't get 3 points");
    std::array<std::optional<osg::Vec3d>, 4> oPoints;
    oPoints[0] = points[0];
    oPoints[1] = points[1];
    oPoints[2] = points[2];
    
    auto om = tls::matrixFromPoints(oPoints);
    if (!om)
      throw std::runtime_error("failed getting plane from 3 points");
    
    osg::Vec3d center = points.at(0) + points.at(1) + points.at(2);
    center /= 3.0;
    om->setTrans(center);
    
    csys.setValue(*om);
    const auto &p = points;
    const auto &c = center;
    cachedSize = std::max(std::max((p[0] - c).length(), (p[1] - c).length()), (p[2] - c).length());
  }
};

//default constructed plane is 2.0 x 2.0
Feature::Feature() :
Base()
, stow(std::make_unique<Stow>(*this))
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionDatumPlane.svg");
  
  name = QObject::tr("Datum Plane");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  stow->prmActiveSync();
  stow->updateGeometry();
}

Feature::~Feature(){}

void Feature::updateModel(const UpdatePayload &pli)
{
  setFailure();
  lastUpdateLog.clear();
  try
  {
    prm::ObserverBlocker block(stow->dirtyObserver);
    
    switch (stow->dpType.getInt())
    {
      case 0:
      {
        stow->goUpdateConstant();
        break;
      }
      case 1:
      {
        stow->goUpdateLink(pli);
        break;
      }
      case 2:
      {
        stow->goUpdatePOffset(pli);
        break;
      }
      case 3:
      {
        stow->goUpdatePCenter(pli);
        break;
      }
      case 4:
      {
        stow->goUpdateAAngleP(pli);
        break;
      }
      case 5:
      {
        stow->goUpdateAverage3P(pli);
        break;
      }
      case 6:
      {
        stow->goUpdateThrough3P(pli);
        break;
      }
      default:
      {
        throw::std::runtime_error("Unrecognized Datum Plane Type");
        break;
      }
    }
    
    if (stow->flip.getBool())
    {
      osg::Matrixd cm = stow->csys.getMatrix();
      osg::Quat r(osg::PI, gu::getXVector(cm));
      osg::Matrixd nm(cm.getRotate() * r);
      nm.setTrans(cm.getTrans());
      stow->csys.setValue(nm);
    }
    
    if (stow->autoSize.getBool())
      stow->size.setValue(stow->cachedSize);
    
    mainTransform->setMatrix(stow->csys.getMatrix());
    
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

void Feature::updateVisual()
{
  stow->updateGeometry();
  stow->updateLabelPositions();
  setVisualClean();
}

QTextStream& Feature::getInfo(QTextStream &streamIn) const 
{
    Base::getInfo(streamIn);
    
    streamIn << "System is: " << Qt::endl;
    gu::osgMatrixOut(streamIn, stow->csys.getMatrix());
    
    return streamIn;
}

void Feature::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::dtps::DatumPlane datumPlaneOut
  (
    Base::serialOut(),
    stow->dpType.serialOut(),
    stow->csys.serialOut(),
    stow->flip.serialOut(),
    stow->autoSize.serialOut(),
    stow->size.serialOut(),
    stow->offset.serialOut(),
    stow->angle.serialOut(),
    stow->csysDragger.serialOut(),
    stow->flipLabel->serialOut(),
    stow->autoSizeLabel->serialOut(),
    stow->angleLabel->serialOut(),
    stow->sizeIP->serialOut(),
    stow->offsetIP->serialOut()
  );
  
  switch (stow->dpType.getInt())
  {
    case 1: datumPlaneOut.picks() = stow->linkPicks.serialOut(); break;
    case 2: datumPlaneOut.picks() = stow->offsetPicks.serialOut(); break;
    case 3: datumPlaneOut.picks() = stow->centerPicks.serialOut(); break;
    case 4: datumPlaneOut.picks() = stow->axisAnglePicks.serialOut(); break;
    case 5: datumPlaneOut.picks() = stow->averagePicks.serialOut(); break;
    case 6: datumPlaneOut.picks() = stow->pointsPicks.serialOut(); break;
  }
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::dtps::datumPlane(stream, datumPlaneOut, infoMap);
}

void Feature::serialRead(const prj::srl::dtps::DatumPlane &dpi)
{
  Base::serialIn(dpi.base());
  stow->dpType.serialIn(dpi.dpType());
  stow->csys.serialIn(dpi.csys());
  stow->flip.serialIn(dpi.flip());
  stow->autoSize.serialIn(dpi.autoSize());
  stow->size.serialIn(dpi.size());
  stow->offset.serialIn(dpi.offset());
  stow->angle.serialIn(dpi.angle());
  stow->csysDragger.serialIn(dpi.csysDragger());
  stow->flipLabel->serialIn(dpi.flipLabel());
  stow->autoSizeLabel->serialIn(dpi.autoSizeLabel());
  stow->angleLabel->serialIn(dpi.angleLabel());
  stow->sizeIP->serialIn(dpi.sizeIP());
  stow->offsetIP->serialIn(dpi.offsetIP());
  
  if (dpi.picks())
  {
    switch (stow->dpType.getInt())
    {
      case 1: stow->linkPicks.serialIn(dpi.picks().get()); break;
      case 2: stow->offsetPicks.serialIn(dpi.picks().get()); break;
      case 3: stow->centerPicks.serialIn(dpi.picks().get()); break;
      case 4: stow->axisAnglePicks.serialIn(dpi.picks().get()); break;
      case 5: stow->averagePicks.serialIn(dpi.picks().get()); break;
      case 6: stow->pointsPicks.serialIn(dpi.picks().get()); break;
    }
  }

  stow->cachedSize = stow->size.getDouble();
  mainTransform->setMatrix(stow->csys.getMatrix());
  
//   stow->updateLabelPositions();
  stow->updateGeometry();
}
