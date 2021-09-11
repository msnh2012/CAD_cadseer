/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2019 Thomas S. Anderson blobfish.at.gmx.com
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

#include <osg/Switch>

#include <TopoDS.hxx>
#include <BRep_Tool.hxx>
#include <GeomAPI_Interpolate.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <Precision.hxx>
#include <TopExp.hxx>

#include "globalutilities.h"
#include "annex/annseershape.h"
#include "library/lbrplabel.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "tools/occtools.h"
#include "tools/featuretools.h"
#include "tools/idtools.h"
#include "feature/ftrshapecheck.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinputtype.h"
#include "project/serial/generated/prjsrltscstransitioncurve.h"
#include "feature/ftrtransitioncurve.h"

using boost::uuids::uuid;
using namespace ftr::TransitionCurve;
QIcon Feature::icon = QIcon(":/resources/images/sketchBezeir.svg");

struct Feature::Stow
{
  Feature &feature;
  prm::Parameter picks{prm::Names::Picks, ftr::Picks(), prm::Tags::Picks};
  prm::Parameter direction0{QObject::tr("Direction 0"), false, PrmTags::direction0};
  prm::Parameter direction1{QObject::tr("Direction 1"), false, PrmTags::direction1};
  prm::Parameter magnitude0{QObject::tr("Magnitude 0"), 1.0, PrmTags::magnitude0};
  prm::Parameter magnitude1{QObject::tr("Magnitude 1"), 1.0, PrmTags::magnitude1};
  prm::Parameter autoScale{QObject::tr("Auto Scale"), true, PrmTags::autoScale};
  prm::Observer syncObserver{std::bind(&Stow::prmActiveSync, this)};
  prm::Observer blockObserver{std::bind(&Feature::setModelDirty, &feature)};
  
  ann::SeerShape sShape;
  
  osg::ref_ptr<lbr::PLabel> direction0Label{new lbr::PLabel(&direction0)};
  osg::ref_ptr<lbr::PLabel> direction1Label{new lbr::PLabel(&direction1)};
  osg::ref_ptr<lbr::PLabel> magnitude0Label{new lbr::PLabel(&magnitude0)};
  osg::ref_ptr<lbr::PLabel> magnitude1Label{new lbr::PLabel(&magnitude1)};
  osg::ref_ptr<lbr::PLabel> autoScaleLabel{new lbr::PLabel(&autoScale)};
  
  uuid curveId{gu::createRandomId()};
  uuid vertex0Id{gu::createRandomId()};
  uuid vertex1Id{gu::createRandomId()};
  
  Stow() = delete;
  Stow(Feature &fIn)
  : feature(fIn)
  {
    picks.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&picks);
    
    direction0.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&direction0);
    
    direction1.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&direction1);
    
    magnitude0.connect(blockObserver);
    feature.parameters.push_back(&magnitude0);
    
    magnitude1.connect(blockObserver);
    feature.parameters.push_back(&magnitude1);
    
    autoScale.connectValue(std::bind(&Feature::setModelDirty, &feature));
    autoScale.connect(syncObserver);
    feature.parameters.push_back(&autoScale);
    
    feature.annexes.insert(std::make_pair(ann::Type::SeerShape, &sShape));
    
    feature.overlaySwitch->addChild(direction0Label.get());
    feature.overlaySwitch->addChild(direction1Label.get());
    feature.overlaySwitch->addChild(magnitude0Label.get());
    feature.overlaySwitch->addChild(magnitude1Label.get());
    feature.overlaySwitch->addChild(autoScaleLabel.get());
    
    prmActiveSync();
  }
  
  void prmActiveSync()
  {
    if (autoScale.getBool())
    {
      magnitude0.setActive(false);
      magnitude1.setActive(false);
    }
    else
    {
      magnitude0.setActive(true);
      magnitude1.setActive(true);
    }
  }
};

Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  name = QObject::tr("Transition Curve");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
}

Feature::~Feature() = default;

/*! @brief Detects default direction for first pick
 * 
 * @param eIn is the edge used for detection
 * @details asserts that picks have been set. @see setPicks
 * 
 */
void Feature::gleanDirection0(const TopoDS_Edge &eIn)
{
  /* Note: StartPoint and EndPoint consider edge orientation. The
   * parameter driven selection types don't consider edge orientation.
   * see ftr::Pick
   */
  const auto &picks = stow->picks.getPicks();
  assert(!picks.empty());
  
  bool outValue = false;
  const Pick &p = picks.at(0);
  if (p.selectionType == slc::Type::StartPoint && eIn.Orientation() == TopAbs_FORWARD)
    outValue = !outValue;
  if (p.selectionType == slc::Type::EndPoint && eIn.Orientation() == TopAbs_REVERSED)
    outValue = !outValue;
  if (p.selectionType == slc::Type::NearestPoint || p.selectionType == slc::Type::QuadrantPoint)
  {
    if (p.u < 0.5)
      outValue = !outValue;
  }
  
  stow->direction0.setValue(outValue);
}

