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

#include <BRepFilletAPI_MakeChamfer.hxx>
#include <TopExp.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <Geom_Curve.hxx>

#include <osg/Switch>

#include "globalutilities.h"
#include "tools/idtools.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "project/serial/generated/prjsrlchmschamfer.h"
#include "annex/annseershape.h"
#include "feature/ftrshapecheck.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrupdatepayload.h"
#include "parameter/prmparameter.h"
#include "tools/featuretools.h"
#include "feature/ftrchamfer.h"

using namespace ftr::Chamfer;
using boost::uuids::uuid;

QIcon Feature::icon;

Entry::Entry(const Entry &other, bool)
{
  style = other.style;
  if (other.parameter1)
  {
    parameter1 = std::make_shared<prm::Parameter>(*other.parameter1);
    label1 = new lbr::PLabel(parameter1.get());
  }
  if (other.parameter2)
  {
    parameter2 = std::make_shared<prm::Parameter>(*other.parameter2);
    label2 = new lbr::PLabel(parameter2.get());
  }
  edgePicks = other.edgePicks;
  facePicks = other.facePicks;
}

Entry::Entry(const prj::srl::chms::Entry &eIn) : Entry()
{
  serialIn(eIn);
}

prj::srl::chms::Entry Entry::serialOut() const
{
  prj::srl::chms::Entry out
  (
    static_cast<int>(style)
    , parameter1->serialOut()
    , label1->serialOut()
  );
  for(const auto &ep : edgePicks)
    out.edgePicks().push_back(ep);
  for(const auto &fp : facePicks)
    out.facePicks().push_back(fp);
  
  if (parameter2)
    out.parameter2() = parameter2->serialOut();
  if (label2)
    out.label2() = label2->serialOut();
  
  return out;
}

void Entry::serialIn(const prj::srl::chms::Entry &eIn)
{
  style = static_cast<Style>(eIn.style());
  parameter1 = std::make_shared<prm::Parameter>(eIn.parameter1());
  label1 = new lbr::PLabel(parameter1.get());
  label1->serialIn(eIn.label1());
  prj::srl::chms::Entry::EdgePicksSequence edgePicksOut;
  for (const auto &epi : eIn.edgePicks())
    edgePicks.emplace_back(epi);
  for (const auto &fpi : eIn.facePicks())
    facePicks.emplace_back(fpi);
  
  if (eIn.parameter2().present())
    parameter2 = std::make_shared<prm::Parameter>(eIn.parameter2().get());
  if (eIn.label2().present() && parameter2)
  {
    label2 = new lbr::PLabel(parameter2.get());
    label2->serialIn(eIn.label2().get());
  }
}

inline static std::shared_ptr<prm::Parameter> makeDistance()
{
  return std::make_shared<prm::Parameter>
  (
    prm::Names::Distance
    , prf::manager().rootPtr->features().chamfer().get().distance()
    , prm::Tags::Distance
  );
}

Entry Entry::buildDefaultSymmetric()
{
  Entry out;
  out.style = Style::Symmetric;
  out.parameter1 = makeDistance();
  out.parameter1->setConstraint(prm::Constraint::buildNonZeroPositive());
  out.label1 = new lbr::PLabel(out.parameter1.get());
  return out;
}

Entry Entry::buildDefaultTwoDistances()
{
  Entry out;
  out.style = Style::TwoDistances;
  out.parameter1 = makeDistance();
  out.parameter1->setConstraint(prm::Constraint::buildNonZeroPositive());
  out.label1 = new lbr::PLabel(out.parameter1.get());
  out.parameter2 = makeDistance();
  out.parameter2->setConstraint(prm::Constraint::buildNonZeroPositive());
  out.label2 = new lbr::PLabel(out.parameter2.get());
  return out;
}

Entry Entry::buildDefaultDistanceAngle()
{
  Entry out;
  out.style = Style::DistanceAngle;
  out.parameter1 = makeDistance();
  out.parameter1->setConstraint(prm::Constraint::buildNonZeroPositive());
  out.label1 = new lbr::PLabel(out.parameter1.get());
  out.parameter2 = std::make_shared<prm::Parameter>
  (
    prm::Names::Angle
    , 30.0
    ,prm::Tags::Angle
  ); //todo add angle to preferences.
  out.parameter2->setConstraint(prm::Constraint::buildNonZeroPositiveAngle());
  out.label2 = new lbr::PLabel(out.parameter2.get());
  return out;
}

