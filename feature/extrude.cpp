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

#include <osg/Switch>

#include <TopoDS.hxx>
#include <TopExp.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BOPAlgo_Tools.hxx>

#include "globalutilities.h"
#include "tools/occtools.h"
#include "annex/seershape.h"
#include "library/plabel.h"
#include "library/ipgroup.h"
#include "feature/shapecheck.h"
#include "feature/updatepayload.h"
#include "feature/inputtype.h"
#include "feature/parameter.h"
#include "feature/datumaxis.h"
#include "tools/featuretools.h"
#include "tools/idtools.h"
#include "project/serial/xsdcxxoutput/featureextrude.h"
#include "feature/extrude.h"

using namespace ftr;
using boost::uuids::uuid;

QIcon Extrude::icon;

static const std::map<BRepBuilderAPI_FaceError, std::string> errorMap
{
  std::make_pair(BRepBuilderAPI_FaceDone, "BRepBuilderAPI_FaceDone")
  , std::make_pair(BRepBuilderAPI_NoFace, "BRepBuilderAPI_NoFace")
  , std::make_pair(BRepBuilderAPI_NotPlanar, "BRepBuilderAPI_NotPlanar")
  , std::make_pair(BRepBuilderAPI_CurveProjectionFailed, "BRepBuilderAPI_CurveProjectionFailed")
  , std::make_pair(BRepBuilderAPI_ParametersOutOfRange, "BRepBuilderAPI_ParametersOutOfRange")
};

static boost::optional<osg::Vec3d> inferDirection(const occt::ShapeVector &svIn)
{
  //look for a planar face and collect points
  TopTools_MapOfShape vertices;
  auto checkPoints = [&]() -> boost::optional<osg::Vec3d>
  {
    if (vertices.Extent() < 3)
      return boost::none;
    
    auto it = vertices.cbegin();
    osg::Vec3d p0 = gu::toOsg(BRep_Tool::Pnt(TopoDS::Vertex(*it++)));
    osg::Vec3d p1 = gu::toOsg(BRep_Tool::Pnt(TopoDS::Vertex(*it++)));
    osg::Vec3d p2 = gu::toOsg(BRep_Tool::Pnt(TopoDS::Vertex(*it)));
    
    osg::Vec3d v1 = p1 - p0;
    osg::Vec3d v2 = p2 - p0;
    if (v1.isNaN() || v2.isNaN())
    {
      vertices.Remove(*(vertices.cbegin()));
      return boost::none;
    }
    if (v1.length() < std::numeric_limits<float>::epsilon())
    {
      vertices.Remove(*(vertices.cbegin()));
      return boost::none;
    }
    if (v2.length() < std::numeric_limits<float>::epsilon())
    {
      vertices.Remove(*(vertices.cbegin()));
      return boost::none;
    }
    v1.normalize();
    v2.normalize();
    
    if ((1 - std::fabs(v1 * v2)) < std::numeric_limits<float>::epsilon())
    {
      vertices.Remove(*(vertices.cbegin()));
      return boost::none;
    }
    
    osg::Vec3d cross(v1 ^ v2);
    if (cross.isNaN() || cross.length() < std::numeric_limits<float>::epsilon())
    {
      vertices.Remove(*(vertices.cbegin()));
      return boost::none;
    }
    cross.normalize();
    return cross;
  };
  
  auto checkEdge = [&](const TopoDS_Edge &eIn) -> boost::optional<osg::Vec3d>
  {
    vertices.Add(TopExp::FirstVertex(eIn));
    auto normal = checkPoints();
    if (normal)
      return normal;
    vertices.Add(TopExp::LastVertex(eIn));
    normal = checkPoints();
    return normal;
  };
  
  for (const auto &s : svIn)
  {
    if (s.ShapeType() == TopAbs_SHELL)
    {
      TopTools_IndexedMapOfShape fm;
      TopExp::MapShapes(s, TopAbs_FACE, fm);
      occt::FaceVector fs = static_cast<occt::FaceVector>(occt::ShapeVectorCast(fm));
      for (const auto &f : fs)
      {
        BRepAdaptor_Surface sa(f);
        if (sa.GetType() == GeomAbs_Plane)
          return gu::toOsg(occt::getNormal(f, 0.0, 0.0));
      }
    }
    else if (s.ShapeType() == TopAbs_FACE)
    {
      const TopoDS_Face &f = TopoDS::Face(s);
      BRepAdaptor_Surface sa(f);
      if (sa.GetType() == GeomAbs_Plane)
        return gu::toOsg(occt::getNormal(f, 0.0, 0.0));
    }
    else if (s.ShapeType() == TopAbs_WIRE)
    {
      TopTools_IndexedMapOfShape em;
      TopExp::MapShapes(s, TopAbs_EDGE, em);
      occt::EdgeVector es = static_cast<occt::EdgeVector>(occt::ShapeVectorCast(em));
      for (const auto &e : es)
      {
        auto normal = checkEdge(e);
        if (normal)
          return normal;
      }
    }
    else if (s.ShapeType() == TopAbs_EDGE)
    {
      const TopoDS_Edge &e = TopoDS::Edge(s);
      auto normal = checkEdge(e);
      if (normal)
        return normal;
    }
  }
  return boost::none;
}

