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

#include <memory>

#include <boost/filesystem/path.hpp>

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
#include "feature/ftrdatumaxis.h"

using namespace ftr::DatumAxis;

using boost::uuids::uuid;

QIcon Feature::icon = QIcon(":/resources/images/constructionDatumAxis.svg");

struct Feature::Stow
{
  Feature &feature;
  prm::Parameter axisType;
  prm::Parameter origin;
  prm::Parameter direction;
  prm::Parameter csys; //only used for dragger.
  prm::Parameter autoSize;
  prm::Parameter size;
  prm::Parameter linkedAxis;
  prm::Parameter linkedPicks;
  prm::Parameter pointsPicks;
  prm::Parameter intersectionPicks;
  prm::Parameter geometryPicks;
  prm::Observer sizeObserver;
  prm::Observer prmObserver;
  prm::Observer syncObserver;
  ann::CSysDragger csysDragger; //!< for constant type
  osg::ref_ptr<lbr::PLabel> originLabel;
  osg::ref_ptr<lbr::PLabel> directionLabel;
  osg::ref_ptr<lbr::PLabel> autoSizeLabel;
  osg::ref_ptr<lbr::PLabel> sizeLabel;
  osg::ref_ptr<mdv::DatumAxis> display;
  double cachedSize; //!< always set by update when possible.
  
  Stow(Feature &fIn)
  : feature(fIn)
  , axisType(QObject::tr("Axis Type"), 0, Tags::AxisType)
  , origin(prm::Names::Origin, osg::Vec3d(), prm::Tags::Origin)
  , direction(prm::Names::Direction, osg::Vec3d(0.0, 0.0, 1.0), prm::Tags::Direction)
  , csys(prm::Names::CSys, osg::Matrixd::identity(), prm::Tags::CSys)
  , autoSize(prm::Names::AutoSize, true, prm::Tags::AutoSize)
  , size(prm::Names::Size, 20.0, prm::Tags::Size)
  , linkedAxis(QObject::tr("Axis"), 0, Tags::LinkedAxis)
  , linkedPicks(QObject::tr("Linked Picks"), ftr::Picks(), Tags::Linked)
  , pointsPicks(QObject::tr("Point Picks"), ftr::Picks(), Tags::Points)
  , intersectionPicks(QObject::tr("Intersection Picks"), ftr::Picks(), Tags::Intersection)
  , geometryPicks(QObject::tr("Geometry Picks"), ftr::Picks(), Tags::Geometry)
  , sizeObserver(std::bind(&Feature::setVisualDirty, &feature))
  , prmObserver(std::bind(&Feature::setModelDirty, &feature))
  , syncObserver(std::bind(&Stow::prmActiveSync, this))
  , csysDragger(&feature, &csys)
  , originLabel(new lbr::PLabel(&origin))
  , directionLabel(new lbr::PLabel(&direction))
  , autoSizeLabel(new lbr::PLabel(&autoSize))
  , sizeLabel(new lbr::PLabel(&size))
  , display(new mdv::DatumAxis())
  , cachedSize(20.0)
  {
    QStringList tStrings = //keep in sync with enum in header.
    {
      QObject::tr("Constant")
      , QObject::tr("Parameters")
      , QObject::tr("Linked")
      , QObject::tr("Points")
      , QObject::tr("Intersection")
      , QObject::tr("Geometry")
    };
    axisType.setEnumeration(tStrings);
    axisType.connect(syncObserver);
    axisType.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&axisType);
    
    origin.connect(prmObserver);
    feature.parameters.push_back(&origin);
    
    direction.connect(prmObserver);
    feature.parameters.push_back(&direction);
    
    csys.connect(prmObserver);
    feature.parameters.push_back(&csys);
    
    autoSize.connect(prmObserver);
    autoSize.connect(syncObserver);
    feature.parameters.push_back(&autoSize);
    
    size.connect(sizeObserver);
    feature.parameters.push_back(&size);
    
