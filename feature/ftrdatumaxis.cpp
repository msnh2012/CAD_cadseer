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
#include "project/serial/generated/prjsrldtasdatumaxis.h"
#include "parameter/prmconstants.h"
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
, csys(std::make_unique<prm::Parameter>(prm::Names::CSys, osg::Matrixd::identity(), prm::Tags::CSys))
, autoSize(std::make_unique<prm::Parameter>(prm::Names::AutoSize, false, prm::Tags::AutoSize))
, size(std::make_unique<prm::Parameter>(prm::Names::Size, 20.0, prm::Tags::Size))
, prmObserver(std::make_unique<prm::Observer>(std::bind(&DatumAxis::setVisualDirty, this)))
, csysDragger(std::make_unique<ann::CSysDragger>(this, csys.get()))
, cachedSize(20.0)
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionDatumAxis.svg");
  name = QObject::tr("Datum Axis");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  //little unusual as a parameters don't actually affect much.
  parameters.push_back(autoSize.get());
  autoSize->setActive(false); //default state
  autoSize->connectValue(std::bind(&DatumAxis::prmActiveSync, this));
  autoSize->connectValue(std::bind(&DatumAxis::setVisualDirty, this));
  
  parameters.push_back(size.get());
  size->connect(*prmObserver);
  
  parameters.push_back(csys.get());
  csys->connectValue(std::bind(&DatumAxis::setModelDirty, this)); //for dragger
  
  autoSizeLabel = new lbr::PLabel(autoSize.get());
  overlaySwitch->addChild(autoSizeLabel.get());
  
  sizeLabel = new lbr::PLabel(size.get());
  overlaySwitch->addChild(sizeLabel.get());
  
  overlaySwitch->addChild(csysDragger->dragger);
  
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
    picks.clear();
    csys->setActive(true);
  }
  else
    csys->setActive(false);
  
  prmActiveSync();
  this->setModelDirty();
}

void DatumAxis::setToSystem(const osg::Matrixd &mIn)
{
  origin = mIn.getTrans();
  direction = gu::getZVector(mIn);
  csysDragger->setCSys(mIn);
  this->setModelDirty();
}

void DatumAxis::setSize(double sIn)
{
  size->setValue(sIn);
}