Extrude::Extrude():
Base()
, sShape(new ann::SeerShape())
, direction(new prm::Parameter(prm::Names::Direction, osg::Vec3d(0.0, 0.0, 1.0)))
, distance(new prm::Parameter(prm::Names::Distance, 1.0))
, offset(new prm::Parameter(prm::Names::Offset, 0.0))
, directionLabel(new lbr::PLabel(direction.get()))
, distanceLabel(new lbr::IPGroup(distance.get()))
, offsetLabel(new lbr::IPGroup(offset.get()))
, internalSwitch(new osg::Switch())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionExtrude.svg");
  
  name = QObject::tr("Extrude");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  direction->connectValue(boost::bind(&Extrude::setModelDirty, this));
  parameters.push_back(direction.get());
  
  distance->connectValue(boost::bind(&Extrude::setModelDirty, this));
  parameters.push_back(distance.get());
  
  offset->connectValue(boost::bind(&Extrude::setModelDirty, this));
  parameters.push_back(offset.get());
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
  
  setupLabels();
}

Extrude::~Extrude(){}

void Extrude::setPicks(const Picks &pIn)
{
  picks = pIn;
  setModelDirty();
}

void Extrude::setAxisPicks(const Picks &pIn)
{
  axisPicks = pIn;
  setDirectionType(DirectionType::Picks);
}

void Extrude::setDirectionType(DirectionType dt)
{
  directionType = dt;
  if (directionType != DirectionType::Picks)
    axisPicks.clear();
  setModelDirty();
  updateLabelVisibility();
}

void Extrude::setupLabels()
{
  overlaySwitch->addChild(internalSwitch.get());
  
  directionLabel->showName = true;
  directionLabel->valueHasChanged();
  directionLabel->constantHasChanged();
  internalSwitch->addChild(directionLabel.get());
  
  distanceLabel->setMatrixDims(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(1.0, 0.0, 0.0)));
  distanceLabel->setRotationAxis(osg::Vec3d(0.0, 0.0, 1.0), osg::Vec3d(0.0, 1.0, 0.0));
  distanceLabel->setMatrix(osg::Matrixd::identity());
  distanceLabel->noAutoRotateDragger();
  internalSwitch->addChild(distanceLabel.get());
  
  offsetLabel->setMatrixDims(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(1.0, 0.0, 0.0)));
  offsetLabel->setRotationAxis(osg::Vec3d(0.0, 0.0, 1.0), osg::Vec3d(0.0, -1.0, 0.0));
  offsetLabel->setMatrix(osg::Matrixd::identity());
  offsetLabel->noAutoRotateDragger();
  internalSwitch->addChild(offsetLabel.get());
  
  directionLabel->valueHasChanged();
  directionLabel->constantHasChanged();
  distanceLabel->valueHasChanged();
  distanceLabel->constantHasChanged();
  offsetLabel->valueHasChanged();
  offsetLabel->constantHasChanged();
}

