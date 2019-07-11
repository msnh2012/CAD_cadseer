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

#include <boost/optional/optional.hpp>

#include <TopoDS.hxx>
#include <BRep_Tool.hxx>

#include "globalutilities.h"
#include "annex/annseershape.h"
#include "selection/slcmessage.h"
#include "selection/slccontainer.h"
#include "feature/ftrshapehistory.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrpick.h"
#include "feature/ftrbase.h"
#include "tools/idtools.h"
#include "tools/featuretools.h"

using boost::uuids::uuid;

using namespace tls;

Resolved::Resolved(const ftr::Base *fIn, const boost::uuids::uuid &idIn, const ftr::Pick &pIn):
feature(fIn),
resultId(idIn),
pick(pIn)
{}

Resolved::Resolved(const ftr::Base *fIn, const ftr::Pick &pIn):
feature(fIn),
pick(pIn)
{}

Resolved::Resolved(const ftr::Pick &pIn):
pick(pIn)
{}

bool Resolved::operator< (const Resolved &other) const
{
  if (feature < other.feature)
    return true;
  if (resultId < other.resultId)
    return true;
  
  return false;
}

bool Resolved::operator== (const Resolved &other) const
{
  if
  (
    (feature == other.feature)
    && (resultId == other.resultId)
  )
  return true;
  
  return false;
}

/*! @brief get occt shapes.
 * 
 * @return can be empty
 * @details assumes Pick::resolvedIds have been filled in.
 * startPoint and endPoints will return a TopoDS_Vertex.
 * other points will return a TopoDS_Edge.
 */
occt::ShapeVector Resolved::getShapes() const
{
  occt::ShapeVector out;
  
  if (!feature || !feature->hasAnnex(ann::Type::SeerShape))
    return out;
  
  const ann::SeerShape &ss = feature->getAnnex<ann::SeerShape>();
  if (ss.isNull())
    return out;
  
  if (pick.selectionType == slc::Type::Object || pick.selectionType == slc::Type::Feature)
  {
    const TopoDS_Shape &shape = ss.getRootOCCTShape();
    if (!shape.IsNull())
      out.push_back(shape);
    return out;
  }
  
  for (const auto rid : pick.resolvedIds)
  {
    assert(ss.hasId(rid));
    out.push_back(ss.getOCCTShape(rid));
  }
  
  return out;
}

boost::optional<osg::Vec3d> Resolved::getPoint(const ann::SeerShape &ssIn) const
{
  if (!slc::isPointType(pick.selectionType))
    return boost::none;
  assert(ssIn.hasId(resultId));
  if(!ssIn.hasId(resultId))
    return boost::none;
  const TopoDS_Shape &rs = ssIn.getOCCTShape(resultId);
  if (rs.ShapeType() == TopAbs_VERTEX)
  {
    return gu::toOsg(BRep_Tool::Pnt(TopoDS::Vertex(rs)));
  }
  if (rs.ShapeType() == TopAbs_EDGE)
  {
    boost::optional<osg::Vec3d> p;
    if (pick.selectionType == slc::Type::StartPoint)
      p = gu::toOsg(BRep_Tool::Pnt(TopoDS::Vertex(ssIn.getOCCTShape(ssIn.useGetStartVertex(resultId)))));
    if (pick.selectionType == slc::Type::EndPoint)
      p = gu::toOsg(BRep_Tool::Pnt(TopoDS::Vertex(ssIn.getOCCTShape(ssIn.useGetEndVertex(resultId)))));
    if (pick.selectionType == slc::Type::MidPoint)
    {
      auto a = ssIn.useGetMidPoint(resultId);
      if (!a.empty())
        p = a.front();
    }
    if (pick.selectionType == slc::Type::CenterPoint)
    {
      auto a = ssIn.useGetCenterPoint(resultId);
      if (!a.empty())
        p = a.front();
    }
    if (pick.selectionType == slc::Type::QuadrantPoint || pick.selectionType == slc::Type::NearestPoint)
    {
      p = pick.getPoint(TopoDS::Edge(rs));
    }
    if (!p)
      return boost::none;
    
    return p.get();
  }
  
  return boost::none;
}