    QStringList linkedAxisStrings =
    {
      QObject::tr("X")
      , QObject::tr("Y")
      , QObject::tr("Z")
    };
    linkedAxis.setEnumeration(linkedAxisStrings);
    linkedAxis.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&linkedAxis);
    
    auto initPick = [&](auto &prm)
    {
      prm.setExpressionLinkable(false);
      prm.connectValue(std::bind(&Feature::setModelDirty, &feature));
      feature.parameters.push_back(&prm);
    };
    initPick(linkedPicks);
    initPick(pointsPicks);
    initPick(intersectionPicks);
    initPick(geometryPicks);
    
    feature.annexes.insert(std::make_pair(ann::Type::CSysDragger, &csysDragger));
    feature.overlaySwitch->addChild(csysDragger.dragger.get());
    
    feature.overlaySwitch->addChild(originLabel.get());
    feature.overlaySwitch->addChild(directionLabel.get());
    feature.overlaySwitch->addChild(autoSizeLabel.get());
    feature.overlaySwitch->addChild(sizeLabel.get());
    
    display = new mdv::DatumAxis();
    feature.mainTransform->addChild(display.get());
  }
  
  void prmActiveSync()
  {
    prm::ObserverBlocker blocker0(prmObserver);
    prm::ObserverBlocker blocker1(sizeObserver);
    prm::ObserverBlocker blocker3(syncObserver);
    
    switch (static_cast<AxisType>(axisType.getInt()))
    {
      case AxisType::Constant:{
        //We let csys and dragger get out of sync with the orign and direction
        //when we are not in constant mode. so sync them now.
        auto m = csys.getMatrix();
        auto z = gu::getZVector(m);
        m = osg::Matrixd::rotate(z, direction.getVector()) * m;
        m.setTrans(origin.getVector());
        csys.setValue(m);
        csysDragger.draggerUpdate();
        origin.setActive(false);
        direction.setActive(false);
        csys.setActive(true);
        autoSize.setValue(false);
        autoSize.setActive(false);
        size.setActive(true);
        linkedAxis.setActive(false);
        linkedPicks.setActive(false);
        pointsPicks.setActive(false);
        intersectionPicks.setActive(false);
        geometryPicks.setActive(false);
        break;}
      case AxisType::Parameters:{
        origin.setActive(true);
        direction.setActive(true);
        csys.setActive(false);
        autoSize.setValue(false);
        autoSize.setActive(false);
        size.setActive(true);
        linkedAxis.setActive(false);
        linkedPicks.setActive(false);
        pointsPicks.setActive(false);
        intersectionPicks.setActive(false);
        geometryPicks.setActive(false);
        break;}
      case AxisType::Linked:{
        origin.setActive(false);
        direction.setActive(false);
        csys.setActive(false);
        autoSize.setValue(false);
        autoSize.setActive(false);
        size.setActive(true);
        linkedAxis.setActive(true);
        linkedPicks.setActive(true);
        pointsPicks.setActive(false);
        intersectionPicks.setActive(false);
        geometryPicks.setActive(false);
        break;}
      case AxisType::Points:{
        origin.setActive(false);
        direction.setActive(false);
        csys.setActive(false);
        autoSize.setActive(true);
        size.setActive(!autoSize.getBool());
        linkedAxis.setActive(false);
        linkedPicks.setActive(false);
        pointsPicks.setActive(true);
        intersectionPicks.setActive(false);
        geometryPicks.setActive(false);
        break;}
      case AxisType::Intersection:{
        origin.setActive(false);
        direction.setActive(false);
        csys.setActive(false);
        autoSize.setActive(true);
        size.setActive(!autoSize.getBool());
        linkedAxis.setActive(false);
        linkedPicks.setActive(false);
        pointsPicks.setActive(false);
        intersectionPicks.setActive(true);
        geometryPicks.setActive(false);
        break;}
      case AxisType::Geometry:{
        origin.setActive(false);
        direction.setActive(false);
        csys.setActive(false);
        autoSize.setActive(true);
        size.setActive(!autoSize.getBool());
        linkedAxis.setActive(false);
        linkedPicks.setActive(false);
        pointsPicks.setActive(false);
        intersectionPicks.setActive(false);
        geometryPicks.setActive(true);
        break;}
    }
  }
  
  void goUpdateConstant()
  {
    osg::Matrixd t = csys.getMatrix();
    origin.setValue(t.getTrans());
    direction.setValue(gu::getZVector(t));
  }
  
  void goUpdateParameters()
  {
    //just make sure direction is unit
    osg::Vec3d du = direction.getVector();
    du.normalize();
    direction.setValue(du);
  }
  
  void goUpdateLinked(const UpdatePayload &pli)
  {
    const auto &cp = linkedPicks.getPicks();
    if (cp.size() != 1)
      throw std::runtime_error("Wrong number of linked picks");
    
    tls::Resolver resolver(pli);
    if (!resolver.resolve(cp.front()))
      throw std::runtime_error("Couldn't resolve linked pick");
    
    auto csysPrms = resolver.getFeature()->getParameters(prm::Tags::CSys);
    if (csysPrms.empty())
      throw std::runtime_error("Linked feature has no csys parameter");
    const auto &cm = csysPrms.front()->getMatrix();
    origin.setValue(cm.getTrans());
    int la = linkedAxis.getInt();
    switch (la)
    {
      case 0: direction.setValue(gu::getXVector(cm)); break;
      case 1: direction.setValue(gu::getYVector(cm)); break;
      case 2: direction.setValue(gu::getZVector(cm)); break;
      default: assert(0); throw std::runtime_error("Invalid linked axis type"); break;
    }
  }
  
  void goUpdatePoints(const UpdatePayload &pli)
  {
    const auto &cp = pointsPicks.getPicks();
    if (cp.size() != 2)
      throw std::runtime_error("Wrong number of points picks");
    
    tls::Resolver resolver(pli);
    resolver.resolve(cp.front());
    auto points0 = resolver.getPoints();
    if (points0.empty())
      throw std::runtime_error("No points for first pick");
    if (points0.size() > 1)
    {
      std::ostringstream s; s << "WARNING. more than 1 point for first pick" << std::endl;
      feature.lastUpdateLog += s.str();
    }
    
    resolver.resolve(cp.back());
    auto points1 = resolver.getPoints();
    if (points1.empty())
      throw std::runtime_error("No points for second pick");
    if (points1.size() > 1)
    {
      std::ostringstream s; s << "WARNING. more than 1 point for second pick" << std::endl;
      feature.lastUpdateLog += s.str();
    }
    
    osg::Vec3d temp = points0.front() - points1.front();
    if (temp.isNaN())
      throw std::runtime_error("direction is invalid");
    if (temp.length() < std::numeric_limits<float>::epsilon())
      throw std::runtime_error("direction length is invalid");
    cachedSize = temp.length();
    temp.normalize();
    direction.setValue(temp);
    origin.setValue(points1.front() + temp * cachedSize * 0.5);
  }
  
  void goUpdateIntersection(const UpdatePayload &pli)
  {
    using opencascade::handle;
    std::vector<handle<Geom_Surface>, NCollection_StdAllocator<handle<Geom_Surface>>> planes;
    double tSize = 1.0;
    osg::Vec3d to; //temp origin. need to project to intersection line.
    
    tls::Resolver resolver(pli);
    for (const auto &p : intersectionPicks.getPicks())
    {
      if (!resolver.resolve(p))
        continue;
      auto csysPrms = resolver.getFeature()->getParameters(prm::Tags::CSys);
      auto sizePrms = resolver.getFeature()->getParameters(prm::Tags::Size);
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
        planes.push_back(dynamic_cast<Geom_Surface*>(gsa.Surface()->Transformed(sa.Trsf()).get()));
        occt::BoundingBox fbb(rShapes.front());
        if (fbb.getDiagonal() > tSize)
        {
          tSize = fbb.getDiagonal();
          to = gu::toOsg(fbb.getCenter());
        }
      }
      else if (!csysPrms.empty())
      {
        osg::Matrixd dpSys = csysPrms.front()->getMatrix();
        gp_Pnt o = gu::toOcc(dpSys.getTrans()).XYZ();
        gp_Dir d = gu::toOcc(gu::getZVector(dpSys));
        planes.push_back(new Geom_Plane(o, d));
        if (!sizePrms.empty())
        {
          double inputSize = sizePrms.front()->getDouble() * 2.0;
          if (inputSize > tSize)
          {
            tSize = inputSize;
            to = dpSys.getTrans();
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
    
    direction.setValue(gu::toOsg(gp_Vec(l->Position().Direction().XYZ())));
    origin.setValue(gu::toOsg(l->Position().Location()));
    cachedSize = tSize;
    
    //project center of parent geometry onto line and move half of tSize.
    GeomAPI_ProjectPointOnCurve proj(gp_Pnt(gu::toOcc(to).XYZ()), c); 
    if (proj.NbPoints() > 0)
      origin.setValue(gu::toOsg(proj.NearestPoint()));
  }
  
  void goUpdateGeometry(const UpdatePayload &pli)
  {
    const auto &cp = geometryPicks.getPicks();
    if (cp.size() != 1)
      throw std::runtime_error("geometry pick count should be 1");
    tls::Resolver resolver(pli);
    if (!resolver.resolve(cp.front()))
      throw std::runtime_error("Couldn't resolve geometry pick");
    auto rShapes = resolver.getShapes();
    if (rShapes.empty())
      throw std::runtime_error("no resolved shapes");
    if (rShapes.size() > 1)
    {
      std::ostringstream s; s << "WARNING. more than 1 resolved" << std::endl;
      feature.lastUpdateLog += s.str();
    }
    auto axisPair = occt::gleanAxis(rShapes.front());
    if (!axisPair.second)
      throw std::runtime_error("couldn't glean axis");
    gp_Ax1 axis = axisPair.first;
    origin.setValue(gu::toOsg(axis.Location()));
    direction.setValue(gu::toOsg(axis.Direction()));
    
    occt::BoundingBox bb(rShapes.front());
    cachedSize = bb.getDiagonal();
    gp_Pnt center = bb.getCenter();
    opencascade::handle<Geom_Line> pLine = new Geom_Line(axis);
    GeomAPI_ProjectPointOnCurve proj(center, pLine); 
    if (proj.NbPoints() > 0)
      origin.setValue(gu::toOsg(proj.NearestPoint()));
  }
  
  void setMainTransform()
  {
    //orient main transform to origin and direction.
    osg::Matrixd rotation = osg::Matrixd::rotate(osg::Vec3d(0.0, 0.0, 1.0), direction.getVector());
    osg::Matrixd mt =
      osg::Matrixd::translate(osg::Vec3d(0.0, 0.0, size.getDouble() * -0.5))
      * rotation
      * osg::Matrixd::translate(origin.getVector());
    feature.mainTransform->setMatrix(mt);
  }
  
  void updateVisual()
  {
    if (autoSize.getBool())
    {
      prm::ObserverBlocker block(sizeObserver);
      size.setValue(cachedSize);
    }
    
    double ts = size.getDouble();
    display->setHeight(ts);
    setMainTransform();
    auto mt = feature.mainTransform->getMatrix();
    
    originLabel->setMatrix(osg::Matrixd::translate(osg::Vec3d(0.0, 0.0, 0.0) * mt));
    directionLabel->setMatrix(osg::Matrixd::translate(osg::Vec3d(0.0, 0.0, ts) * mt));
    sizeLabel->setMatrix(osg::Matrixd::translate(osg::Vec3d(ts * -0.25, 0.0, ts * 0.5) * mt));
    autoSizeLabel->setMatrix(osg::Matrixd::translate(osg::Vec3d(ts * 0.25, 0.0, ts * 0.5) * mt));
  }
};

Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  name = QObject::tr("Datum Axis");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  stow->prmActiveSync();
}

Feature::~Feature(){}

void Feature::updateModel(const UpdatePayload &pli)
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
    
    prm::ObserverBlocker blocker(stow->prmObserver);
    prm::ObserverBlocker blocker2(stow->sizeObserver);
    
    switch (static_cast<AxisType>(stow->axisType.getInt()))
    {
      case AxisType::Constant: stow->goUpdateConstant(); break;
      case AxisType::Parameters: stow->goUpdateParameters(); break;
      case AxisType::Linked: stow->goUpdateLinked(pli); break;
      case AxisType::Points: stow->goUpdatePoints(pli); break;
      case AxisType::Intersection: stow->goUpdateIntersection(pli); break;
      case AxisType::Geometry: stow->goUpdateGeometry(pli); break;
    }
    
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

void Feature::updateVisual()
{
  stow->updateVisual();
  setVisualClean();
}

/* I tried to make overlay geometry optional, but the visibility
 * state of such are encoded into the objects themselves. So If I
 * don't serialize them out they can visible or invisible when they
 * shouldn't be.
 */
void Feature::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::dtas::DatumAxis dao
  (
    Base::serialOut()
    , stow->axisType.serialOut()
    , stow->origin.serialOut()
    , stow->direction.serialOut()
    , stow->csys.serialOut()
    , stow->autoSize.serialOut()
    , stow->size.serialOut()
    , stow->csysDragger.serialOut()
    , stow->originLabel->serialOut()
    , stow->directionLabel->serialOut()
    , stow->autoSizeLabel->serialOut()
    , stow->sizeLabel->serialOut()
    , stow->cachedSize
  );
  
  auto ct = static_cast<AxisType>(stow->axisType.getInt());
  switch (ct)
  {
    case AxisType::Constant:{break;} //nothing
    case AxisType::Parameters:{break;} //nothing
    case AxisType::Linked:{
      dao.linkedAxis() = stow->linkedAxis.serialOut();
      dao.picks() = stow->linkedPicks.serialOut();
      break;}
    case AxisType::Points:{
      dao.picks() = stow->pointsPicks.serialOut();
      break;}
    case AxisType::Intersection:{
      dao.picks() = stow->intersectionPicks.serialOut();
      break;}
    case AxisType::Geometry:{
      dao.picks() = stow->geometryPicks.serialOut();
      break;}
  }
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::dtas::datumAxis(stream, dao, infoMap);
}

