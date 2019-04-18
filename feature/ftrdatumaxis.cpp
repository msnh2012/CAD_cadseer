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

#include <boost/filesystem/path.hpp>
#include <boost/optional/optional.hpp>

#include <BRep_Tool.hxx>
#include <TopoDS.hxx>
#include <Geom_Surface.hxx>
#include <Geom_Plane.hxx>
#include <Geom_Line.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <GeomAPI_IntSS.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>

#include <osg/Switch>
#include <osg/MatrixTransform>

#include "globalutilities.h"
#include "tools/idtools.h"
#include "tools/occtools.h"
#include "tools/featuretools.h"
#include "annex/annseershape.h"
#include "modelviz/mdvdatumaxis.h"
#include "library/lbrcsysdragger.h"
#include "library/lbrplabel.h"
#include "annex/anncsysdragger.h"
#include "project/serial/xsdcxxoutput/featuredatumaxis.h"
#include "parameter/prmparameter.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrdatumplane.h"
#include "feature/ftrdatumaxis.h"

using namespace ftr;

using boost::uuids::uuid;

QIcon DatumAxis::icon;

DatumAxis::DatumAxis() : Base()
, axisType(AxisType::Constant)
, picks()
, origin(0.0, 0.0, 0.0)
, direction(0.0, 0.0, 1.0)
, csys(new prm::Parameter(prm::Names::CSys, osg::Matrixd::identity()))
, autoSize(new prm::Parameter(QObject::tr("Auto Size"), false))
, size(new prm::Parameter(QObject::tr("Size"), 20.0))
, csysDragger(new ann::CSysDragger(this, csys.get()))
, cachedSize(20.0)
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionDatumAxis.svg");
  name = QObject::tr("Datum Axis");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  parameters.push_back(csys.get()); //should we add and remove this when type switching? PIA
  csys->connectValue(std::bind(&DatumAxis::setModelDirty, this));
  overlaySwitch->addChild(csysDragger->dragger);
  
  parameters.push_back(autoSize.get());
  autoSize->connectValue(std::bind(&DatumAxis::setVisualDirty, this));
  
  parameters.push_back(size.get());
  size->connectValue(std::bind(&DatumAxis::setVisualDirty, this));
  
  autoSizeLabel = new lbr::PLabel(autoSize.get());
  autoSizeLabel->showName = true;
  autoSizeLabel->valueHasChanged();
  autoSizeLabel->constantHasChanged();
  overlaySwitch->addChild(autoSizeLabel.get());
  
  //size label will get added or removed from overlay as needed.
  sizeLabel = new lbr::PLabel(size.get());
  sizeLabel->showName = true;
  sizeLabel->valueHasChanged();
  sizeLabel->constantHasChanged();
  
  display = new mdv::DatumAxis();
  mainTransform->addChild(display.get());
}

DatumAxis::~DatumAxis() {}

void DatumAxis::setAxisType(AxisType tIn)
{
  if (tIn == axisType)
    return;
  axisType = tIn;
  
  if (axisType == AxisType::Constant)
  {
    //update dragger and parameter to current axis. origin and direction always up to date.
    csysDragger->setCSys(osg::Matrixd::rotate(osg::Vec3d(0.0, 0.0, 1.0), direction) * osg::Matrixd::translate(origin));
    autoSize->setValue(false);
  }
  
  updateDraggerViz();
  updateLabelViz();
  this->setModelDirty();
}

void DatumAxis::setOrigin(const osg::Vec3d &oIn)
{
  axisType = AxisType::Constant;
  origin = oIn;
}

void DatumAxis::setDirection(const osg::Vec3d &dIn)
{
  axisType = AxisType::Constant;

  osg::Vec3d dn = dIn;
  dn.normalize();
  direction = dn;
}

double DatumAxis::getSize() const
{
  return static_cast<double>(*size);
}

void DatumAxis::setPicks(const Picks &pIn)
{
  picks = pIn;
  this->setModelDirty();
}

void DatumAxis::setAutoSize(bool vIn)
{
  autoSize->setValue(vIn);
}

