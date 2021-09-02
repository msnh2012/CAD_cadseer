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
#include <boost/optional/optional.hpp>

#include <BRepFill.hxx>
#include <TopoDS.hxx>
#include <TopExp.hxx>
#include <BRep_Tool.hxx>
#include <BRepTools.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <BRepBuilderAPI_Sewing.hxx>

#include <osg/Switch>

#include "globalutilities.h"
#include "annex/annseershape.h"
#include "tools/occtools.h"
#include "tools/featuretools.h"
#include "tools/idtools.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "feature/ftrshapecheck.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinputtype.h"
#include "project/serial/generated/prjsrlrldsruled.h"
#include "feature/ftrruled.h"

using namespace ftr::Ruled;
using boost::uuids::uuid;
QIcon Feature::icon = QIcon(":/resources/images/constructionRuled.svg");

namespace
{
  using IdSet = std::set<uuid>;
  using SetToIdMap = std::map<IdSet, uuid>;
}

struct Feature::Stow
{
  Feature &feature;
  prm::Parameter picks{QObject::tr("Edge Picks"), ftr::Picks(), prm::Tags::Picks};
  ann::SeerShape sShape;
  uuid parentId = {gu::createRandomId()}; //!< edge to edge = face id. wire to wire = shell id.
  SetToIdMap efMap; //!< map 2 edges to one generated face. only used on wires
  SetToIdMap veMap; //!< map 2 verts to one projected edge.
  std::map<uuid, uuid> outerWireMap; //!< map face id to wire id
  
  Stow() = delete;
  Stow(Feature &fIn)
  : feature(fIn)
  {
    picks.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&picks);
    
    feature.annexes.insert(std::make_pair(ann::Type::SeerShape, &sShape));
  }
  
  void goRuledMapping()
  {
    //first we are going to assign ids to the faces.
    //the seer shape shapeMatching function calls should assign an id to 2 edges of each face.
    //note the assigning of face ids is only needed for the 'wires to shell' mode.
    //in the 'edges to face' mode, parentId is assigned to the one face.
    auto getFaceEdgesSet = [&](const TopoDS_Shape &fIn) -> IdSet
    {
      IdSet out;
      auto edges = sShape.useGetChildrenOfType(fIn, TopAbs_EDGE);
      for (const auto &e : edges)
      {
        const auto &eId = sShape.findId(e);
        if (!eId.is_nil())
          out.insert(eId);
      }
      return out;
    };
    
    auto faces = sShape.useGetChildrenOfType(sShape.getRootOCCTShape(), TopAbs_FACE);
    for (const auto &f : faces)
    {
      auto fId = sShape.findId(f);
      if (fId.is_nil())
      {
        auto eIdSet = getFaceEdgesSet(f);
        if (eIdSet.size() != 2)
        {
          std::cout << "Warning: Have a face that doesn't have 2 edges with an id in: " << BOOST_CURRENT_FUNCTION << std::endl;
          continue;
        }
        if (efMap.count(eIdSet) != 0)
          fId = efMap.at(eIdSet);
        else
        {
          fId = gu::createRandomId();
          efMap.insert(std::make_pair(eIdSet, fId));
        }
        assert(!fId.is_nil());
        sShape.updateId(f, fId);
      }
      if (!sShape.hasEvolveRecordOut(fId))
        sShape.insertEvolve(gu::createNilId(), fId);
      
      //do the outer wire.
      TopoDS_Wire oWire = BRepTools::OuterWire(TopoDS::Face(f));
      assert(sShape.hasShape(oWire));
      if (!sShape.findId(oWire).is_nil())
        continue; //not sure how this could be, but safety.
      uuid theOuterWireId;
      if (outerWireMap.count(fId) != 0)
        theOuterWireId = outerWireMap.at(fId);
      else
      {
        theOuterWireId = gu::createRandomId();
        outerWireMap.insert(std::make_pair(fId, theOuterWireId));
      }
      assert(!theOuterWireId.is_nil());
      sShape.updateId(oWire, theOuterWireId);
      if (!sShape.hasEvolveRecordOut(theOuterWireId))
        sShape.insertEvolve(gu::createNilId(), theOuterWireId);
    }
    
    //now assign ids to the projected edges. These are the new edges generated between
    //the corresponding vertices of the edges. We map these from the vertices that should
    //already have ids by the seer shape shapeMatching function calls.
    auto getEdgeVerticesSet = [&](const TopoDS_Shape &eIn) -> IdSet
    {
      IdSet out;
      auto vertices = sShape.useGetChildrenOfType(eIn, TopAbs_VERTEX);
      for (const auto &v : vertices)
      {
        const auto &vId = sShape.findId(v);
        if (!vId.is_nil())
          out.insert(vId);
      }
      return out;
    };
    
    auto edges = sShape.useGetChildrenOfType(sShape.getRootOCCTShape(), TopAbs_EDGE);
    for (const auto &e : edges)
    {
      const auto &eId = sShape.findId(e);
      if (!eId.is_nil())
        continue;
      auto vIdSet = getEdgeVerticesSet(e);
      if (vIdSet.size() != 2)
      {
        std::cout << "Warning: Have an edge that doesn't have 2 vertices with an id in: " << BOOST_CURRENT_FUNCTION << std::endl;
        continue;
      }
      uuid theEdgeId;
      if (veMap.count(vIdSet) != 0)
        theEdgeId = veMap.at(vIdSet);
      else
      {
        theEdgeId = gu::createRandomId();
        veMap.insert(std::make_pair(vIdSet, theEdgeId));
      }
      sShape.updateId(e, theEdgeId);
      if (!sShape.hasEvolveRecordOut(theEdgeId))
        sShape.insertEvolve(gu::createNilId(), theEdgeId);
    }
  }
};

Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  name = QObject::tr("Ruled");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
}