/*! @brief Detects default direction for second pick
 * 
 * @param eIn is the edge used for detection
 * @details asserts that picks have been set. @see setPicks
 * 
 */
void Feature::gleanDirection1(const TopoDS_Edge &eIn)
{
  const auto &picks = stow->picks.getPicks();
  assert(picks.size() > 1);
  
  bool outValue = false;
  const Pick &p = picks.at(1);
  if (p.selectionType == slc::Type::StartPoint && eIn.Orientation() == TopAbs_REVERSED)
    outValue = !outValue;
  if (p.selectionType == slc::Type::EndPoint && eIn.Orientation() == TopAbs_FORWARD)
    outValue = !outValue;
  if (p.selectionType == slc::Type::NearestPoint || p.selectionType == slc::Type::QuadrantPoint)
  {
    if (p.u > 0.5)
      outValue = !outValue;
  }
  
  stow->direction1.setValue(outValue);
}

void Feature::updateModel(const UpdatePayload &pIn)
{
  setFailure();
  lastUpdateLog.clear();
  stow->sShape.reset();
  try
  {
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    const auto &picks = stow->picks.getPicks();
    if (picks.size() != 2)
      throw std::runtime_error("wrong number of picks");
    tls::Resolver resolver(pIn);
    
    resolver.resolve(picks.front());
    auto rShapes0 = resolver.getShapes(false);
    if (rShapes0.empty())
      throw std::runtime_error("invalid first pick resolution");
    if (rShapes0.front().ShapeType() != TopAbs_EDGE)
      throw std::runtime_error("invalid first pick resolution shape type");
    const TopoDS_Edge &edge0 = TopoDS::Edge(rShapes0.front());
    BRepAdaptor_Curve adapt0(edge0);
    auto rPoints0 = resolver.getPoints();
    if (rPoints0.empty())
      throw std::runtime_error("invalid first point resolution");
    gp_Pnt p0 = gu::toOcc(rPoints0.front()).XYZ();
    
    resolver.resolve(picks.back());
    auto rShapes1 = resolver.getShapes(false);
    if (rShapes1.empty())
      throw std::runtime_error("invalid second pick resolution");
    if (rShapes1.front().ShapeType() != TopAbs_EDGE)
      throw std::runtime_error("invalid second pick resolution shape type");
    const TopoDS_Edge &edge1 = TopoDS::Edge(rShapes1.front());
    BRepAdaptor_Curve adapt1(edge1);
    auto rPoints1 = resolver.getPoints();
    if (rPoints1.empty())
      throw std::runtime_error("invalid second point resolution");
    gp_Pnt p1 = gu::toOcc(rPoints1.front()).XYZ();
    
    double prm0, prm1;
    bool result;
    std::tie(prm0, result) = occt::pointToParameter(edge0, p0);
    if (!result)
      throw std::runtime_error("couldn't get parameter for p0");
    std::tie(prm1, result) = occt::pointToParameter(edge1, p1);
    if (!result)
      throw std::runtime_error("couldn't get parameter for p1");
    
    gp_Vec v0 = adapt0.DN(prm0, 1);
    gp_Vec v1 = adapt1.DN(prm1, 1);
    
    p0.Transform(adapt0.Trsf());
    v0.Transform(adapt0.Trsf());
    p1.Transform(adapt1.Trsf());
    v1.Transform(adapt1.Trsf());
    
    if (stow->direction0.getBool())
      v0.Reverse();
    if (stow->direction1.getBool())
      v1.Reverse();
    
    v0 *= stow->magnitude0.getDouble();
    v1 *= stow->magnitude1.getDouble();
    
    stow->direction0Label->setMatrix(osg::Matrixd::translate(gu::toOsg(p0)));
    stow->direction1Label->setMatrix(osg::Matrixd::translate(gu::toOsg(p1)));
    stow->magnitude0Label->setMatrix(osg::Matrixd::translate(gu::toOsg(p0) + gu::toOsg(v0) * stow->magnitude0.getDouble()));
    stow->magnitude1Label->setMatrix(osg::Matrixd::translate(gu::toOsg(p1) + gu::toOsg(-v1) * stow->magnitude1.getDouble()));
    
    opencascade::handle<TColgp_HArray1OfPnt> points = new TColgp_HArray1OfPnt(1,2);
    points->SetValue(1, p0);
    points->SetValue(2, p1);
    
    GeomAPI_Interpolate trans(points, Standard_False, Precision::Confusion());
    trans.Load(v0, v1, stow->autoScale.getBool());
    trans.Perform();
    if (!trans.IsDone())
      throw std::runtime_error("Perform failed");
    
    BRepBuilderAPI_MakeEdge edgeMaker(trans.Curve());
    if (!edgeMaker.IsDone())
      throw std::runtime_error("edgeMaker failed");
    
    if (stow->autoScale.getBool())
    {
      //update the magnitudes if auto calc.
      prm::ObserverBlocker(stow->blockObserver);
      BRepAdaptor_Curve ca(edgeMaker);
      auto getMag = [&](double pIn) -> double
      {
        gp_Pnt point;
        gp_Vec vector;
        ca.D1(pIn, point, vector);
        return vector.Magnitude();
      };
      //do I need to reverse.
      stow->magnitude0.setValue(getMag(ca.FirstParameter()));
      stow->magnitude1.setValue(getMag(ca.LastParameter()));
    }
    
    stow->sShape.setOCCTShape(edgeMaker.Edge(), getId());
    stow->sShape.updateId(edgeMaker.Edge(), stow->curveId);
    if (!stow->sShape.hasEvolveRecordOut(stow->curveId))
      stow->sShape.insertEvolve(gu::createNilId(), stow->curveId);
    
    TopoDS_Vertex vertex0 = TopExp::FirstVertex(edgeMaker.Edge(), Standard_True);
    assert(stow->sShape.hasShape(vertex0));
    stow->sShape.updateId(vertex0, stow->vertex0Id);
    if (!stow->sShape.hasEvolveRecordOut(stow->vertex0Id))
      stow->sShape.insertEvolve(gu::createNilId(), stow->vertex0Id);
    
    TopoDS_Vertex vertex1 = TopExp::LastVertex(edgeMaker.Edge(), Standard_True);
    assert(stow->sShape.hasShape(vertex1));
    stow->sShape.updateId(vertex1, stow->vertex1Id);
    if (!stow->sShape.hasEvolveRecordOut(stow->vertex1Id))
      stow->sShape.insertEvolve(gu::createNilId(), stow->vertex1Id);
    
    stow->sShape.ensureNoNils();
    stow->sShape.ensureNoDuplicates();
    
    auto midPoints = stow->sShape.useGetMidPoint(stow->curveId);
    if (!midPoints.empty())
      stow->autoScaleLabel->setMatrix(osg::Matrixd::translate(midPoints.front()));
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in " << getTypeString() << " update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in " << getTypeString() << " update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in " << getTypeString() << " update." << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Feature::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::tscs::TransitionCurve so
  (
    Base::serialOut()
    , stow->picks.serialOut()
    , stow->direction0.serialOut()
    , stow->direction1.serialOut()
    , stow->magnitude0.serialOut()
    , stow->magnitude1.serialOut()
    , stow->autoScale.serialOut()
    , stow->sShape.serialOut()
    , stow->direction0Label->serialOut()
    , stow->direction1Label->serialOut()
    , stow->magnitude0Label->serialOut()
    , stow->magnitude1Label->serialOut()
    , stow->autoScaleLabel->serialOut()
    , gu::idToString(stow->curveId)
    , gu::idToString(stow->vertex0Id)
    , gu::idToString(stow->vertex1Id)
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::tscs::transitionCurve(stream, so, infoMap);
}

void Feature::serialRead(const prj::srl::tscs::TransitionCurve &so)
{
  Base::serialIn(so.base());
  stow->picks.serialIn(so.picks());
  stow->direction0.serialIn(so.direction0());
  stow->direction1.serialIn(so.direction1());
  stow->magnitude0.serialIn(so.magnitude0());
  stow->magnitude1.serialIn(so.magnitude1());
  stow->autoScale.serialIn(so.autoScale());
  stow->sShape.serialIn(so.seerShape());
  stow->direction0Label->serialIn(so.direction0Label());
  stow->direction1Label->serialIn(so.direction1Label());
  stow->magnitude0Label->serialIn(so.magnitude0Label());
  stow->magnitude1Label->serialIn(so.magnitude1Label());
  stow->autoScaleLabel->serialIn(so.autoScaleLabel());
  stow->curveId = gu::stringToId(so.curveId());
  stow->vertex0Id = gu::stringToId(so.vertex0Id());
  stow->vertex1Id = gu::stringToId(so.vertex1Id());
}
