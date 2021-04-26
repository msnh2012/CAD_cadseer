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

#include <osg/Switch>

#include <TopoDS.hxx>
#include <TopExp.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BOPAlgo_Tools.hxx>

#include "globalutilities.h"
#include "tools/occtools.h"
#include "tools/tlsosgtools.h"
#include "annex/annseershape.h"
#include "library/lbrplabel.h"
#include "library/lbripgroup.h"
#include "feature/ftrshapecheck.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinputtype.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "feature/ftrdatumaxis.h"
#include "tools/featuretools.h"
#include "tools/idtools.h"
#include "project/serial/generated/prjsrlexrsextrude.h"
#include "feature/ftrextrude.h"

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
    
    auto ocsys = tls::matrixFrom3Points(p0, p1, p2);
    if (!ocsys)
    {
      vertices.Remove(*(vertices.cbegin()));
      return boost::none;
    }
    
    return gu::getZVector(ocsys.get());
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
  
  if (svIn.size() == 1)
  {
    auto result = occt::gleanAxis(svIn.front());
    if (result.second)
      return gu::toOsg(result.first.Direction());
  }
  
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
, sShape(std::make_unique<ann::SeerShape>())
, direction(std::make_unique<prm::Parameter>(prm::Names::Direction, osg::Vec3d(0.0, 0.0, 1.0), prm::Tags::Direction))
, distance(std::make_unique<prm::Parameter>(prm::Names::Distance, 1.0, prm::Tags::Distance))
, offset(std::make_unique<prm::Parameter>(prm::Names::Offset, 0.0, prm::Tags::Offset))
, directionObserver(std::make_unique<prm::Observer>(std::bind(&Extrude::setModelDirty, this)))
, directionLabel(new lbr::PLabel(direction.get()))
, distanceLabel(new lbr::IPGroup(distance.get()))
, offsetLabel(new lbr::IPGroup(offset.get()))
, internalSwitch(new osg::Switch())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionExtrude.svg");
  
  name = QObject::tr("Extrude");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  direction->connect(*directionObserver);
  parameters.push_back(direction.get());
  
  distance->connectValue(std::bind(&Extrude::setModelDirty, this));
  parameters.push_back(distance.get());
  
  offset->connectValue(std::bind(&Extrude::setModelDirty, this));
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
}