void Extrude::updateLabels(occt::BoundingBox &bb)
{
  osg::Vec3d z = static_cast<osg::Vec3d>(*direction);
  osg::Vec3d vtc; //vector to cross.
  double dot = 1.0;
  auto check = [&](const osg::Vec3d &vIn)
  {
    double t = std::fabs(z * vIn);
    if (t < dot)
    {
      dot = t;
      vtc = vIn;
    }
  };
  check(osg::Vec3d(1.0, 0.0, 0.0));
  check(osg::Vec3d(0.0, 1.0, 0.0));
  check(osg::Vec3d(0.0, 0.0, 1.0));
  osg::Vec3d y = z ^ vtc; y.normalize();
  osg::Vec3d x = y ^ z; x.normalize();
  osg::Vec3d o = gu::toOsg(bb.getCenter());
  osg::Matrixd m
  (
    x.x(), x.y(), x.z(), 0.0,
    y.x(), y.y(), y.z(), 0.0,
    z.x(), z.y(), z.z(), 0.0,
    o.x(), o.y(), o.z(), 1.0
  );
  
  directionLabel->setMatrix(osg::Matrixd::translate(o + x * bb.getDiagonal() / 2.0));
  distanceLabel->setMatrix(m);
  offsetLabel->setMatrix(m);
  
  updateLabelVisibility();
}

void Extrude::updateLabelVisibility()
{
  if (directionType == DirectionType::Parameter)
    internalSwitch->setChildValue(directionLabel, true);
  else
    internalSwitch->setChildValue(directionLabel, false);
}