Feature::Feature() : Base(), sShape(std::make_unique<ann::SeerShape>())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionChamfer.svg");
  
  name = QObject::tr("Chamfer");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
}

Feature::~Feature(){}

void Feature::setMode(Mode mIn)
{
  for (const auto &e : entries)
    detachEntry(e);
  entries.clear();
  mode = mIn;
  setModelDirty();
}

int Feature::addSymmetric()
{
  assert(mode == Mode::Classic || mode == Mode::Throat);
  entries.push_back(Entry::buildDefaultSymmetric());
  attachEntry(entries.back());
  setModelDirty();
  return static_cast<int>(entries.size()) - 1;
}

int Feature::addTwoDistances()
{
  assert(mode == Mode::Classic || mode == Mode::ThroatPenetration);
  entries.push_back(Entry::buildDefaultTwoDistances());
  attachEntry(entries.back());
  setModelDirty();
  return static_cast<int>(entries.size()) - 1;
}

int Feature::addDistanceAngle()
{
  assert(mode == Mode::Classic);
  entries.push_back(Entry::buildDefaultDistanceAngle());
  attachEntry(entries.back());
  setModelDirty();
  return static_cast<int>(entries.size()) - 1;
}

void Feature::removeEntry(int index)
{
  assert(index >= 0 && index < static_cast<int>(entries.size()));
  const auto &e = entries.at(index);
  detachEntry(e);
  entries.erase(entries.begin() + index);
  setModelDirty();
}

const Entry& Feature::getEntry(int index)
{
  assert(index >= 0 && index < static_cast<int>(entries.size()));
  return entries.at(index);
}

void Feature::attachEntry(const Entry &eIn)
{
  //assumes entry is already added to vector
  auto setupParameter = [&](prm::Parameter *p)
  {
    p->connectValue(std::bind(&Feature::setModelDirty, this));
    parameters.push_back(p);
  };

  if (eIn.parameter1)
    setupParameter(eIn.parameter1.get());
  if (eIn.parameter2)
    setupParameter(eIn.parameter2.get());
  if (eIn.label1)
    overlaySwitch->addChild(eIn.label1.get());
  if (eIn.label2)
    overlaySwitch->addChild(eIn.label2.get());
}

void Feature::detachEntry(const Entry &eIn)
{
  if (eIn.parameter1)
    removeParameter(eIn.parameter1.get());
  if (eIn.parameter2)
    removeParameter(eIn.parameter2.get());
  if (eIn.label1)
    overlaySwitch->removeChild(eIn.label1.get());
  if (eIn.label2)
    overlaySwitch->removeChild(eIn.label2.get());
}

void Feature::setEdgePicks(int index, const Picks &psIn)
{
  assert(index >= 0 && index < static_cast<int>(entries.size()));
  entries.at(index).edgePicks = psIn;
  setModelDirty();
}

void Feature::setFacePicks(int index, const Picks &psIn)
{
  assert(index >= 0 && index < static_cast<int>(entries.size()));
  assert(entries.at(index).style != Style::Symmetric);
  entries.at(index).facePicks = psIn;
  setModelDirty();
}