void Feature::serialRead(const prj::srl::dtas::DatumAxis &dai)
{
  Base::serialIn(dai.base());
  stow->axisType.serialIn(dai.axisType());
  stow->origin.serialIn(dai.origin());
  stow->direction.serialIn(dai.direction());
  stow->csys.serialIn(dai.csys());
  stow->autoSize.serialIn(dai.autoSize());
  stow->size.serialIn(dai.size());
  stow->csysDragger.serialIn(dai.csysDragger());
  stow->originLabel->serialIn(dai.originLabel());
  stow->directionLabel->serialIn(dai.directionLabel());
  stow->autoSizeLabel->serialIn(dai.autoSizeLabel());
  stow->sizeLabel->serialIn(dai.sizeLabel());
  stow->cachedSize = dai.cachedSize();
  
  auto ct = static_cast<AxisType>(stow->axisType.getInt());
  switch (ct)
  {
    case AxisType::Constant:{break;} //nothing
    case AxisType::Parameters:{break;} //nothing
    case AxisType::Linked:{
      if (dai.linkedAxis())
        stow->linkedAxis.serialIn(dai.linkedAxis().get());
      if (dai.picks())
        stow->linkedPicks.serialIn(dai.picks().get());
      break;}
    case AxisType::Points:{
      if (dai.picks())
        stow->pointsPicks.serialIn(dai.picks().get());
      break;}
    case AxisType::Intersection:{
      if (dai.picks())
        stow->intersectionPicks.serialIn(dai.picks().get());
      break;}
    case AxisType::Geometry:{
      if (dai.picks())
        stow->geometryPicks.serialIn(dai.picks().get());
      break;}
  }
  
  stow->setMainTransform();
}
