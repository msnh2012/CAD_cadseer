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

#include <iostream>
#include <sstream>

#include <boost/optional/optional.hpp>

#include <osg/View>
#include <osg/Geometry>
#include <osg/io_utils>

#include "modelviz/nodemaskdefs.h"
#include "visual.h"
#include "selection.h"

using namespace skt;

//build polytope at origin. 8 sided.
static osg::Polytope buildBasePolytope(double radius)
{
  //apparently the order of the addition matters. intersector
  //wants opposites sequenced. This seems to be working.
  osg::Polytope out;
  osg::Matrixd rotation = osg::Matrixd::rotate(osg::DegreesToRadians(45.0), osg::Vec3d(0.0, 0.0, 1.0));
  osg::Vec3d base = osg::Vec3d(1.0, 0.0, 0.0);
  
  out.add(osg::Plane(base, radius));
  out.add(osg::Plane(-base, radius));
  
  base = rotation * base;
  
  out.add(osg::Plane(base, radius));
  out.add(osg::Plane(-base, radius));
  
  base = rotation * base;
  
  out.add(osg::Plane(base, radius));
  out.add(osg::Plane(-base, radius));
  
  base = rotation * base;
  
  out.add(osg::Plane(base, radius));
  out.add(osg::Plane(-base, radius));
  
  out.add(osg::Plane(0.0,0.0,1.0, 0.0)); //last has to be 'cap'
  
  return out;
}

static osg::Polytope buildPolytope(double x, double y, double radius)
{
  static osg::Polytope base = buildBasePolytope(radius);
  osg::Polytope out(base);
  out.transform(osg::Matrixd::translate(x, y, 0.0));
  
  return out;
}

static osg::Camera* findOverlayCam(const osg::View *view)
{
  for (unsigned int i = 0; i < view->getNumSlaves(); ++i)
  {
    if (view->getSlave(i)._camera->getName() == "overlay")
      return view->getSlave(i)._camera.get();
  }
  assert(0); //no slave camera
  return nullptr;
}

//!@brief Create selection object with visual target
Selection::Selection(Visual *vIn):
visual(vIn)
{
  timer.stop(); // turn on as needed.
};

//!@brief Intersection visitors will visit all nodes.
void Selection::setMaskAll()
{
  mask = 0xffffffff;
}

//!@brief Intersection visitors will visit no nodes.
void Selection::setMaskNone()
{
  mask = 0;
}

//!@brief Get the current node mask.
NodeMask Selection::getMask()
{
  return mask;
}

//!@brief Set the current node mask.
void Selection::setMask(NodeMask mIn)
{
  mask = mIn;
}

//!@brief Allow intersection visitors to visit invisible selection plane.
void Selection::selectionPlaneOn()
{
  mask |= SelectionPlane;
}

//!@brief Don't allow intersection visitors to visit invisible selection plane.
void Selection::selectionPlaneOff()
{
  mask &= ~SelectionPlane;
}

//!@brief Allow intersection visitors to visit the visible plane.
void Selection::workPlaneOn()
{
  mask |= WorkPlane;
}

//!@brief Don't allow intersection visitors to visit the visible plane.
void Selection::workPlaneOff()
{
  mask &= ~WorkPlane;
}

//!@brief Allow intersection visitors to visit the work plane axes.
void Selection::workPlaneAxesOn()
{
  mask |= WorkPlaneAxis;
}

//!@brief Don't allow intersection visitors to visit the work plane axes.
void Selection::workPlaneAxesOff()
{
  mask &= ~WorkPlaneAxis;
}

//!@brief Allow intersection visitors to visit the work plane origin.
void Selection::workPlaneOriginOn()
{
  mask |= WorkPlaneOrigin;
}

//!@brief Don't allow intersection visitors to visit the work plane origin.
void Selection::workPlaneOriginOff()
{
  mask &= ~WorkPlaneOrigin;
}

//!@brief Allow intersection visitors to visit the entities group.
void Selection::entitiesOn()
{
  mask |= Entity;
}

//!@brief Don't Allow intersection visitors to visit the entities group.
void Selection::entitiesOff()
{
  mask &= ~Entity;
}