void Feature::updateModel(const UpdatePayload &payloadIn)
{
  setFailure();
  lastUpdateLog.clear();
  sShape->reset();
  try
  {
    std::vector<const Base*> targetFeatures = payloadIn.getFeatures(std::string());
    if (targetFeatures.size() != 1)
      throw std::runtime_error("wrong number of parents");
    const Base* tf = targetFeatures.front();
    if (!tf->hasAnnex(ann::Type::SeerShape))
      throw std::runtime_error("parent doesn't have seer shape");
    const ann::SeerShape &targetSeerShape = tf->getAnnex<ann::SeerShape>();
    if (targetSeerShape.isNull())
      throw std::runtime_error("target seer shape is null");
    
    //setup failure state.
    sShape->setOCCTShape(targetSeerShape.getRootOCCTShape(), getId());
    sShape->shapeMatch(targetSeerShape);
    sShape->uniqueTypeMatch(targetSeerShape);
    sShape->outerWireMatch(targetSeerShape);
    sShape->derivedMatch();
    sShape->ensureNoNils();
    sShape->ensureNoDuplicates();
    
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    auto pairing = [&](const std::vector<uuid> &edges, const std::vector<uuid> &faces) -> std::vector<std::pair<uuid, uuid>>
    {
      std::vector<std::pair<uuid, uuid>> out;
      for (const auto &e : edges)
      {
        for (const auto &f : faces)
        {
          if (targetSeerShape.useIsEdgeOfFace(e, f))
          {
            out.push_back(std::make_pair(e, f));
          }
        }
      }
      return out;
    };
    
    tls::Resolver resolver(payloadIn);
    BRepFilletAPI_MakeChamfer chamferMaker(targetSeerShape.getRootOCCTShape());
    
    chamferMaker.SetMode(static_cast<ChFiDS_ChamfMode>(mode));
    for (const auto &e : entries)
    {
      if (mode == Mode::Throat && e.style != Style::Symmetric)
        throw std::runtime_error("Throat mode only uses symmetric style");
      if (mode == Mode::ThroatPenetration && e.style != Style::TwoDistances)
        throw std::runtime_error("Throat penetration mode only uses 2 distances style");
      //classic mode can use any style
      
      if (e.style == Style::Symmetric)
      {
        bool labelDone = false;
        for (const auto &ep : e.edgePicks)
        {
          if (!resolver.resolve(ep))
            continue;
          for (const auto &s : resolver.getShapes())
          {
            if (s.ShapeType() != TopAbs_EDGE)
            {
              std::ostringstream s; s << "Warning: Skipping non edge shape." << std::endl;
              lastUpdateLog += s.str();
              continue;
            }
            chamferMaker.Add(static_cast<double>(*e.parameter1), TopoDS::Edge(s));
            if (!labelDone)
            {
              labelDone = true;
              e.label1->setMatrix(osg::Matrixd::translate(ep.getPoint(TopoDS::Edge(s))));
            }
          }
        }
      }
      else if (e.style == Style::TwoDistances || e.style == Style::DistanceAngle)
      {
        if (e.edgePicks.size() != e.facePicks.size())
          throw std::runtime_error("need same number edges and faces for two distance chamfer");
        bool labelDone1 = false;
        bool labelDone2 = false;
        for (std::size_t index = 0; index < e.edgePicks.size(); ++index)
        {
          //we have to resolve multiple edges and faces
          if (!resolver.resolve(e.edgePicks.at(index)))
            continue;
          std::vector<uuid> edgeIds = resolver.getResolvedIds();
          
          if (!resolver.resolve(e.facePicks.at(index)))
            continue;
          std::vector<uuid> faceIds = resolver.getResolvedIds();
          
          std::vector<std::pair<uuid, uuid>> pairs = pairing(edgeIds, faceIds);
          for (const auto &p : pairs)
          {
            const TopoDS_Edge &edge = TopoDS::Edge(targetSeerShape.getOCCTShape(p.first));
            const TopoDS_Face &face = TopoDS::Face(targetSeerShape.getOCCTShape(p.second));
            if (e.style == Style::TwoDistances)
            {
              chamferMaker.Add
              (
                static_cast<double>(*e.parameter1)
                , static_cast<double>(*e.parameter2)
                , edge
                , face
              );
            }
            else if (e.style == Style::DistanceAngle)
            {
              chamferMaker.AddDA
              (
                static_cast<double>(*e.parameter1)
                , osg::DegreesToRadians(static_cast<double>(*e.parameter2))
                , edge
                , face
              );
            }
            if (!labelDone1)
            {
              labelDone1 = true;
              e.label1->setMatrix(osg::Matrixd::translate(e.facePicks.at(index).getPoint(face)));
            }
            if (!labelDone2)
            {
              labelDone2 = true;
              e.label2->setMatrix(osg::Matrixd::translate(e.edgePicks.at(index).getPoint(edge)));
            }
          }
        }
      }
    }
    chamferMaker.Build();
    if (!chamferMaker.IsDone())
      throw std::runtime_error("chamferMaker failed");
    ShapeCheck check(chamferMaker.Shape());
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");

    sShape->setOCCTShape(chamferMaker.Shape(), getId());
    sShape->shapeMatch(targetSeerShape);
    sShape->uniqueTypeMatch(targetSeerShape);
    sShape->modifiedMatch(chamferMaker, targetSeerShape);
    generatedMatch(chamferMaker, targetSeerShape);
    sShape->outerWireMatch(targetSeerShape);
    sShape->derivedMatch();
    sShape->dumpNils("chamfer feature"); //only if there are shapes with nil ids.
    sShape->dumpDuplicates("chamfer feature");
    sShape->ensureNoNils();
    sShape->ensureNoDuplicates();
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in chamfer update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in chamfer update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in chamfer update." << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

//duplicated with blend.
void Feature::generatedMatch(BRepFilletAPI_MakeChamfer &chamferMakerIn, const ann::SeerShape &targetShapeIn)
{
  using boost::uuids::uuid;
  
  std::vector<uuid> targetShapeIds = targetShapeIn.getAllShapeIds();
  
  for (const auto &cId : targetShapeIds)
  {
    const TopoDS_Shape &currentShape = targetShapeIn.findShape(cId);
    TopTools_ListOfShape generated = chamferMakerIn.Generated(currentShape);
    if (generated.IsEmpty())
      continue;
    if(generated.Extent() != 1)
    {
      std::ostringstream s; s << "Warning: more than one generated shape in chamfer::generatedMatch" << std::endl;
      lastUpdateLog += s.str();
    }
    const TopoDS_Shape &chamferFace = generated.First();
    assert(!chamferFace.IsNull());
    assert(chamferFace.ShapeType() == TopAbs_FACE);
    
    std::map<uuid, uuid>::iterator mapItFace;
    bool dummy;
    std::tie(mapItFace, dummy) = shapeMap.insert(std::make_pair(cId, gu::createRandomId()));
    sShape->updateId(chamferFace, mapItFace->second);
    if (dummy) //insertion took place.
      sShape->insertEvolve(gu::createNilId(), mapItFace->second);
    
    //now look for outerwire for newly generated face.
    //we use the generated face id to map to outer wire.
    std::map<uuid, uuid>::iterator mapItWire;
    std::tie(mapItWire, dummy) = shapeMap.insert(std::make_pair(mapItFace->second, gu::createRandomId()));
    //now get the wire and update the result to id.
    const TopoDS_Shape &chamferedFaceWire = BRepTools::OuterWire(TopoDS::Face(chamferFace));
    sShape->updateId(chamferedFaceWire, mapItWire->second);
    if (dummy) //insertion took place.
      sShape->insertEvolve(gu::createNilId(), mapItWire->second);
  }
}

void Feature::updateShapeMap(const boost::uuids::uuid &resolvedId, const ShapeHistory &pick)
{
  for (const auto &historyId : pick.getAllIds())
  {
    assert(shapeMap.count(historyId) < 2);
    auto it = shapeMap.find(historyId);
    if (it == shapeMap.end())
      continue;
    auto entry = std::make_pair(resolvedId, it->second);
    shapeMap.erase(it);
    shapeMap.insert(entry);
  }
}

void Feature::serialWrite(const boost::filesystem::path &dIn)
{
  namespace serial = prj::srl::chms;
  serial::Chamfer::ShapeMapSequence shapeMapOut;
  for (const auto &p : shapeMap)
  {
    serial::Chamfer::ShapeMapType eRecord
    (
      gu::idToString(p.first),
      gu::idToString(p.second)
    );
    shapeMapOut.push_back(eRecord);
  }
  
  serial::Chamfer chamferOut
  (
    Base::serialOut()
    , sShape->serialOut()
    , static_cast<int>(mode)
  );
  chamferOut.shapeMap() = shapeMapOut;
  for (const auto &e : entries)
    chamferOut.entries().push_back(e.serialOut());
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  serial::chamfer(stream, chamferOut, infoMap);
}

void Feature::serialRead(const prj::srl::chms::Chamfer &sChamferIn)
{
  Base::serialIn(sChamferIn.base());
  sShape->serialIn(sChamferIn.seerShape());
  
  shapeMap.clear();
  for (const auto &sERecord : sChamferIn.shapeMap())
  {
    std::pair<uuid, uuid> record;
    record.first = gu::stringToId(sERecord.idIn());
    record.second = gu::stringToId(sERecord.idOut());
    shapeMap.insert(record);
  }
  
  mode = static_cast<Mode>(sChamferIn.mode());
  for (const auto &e : sChamferIn.entries())
  {
    entries.emplace_back(e);
    attachEntry(entries.back());
  }
}