std::vector<Resolved> tls::resolvePicks
(
  const std::vector<const ftr::Base*> &features,
  std::vector<ftr::Pick> picks, //make a work copy of the picks.
  const ftr::ShapeHistory &pHistory
)
{
  std::vector<Resolved> out;
  
  for (const auto *tf : features)
  {
    //i can see wrong resolution happening here. the input graph edges don't store the shapeid and the picks
    //don't store the feature id. When shape history has splits and joins and we have multiple tools with picks,
    //it will be easy to resolve to a different shape then intended by user. Not sure what to do.
    assert(tf->hasAnnex(ann::Type::SeerShape)); //caller verifies.
    if (!tf->hasAnnex(ann::Type::SeerShape))
      continue;
    const ann::SeerShape &toolSeerShape = tf->getAnnex<ann::SeerShape>();
    assert(!toolSeerShape.isNull()); //caller verifies.
    if (toolSeerShape.isNull())
      continue;
    
    bool foundPick = false;
    for (auto it = picks.begin(); it != picks.end();)
    {
      std::vector<boost::uuids::uuid> rids = pHistory.resolveHistories(it->shapeHistory, tf->getId());
      if (rids.empty())
      {
        ++it;
        continue;
      }
      for (const auto rid : rids)
      {
        assert(toolSeerShape.hasId(rid)); //project shape history and the feature shapeid records out of sync.
        if (!toolSeerShape.hasId(rid))
          continue;
        out.push_back(Resolved(tf, rid, *it));
      }
      foundPick = true;
      it = picks.erase(it);
    }
    if (!foundPick) //we assume the whole shape.
      out.push_back(Resolved(tf, gu::createNilId(), ftr::Pick()));
  }
  gu::uniquefy(out);
  
  return out;
}

std::vector<Resolved> tls::resolvePicks
(
  const ftr::Base *feature,
  const ftr::Pick &pick,
  const ftr::ShapeHistory &pHistory
)
{
  std::vector<const ftr::Base*> features(1, feature);
  ftr::Picks picks(1, pick);
  return tls::resolvePicks(features, picks, pHistory);
}

Resolved tls::resolvePick
(
  const ftr::Pick &pick
  , const ftr::UpdatePayload &payload
)
{
  Resolved out(pick);
  out.pick.resolvedIds.clear();
  
  auto features = payload.getFeatures(pick.tag);
  if (features.empty())
    return out;
  //we should have a 1 to 1 mapping between features and picks now. so assert.
  assert(features.size() == 1);
  out.feature = features.front();
  
  if (pick.isEmpty())
    return out;
  
  if (!out.feature->hasAnnex(ann::Type::SeerShape))
    return out;
  const ann::SeerShape &ss = out.feature->getAnnex<ann::SeerShape>();
  if (ss.isNull())
    return out;
  
  out.pick.resolvedIds = payload.shapeHistory.resolveHistories(pick.shapeHistory, out.feature->getId());
  
  return out;
}

ftr::Pick tls::convertToPick
(
  const slc::Message &messageIn
  , const ann::SeerShape &sShapeIn
  , const ftr::ShapeHistory &pHistory
)
{
  ftr::Pick out;
  out.selectionType = messageIn.type;
  
  if ((messageIn.type != slc::Type::Object) && (messageIn.type != slc::Type::Feature))
  {
    assert(sShapeIn.hasId(messageIn.shapeId)); //don't set me up
    //create devolve history from original pick not resolved
    out.shapeHistory = pHistory.createDevolveHistory(messageIn.shapeId);
    
    uuid resolvedId = messageIn.shapeId; //this mutates upon points to vertices.
    /* currently picks don't support parameters for anything higher than faces.
    * i.e. no shells, solids. this might be a problem.
    */
    const TopoDS_Shape &s = sShapeIn.getOCCTShape(resolvedId);
    if (messageIn.type == slc::Type::StartPoint)
    {
      assert(s.ShapeType() == TopAbs_EDGE);
      resolvedId = sShapeIn.useGetStartVertex(resolvedId);
    }
    else if (messageIn.type == slc::Type::EndPoint)
    {
      assert(s.ShapeType() == TopAbs_EDGE);
      resolvedId = sShapeIn.useGetEndVertex(resolvedId);
    }
    else if
    (
      messageIn.type == slc::Type::Edge
      || messageIn.type == slc::Type::MidPoint
      || messageIn.type == slc::Type::NearestPoint
      || messageIn.type == slc::Type::QuadrantPoint
    )
    {
      assert(s.ShapeType() == TopAbs_EDGE);
      out.setParameter(TopoDS::Edge(s), messageIn.pointLocation);
    }
    else if (messageIn.type == slc::Type::Face)
    {
      assert(s.ShapeType() == TopAbs_FACE);
      out.setParameter(TopoDS::Face(s), messageIn.pointLocation);
    }
    
    out.resolvedIds.push_back(resolvedId);
    out.highlightIds.push_back(resolvedId);
    out.shapeHistory = pHistory.createDevolveHistory(resolvedId);
  }
  
  return out;
}