Feature::~Feature() = default;

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
      throw std::runtime_error("Don't have 2 picks");
    tls::Resolver resolver(pIn);
    
    resolver.resolve(picks.front());
    if (!resolver.getSeerShape())
      throw std::runtime_error("no seer shape for first pick");
    const ann::SeerShape &tss0 = *resolver.getSeerShape();
    auto rShapes0 = resolver.getShapes();
    if (rShapes0.empty())
      throw std::runtime_error("no resolved shapes for first pick");
    if (rShapes0.size() > 1)
    {
      std::ostringstream s; s << "WARNING: more than 1 shape for first pick" << std::endl;
      lastUpdateLog += s.str();
    }
    TopoDS_Shape shape0 = rShapes0.front();
    if (!slc::isShapeType(picks.front().selectionType))
      shape0 = occt::getFirstNonCompound(shape0);
    
    resolver.resolve(picks.back());
    if (!resolver.getSeerShape())
      throw std::runtime_error("no seer shape for second pick");
    const ann::SeerShape &tss1 = *resolver.getSeerShape();
    auto rShapes1 = resolver.getShapes();
    if (rShapes1.empty())
      throw std::runtime_error("no resolved shapes for first pick");
    if (rShapes1.size() > 1)
    {
      std::ostringstream s; s << "WARNING: more than 1 shape for second pick" << std::endl;
      lastUpdateLog += s.str();
    }
    TopoDS_Shape shape1 = rShapes1.front();
    if (!slc::isShapeType(picks.back().selectionType))
      shape1 = occt::getFirstNonCompound(shape1);

    if (shape0.IsNull() || shape1.IsNull())
      throw std::runtime_error("Null resolved shapes");
    
    //occt doesn't orient the edges by default and we get 'twisted' surfaces
    //so we reverse one edge to avoid it.
    if (shape0.ShapeType() == TopAbs_EDGE && shape1.ShapeType() == TopAbs_EDGE)
    {
      TopoDS_Edge edge0 = TopoDS::Edge(shape0);
      TopoDS_Edge edge1 = TopoDS::Edge(shape1);
      TopoDS_Vertex e0f, e0l, e1f, e1l;
      TopExp::Vertices(edge0, e0f, e0l, Standard_True);
      TopExp::Vertices(edge1, e1f, e1l, Standard_True);
      double distance0 = BRep_Tool::Pnt(e0f).Distance(BRep_Tool::Pnt(e1f));
      double distance1 = BRep_Tool::Pnt(e0f).Distance(BRep_Tool::Pnt(e1l));
      if (distance0 > distance1)
        edge1.Reverse();
      
      TopoDS_Face face = BRepFill::Face(edge0, edge1);
      ShapeCheck check(face);
      if (!check.isValid())
        throw std::runtime_error("BRepFill::Face result is invalid");
      stow->sShape.setOCCTShape(face, getId());
      stow->sShape.updateId(face, stow->parentId);
    }
    else if (shape0.ShapeType() == TopAbs_WIRE && shape1.ShapeType() == TopAbs_WIRE)
    {
      //BRepFill::Shell is completely FUBAR as of occt 7.3
      //determine if we shape will twist and reverse 1 wire if necessary.
      TopoDS_Wire wire0 = TopoDS::Wire(shape0);
      TopoDS_Wire wire1 = TopoDS::Wire(shape1);
      TopoDS_Vertex w0f, w0l, w1f, w1l;
      TopExp::Vertices(wire0, w0f, w0l);
      TopExp::Vertices(wire1, w1f, w1l);
      double distance0 = BRep_Tool::Pnt(w0f).Distance(BRep_Tool::Pnt(w1f));
      double distance1 = BRep_Tool::Pnt(w0f).Distance(BRep_Tool::Pnt(w1l));
      if (distance0 > distance1)
        wire1.Reverse();
      
      //it looks like BRepTools_WireExplorer is ignoring orientation of the wire.
      //so load edges into an EdgeVector and reverse if necessary.
      auto extractEdges = [](const TopoDS_Wire &wIn) -> occt::EdgeVector
      {
        occt::EdgeVector out;
        for (BRepTools_WireExplorer exp(wIn); exp.More(); exp.Next())
          out.push_back(exp.Current());
        if (wIn.Orientation() == TopAbs_REVERSED)
        {
          std::reverse(out.begin(), out.end());
          for (auto &e : out)
            e.Reverse();
        }
        return out;
      };
      occt::EdgeVector edges0 = extractEdges(wire0);
      occt::EdgeVector edges1 = extractEdges(wire1);
      if (edges0.size() != edges1.size() || edges0.empty())
        throw std::runtime_error("empty or mismatched edge vectors");

      BRepBuilderAPI_Sewing builder(0.000001, true, true, true, false);
      for (std::size_t i = 0; i < edges0.size(); ++i)
      {
        TopoDS_Face face = BRepFill::Face(edges0.at(i), edges1.at(i));
        if (!face.IsNull())
          builder.Add(face);
      }
      builder.Perform();
      ShapeCheck check(builder.SewedShape());
      if (!check.isValid())
        throw std::runtime_error("SewedShape result is invalid");
      TopoDS_Shape nc = occt::getFirstNonCompound(builder.SewedShape());
      if (nc.IsNull() || nc.ShapeType() != TopAbs_SHELL)
        throw std::runtime_error("SewedShape result is not a shell");
      
      stow->sShape.setOCCTShape(builder.SewedShape(), getId());
      stow->sShape.updateId(builder.SewedShape(), stow->parentId);
    }
    else
      throw std::runtime_error("Unsupported shape types");
    
    stow->sShape.shapeMatch(tss0);
    stow->sShape.shapeMatch(tss1);
    stow->goRuledMapping();
    stow->sShape.dumpNils("ruled");
    stow->sShape.ensureNoNils();
    stow->sShape.ensureNoDuplicates();
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
  prj::srl::rlds::Ruled so
  (
    Base::serialOut()
    , stow->picks.serialOut()
    , stow->sShape.serialOut()
    , gu::idToString(stow->parentId)
  );
  for (const auto &r : stow->efMap)
  {
    prj::srl::spt::DerivedRecord ro(gu::idToString(r.second));
    for (const auto &s : r.first)
      ro.idSet().push_back(gu::idToString(s));
    so.efMap().push_back(ro);
  }
  for (const auto &r : stow->veMap)
  {
    prj::srl::spt::DerivedRecord ro(gu::idToString(r.second));
    for (const auto &s : r.first)
      ro.idSet().push_back(gu::idToString(s));
    so.veMap().push_back(ro);
  }
  for (const auto &p : stow->outerWireMap)
    so.outerWireMap().push_back(prj::srl::spt::EvolveRecord(gu::idToString(p.first), gu::idToString(p.second)));

  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::rlds::ruled(stream, so, infoMap);
}

void Feature::serialRead(const prj::srl::rlds::Ruled &so)
{
  Base::serialIn(so.base());
  stow->picks.serialIn(so.picks());
  stow->sShape.serialIn(so.seerShape());
  stow->parentId = gu::stringToId(so.parentId());
  
  for (const auto &p : so.efMap())
  {
    IdSet set;
    for (const auto &sids : p.idSet())
      set.insert(gu::stringToId(sids));
    stow->efMap.insert(std::make_pair(set, gu::stringToId(p.id())));
  }
  for (const auto &p : so.veMap())
  {
    IdSet set;
    for (const auto &sids : p.idSet())
      set.insert(gu::stringToId(sids));
    stow->veMap.insert(std::make_pair(set, gu::stringToId(p.id())));
  }
  for (const auto &owm : so.outerWireMap())
    stow->outerWireMap.insert(std::make_pair(gu::stringToId(owm.idIn()), gu::stringToId(owm.idOut())));
}