void Extrude::updateModel(const UpdatePayload &pIn)
{
  setFailure();
  lastUpdateLog.clear();
  sShape->reset();
  try
  {
    std::vector<const Base*> tfs = pIn.getFeatures(InputType::target);
    if (tfs.size() != 1)
      throw std::runtime_error("wrong number of parents");
    if (!tfs.front()->hasAnnex(ann::Type::SeerShape))
      throw std::runtime_error("parent doesn't have seer shape.");
    const ann::SeerShape &tss = tfs.front()->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
    if (tss.isNull())
      throw std::runtime_error("target seer shape is null");
    
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    //make sure that distance > offset.
    if (static_cast<double>(*offset) >= static_cast<double>(*distance))
      throw std::runtime_error("offset needs to be less than distance");
    //force direction to be unit vector.
    osg::Vec3d td = static_cast<osg::Vec3d>(*direction);
    if (td.length() < std::numeric_limits<float>::epsilon())
      throw std::runtime_error("direction is zero vector");
    td.normalize();
    direction->setValueQuiet(td);
    
    occt::BoundingBox bb;
    occt::ShapeVector ste; //shapes to extrude
    if (picks.empty())
    {
      //treat sketches special
      if (tfs.front()->getType() == Type::Sketch)
      {
        occt::ShapeVectorCast cast(TopoDS::Compound(tss.getRootOCCTShape()));
        occt::WireVector wv = static_cast<occt::WireVector>(cast);
        if (wv.empty())
          throw std::runtime_error("No wires found in sketch");
        TopoDS_Face fte = occt::buildFace(wv);
        if (fte.IsNull())
          throw std::runtime_error("Face construction for sketch failed");
        ShapeCheck fc(fte);
        if (!fc.isValid())
          throw std::runtime_error("Face check failed");
        ste.push_back(fte);
        if (directionType == DirectionType::Infer)
        {
          assert(tfs.front()->hasParameter(prm::Names::CSys));
          const prm::Parameter *sketchSys = tfs.front()->getParameter(prm::Names::CSys);
          osg::Matrixd sm = static_cast<osg::Matrixd>(*sketchSys);
          direction->setValueQuiet(gu::getZVector(sm));
        }
      }
      else //not a sketch
      {
        occt::ShapeVector shapes = occt::getNonCompounds(tss.getRootOCCTShape());
        for (const auto &s : shapes)
        {
          TopAbs_ShapeEnum t = s.ShapeType();
          if
          (
            t == TopAbs_SHELL
            || t == TopAbs_FACE
            || t == TopAbs_WIRE
            || t == TopAbs_EDGE
            || t == TopAbs_VERTEX
          )
            ste.push_back(s);
        }
        if (directionType == DirectionType::Infer)
        {
          auto tempDirection = inferDirection(ste);
          if (tempDirection)
            direction->setValueQuiet(tempDirection.get());
          else
            throw std::runtime_error("couldn't determine direction from geometry");
        }
      }
    }
    else
    {
      //we have sub shape picks.
      std::vector<tls::Resolved> resolved;
      resolved = tls::resolvePicks(tfs, picks, pIn.shapeHistory);
      
      for (const auto &r : resolved)
      {
        if (r.resultId.is_nil())
          continue;
        assert(tss.hasId(r.resultId));
        if (!tss.hasId(r.resultId))
          continue;
        ste.push_back(tss.getOCCTShape(r.resultId));
      }
      if (directionType == DirectionType::Infer)
      {
        auto tempDirection = inferDirection(ste);
        if (tempDirection)
          direction->setValueQuiet(tempDirection.get());
        else
          throw std::runtime_error("couldn't determine direction from pick geometry");
      }
    }
    
    if (ste.empty())
      throw std::runtime_error("Nothing to extrude");
    
    //try to create wires from edges.
    occt::ShapeVector edges;
    for (auto it = ste.begin(); it != ste.end();)
    {
      if (it->ShapeType() == TopAbs_EDGE)
      {
        edges.push_back(*it);
        it = ste.erase(it);
      }
      else
        it++;
    }
    if (!edges.empty())
    {
      TopoDS_Compound wiresOut;
      int result = BOPAlgo_Tools::EdgesToWires(static_cast<TopoDS_Compound>(occt::ShapeVectorCast(edges)), wiresOut);
      if (result != 0)
      {
        //didn't work, add edges back to shapes to extrude.
        std::copy(edges.begin(), edges.end(), std::back_inserter(ste));
      }
      else
      {
        occt::ShapeVector wo = static_cast<occt::ShapeVector>(occt::ShapeVectorCast(wiresOut));
        //it appears that we get only wires. So we have some 1 edge wires which extrudes to a shell. Yuck
        //so here we will detect that condition and avoid it.
        for (const auto &w : wo)
        {
          assert(w.ShapeType() == TopAbs_WIRE);
          occt::EdgeVector ev = static_cast<occt::EdgeVector>(occt::ShapeVectorCast(occt::mapShapes(TopoDS::Wire(w))));
          if (ev.size() == 1)
            ste.push_back(ev.front());
          else
            ste.push_back(w);
        }
      }
    }
    
    if (directionType == DirectionType::Picks)
    {
      std::vector<const Base*> afs = pIn.getFeatures(axisName);
      if (afs.size() == 1 && afs.front()->getType() == Type::DatumAxis)
      {
        const DatumAxis *da = dynamic_cast<const DatumAxis*>(afs.front());
        assert(da);
        direction->setValueQuiet(da->getDirection());
      }
      else if (!afs.empty())
      {
        std::vector<tls::Resolved> resolved;
        resolved = tls::resolvePicks(afs, axisPicks, pIn.shapeHistory);
        boost::optional<osg::Vec3d> tempAxis;
        std::vector<osg::Vec3d> points;
        for (const auto &r : resolved)
        {
          if (r.resultId.is_nil())
            continue;
          const ann::SeerShape &ass = r.feature->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
          if (slc::isPointType(r.pick.selectionType))
          {
            auto tempPoint = r.getPoint(ass);
            if (tempPoint)
            {
              points.push_back(tempPoint.get());
              if (points.size() == 2)
              {
                tempAxis = points.front() - points.back();
                break;
              }
            }
          }
          else
          {
            auto glean = occt::gleanAxis(ass.getOCCTShape(r.resultId));
            if (glean.second)
            {
              tempAxis = gu::toOsg(gp_Vec(glean.first.Direction()));
              break;
            }
          }
        }
        if (tempAxis)
        {
          tempAxis.get().normalize();
          direction->setValueQuiet(tempAxis.get());
        }
        else
          throw std::runtime_error("Couldn't determine direction from pick inputs");
      }
      else
        throw std::runtime_error("No axis picks");
    }
    
    bb.add(ste);
    
    //put all shapes into a compound so we have 1 extrusion
    //this should make mapping a little easier.
    TopoDS_Compound stec = static_cast<TopoDS_Compound>(occt::ShapeVectorCast(ste));
    
    //store offset ids for use after we move the shape.
    occt::ShapeVector previousMap = occt::mapShapes(stec);
    std::vector<uuid> oldIds;
    for (const auto &s : previousMap)
    {
      //we create some shapes, so skip those.
      if (!tss.hasShape(s))
      {
        oldIds.push_back(gu::createNilId()); //place holder.
        continue;
      }
      uuid tId = tss.findId(s);
      oldIds.push_back(tId);
    }
    
    double lo = static_cast<double>(*offset);
    double ld = static_cast<double>(*distance);
    osg::Vec3d ldir = static_cast<osg::Vec3d>(*direction);
    
    //move shape, if necessary, and extrude
    //moving of shape will mess up id mapping.
    if (lo != 0.0)
      occt::moveShape(stec, gu::toOcc(ldir), lo);
    BRepPrimAPI_MakePrism extruder(stec, gu::toOcc(ldir * (ld - lo)));
    if (!extruder.IsDone())
      throw std::runtime_error("Extrusion operation failed");
    
    sShape->setOCCTShape(extruder.Shape());
    
    occt::ShapeVector postMap = occt::mapShapes(stec);
    assert((previousMap.size() == postMap.size()) && (postMap.size() == oldIds.size()));
    int count = -1;
    for (const auto &s : postMap)
    {
      count++;
      if (!tss.hasShape(s))
        continue;
      
      //this is for original shapes and uses original map.
      //in BRepPrimAPI_MakePrism terminology these are 'FirstShape'
      uuid oldId = oldIds.at(count);
      uuid newId = gu::createRandomId();
      auto oIt = originalMap.find(oldId);
      if (oIt == originalMap.end())
        originalMap.insert(std::make_pair(oldId, newId));
      else
        newId = oIt->second;
      sShape->updateId(s, newId);
      if (!sShape->hasEvolveRecord(oldId, newId))
      {
        assert(!sShape->hasEvolveRecordIn(oldId));
        sShape->insertEvolve(oldId, newId);
      }
      
      //this is for the 'transition' shapes and uses generated map.
      //transition from FirstShapes to LastShapes.
      //I am getting a wire that generates a shell but the shell is not in the shape. bug?
      const TopTools_ListOfShape &generated = extruder.Generated(s);
      if (!generated.IsEmpty() && sShape->hasShape(generated.First()))
      {
        uuid gId = gu::createRandomId();
        auto gIt = generatedMap.find(oldId);
        if (gIt == generatedMap.end())
          generatedMap.insert(std::make_pair(oldId, gId));
        else
          gId = gIt->second;
        sShape->updateId(generated.First(), gId);
        if (!sShape->hasEvolveRecord(gu::createNilId(), gId))
        {
          assert(!sShape->hasEvolveRecordOut(gId));
          sShape->insertEvolve(gu::createNilId(), gId);
        }
        if (generated.Extent() > 1)
          std::cout << "Warning: more than 1 generated shape in extrude." << std::endl;
      }
      
      //this is for the new shapes at the top of the prism.
      //in BRepPrimAPI_MakePrism terminology these are 'LastShape'
      const TopoDS_Shape &last = extruder.LastShape(s);
      if (!last.IsNull() && sShape->hasShape(last))
      {
        uuid lId = gu::createRandomId();
        auto lIt = lastMap.find(oldId);
        if (lIt == lastMap.end())
          lastMap.insert(std::make_pair(oldId, lId));
        else
          lId = lIt->second;
        sShape->updateId(last, lId);
        if (!sShape->hasEvolveRecord(gu::createNilId(), lId))
        {
          assert(!sShape->hasEvolveRecordOut(lId));
          sShape->insertEvolve(gu::createNilId(), lId);
        }
      }
    }
    
    sShape->outerWireMatch(tss);
    
    //this should assign ids to nil outerwires based upon parent face id.
    occt::ShapeVector ns = sShape->getAllNilShapes();
    for (const auto &s : ns)
    {
      if (s.ShapeType() != TopAbs_WIRE)
        continue;
      occt::ShapeVector ps = sShape->useGetParentsOfType(s, TopAbs_FACE);
      if (ps.empty())
        continue;
      uuid fId = sShape->findId(ps.front());
      if (fId.is_nil())
        continue;
      uuid wId = gu::createRandomId();
      auto it = oWireMap.find(fId);
      if (it == oWireMap.end())
        oWireMap.insert(std::make_pair(fId, wId));
      else
        wId = it->second;
      sShape->updateId(s, wId);
      if (!sShape->hasEvolveRecord(gu::createNilId(), wId))
      {
        assert(!sShape->hasEvolveRecordOut(wId));
        sShape->insertEvolve(gu::createNilId(), wId);
      }
    }
    
    // we are currently not consistently identifying some constructed
    // entities. For example: wires created from connecting edges. 
    
    sShape->derivedMatch();
    sShape->uniqueTypeMatch(tss);
    sShape->dumpNils("extrude feature");
    sShape->dumpDuplicates("extrude feature");
    sShape->ensureNoNils();
    sShape->ensureNoDuplicates();
    
    setSuccess();
    updateLabels(bb);
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in extrude update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in extrude update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in extrude update. " << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Extrude::serialWrite(const boost::filesystem::path &dIn)
{
  auto serializeMap = [](const std::map<uuid, uuid> &map) -> prj::srl::EvolveContainer
  {
    prj::srl::EvolveContainer out;
    for (const auto &p : map)
    {
      prj::srl::EvolveRecord r
      (
        gu::idToString(p.first),
        gu::idToString(p.second)
      );
      out.evolveRecord().push_back(r);
    }
    return out;
  };
  
  prj::srl::FeatureExtrude so
  (
    Base::serialOut(),
    ftr::serialOut(picks),
    ftr::serialOut(axisPicks),
    direction->serialOut(),
    directionLabel->serialOut(),
    distance->serialOut(),
    distanceLabel->serialOut(),
    offset->serialOut(),
    offsetLabel->serialOut(),
    static_cast<int>(directionType),
    serializeMap(originalMap),
    serializeMap(generatedMap),
    serializeMap(lastMap),
    serializeMap(oWireMap)
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::extrude(stream, so, infoMap);
}

void Extrude::serialRead(const prj::srl::FeatureExtrude &so)
{
  auto serializeMap = [](const prj::srl::EvolveContainer &container) -> std::map<uuid, uuid>
  {
    std::map<uuid, uuid> out;
    for (const auto &r : container.evolveRecord())
      out.insert(std::make_pair(gu::stringToId(r.idIn()), gu::stringToId(r.idOut())));
    return out;
  };
  
  Base::serialIn(so.featureBase());
  picks = ftr::serialIn(so.picks());
  axisPicks = ftr::serialIn(so.axisPicks());
  direction->serialIn(so.direction());
  directionLabel->serialIn(so.directionLabel());
  distance->serialIn(so.distance());
  distanceLabel->serialIn(so.distanceLabel());
  offset->serialIn(so.offset());
  offsetLabel->serialIn(so.offsetLabel());
  directionType = static_cast<DirectionType>(so.directionType());
  originalMap = serializeMap(so.originalMap());
  generatedMap = serializeMap(so.generatedMap());
  lastMap = serializeMap(so.lastMap());
  oWireMap = serializeMap(so.oWireMap());
  
  directionLabel->valueHasChanged();
  directionLabel->constantHasChanged();
  distanceLabel->valueHasChanged();
  distanceLabel->constantHasChanged();
  offsetLabel->valueHasChanged();
  offsetLabel->constantHasChanged();
  updateLabelVisibility();
}