ftr::Pick tls::convertToPick
(
  const slc::Container &containerIn
  , const ann::SeerShape &sShapeIn
  , const ftr::ShapeHistory &pHistory
)
{
  ftr::Pick out;
  out.selectionType = containerIn.selectionType;
  
  if ((containerIn.selectionType != slc::Type::Object) && (containerIn.selectionType != slc::Type::Feature))
  {
    assert(sShapeIn.hasId(containerIn.shapeId)); //don't set me up
    //create devolve history from original pick not resolved
    out.shapeHistory = pHistory.createDevolveHistory(containerIn.shapeId);
    
    uuid resolvedId = containerIn.shapeId; //this mutates upon points to vertices.
    /* currently picks don't support parameters for anything higher than faces.
    * i.e. no shells, solids. this might be a problem.
    */
    const TopoDS_Shape &s = sShapeIn.getOCCTShape(resolvedId);
    if (containerIn.selectionType == slc::Type::StartPoint)
    {
      assert(s.ShapeType() == TopAbs_EDGE);
      resolvedId = sShapeIn.useGetStartVertex(resolvedId);
    }
    else if (containerIn.selectionType == slc::Type::EndPoint)
    {
      assert(s.ShapeType() == TopAbs_EDGE);
      resolvedId = sShapeIn.useGetEndVertex(resolvedId);
    }
    else if
    (
      containerIn.selectionType == slc::Type::Edge
      || containerIn.selectionType == slc::Type::MidPoint
      || containerIn.selectionType == slc::Type::NearestPoint
      || containerIn.selectionType == slc::Type::QuadrantPoint
    )
    {
      assert(s.ShapeType() == TopAbs_EDGE);
      out.setParameter(TopoDS::Edge(s), containerIn.pointLocation);
    }
    else if (containerIn.selectionType == slc::Type::Face)
    {
      assert(s.ShapeType() == TopAbs_FACE);
      out.setParameter(TopoDS::Face(s), containerIn.pointLocation);
    }
    
    out.resolvedIds.push_back(resolvedId);
    out.highlightIds.push_back(resolvedId);
  }
  
  return out;
}

slc::Messages tls::convertToMessage(const ftr::Pick &pickIn, const ftr::Base *featureIn)
{
  slc::Messages out;
  slc::Message proto;
  proto.type = pickIn.selectionType;
  proto.featureType = featureIn->getType();
  proto.featureId = featureIn->getId();
  
  if (!slc::isShapeType(pickIn.selectionType))
    return slc::Messages(1, proto);
  
  assert(featureIn->hasAnnex(ann::Type::SeerShape)); //caller verifies.
  const ann::SeerShape &sShape = featureIn->getAnnex<ann::SeerShape>();
  assert(!sShape.isNull()); //caller verifies.
  
  //lambda to fill add one id to out.
  auto fillInMessage = [&](const uuid &rid)
  {
    slc::Message current = proto;
    assert(sShape.hasId(rid));
    current.shapeId = rid; // start and end points will overwrite.
    const TopoDS_Shape &s = sShape.getOCCTShape(rid);
    
    if (pickIn.selectionType == slc::Type::StartPoint || pickIn.selectionType == slc::Type::EndPoint)
    {
      assert(s.ShapeType() == TopAbs_VERTEX);
      
      std::vector<uuid> parentEdges = sShape.useGetParentsOfType(rid, TopAbs_EDGE);
      for (const auto &edge : parentEdges)
      {
        if
        (
          (
            (pickIn.selectionType == slc::Type::StartPoint)
            && (sShape.useGetStartVertex(edge) == rid)
          )
          ||
          (
            (pickIn.selectionType == slc::Type::EndPoint)
            && (sShape.useGetEndVertex(edge) == rid)
          )
        )
        {
          current.shapeId = edge;
          break;
        }
      }
      if (current.shapeId.is_nil())
      {
        std::cout << "ERROR: couldn't get edge from vertex in: " << BOOST_CURRENT_FUNCTION << std::endl;
        return;
      }
      current.pointLocation = gu::toOsg(TopoDS::Vertex(s));
    }
    if
    (
      pickIn.selectionType == slc::Type::Edge
      || pickIn.selectionType == slc::Type::MidPoint
      || pickIn.selectionType == slc::Type::NearestPoint
      || pickIn.selectionType == slc::Type::QuadrantPoint
    )
    {
      assert(s.ShapeType() == TopAbs_EDGE);
      current.pointLocation = pickIn.getPoint(TopoDS::Edge(s));
    }
    if (pickIn.selectionType == slc::Type::Face)
    {
      assert(s.ShapeType() == TopAbs_FACE);
      current.pointLocation = pickIn.getPoint(TopoDS::Face(s));
    }
    
    out.push_back(current);
  };
  
  //we can use picks that have not and don't need to be resolved
  if (pickIn.resolvedIds.empty())
  {
    const uuid &rootId = pickIn.shapeHistory.getRootId();
    if ((!rootId.is_nil()) && (sShape.hasId(rootId)))
      fillInMessage(rootId);
  }
  else
  {
    for (const auto rid : pickIn.resolvedIds)
      fillInMessage(rid);
  }
  
  return out;
}