void Extrude::updateLabels(occt::BoundingBox &bb)
{
  osg::Vec3d z = direction->getVector();
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
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    prm::ObserverBlocker directionBlock(*directionObserver);
    
    if (picks.size() != 1) //only 1 for now.
      throw std::runtime_error("wrong number of target picks.");
    tls::Resolver tResolver(pIn);
    tResolver.resolve(picks.front());
    if (!tResolver.getFeature() || !tResolver.getSeerShape() || tResolver.getShapes().empty())
      throw std::runtime_error("target resolution failed.");
    
    //make sure that distance > offset.
    if (offset->getDouble() >= distance->getDouble())
      throw std::runtime_error("offset needs to be less than distance");
    //force direction to be unit vector.
    osg::Vec3d td = direction->getVector();
    if (td.length() < std::numeric_limits<float>::epsilon())
      throw std::runtime_error("direction is zero vector");
    td.normalize();
    direction->setValue(td);
    
    occt::ShapeVector ste; //shapes to extrude
    if (slc::isObjectType(picks.front().selectionType))
    {
      //treat sketches special
      if (tResolver.getFeature()->getType() == Type::Sketch)
      {
        occt::ShapeVectorCast cast(TopoDS::Compound(tResolver.getShapes().front()));
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
          auto systemParameters =  tResolver.getFeature()->getParameters(prm::Tags::CSys);
          assert(!systemParameters.empty()); //sketches should have a csys.
          osg::Matrixd sm = systemParameters.front()->getMatrix();
          direction->setValue(gu::getZVector(sm));
        }
      }
      else //not a sketch
      {
        occt::ShapeVector shapes = tResolver.getSeerShape()->useGetNonCompoundChildren();
        if (shapes.empty())
          throw std::runtime_error("No resolved non compound children");
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
            direction->setValue(tempDirection.get());
          else
            throw std::runtime_error("couldn't determine direction from geometry");
        }
      }
    }
    else //target pick is a shape selection.
    {
      ste = tResolver.getShapes();
      if (directionType == DirectionType::Infer)
      {
        auto tempDirection = inferDirection(ste);
        if (tempDirection)
          direction->setValue(tempDirection.get());
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
      if (axisPicks.empty() || axisPicks.size() > 2)
        throw std::runtime_error("wrong number of axis picks");
      
      
      if (axisPicks.size() == 1)
      {
        if (slc::isPointType(axisPicks.front().selectionType))
          throw std::runtime_error("Can't determine direction from 1 point");
        tls::Resolver aResolver(pIn);
        if (!aResolver.resolve(axisPicks.front()))
          throw std::runtime_error("Resolution failure for first axis pick");
        if (slc::isObjectType(axisPicks.front().selectionType))
        {
          auto csysPrms = aResolver.getFeature()->getParameters(prm::Tags::CSys);
          if (!csysPrms.empty())
          {
            direction->setValue(gu::getZVector(csysPrms.front()->getMatrix()));
          }
          else if (aResolver.getFeature()->getType() == Type::DatumAxis)
          {
            const DatumAxis *da = dynamic_cast<const DatumAxis*>(aResolver.getFeature());
            assert(da);
            direction->setValue(da->getDirection());
          }
        }
        else 
        {
          auto shapes = aResolver.getShapes();
          if (shapes.empty())
            throw std::runtime_error("No resolved shapes for 1 axis pick");
          if (shapes.size() > 1)
          {
            std::ostringstream s; s << "WARNING: More than 1 shape resolved for 1 axis pick." << std::endl;
            lastUpdateLog += s.str();
          }
          auto glean = occt::gleanAxis(shapes.front());
          if (glean.second)
            direction->setValue(gu::toOsg(gp_Vec(glean.first.Direction())));
          else
            throw std::runtime_error("Couldn't glean direction for 1 axis pick");
        }
      }
      else //2 pick axis
      {
        if (!slc::isPointType(axisPicks.front().selectionType) || !slc::isPointType(axisPicks.back().selectionType))
          throw std::runtime_error("Wrong type for 2 pick axis");
        tls::Resolver resolver(pIn);
        resolver.resolve(axisPicks.front());
        auto points0 = resolver.getPoints();
        resolver.resolve(axisPicks.back());
        auto points1 = resolver.getPoints();
        if (points0.empty() || points1.empty())
          throw std::runtime_error("Failed to resolve 2 axis pick points");
        if (points0.size() > 1 || points1.size() > 1)
        {
          std::ostringstream s; s << "WARNING: More than 1 shape resolved for 2 pick axis." << std::endl;
          lastUpdateLog += s.str();
        }
        osg::Vec3d tAxis = points0.front() - points1.front();
        tAxis.normalize();
        direction->setValue(tAxis);
      }
    }
    
    occt::BoundingBox bb;
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
      if (!tResolver.getSeerShape()->hasShape(s))
      {
        oldIds.push_back(gu::createNilId()); //place holder.
        continue;
      }
      uuid tId = tResolver.getSeerShape()->findId(s);
      oldIds.push_back(tId);
    }
    
    double lo = offset->getDouble();
    double ld = distance->getDouble();
    osg::Vec3d ldir = direction->getVector();
    
    //move shape, if necessary, and extrude
    //moving of shape will mess up id mapping.
    if (lo != 0.0)
      occt::moveShape(stec, gu::toOcc(ldir), lo);
    BRepPrimAPI_MakePrism extruder(stec, gu::toOcc(ldir * (ld - lo)));
    if (!extruder.IsDone())
      throw std::runtime_error("Extrusion operation failed");
    
    sShape->setOCCTShape(extruder.Shape(), getId());
    
    occt::ShapeVector postMap = occt::mapShapes(stec);
    assert((previousMap.size() == postMap.size()) && (postMap.size() == oldIds.size()));
    int count = -1;
    for (const auto &s : postMap)
    {
      count++;
      if (oldIds.at(count).is_nil())
        continue;
      
      if (!sShape->hasShape(s))
        continue; //container shapes like: wire and shell won't be present in the output so skip.
      
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
    
    sShape->outerWireMatch(*tResolver.getSeerShape());
    
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
    sShape->uniqueTypeMatch(*tResolver.getSeerShape());
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
  auto serializeMap = [](const std::map<uuid, uuid> &map) -> prj::srl::spt::SeerShape::EvolveContainerSequence
  {
    prj::srl::spt::SeerShape::EvolveContainerSequence out;
    for (const auto &p : map)
    {
      prj::srl::spt::SeerShape::EvolveContainerType r
      (
        gu::idToString(p.first),
        gu::idToString(p.second)
      );
      out.push_back(r);
    }
    return out;
  };
  
  prj::srl::exrs::Extrude so
  (
    Base::serialOut(),
    sShape->serialOut(),
    direction->serialOut(),
    directionLabel->serialOut(),
    distance->serialOut(),
    distanceLabel->serialOut(),
    offset->serialOut(),
    offsetLabel->serialOut(),
    static_cast<int>(directionType)
  );
  for (const auto &p : picks)
    so.picks().push_back(p);
  for (const auto &p : axisPicks)
    so.axisPicks().push_back(p);
  so.originalMap() = serializeMap(originalMap);
  so.generatedMap() = serializeMap(generatedMap);
  so.lastMap() = serializeMap(lastMap);
  so.oWireMap() = serializeMap(oWireMap);
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::exrs::extrude(stream, so, infoMap);
}

void Extrude::serialRead(const prj::srl::exrs::Extrude &so)
{
  auto serializeMap = [](const prj::srl::spt::SeerShape::EvolveContainerSequence &container) -> std::map<uuid, uuid>
  {
    std::map<uuid, uuid> out;
    for (const auto &r : container)
      out.insert(std::make_pair(gu::stringToId(r.idIn()), gu::stringToId(r.idOut())));
    return out;
  };
  
  Base::serialIn(so.base());
  sShape->serialIn(so.seerShape());
  direction->serialIn(so.direction());
  directionLabel->serialIn(so.directionLabel());
  distance->serialIn(so.distance());
  distanceLabel->serialIn(so.distanceLabel());
  offset->serialIn(so.offset());
  offsetLabel->serialIn(so.offsetLabel());
  directionType = static_cast<DirectionType>(so.directionType());
  for (const auto &p : so.picks())
    picks.emplace_back(p);
  for (const auto &p : so.axisPicks())
    axisPicks.emplace_back(p);
  originalMap = serializeMap(so.originalMap());
  generatedMap = serializeMap(so.generatedMap());
  lastMap = serializeMap(so.lastMap());
  oWireMap = serializeMap(so.oWireMap());
  
  updateLabelVisibility();
}
