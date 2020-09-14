/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2015  Thomas S. Anderson blobfish.at.gmx.com
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

#include <cassert>

#include "globalutilities.h"
#include "tools/idtools.h"
#include "modelviz/mdvnodemaskdefs.h"
#include "application/appapplication.h"
#include "project/prjproject.h"
#include "feature/ftrbase.h"
#include "annex/annseershape.h"
#include "modelviz/mdvshapegeometry.h"
#include "selection/slcvisitors.h"
#include "selection/slcinterpreter.h"

using namespace slc;

using boost::uuids::uuid;

//We need to make the containers a set so we don't put multiple items in.

namespace
{
  std::tuple<uuid, ftr::Type> getFeatureInfo(osg::Drawable &drawableIn)
  {
    ParentMaskVisitor visitor(mdv::object);
    drawableIn.accept(visitor);
    osg::Node *featureRoot = visitor.out;
    assert(featureRoot);
    uuid fId = gu::getId(featureRoot);
    
    int featureTypeInteger = 0;
    if (!featureRoot->getUserValue<int>(gu::featureTypeAttributeTitle, featureTypeInteger))
        assert(0);
    ftr::Type fType = static_cast<ftr::Type>(featureTypeInteger);
    
    return std::make_tuple(fId, fType);
  }
}

Interpreter::Interpreter
(
  const Intersections& intersectionsIn,
  Mask selectionMaskIn
) : intersections(intersectionsIn), selectionMask(selectionMaskIn)
{
  go();
}