//!@brief Allow intersection visitors to visit the constraints group.
void Selection::constraintsOn()
{
  mask |= Constraint;
}

//!@brief Don't allow intersection visitors to visit the constraints group.
void Selection::constraintsOff()
{
  mask &= ~Constraint;
}

//!@brief Handle events from openscenegraph event system.
bool Selection::handle(const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &aa, osg::Object*, osg::NodeVisitor*)
{
  bool out = false; //false means we didn't handle event and others should try.
  
//   if (!timer.is_stopped())
//     std::cout << std::endl << "beginning of event: " << timer.format();
  
  //always check button state.
  if (ea.getEventType() == osgGA::GUIEventAdapter::PUSH && ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
    leftButtonDown = true;
  if (ea.getEventType() == osgGA::GUIEventAdapter::RELEASE && ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
    leftButtonDown = false;
  
  osg::View* view = aa.asView();
  if (!view)
    return out;
  osg::Camera *camera = findOverlayCam(view);
  
  auto goPolytopeIntersection = [&](NodeMask nm) -> osgUtil::PolytopeIntersector::Intersections
  {
    /* I can't run an intersection visitor over a subgraph. I have
     * tried! I think I am stuck at push_clone inside intersectionVisitor
     * being protected. Anyway we will have to run it over the camera
     * and we will have to add a node mask for 'active sketch'
     */
    osg::Polytope pt = buildPolytope(ea.getX(), ea.getY(), 16.0);
    
    osg::ref_ptr<osgUtil::PolytopeIntersector> polyPicker = new osgUtil::PolytopeIntersector
    (
      osgUtil::Intersector::WINDOW,
      pt
    );
    osgUtil::IntersectionVisitor polyVisitor(polyPicker.get());
    polyVisitor.setTraversalMask((nm | ActiveSketch).to_ulong() | mdv::overlay);
    camera->accept(polyVisitor);
    return polyPicker->getIntersections();
  };
  
  auto goLineSegmentIntersection = [&](NodeMask nm) -> osgUtil::LineSegmentIntersector::Intersections
  {
    osg::ref_ptr<osgUtil::LineSegmentIntersector> linePicker = new osgUtil::LineSegmentIntersector
    (
      osgUtil::Intersector::WINDOW
      , ea.getX()
      , ea.getY()
    );
    
    osgUtil::IntersectionVisitor lineVisitor(linePicker.get());
    lineVisitor.setTraversalMask((nm | ActiveSketch).to_ulong() | mdv::overlay);
    camera->accept(lineVisitor);
    return linePicker->getIntersections();
  };
  
  if (ea.getEventType() == osgGA::GUIEventAdapter::MOVE)
  {
    visual->move(goPolytopeIntersection(mask)); //we call when drawing so we can preselect endpoints.
    if (visual->getState() != State::selection)
      visual->move(goLineSegmentIntersection(SelectionPlane));
//     else
//       timer.stop();
  }
  
  //can't use leftButtonDown for test. test needs to consider a 'new' event not state.
  if (ea.getEventType() == osgGA::GUIEventAdapter::RELEASE && ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
  {
    if (visual->getState() == State::drag)
    {
      visual->finishDrag(goLineSegmentIntersection(SelectionPlane));
    }
    else
    {
      if (visual->getState() == State::selection)
        visual->pick(goPolytopeIntersection(mask));
      else
        visual->pick(goLineSegmentIntersection(mask));
    }
  }
  
  if (ea.getEventType() == osgGA::GUIEventAdapter::RELEASE && ea.getButton() == osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON)
  {
    visual->clearSelection();
  }
  
  if (leftButtonDown && ea.getEventType() == osgGA::GUIEventAdapter::DRAG)
  {
    if (visual->getState() == State::selection)
    {
      visual->startDrag(goLineSegmentIntersection(mask));
      if (visual->getState() == State::drag) //visual will change it's status if drag is accepeted.
        out = true;
    }
    else if (visual->getState() == State::drag)
    {
      visual->drag(goLineSegmentIntersection(SelectionPlane));
      out = true;
      
//       std::cout << "ending of drag: " << timer.format();
//       timer.start();
    }
  }
    
  return out;
}
