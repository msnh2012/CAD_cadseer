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

ftr::Pick tls::convertToPick
(
  const slc::Message &messageIn
  , const ftr::Base &featureIn
  , const ftr::ShapeHistory &pHistory
)
{
  if (slc::isShapeType(messageIn.type) && featureIn.hasAnnex(ann::Type::SeerShape))
    return convertToPick(messageIn, featureIn.getAnnex<ann::SeerShape>(), pHistory);
  ftr::Pick out;
  out.selectionType = messageIn.type;
  out.accrue = messageIn.accrue;
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
  out.accrue = messageIn.accrue;
  
  if (slc::isShapeType(messageIn.type))
  {
    assert(!messageIn.shapeId.is_nil());
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
  out.accrue = containerIn.accrue;
  
  if (slc::isShapeType(containerIn.selectionType))
  {
    assert(!containerIn.shapeId.is_nil());
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

ftr::Pick tls::convertToPick
(
  const slc::Container &containerIn
  , const ftr::Base &feature
  , const ftr::ShapeHistory &pHistory
)
{
  ftr::Pick out;
  if
  (
    slc::isShapeType(containerIn.selectionType)
    && feature.hasAnnex(ann::Type::SeerShape)
    && !feature.getAnnex<ann::SeerShape>().isNull()
  )
  {
    out = convertToPick(containerIn, feature.getAnnex<ann::SeerShape>(), pHistory);
  }
  else
  {
    out.selectionType = containerIn.selectionType;
    out.accrue = containerIn.accrue;
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

Resolver::Resolver(const ftr::UpdatePayload &pIn)
: payload(&pIn)
{}

Resolver::Resolver(const ftr::Base *fIn)
: feature(fIn)
{}

/*! @brief resolve for a pick
 * 
 * @param pIn to resolve
 * @return true = success, false = failure
 * @see hasSucceeded
 */
bool Resolver::resolve(const ftr::Pick &pIn)
{
  workPick = pIn;
  workPick.resolvedIds.clear();
  workPick.highlightIds.clear();
  
  if (payload)
    resolveViaPayload();
  else if (feature)
    resolveViaFeature();
  
  return hasSucceeded();
}

/*! @brief returns the current state.
 * 
 * @return state from the last call to resolve.
 * @note With object types only feature pointer is guaranteed.
 * seer shape has to be validated by caller if needed.
 * With shape types, feature, seer shape and non-empty
 * resolvedIds are all validated.
 */
bool Resolver::hasSucceeded()
{
  if (slc::isObjectType(workPick.selectionType))
    return feature; //cant do seershape. We have features without it.
  else
    return feature && sShape && !workPick.resolvedIds.empty();
}

/*! @brief resolves pick via the payload and prepares for yield
 * 
 * @parameter pIn is the pick for resolution.
 * @note This method needs object construction with ftr::UpdatePayload parameter
 */
void Resolver::resolveViaPayload()
{
  feature = nullptr;
  sShape = nullptr;
  auto features = payload->getFeatures(workPick.tag);
  if (features.empty())
    return;
  //we should have a 1 to 1 mapping between features and picks now. so assert.
  assert(features.size() == 1);
  if (features.size() != 1)
  {
    std::cout << "ERROR: pick tags required to match only one feature input, in: " << BOOST_CURRENT_FUNCTION << std::endl;
    return;
  }
  feature = features.front();
  
  if (!feature->hasAnnex(ann::Type::SeerShape))
    return;
  sShape = &feature->getAnnex<ann::SeerShape>();
  if (sShape->isNull())
  {
    sShape = nullptr;
    return;
  }
  
  if (!slc::isShapeType(workPick.selectionType) || workPick.isEmpty())
    return;
  
  workPick.resolvedIds = payload->shapeHistory.resolveHistories(workPick.shapeHistory, feature->getId());
  //someday we will consider accrue type and highlightIds != resolvedIds.
  std::copy(workPick.resolvedIds.begin(), workPick.resolvedIds.end(), std::back_inserter(workPick.highlightIds));
}

void Resolver::resolveViaFeature()
{
  //this doesn't do much. we will just copy the root id of the shapehistory into resolvedIds and highlightIds,
  //so internal state will match resolveViaPayload.
  sShape = nullptr;
  if (!feature->hasAnnex(ann::Type::SeerShape))
    return;
  sShape = &feature->getAnnex<ann::SeerShape>();
  if (sShape->isNull())
  {
    sShape = nullptr;
    return;
  }
  
  if (!slc::isShapeType(workPick.selectionType) || workPick.isEmpty())
    return;
  uuid id = workPick.shapeHistory.getRootId();
  if (id.is_nil() || !sShape->hasId(id))
  {
    std::cout << "Warning: id is not in seershape, in: " << BOOST_CURRENT_FUNCTION << std::endl;
    return;
  }
  workPick.resolvedIds.push_back(id);
  //someday we will consider accrue type and highlightIds != resolvedIds.
  workPick.highlightIds.push_back(id);
}

/* this is basically the same as global function.
 * except this version only works on the picks resolvedIds vector.
 * the one above can also work with the shapehistory.
 */
slc::Messages Resolver::convertToMessages() const
{
  slc::Messages out;
  
  if (!feature)
    return out;
  
  slc::Message proto;
  proto.type = workPick.selectionType;
  proto.accrue = workPick.accrue;
  proto.featureType = feature->getType();
  proto.featureId = feature->getId();
  
  if (!slc::isShapeType(workPick.selectionType))
    return slc::Messages(1, proto);
  
  if (!sShape)
    return out;
  
  //lambda to fill add one id to out.
  for (const auto &rid : workPick.resolvedIds)
  {
    slc::Message current = proto;
    assert(sShape->hasId(rid));
    if (!sShape->hasId(rid))
    {
      std::cout << "ERROR: resolved id is not in seer shape, in: " << BOOST_CURRENT_FUNCTION << std::endl;
      continue;
    }
    current.shapeId = rid; // start and end points will overwrite.
    const TopoDS_Shape &s = sShape->getOCCTShape(rid);
    
    if (workPick.selectionType == slc::Type::StartPoint || workPick.selectionType == slc::Type::EndPoint)
    {
      /* Selection doesn't support vertices. It uses endpoints of an edge. I haven't
       * established a standard on what happens when converting end point selections
       * into picks and shape history. Do we use edge or do we resolve it to
       * a vertex? Not sure, so here we will test shape and try derive expected results.
       */
      if (s.ShapeType() == TopAbs_VERTEX)
      {
        std::vector<uuid> parentEdges = sShape->useGetParentsOfType(rid, TopAbs_EDGE);
        for (const auto &edge : parentEdges)
        {
          if
          (
            (
              (workPick.selectionType == slc::Type::StartPoint)
              && (sShape->useGetStartVertex(edge) == rid)
            )
            ||
            (
              (workPick.selectionType == slc::Type::EndPoint)
              && (sShape->useGetEndVertex(edge) == rid)
            )
          )
          {
            current.shapeId = edge;
            break;
          }
        }
        if (current.shapeId == rid)
        {
          std::cout << "ERROR: couldn't get edge from vertex in: " << BOOST_CURRENT_FUNCTION << std::endl;
          continue;
        }
        current.pointLocation = gu::toOsg(TopoDS::Vertex(s));
      }
      else if (s.ShapeType() == TopAbs_EDGE)
      {
        auto eps = sShape->useGetEndPoints(rid);
        if (workPick.selectionType == slc::Type::StartPoint)
          current.pointLocation = eps.front();
        else
          current.pointLocation = eps.back();
      }
    }
    if
    (
      workPick.selectionType == slc::Type::Edge
      || workPick.selectionType == slc::Type::MidPoint
      || workPick.selectionType == slc::Type::NearestPoint
      || workPick.selectionType == slc::Type::QuadrantPoint
    )
    {
      assert(s.ShapeType() == TopAbs_EDGE);
      if (s.ShapeType() != TopAbs_EDGE)
      {
        std::cout << "ERROR: Shape is not an edge, in: " << BOOST_CURRENT_FUNCTION << std::endl;
        continue;
      }
      current.pointLocation = workPick.getPoint(TopoDS::Edge(s));
    }
    if (workPick.selectionType == slc::Type::Face)
    {
      assert(s.ShapeType() == TopAbs_FACE);
      if (s.ShapeType() != TopAbs_FACE)
      {
        std::cout << "ERROR: Shape is not a face, in: " << BOOST_CURRENT_FUNCTION << std::endl;
        continue;
      }
      current.pointLocation = workPick.getPoint(TopoDS::Face(s));
    }
    
    out.push_back(current);
  };
  
  return out;
}

/*! @brief get the occt shapes
 * 
 * @param convertEndPoints controls whether endpoints are converted to vertices
 * or left as edges.
 * @return vector of shapes.
 */
occt::ShapeVector Resolver::getShapes(bool convertEndPoints) const
{
  occt::ShapeVector out;
  if(!feature || !sShape)
    return out;
  
  if (!slc::isShapeType(workPick.selectionType))
  {
    //get the whole shape.
    out.push_back(sShape->getRootOCCTShape());
  }
  else
  {
    for (const auto &rId : workPick.resolvedIds)
    {
      assert(sShape->hasId(rId));
      if (!sShape->hasId(rId))
      {
        std::cout << "ERROR: resolved id is not in seer shape, in: " << BOOST_CURRENT_FUNCTION << std::endl;
        continue;
      }
      TopoDS_Shape theShape = sShape->getOCCTShape(rId);
      if (convertEndPoints && theShape.ShapeType() == TopAbs_EDGE)
      {
        if (workPick.selectionType == slc::Type::StartPoint)
          theShape = sShape->getOCCTShape(sShape->useGetStartVertex(rId));
        else if (workPick.selectionType == slc::Type::EndPoint)
          theShape = sShape->getOCCTShape(sShape->useGetEndVertex(rId));
      }
      out.push_back(theShape);
    }
  }
  
  return out;
}

std::vector<osg::Vec3d> Resolver::getPoints() const
{
  std::vector<osg::Vec3d> out;
  
  if (!slc::isPointType(workPick.selectionType) || !feature || !sShape)
    return out;
  
  for (const auto &rId : workPick.resolvedIds)
  {
    assert(sShape->hasId(rId));
    if(!sShape->hasId(rId))
    {
      std::cout << "ERROR: resolved id is not in seer shape, in: " << BOOST_CURRENT_FUNCTION << std::endl;
      continue;
    }
    const TopoDS_Shape &rs = sShape->getOCCTShape(rId);
    if (rs.ShapeType() == TopAbs_VERTEX)
    {
      out.push_back(gu::toOsg(BRep_Tool::Pnt(TopoDS::Vertex(rs))));
      continue;
    }
    if (rs.ShapeType() == TopAbs_EDGE)
    {
      boost::optional<osg::Vec3d> p;
      if (workPick.selectionType == slc::Type::StartPoint)
        p = gu::toOsg(BRep_Tool::Pnt(TopoDS::Vertex(sShape->getOCCTShape(sShape->useGetStartVertex(rId)))));
      if (workPick.selectionType == slc::Type::EndPoint)
        p = gu::toOsg(BRep_Tool::Pnt(TopoDS::Vertex(sShape->getOCCTShape(sShape->useGetEndVertex(rId)))));
      if (workPick.selectionType == slc::Type::MidPoint)
      {
        auto a = sShape->useGetMidPoint(rId);
        if (!a.empty())
          p = a.front();
      }
      if (workPick.selectionType == slc::Type::CenterPoint)
      {
        auto a = sShape->useGetCenterPoint(rId);
        if (!a.empty())
          p = a.front();
      }
      if (workPick.selectionType == slc::Type::QuadrantPoint || workPick.selectionType == slc::Type::NearestPoint)
      {
        p = workPick.getPoint(TopoDS::Edge(rs));
      }
      if (!p)
        continue;
      
      out.push_back(p.get());
    }
  }
  
  return out;
}

const std::vector<boost::uuids::uuid>& Resolver::getResolvedIds(const ftr::Pick &pIn)
{
  resolve(pIn);
  return getResolvedIds();
}

slc::Messages Resolver::convertToMessages(const ftr::Pick &pIn)
{
  resolve(pIn);
  return convertToMessages();
}

occt::ShapeVector Resolver::getShapes(const ftr::Pick &pIn, bool convertEndPoints)
{
  resolve(pIn);
  return getShapes(convertEndPoints);
}

std::vector<osg::Vec3d> Resolver::getPoints(const ftr::Pick &pIn)
{
  resolve(pIn);
  return getPoints();
}
    