double DatumAxis::getSize() const
{
  return size->getDouble();
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

prm::Parameter* DatumAxis::getCSysParameter()
{
  return csys.get();
}

prm::Parameter* DatumAxis::getAutoSizeParameter()
{
  return autoSize.get();
}

prm::Parameter* DatumAxis::getSizeParameter()
{
  return size.get();
}

void DatumAxis::prmActiveSync()
{
  // we don't need to trigger here
  if (axisType == AxisType::Constant)
  {
    csys->setActive(true);
    autoSize->setActive(false);
    size->setActive(true);
  }
  else
  {
    csys->setActive(false);
    autoSize->setActive(true);
    if (autoSize->getBool())
      size->setActive(false);
    else
      size->setActive(true);
  }
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
  osg::Matrixd t = csys->getMatrix();
  origin = t.getTrans();
  direction = gu::getZVector(t);
}

void DatumAxis::goUpdatePoints(const UpdatePayload &pli)
{
  if (picks.size() != 2)
    throw std::runtime_error("Wrong number of picks");
  
  tls::Resolver resolver(pli);
  
  resolver.resolve(picks.front());
  auto points0 = resolver.getPoints();
  if (points0.empty())
    throw std::runtime_error("No points for first pick");
  if (points0.size() > 1)
  {
    std::ostringstream s; s << "WARNING. more than 1 point for first pick" << std::endl;
    lastUpdateLog += s.str();
  }
  
  resolver.resolve(picks.back());
  auto points1 = resolver.getPoints();
  if (points1.empty())
    throw std::runtime_error("No points for second pick");
  if (points1.size() > 1)
  {
    std::ostringstream s; s << "WARNING. more than 1 point for second pick" << std::endl;
    lastUpdateLog += s.str();
  }
  
  osg::Vec3d temp = points0.front() - points1.front();
  if (temp.isNaN())
    throw std::runtime_error("direction is invalid");
  if (temp.length() < std::numeric_limits<float>::epsilon())
    throw std::runtime_error("direction length is invalid");
  direction = temp;
  cachedSize = direction.length();
  direction.normalize();
  origin = points1.front() + direction * cachedSize * 0.5;
}

void DatumAxis::goUpdateIntersection(const UpdatePayload &pli)
{
  typedef std::vector<opencascade::handle<Geom_Surface>, NCollection_StdAllocator<opencascade::handle<Geom_Surface>>> GeomVector;
  GeomVector planes;
  double tSize = 1.0;
  osg::Vec3d to; //temp origin. need to project to intersection line.
  
  tls::Resolver resolver(pli);
  for (const auto &p : picks)
  {
    resolver.resolve(p);
    if (slc::isShapeType(p.selectionType))
    {
      if (p.selectionType != slc::Type::Face)
        continue;
      auto rShapes = resolver.getShapes();
      if (rShapes.empty() || rShapes.front().ShapeType() != TopAbs_FACE)
        continue;
      BRepAdaptor_Surface sa(TopoDS::Face(rShapes.front()));
      if (sa.GetType() != GeomAbs_Plane)
        continue;
      const auto &gsa = sa.Surface(); //geometry surface adaptor.
      planes.push_back(opencascade::handle<Geom_Surface>::DownCast(gsa.Surface()->Transformed(sa.Trsf())));
      occt::BoundingBox fbb(rShapes.front());
      if (fbb.getDiagonal() > tSize)
      {
        tSize = fbb.getDiagonal();
        to = gu::toOsg(fbb.getCenter());
      }
    }
    else if (resolver.getFeature() && resolver.getFeature()->getType() == Type::DatumPlane)
    {
      const DatumPlane *dp = static_cast<const DatumPlane*>(resolver.getFeature());
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
  if (picks.size() != 1)
    throw std::runtime_error("picks count should be 1");
  tls::Resolver resolver(pli);
  resolver.resolve(picks.front());
  auto rShapes = resolver.getShapes();
  if (rShapes.empty())
    throw std::runtime_error("no resolved shapes");
  if (rShapes.size() > 1)
  {
    std::ostringstream s; s << "WARNING. more than 1 resolved" << std::endl;
    lastUpdateLog += s.str();
  }
  auto axisPair = occt::gleanAxis(rShapes.front());
  if (!axisPair.second)
    throw std::runtime_error("couldn't glean axis");
  gp_Ax1 axis = axisPair.first;
  origin = gu::toOsg(axis.Location());
  direction = gu::toOsg(axis.Direction());
  
  occt::BoundingBox bb(rShapes.front());
  cachedSize = bb.getDiagonal();
  gp_Pnt center = bb.getCenter();
  opencascade::handle<Geom_Line> pLine = new Geom_Line(axis);
  GeomAPI_ProjectPointOnCurve proj(center, pLine); 
  if (proj.NbPoints() > 0)
    origin = gu::toOsg(proj.NearestPoint());
}

void DatumAxis::updateVisual()
{
  updateVisualInternal();
  setVisualClean();
}

void DatumAxis::updateVisualInternal()
{
  if (autoSize->getBool())
  {
    prm::ObserverBlocker block(*prmObserver);
    size->setValue(cachedSize);
  }
  
  double ts = size->getDouble();
  
  display->setHeight(ts);
  osg::Matrixd rotation = osg::Matrixd::rotate(osg::Vec3d(0.0, 0.0, 1.0), direction);
  mainTransform->setMatrix(osg::Matrixd::translate(osg::Vec3d(0.0, 0.0, ts * -0.5)) * rotation * osg::Matrixd::translate(origin));
  
  sizeLabel->setMatrix(osg::Matrixd::translate(osg::Vec3d(0.0, 0.0, ts) * mainTransform->getMatrix()));
  autoSizeLabel->setMatrix(osg::Matrixd::translate(osg::Vec3d(0.0, 0.0, ts * 0.5) * mainTransform->getMatrix()));
}

void DatumAxis::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::dtas::DatumAxis dao
  (
    Base::serialOut(),
    static_cast<int>(axisType),
    prj::srl::spt::Vec3d(origin.x(), origin.y(), origin.z()),
    prj::srl::spt::Vec3d(direction.x(), direction.y(), direction.z()),
    csys->serialOut(),
    autoSize->serialOut(),
    size->serialOut(),
    csysDragger->serialOut(),
    autoSizeLabel->serialOut(),
    sizeLabel->serialOut()
  );
  for (const auto &p : picks)
    dao.picks().push_back(p);
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::dtas::datumAxis(stream, dao, infoMap);
}

void DatumAxis::serialRead(const prj::srl::dtas::DatumAxis &dai)
{
  Base::serialIn(dai.base());
  axisType = static_cast<AxisType>(dai.axisType());
  origin = osg::Vec3d(dai.origin().x(), dai.origin().y(), dai.origin().z());
  direction = osg::Vec3d(dai.direction().x(), dai.direction().y(), dai.direction().z());
  csys->serialIn(dai.csys());
  autoSize->serialIn(dai.autoSize());
  size->serialIn(dai.size());
  csysDragger->serialIn(dai.csysDragger());
  autoSizeLabel->serialIn(dai.autoSizeLabel());
  sizeLabel->serialIn(dai.sizeLabel());
  for (const auto &pIn : dai.picks())
    picks.emplace_back(pIn);
  
  cachedSize = size->getDouble();
  updateVisualInternal();
  if (axisType == AxisType::Constant)
    mainTransform->setMatrix(csys->getMatrix());
}