void DatumAxis::updateModel(const UpdatePayload &pli)
{
  setFailure();
  lastUpdateLog.clear();
  try
  {
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    if (axisType == AxisType::Constant)
      goUpdateConstant();
    if (axisType == AxisType::Points)
      goUpdatePoints(pli);
    else if (axisType == AxisType::Intersection)
      goUpdateIntersection(pli);
    else if (axisType == AxisType::Geometry)
      goUpdateGeometry(pli);
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in datum axis update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in datum axis update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in datum axis update. " << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void DatumAxis::goUpdateConstant()
{
  osg::Matrixd t = static_cast<osg::Matrixd>(*csys);
  origin = t.getTrans();
  direction = gu::getZVector(t);
}

void DatumAxis::goUpdatePoints(const UpdatePayload &pli)
{
  if (picks.size() != 2)
    throw std::runtime_error("Wrong number of picks");
  std::vector<tls::Resolved> resolves = tls::resolvePicks(pli.getFeatures(InputType::create), picks, pli.shapeHistory);
  if (resolves.size() != 2)
    throw std::runtime_error("Wrong number of point resolves");
  
  boost::optional<osg::Vec3d> head, tail;
  for (const auto &r : resolves)
  {
    //features without seershapes are not included in results of resolvePicks.
    const ann::SeerShape &ss = r.feature->getAnnex<ann::SeerShape>();
    assert(ss.hasId(r.resultId));
    const TopoDS_Shape &rs = ss.getOCCTShape(r.resultId);
    if (rs.ShapeType() != TopAbs_EDGE)
      throw std::runtime_error("Point not referencing edge");
    boost::optional<osg::Vec3d> p;
    if (r.pick.selectionType == slc::Type::StartPoint)
      p = gu::toOsg(BRep_Tool::Pnt(TopoDS::Vertex(ss.getOCCTShape(ss.useGetStartVertex(r.resultId)))));
    if (r.pick.selectionType == slc::Type::EndPoint)
      p = gu::toOsg(BRep_Tool::Pnt(TopoDS::Vertex(ss.getOCCTShape(ss.useGetEndVertex(r.resultId)))));
    if (r.pick.selectionType == slc::Type::MidPoint)
    {
      auto a = ss.useGetMidPoint(r.resultId);
      if (!a.empty())
        p = a.front();
    }
    if (r.pick.selectionType == slc::Type::CenterPoint)
    {
      auto a = ss.useGetCenterPoint(r.resultId);
      if (!a.empty())
        p = a.front();
    }
    if (r.pick.selectionType == slc::Type::QuadrantPoint || r.pick.selectionType == slc::Type::NearestPoint)
    {
      p = r.pick.getPoint(TopoDS::Edge(rs));
    }
    if (!p)
      throw std::runtime_error("Couldn't derive point from pick");
    if (r.pick == picks.front())
      head = p.get();
    if (r.pick == picks.back())
      tail = p.get();
  }
  
  if (!head)
    throw std::runtime_error("couldn't get head point of direction vector");
  if (!tail)
    throw std::runtime_error("couldn't get tail point of direction vector");
  
  direction = head.get() - tail.get();
  cachedSize = direction.length();
  direction.normalize();
  origin = tail.get() + direction * cachedSize * 0.5;
}

void DatumAxis::goUpdateIntersection(const UpdatePayload &pli)
{
  const std::vector<const Base*> pfs = pli.getFeatures(InputType::create); //parent features.
  
  typedef std::vector<opencascade::handle<Geom_Surface>, NCollection_StdAllocator<opencascade::handle<Geom_Surface>>> GeomVector;
  GeomVector planes;
  double tSize = 1.0;
  osg::Vec3d to; //temp origin. need to project to intersection line.
  for (const auto &pf : pfs)
  {
    if (pf->getType() == Type::DatumPlane)
    {
      const DatumPlane *dp = static_cast<const DatumPlane*>(pf);
      osg::Matrixd dpSys = dp->getSystem();
      gp_Pnt o = gu::toOcc(dpSys.getTrans()).XYZ();
      gp_Dir d = gu::toOcc(gu::getZVector(dpSys));
      planes.push_back(new Geom_Plane(o, d));
      if (dp->getSize() * 2.0 > tSize)
      {
        tSize = dp->getSize() * 2.0;
        to = dpSys.getTrans();
      }
    }
    else
    {
      for (const auto &p : picks)
      {
        if (p.selectionType != slc::Type::Face)
          continue;
        std::vector<tls::Resolved> resolves = tls::resolvePicks(pf, p, pli.shapeHistory);
        for (const auto &r : resolves)
        {
          if (r.resultId.is_nil())
            continue;
          const ann::SeerShape &ss = r.feature->getAnnex<ann::SeerShape>();
          assert(ss.hasId(r.resultId));
          const TopoDS_Shape &fs = ss.getOCCTShape(r.resultId);
          if (fs.ShapeType() != TopAbs_FACE)
          {
            std::ostringstream s; s << "Unexpected shape type, looking for face." << std::endl;
            lastUpdateLog += s.str();
            continue;
          }
          BRepAdaptor_Surface sa(TopoDS::Face(fs));
          if (sa.GetType() != GeomAbs_Plane)
          {
            std::ostringstream s; s << "Unexpected surface type, looking for plane." << std::endl;
            lastUpdateLog += s.str();
            continue;
          }
          planes.push_back(sa.Surface().Surface());
          occt::BoundingBox fbb(fs);
          if (fbb.getDiagonal() > tSize)
          {
            tSize = fbb.getDiagonal();
            to = gu::toOsg(fbb.getCenter());
          }
        }
      }
    }
  }
  if (planes.size() != 2)
    throw std::runtime_error("Wrong number of planes for intersection");
  
  GeomAPI_IntSS intersector(planes.front(), planes.back(), Precision::Confusion());
  if (!intersector.IsDone() || (intersector.NbLines() != 1))
    throw std::runtime_error("Couldn't intersect planes");
  const opencascade::handle<Geom_Curve> &c = intersector.Line(1);
  const opencascade::handle<Geom_Line> &l = dynamic_cast<Geom_Line*>(c.get());
  if (l.IsNull())
    throw std::runtime_error("Couldn't cast intersection results to line");
  
  direction = gu::toOsg(gp_Vec(l->Position().Direction().XYZ()));
  origin = gu::toOsg(l->Position().Location());
  cachedSize = tSize;
  
  //project center of parent geometry onto line and move half of tSize.
  GeomAPI_ProjectPointOnCurve proj(gp_Pnt(gu::toOcc(to).XYZ()), c); 
  if (proj.NbPoints() > 0)
    origin = gu::toOsg(proj.NearestPoint());
}

void DatumAxis::goUpdateGeometry(const UpdatePayload &pli)
{
  const std::vector<const Base*> pfs = pli.getFeatures(InputType::create); //parent features.
  if (pfs.size() != 1)
    throw std::runtime_error("wrong number of parent features");
  if (!pfs.front()->hasAnnex(ann::Type::SeerShape))
    throw std::runtime_error("parent feature doesn't have seershape");
  if (picks.size() != 1)
    throw std::runtime_error("wrong number of picks");
  std::vector<tls::Resolved> resolves = tls::resolvePicks(pfs.front(), picks.front(), pli.shapeHistory);
  for (const auto &r : resolves)
  {
    if (r.resultId.is_nil())
      continue;
    const ann::SeerShape &ss = r.feature->getAnnex<ann::SeerShape>();
    const TopoDS_Shape &s = ss.getOCCTShape(r.resultId);
    auto axisPair = occt::gleanAxis(s);
    if (!axisPair.second)
      continue;
    gp_Ax1 axis = axisPair.first;
    origin = gu::toOsg(axis.Location());
    direction = gu::toOsg(axis.Direction());
    
    occt::BoundingBox bb(s);
    cachedSize = bb.getDiagonal();
    gp_Pnt center = bb.getCenter();
    opencascade::handle<Geom_Line> pLine = new Geom_Line(axis);
    GeomAPI_ProjectPointOnCurve proj(center, pLine); 
    if (proj.NbPoints() > 0)
      origin = gu::toOsg(proj.NearestPoint());
    break;
  }
}

void DatumAxis::updateVisual()
{
  updateVisualInternal();
  setVisualClean();
}

void DatumAxis::updateVisualInternal()
{
  if (static_cast<bool>(*autoSize))
  {
    size->setValueQuiet(cachedSize);
    sizeLabel->valueHasChanged();
  }
  
  double ts = static_cast<double>(*size);
  
  display->setHeight(ts);
  osg::Matrixd rotation = osg::Matrixd::rotate(osg::Vec3d(0.0, 0.0, 1.0), direction);
  if (axisType != AxisType::Constant) //dragger will update main transform.
    mainTransform->setMatrix(osg::Matrixd::translate(osg::Vec3d(0.0, 0.0, ts * -0.5)) * rotation * osg::Matrixd::translate(origin));
  
  sizeLabel->setMatrix(osg::Matrixd::translate(osg::Vec3d(0.0, 0.0, ts) * mainTransform->getMatrix()));
  autoSizeLabel->setMatrix(osg::Matrixd::translate(osg::Vec3d(0.0, 0.0, ts * 0.5) * mainTransform->getMatrix()));
  
  updateLabelViz();
}

void DatumAxis::updateDraggerViz()
{
  if (axisType == AxisType::Constant && !overlaySwitch->containsNode(csysDragger->dragger))
    overlaySwitch->addChild(csysDragger->dragger);
  if (axisType != AxisType::Constant && overlaySwitch->containsNode(csysDragger->dragger))
    overlaySwitch->removeChild(csysDragger->dragger);
}

void DatumAxis::updateLabelViz()
{
  if (axisType == AxisType::Constant && overlaySwitch->containsNode(autoSizeLabel.get()))
    overlaySwitch->removeChild(autoSizeLabel.get());
  if (axisType != AxisType::Constant && (!overlaySwitch->containsNode(autoSizeLabel.get())))
    overlaySwitch->addChild(autoSizeLabel.get());
  
  if (static_cast<bool>(*autoSize) && (overlaySwitch->containsNode(sizeLabel.get())))
    overlaySwitch->removeChild(sizeLabel.get());
  if (!static_cast<bool>(*autoSize) && (!overlaySwitch->containsNode(sizeLabel.get())))
    overlaySwitch->addChild(sizeLabel.get());
}

void DatumAxis::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::FeatureDatumAxis dao
  (
    Base::serialOut(),
    static_cast<int>(axisType),
    ::ftr::serialOut(picks),
    prj::srl::Vec3d(origin.x(), origin.y(), origin.z()),
    prj::srl::Vec3d(direction.x(), direction.y(), direction.z()),
    csys->serialOut(),
    autoSize->serialOut(),
    size->serialOut(),
    csysDragger->serialOut(),
    autoSizeLabel->serialOut(),
    sizeLabel->serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::datumAxis(stream, dao, infoMap);
}

void DatumAxis::serialRead(const prj::srl::FeatureDatumAxis &dai)
{
  Base::serialIn(dai.featureBase());
  axisType = static_cast<AxisType>(dai.axisType());
  picks = ftr::serialIn(dai.picks());
  origin = osg::Vec3d(dai.origin().x(), dai.origin().y(), dai.origin().z());
  direction = osg::Vec3d(dai.direction().x(), dai.direction().y(), dai.direction().z());
  csys->serialIn(dai.csys());
  autoSize->serialIn(dai.autoSize());
  size->serialIn(dai.size());
  csysDragger->serialIn(dai.csysDragger());
  autoSizeLabel->serialIn(dai.autoSizeLabel());
  sizeLabel->serialIn(dai.sizeLabel());
  
  cachedSize = static_cast<double>(*size);
  updateDraggerViz();
  updateVisualInternal();
  if (axisType == AxisType::Constant)
    mainTransform->setMatrix(static_cast<osg::Matrixd>(*csys));
}