void Interpreter::go()
{
  for (auto const &intersection : intersections)
  {
    osg::Vec3d worldPoint = osg::Matrixd::inverse(*intersection.matrix) * intersection.localIntersectionPoint;
    
    Container container;
    std::tie(container.featureId, container.featureType) = getFeatureInfo(*intersection.drawable);
    container.pointLocation = worldPoint; //default to intersection world point.
    
    int localNodeMask = intersection.nodePath.back()->getNodeMask();
    mdv::ShapeGeometry *shapeGeometry = dynamic_cast<mdv::ShapeGeometry *>(intersection.drawable.get());
    if (!shapeGeometry)
    {
      if (localNodeMask == mdv::datum)
      {
        if (canSelectFeatures(selectionMask))
        {
          container.selectionType = Type::Feature;
          add(containersOut, container);
          continue;
        }
        if (canSelectObjects(selectionMask))
        {
          container.selectionType = Type::Object;
          add(containersOut, container);
          continue;
        }
      }
      continue;
    }
    
    ftr::Base *bf = app::instance()->getProject()->findFeature(container.featureId);
    assert(bf);
    assert(bf->hasAnnex(ann::Type::SeerShape));
    const ann::SeerShape &sShape = bf->getAnnex<ann::SeerShape>();
    
    std::size_t pSetIndex = shapeGeometry->getPSetFromPrimitive(intersection.primitiveIndex);
    uuid selectedId = shapeGeometry->getId(pSetIndex);
    
    //objects can be selected by either edges or faces so we create a lambda here
    //and call it from both if clauses.
    auto goSelectObject = [&]()
    {
      uuid object = sShape.getRootShapeId();
      if (!object.is_nil())
      {
        container.selectionType = Type::Object;
        container.shapeId = gu::createNilId();
        if (!has(containersOut, container)) //don't run again
        {
          container.selectionIds = sShape.useGetChildrenOfType(object, TopAbs_EDGE);
          auto faceIds = sShape.useGetChildrenOfType(object, TopAbs_FACE);
          container.selectionIds.insert(container.selectionIds.end(), faceIds.begin(), faceIds.end());
          add(containersOut, container);
        }
      }
    };
    
    if (localNodeMask == mdv::edge)
    {
      if (canSelectPoints(selectionMask))
      {
        osg::Vec3d iPoint = worldPoint;
        osg::Vec3d snapPoint;
        double distance = std::numeric_limits<double>::max();
        slc::Type sType = slc::Type::None;

        auto updateSnaps = [&](const std::vector<osg::Vec3d> &vecIn) -> bool
        {
          bool out = false;
          for (const auto& point : vecIn)
          {
            double tempDistance = (iPoint - point).length2();
            if (tempDistance < distance)
            {
              snapPoint = point;
              distance = tempDistance;
              out = true;
            }
          }
          return out;
        };

        if (canSelectEndPoints(selectionMask))
        {
          std::vector<osg::Vec3d> endPoints = sShape.useGetEndPoints(selectedId);
          if (endPoints.size() > 0)
          {
            std::vector<osg::Vec3d> startPoint;
            startPoint.push_back(endPoints.at(0));
            if (updateSnaps(startPoint))
              sType = slc::Type::StartPoint;
          }
          if (endPoints.size() > 1)
          {
            std::vector<osg::Vec3d> endPoint;
            endPoint.push_back(endPoints.at(1));
            if (updateSnaps(endPoints))
              sType = slc::Type::EndPoint;
          }
        }

        if (canSelectMidPoints(selectionMask))
        {
          std::vector<osg::Vec3d> midPoints = sShape.useGetMidPoint(selectedId);
          if (updateSnaps(midPoints))
            sType = slc::Type::MidPoint;
        }

        if (canSelectCenterPoints(selectionMask))
        {
          std::vector<osg::Vec3d> centerPoints = sShape.useGetCenterPoint(selectedId);
          if (updateSnaps(centerPoints))
            sType = slc::Type::CenterPoint;
        }

        if (canSelectQuadrantPoints(selectionMask))
        {
          std::vector<osg::Vec3d> quadrantPoints = sShape.useGetQuadrantPoints(selectedId);
          if (updateSnaps(quadrantPoints))
            sType = slc::Type::QuadrantPoint;
        }

        if (canSelectNearestPoints(selectionMask))
        {
          std::vector<osg::Vec3d> nearestPoints = sShape.useGetNearestPoint(selectedId, worldPoint);
          if (updateSnaps(nearestPoints))
          sType = slc::Type::NearestPoint;
        }
        container.selectionType = sType;
        container.shapeId = selectedId;
        container.pointLocation = snapPoint;
        add(containersOut,container);
      }
      if (canSelectEdges(selectionMask))
      {
        container.selectionType = Type::Edge;
        container.shapeId = selectedId;
        container.selectionIds.push_back(selectedId);
        add(containersOut, container);
      }
      if (canSelectWires(selectionMask))
      {
        //only select 'faceless' wires through an edge.
        //if the wire has a face it will be selected through
        //face intersection.
        uuid edgeId = selectedId;
        std::vector<uuid> wireIds = sShape.useGetFacelessWires(edgeId);
        if (!wireIds.empty())
        {
          container.selectionType = Type::Wire;
          container.selectionIds = sShape.useGetChildrenOfType(wireIds.front(), TopAbs_EDGE);
          container.shapeId = wireIds.front();
          add(containersOut, container);
        }
      }
      if (canSelectObjects(selectionMask))
        goSelectObject();
    }
    else if(localNodeMask == mdv::face)
    {
      if (canSelectWires(selectionMask))
      {
        uuid wire = sShape.useGetClosestWire(selectedId, worldPoint);
        if (!wire.is_nil())
        {
          container.selectionIds = sShape.useGetChildrenOfType(wire, TopAbs_EDGE);
          container.selectionType = Type::Wire;
          container.shapeId = wire;
          add(containersOut, container);
        }
      }
      if (canSelectFaces(selectionMask))
      {
        container.selectionType = Type::Face;
        container.shapeId = selectedId;
        container.selectionIds = sShape.useGetChildrenOfType(selectedId, TopAbs_EDGE); //wireframe mode
        container.selectionIds.push_back(selectedId); //shaded mode
        add(containersOut,container);
      }
      if (canSelectShells(selectionMask))
      {
        std::vector<uuid> shells = sShape.useGetParentsOfType(selectedId, TopAbs_SHELL);
        if (shells.size() == 1)
        {
          container.selectionType = Type::Shell;
          container.shapeId = shells.at(0);
          if (!has(containersOut, container)) //don't run again
          {
            container.selectionIds = sShape.useGetChildrenOfType(shells.at(0), TopAbs_EDGE); //wireframe mode
            auto wtf = sShape.useGetChildrenOfType(shells.at(0), TopAbs_FACE);
            container.selectionIds.insert(container.selectionIds.begin(), wtf.begin(), wtf.end()); //shaded mode
            add(containersOut, container);
          }
        }
      }
      if (canSelectSolids(selectionMask))
      {
        std::vector<uuid> solids = sShape.useGetParentsOfType(selectedId, TopAbs_SOLID);
        //should be only 1 solid
        if (solids.size() == 1)
        {
          container.selectionType = Type::Solid;
          container.shapeId = solids.at(0);
          if (!has(containersOut, container)) //don't run again
          {
            container.selectionIds = sShape.useGetChildrenOfType(solids.at(0), TopAbs_EDGE); //wireframe mode
            auto wtf = sShape.useGetChildrenOfType(solids.at(0), TopAbs_FACE);
            container.selectionIds.insert(container.selectionIds.begin(), wtf.begin(), wtf.end()); //shaded mode
            add(containersOut, container);
          }
        }
      }
      if (canSelectObjects(selectionMask))
        goSelectObject();
    }
  }
}
