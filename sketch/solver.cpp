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

#include <cassert>
#include <iostream>

#include <boost/current_function.hpp>

#include "solver.h"

using namespace skt;

//! @details return the handle to the current work plane origin
Slvs_hEntity Solver::getWPOrigin() const
{
  assert(workPlane != 0);
  auto oe = findEntity(workPlane);
  assert(oe);
  assert(oe.get().type == SLVS_E_WORKPLANE);
  return oe.get().point[0];
}

//! @details return the handle to the current work plane orientation.
Slvs_hEntity Solver::getWPNormal3d() const
{
  assert(workPlane != 0);
  auto oe = findEntity(workPlane);
  assert(oe);
  assert(oe.get().type == SLVS_E_WORKPLANE);
  return oe.get().normal;
}

//! @details Set the work plane to absolute. Think identity matrix.
void Solver::setWorkPlane()
{
  Slvs_hEntity origin = addPoint3d(0.0, 0.0, 0.0);
  Slvs_hEntity rotation = addNormal3d(1.0, 0.0, 0.0, 0.0, 1.0, 0.0);
  Slvs_hEntity wph = addWorkPlane(origin, rotation);
  setWorkPlane(wph);
}

//! @details Create a unit line segment in the current work plane, in the current group for the x axis.
void Solver::createXAxis()
{
  assert(getWorkPlane());
  Slvs_hEntity head = addPoint2d(1.0, 0.0);
  Slvs_hEntity tail = addPoint2d(0.0, 0.0);
  xAxis = addLineSegment(head, tail);
}

//! @details Create a unit line segment in the current work plane, in the current group for the y axis.
void Solver::createYAxis()
{
  assert(getWorkPlane());
  Slvs_hEntity head = addPoint2d(0.0, 1.0);
  Slvs_hEntity tail = addPoint2d(0.0, 0.0);
  yAxis = addLineSegment(head, tail);
}

/*! @details create a parameter
 * 
 * @param value for the newly created parameter.
 * @return the handle for the newly created parameter.
 */
Slvs_hParam Solver::addParameter(double value)
{
  parameters.push_back(Slvs_MakeParam(nph, group, value));
  nph++;
  return parameters.back().h;
}

/*! @details Remove a parameter from the system
 * 
 * @param ph is the handle of the parameter to remove.
 * @note This is just removal of the parameter. No reference validation is performed.
 * @see clean.
 */
void Solver::removeParameter(Slvs_hParam ph)
{
  for (auto it = parameters.begin(); it != parameters.end(); ++it)
  {
    if (it->h == ph)
    {
      parameters.erase(it);
      break;
    }
  }
}

/*! @details Get a vector of orphaned parameter handles.
 *
 * an orphaned parameter is a parameter that is not referenced by
 * any entity. They are useless in a fully constructed system.
 * @see removeOrpanedParameters
 */ 
std::vector<Slvs_hParam> Solver::getOrphanedParameters() const
{
  std::vector<Slvs_hParam> out;
  for (const auto &p : parameters)
  {
    bool found = false;
    Slvs_hParam ch = p.h; //current handle
    for (const auto &e : entities)
    {
      if
      (
        e.param[0] == ch
        || e.param[1] == ch
        || e.param[2] == ch
        || e.param[3] == ch
      )
      {
        found = true;
        break;
      }
    }
    if (!found)
      out.push_back(ch);
  }
  return out;
}

/*! @details Scan and remove all the orphaned parameters in the system
 * 
 * @return the number of parameters removed.
 * @see getOrphanedParameters for definition of an 'orphaned parameter'.
 * @see clean is probably desired over this function.
 */
int Solver::removeOrphanedParameters()
{
  int count = 0;
  for (auto ph : getOrphanedParameters())
  {
    removeParameter(ph);
    count++;
  }
  return count;
}

/*! @details Add a new 3d point. 
 * 
 * @param px is the parameter handle for the x coordinate.
 * @param py is the parameter handle for the y coordinate.
 * @param pz is the parameter handle for the z coordinate.
 * @return the handle to the newly created point entity.
 * @note doesn't reference work plane.
 */
Slvs_hEntity Solver::addPoint3d(Slvs_hParam px, Slvs_hParam py, Slvs_hParam pz)
{
  assert(findParameter(px));
  assert(findParameter(py));
  assert(findParameter(pz));
  
  entities.push_back(Slvs_MakePoint3d(neh, group, px, py, pz));
  neh++;
  return entities.back().h;
}

/*! @details Add a new 3d point.
 * @param x is the value for a newly created parameter for x point coordinate.
 * @param y is the value for a newly created parameter for y point coordinate.
 * @param z is the value for a newly created parameter for z point coordinate.
 * @return the handle to the newly created point entity.
 * @note doesn't reference work plane.
 */
Slvs_hEntity Solver::addPoint3d(double x, double y, double z)
{
  Slvs_hParam px = addParameter(x);
  Slvs_hParam py = addParameter(y);
  Slvs_hParam pz = addParameter(z);
  return addPoint3d(px, py, pz);
}

/*! @details Add a new 2d point referencing work plane
 * 
 * @param pu is the handle of parameter for u axis.
 * @param pv is the handle of parameter for v axis.
 * @return the handle to the newly created point entity.
 */
Slvs_hEntity Solver::addPoint2d(Slvs_hParam pu, Slvs_hParam pv)
{
  assert(findParameter(pu));
  assert(findParameter(pv));
  
  entities.push_back(Slvs_MakePoint2d(neh, group, workPlane, pu, pv));
  neh++;
  return entities.back().h;
}

/*! @details Add a new 2d point referencing work plane
 * 
 * @param u is the value for a new parameter for u axis.
 * @param v is the value for a new parameter for v axis.
 * @return the handle to the newly created point entity.
 */
Slvs_hEntity Solver::addPoint2d(double u, double v)
{
  Slvs_hParam pu = addParameter(u);
  Slvs_hParam pv = addParameter(v);
  return addPoint2d(pu, pv);
}

/*! @details create an quaternion orientation.
 * 
 * @param qwh is the parameter handle to scalar part
 * @param qxh is the parameter handle to vector part x
 * @param qyh is the parameter handle to vector part y
 * @param qzh is the parameter handle to vector part z
 * @return the handle to the newly created orientation entity.
 */
Slvs_hEntity Solver::addNormal3d(Slvs_hParam qwh, Slvs_hParam qxh, Slvs_hParam qyh, Slvs_hParam qzh)
{
  assert(findParameter(qwh));
  assert(findParameter(qxh));
  assert(findParameter(qyh));
  assert(findParameter(qzh));
  
  entities.push_back(Slvs_MakeNormal3d(neh, group, qwh, qxh, qyh, qzh));
  neh++;
  return entities.back().h;
}

/*! @details create and orientation from an x and y vectors.
 * 
 * @param ux is the new parameter value for the x component of the u vector.
 * @param uy is the new parameter value for the y component of the u vector.
 * @param uz is the new parameter value for the z component of the u vector.
 * @param vx is the new parameter value for the x component of the v vector.
 * @param vy is the new parameter value for the y component of the v vector.
 * @param vz is the new parameter value for the z component of the v vector.
 * @return the handle to the newly created orientation entity.
 */
Slvs_hEntity Solver::addNormal3d(double ux, double uy, double uz, double vx, double vy, double vz)
{
  double qw, qx, qy, qz;
  Slvs_MakeQuaternion(ux, uy, uz, vx, vy, vz, &qw, &qx, &qy, &qz);
  Slvs_hParam qwh = addParameter(qw);
  Slvs_hParam qxh = addParameter(qx);
  Slvs_hParam qyh = addParameter(qy);
  Slvs_hParam qzh = addParameter(qz);
  return addNormal3d(qwh, qxh, qyh, qzh);
}

/*! @details create a new work plane.
 * 
 * @param origin is the origin of the new work plane.
 * @param normal is the orientation of the new work plane.
 * @return the handle of new work plane.
 * @note new work plane will NOT be set as the current work plane. @ref WorkPlane
 */
Slvs_hEntity Solver::addWorkPlane(Slvs_hEntity origin, Slvs_hEntity normal)
{
  assert(isEntityType(origin, SLVS_E_POINT_IN_3D));
  assert(isEntityType(normal, SLVS_E_NORMAL_IN_3D));
  
  entities.push_back(Slvs_MakeWorkplane(neh, group, origin, normal));
  neh++;
  return entities.back().h;
}

/*! @details create a line segment.
 * 
 * @param p1 is the start point(2d or 3d) of line. 
 * @param p2 is the end point(2d or 3d) of line. 
 * @return handle of new line segment.
 */
Slvs_hEntity Solver::addLineSegment(Slvs_hEntity p1, Slvs_hEntity p2)
{
  assert(isEntityType(p1, SLVS_E_POINT_IN_2D) || isEntityType(p1, SLVS_E_POINT_IN_3D));
  assert(isEntityType(p2, SLVS_E_POINT_IN_2D) || isEntityType(p2, SLVS_E_POINT_IN_3D));
  
  entities.push_back(Slvs_MakeLineSegment(neh, group, workPlane, p1, p2));
  neh++;
  return entities.back().h;
}

/*! @details create an addArcOfCircle
 * 
 * @param orient is the 3d orientation of the arc. Often this will be from the work plane. @ref getWPNormal3d.
 * @param center is the 2d center point.
 * @param start is the 2d start point.
 * @param end is the 2d end point.
 * @return handle of new arc entity.
 * @note center is the center of the circle not the center of the curve.
 */
Slvs_hEntity Solver::addArcOfCircle
(
  Slvs_hEntity orient
  , Slvs_hEntity center
  , Slvs_hEntity start
  , Slvs_hEntity end
)
{
  assert(isEntityType(orient, SLVS_E_NORMAL_IN_3D));
  assert(isEntityType(center, SLVS_E_POINT_IN_2D));
  assert(isEntityType(start, SLVS_E_POINT_IN_2D));
  assert(isEntityType(end, SLVS_E_POINT_IN_2D));
  
  entities.push_back(Slvs_MakeArcOfCircle(neh, group, workPlane, orient, center, start, end));
  neh++;
  return entities.back().h;
}

/*! @details create distance entity.
 * 
 * @param value is distance for the new parameter.
 * @return the handle of the new distance entity.
 */
Slvs_hEntity Solver::addDistance(double value)
{
  Slvs_hParam dp = addParameter(value);
  return addDistance(dp);
}

/*! @details create distance entity.
 * 
 * @param p is the parameter to reference.
 * @return the handle of new distance entity.
 */
Slvs_hEntity Solver::addDistance(Slvs_hParam p)
{
  assert(findParameter(p));
  
  entities.push_back(Slvs_MakeDistance(neh, group, workPlane, p));
  neh++;
  return entities.back().h;
}

/*! @details create circle entity.
 * 
 * @param point is the 2d point for center.
 * @param distance is the radius.
 * @return handle of new circle entity.
 */
Slvs_hEntity Solver::addCircle(Slvs_hEntity point, Slvs_hEntity distance)
{
  assert(isEntityType(point, SLVS_E_POINT_IN_2D));
  assert(isEntityType(distance, SLVS_E_DISTANCE));
  
  entities.push_back(Slvs_MakeCircle(neh, group, workPlane, point, getWPNormal3d(), distance));
  neh++;
  return entities.back().h;
}

/*! @details create a cubic bezier entity.
 * 
 * @param p1 is the 2d point for start.
 * @param p2 is the 2d point for control.
 * @param p3 is the 2d point for control.
 * @param p4 is the 2d point for finish.
 * @return handle of new cubic bezier entity.
 */
Slvs_hEntity Solver::addCubicBezier(Slvs_hEntity p1, Slvs_hEntity p2, Slvs_hEntity p3, Slvs_hEntity p4)
{
  assert(isEntityType(p1, SLVS_E_POINT_IN_2D));
  assert(isEntityType(p2, SLVS_E_POINT_IN_2D));
  assert(isEntityType(p3, SLVS_E_POINT_IN_2D));
  assert(isEntityType(p4, SLVS_E_POINT_IN_2D));
  
  entities.push_back(Slvs_MakeCubic(neh, group, workPlane, p1, p2, p3, p4));
  neh++;
  return entities.back().h;
}

/*! @details remove an entity from the system.
 * 
 * @param eh is the handle of the entity to remove.
 * @note This is just removal of the entity. No reference validation is performed.
 * @see clean.
 */
void Solver::removeEntity(Slvs_hEntity eh)
{
  for (auto it = entities.begin(); it != entities.end(); ++it)
  {
    if (it->h == eh)
    {
      entities.erase(it);
      break;
    }
  }
}

/*! @details Get all the orphaned entities. An orphaned entity is one 
 * that has references to a non-existent parameter or entity.
 * 
 * @return std::vector of orphaned entity handles
 * @see removeOrpanedEntities.
 */
std::vector<Slvs_hEntity> Solver::getOrphanedEntities() const
{
  std::vector<Slvs_hParam> aph; //all parameter handles
  for (const auto &p : parameters)
    aph.push_back(p.h);
  uniquefy(aph);
  
  std::vector<Slvs_hEntity> aeh; //all entity handles
  for (const auto &e : entities)
    aeh.push_back(e.h);
  uniquefy(aeh);
  
  std::vector<Slvs_hEntity> out;
  for (const auto &e : entities)
  {
    std::vector<Slvs_hParam> cph; //current entity handles
    cph.push_back(e.param[0]);
    cph.push_back(e.param[1]);
    cph.push_back(e.param[2]);
    cph.push_back(e.param[3]);
    uniquefy(cph);
    
    std::vector<Slvs_hEntity> ceh; //current entity handles
    ceh.push_back(e.wrkpl);
    ceh.push_back(e.point[0]);
    ceh.push_back(e.point[1]);
    ceh.push_back(e.point[2]);
    ceh.push_back(e.point[3]);
    ceh.push_back(e.normal);
    ceh.push_back(e.distance);
    uniquefy(ceh);
    
    std::vector<Slvs_hParam> tph; //temp parameter handles
    std::set_intersection(cph.begin(), cph.end(), aph.begin(), aph.end(), std::back_inserter(tph));
    if (tph.size() != cph.size())
    {
      out.push_back(e.h);
      continue;
    }
    
    std::vector<Slvs_hEntity> teh; //temp entity handles
    std::set_intersection(ceh.begin(), ceh.end(), aeh.begin(), aeh.end(), std::back_inserter(teh));
    if (teh.size() != ceh.size())
    {
      out.push_back(e.h);
      continue;
    }
  }
  
  return out;
}

/*! @details Scan and remove all the orphaned entities in the system.
 * 
 * @return The number of entities removed.
 * @see getOrphanedEntities for definition of an 'orphaned entity'.
 * @see clean is probably desired over this function.
 */
int Solver::removeOrphanedEntities()
{
  int count = 0;
  for (auto eh : getOrphanedEntities())
  {
    removeEntity(eh);
    count++;
  }
  return count;
}


/*! @details Add a completed constraint.
 * 
 * @param cIn is a fully constructed constraint. See note about handle.
 * @returns The handle to the new created constraint.
 * @note This is for general purpose and typically other functions are more convenient.
 * cIn.h should be value from @ref getNextConstraintHandle or 0. Don't
 * pass in random values for the handle.
 */
Slvs_hConstraint Solver::addConstraint(const Slvs_Constraint &cIn)
{
  constraints.push_back(cIn);
  if (constraints.back().h == 0)
    constraints.back().h = nch;
  nch++;
  return constraints.back().h;
}

/*! @details Add a coincident constraint between 2 points.
 * 
 * @param point1 is a handle to a 2d or 3d point.
 * @param point2 is a handle to a 2d or 3d point.
 * @return The handle of the new point coincident constraint.
 */
Slvs_hConstraint Solver::addPointsCoincident(Slvs_hEntity point1, Slvs_hEntity point2)
{
  assert(isEntityType(point1, SLVS_E_POINT_IN_2D) || isEntityType(point1, SLVS_E_POINT_IN_3D));
  assert(isEntityType(point2, SLVS_E_POINT_IN_2D) || isEntityType(point2, SLVS_E_POINT_IN_3D));
  return addConstraint
  (
    Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_POINTS_COINCIDENT
      , workPlane
      , 0.0
      , point1
      , point2
      , 0
      , 0
    )
  );
}

/*! @details Add a point on circle constraint.
 * 
 * @param point is a handle to a 2d or 3d point.
 * @param circle is a handle to a circle or arc.
 * @return handle to newly constructed constraint.
 * @todo Investigate if circle can be arc.
 */
Slvs_hConstraint Solver::addPointOnCircle(Slvs_hEntity point, Slvs_hEntity circle)
{
  assert(isEntityType(point, SLVS_E_POINT_IN_2D) || isEntityType(point, SLVS_E_POINT_IN_3D));
  assert(isEntityType(circle, SLVS_E_CIRCLE) || isEntityType(circle, SLVS_E_ARC_OF_CIRCLE));
    return addConstraint
  (
    Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_PT_ON_CIRCLE
      , workPlane
      , 0
      , point
      , 0
      , circle
      , 0
    )
  );
}

/*! @details Add a point to point distance constraint.
 * 
 * @param distance is the distance between points.
 * @param point1 is the handle to a 2d or 3d point.
 * @param point2 is the handle to a 2d or 3d point.
 * @return The handle to the newly constructed constraint.
 */
Slvs_hConstraint Solver::addPointPointDistance(double distance, Slvs_hEntity point1, Slvs_hEntity point2)
{
  assert(isEntityType(point1, SLVS_E_POINT_IN_2D) || isEntityType(point1, SLVS_E_POINT_IN_3D));
  assert(isEntityType(point2, SLVS_E_POINT_IN_2D) || isEntityType(point2, SLVS_E_POINT_IN_3D));
  return addConstraint
  (
    Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_PT_PT_DISTANCE
      , workPlane
      , distance
      , point1
      , point2
      , 0
      , 0
    )
  );
}

/*! @details Add a point to line distance constraint.
 * 
 * @param distance is the distance between point and line.
 * @param point is a handle to a 2d or 3d point.
 * @param line is a handle to a line segment.
 * @return The handle to the newly constructed constraint.
 * @note This constraint uses the line to sense direction. This
 * means that the distance can be negative or positive controlling
 * which side of the line the point lies.
 */
Slvs_hConstraint Solver::addPointLineDistance(double distance, Slvs_hEntity point, Slvs_hEntity line)
{
  assert(isEntityType(point, SLVS_E_POINT_IN_2D) || isEntityType(point, SLVS_E_POINT_IN_3D));
  assert(isEntityType(line, SLVS_E_LINE_SEGMENT));
  
  return addConstraint
  (
    Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_PT_LINE_DISTANCE
      , workPlane
      , distance
      , point
      , 0
      , line
      , 0
    )
  );
}

/*! @details Add a point to plane distance constraint.
 * 
 * @param distance is the distance between point and plane.
 * @param point is a handle to a 2d or 3d point.
 * @param plane is a handle to a work plane.
 * @return The handle to the newly constructed constraint.
 */
Slvs_hConstraint Solver::addPointPlaneDistance(double distance, Slvs_hEntity point, Slvs_hEntity plane)
{
  assert(isEntityType(point, SLVS_E_POINT_IN_2D) || isEntityType(point, SLVS_E_POINT_IN_3D));
  assert(isEntityType(plane, SLVS_E_WORKPLANE));
  
  return addConstraint
  (
    Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_PT_PLANE_DISTANCE
      , 0
      , distance
      , point
      , 0
      , plane
      , 0
    )
  );
}

/*! @details Add a point in plane constraint
 * 
 * @param point Is a handle to a 2d or 3d point.
 * @param plane is a handle to a work plane.
 * @return The handle to the newly constructed constraint.
 * @todo explore if we can use beyond just work plane.
 */
Slvs_hConstraint Solver::addPointInPlane(Slvs_hEntity point, Slvs_hEntity plane)
{
  assert(isEntityType(point, SLVS_E_POINT_IN_2D) || isEntityType(point, SLVS_E_POINT_IN_3D));
  assert(isEntityType(plane, SLVS_E_WORKPLANE));
  
  return addConstraint
  (
    Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_PT_IN_PLANE
      , 0
      , 0
      , point
      , 0
      , plane
      , 0
    )
  );
}

/*! @details Add a point on a line constraint
 * 
 * @param point Is a handle to a 2d or 3d point.
 * @param line is a handle to a line segment.
 * @return The handle to the newly constructed constraint.
 * @note 2d point will be on current work plane.
 * 3d point will be free from work plane.
 */
Slvs_hConstraint Solver::addPointOnLine(Slvs_hEntity point, Slvs_hEntity line)
{
  assert(isEntityType(point, SLVS_E_POINT_IN_2D) || isEntityType(point, SLVS_E_POINT_IN_3D));
  assert(isEntityType(line, SLVS_E_LINE_SEGMENT));
  
  Slvs_hEntity wp = 0;
  if (isEntityType(point, SLVS_E_POINT_IN_2D))
    wp = workPlane;
  
  return addConstraint
  (
    Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_PT_ON_LINE
      , wp
      , 0
      , point
      , 0
      , line
      , 0
    )
  );
}

/*! @details Add a vertical constraint to a line segment.
 * 
 * @param line is a handle to a line segment
 * @return The handle to the newly constructed constraint.
 */
Slvs_hConstraint Solver::addVertical(Slvs_hEntity line)
{
  assert(isEntityType(line, SLVS_E_LINE_SEGMENT));
  
  return addConstraint
  (
    Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_VERTICAL
      , workPlane
      , 0.0
      , 0
      , 0
      , line
      , 0
    )
  );
}

/*! @details Add a horizontal constraint to a line segment.
 * 
 * @param line is a handle to a line segment
 * @return The handle to the newly constructed constraint.
 */
Slvs_hConstraint Solver::addHorizontal(Slvs_hEntity line)
{
  assert(isEntityType(line, SLVS_E_LINE_SEGMENT));
  
  return addConstraint
  (
    Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_HORIZONTAL
      , workPlane
      , 0.0
      , 0
      , 0
      , line
      , 0
    )
  );
}

/*! @details Add a parallel constraint between 2 lines.
 * 
 * @param l1 is a handle to a line segment.
 * @param l2 is a handle to a line segment.
 * @return The handle to the newly constructed constraint.
 * @todo investigate parallel between work planes.
 */
Slvs_hConstraint Solver::addParallel(Slvs_hEntity l1, Slvs_hEntity l2)
{
  assert(isEntityType(l1, SLVS_E_LINE_SEGMENT));
  assert(isEntityType(l2, SLVS_E_LINE_SEGMENT));
  
  return addConstraint
  (
    Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_PARALLEL
      , workPlane
      , 0
      , 0
      , 0
      , l1
      , l2
    )
  );
}

/*! @details Add a perpendicular constraint between 2 lines.
 * 
 * @param l1 is a handle to a line segment.
 * @param l2 is a handle to a line segment.
 * @return The handle to the newly constructed constraint.
 * @todo investigate perpendicular outside of just lines.
 */
Slvs_hConstraint Solver::addPerpendicular(Slvs_hEntity l1, Slvs_hEntity l2)
{
  assert(isEntityType(l1, SLVS_E_LINE_SEGMENT));
  assert(isEntityType(l2, SLVS_E_LINE_SEGMENT));
  
  return addConstraint
  (
    Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_PERPENDICULAR
      , workPlane
      , 0
      , 0
      , 0
      , l1
      , l2
    )
  );
}

/*! @details Add an equal radius constraint between 2 arc/circles.
 * 
 * @param angle inclusive angle in degrees.
 * @param l1 is a handle to a line segment.
 * @param l2 is a handle to a line segment.
 * @param reversedSense is a signal on whether the lines are oriented the same.
 * @return The handle to the newly constructed constraint.
 */
Slvs_hConstraint Solver::addAngle(double angle, Slvs_hEntity l1, Slvs_hEntity l2, bool reversedSense)
{
  assert(isEntityType(l1, SLVS_E_LINE_SEGMENT));
  assert(isEntityType(l2, SLVS_E_LINE_SEGMENT));
  
  Slvs_Constraint c = Slvs_MakeConstraint
  (
    nch
    , group
    , SLVS_C_ANGLE
    , workPlane
    , angle
    , 0
    , 0
    , l1
    , l2
  );
  c.other = reversedSense;
  
  return addConstraint(c);
}

/*! @details Add an equal radius constraint between 2 arc/circles.
 * 
 * @param e1 is a handle to an arc or circle.
 * @param e2 is a handle to an arc or circle.
 * @return The handle to the newly constructed constraint.
 */
Slvs_hConstraint Solver::addEqualRadius(Slvs_hEntity e1, Slvs_hEntity e2)
{
  assert(isEntityType(e1, SLVS_E_CIRCLE) || isEntityType(e1, SLVS_E_ARC_OF_CIRCLE));
  assert(isEntityType(e2, SLVS_E_CIRCLE) || isEntityType(e2, SLVS_E_ARC_OF_CIRCLE));
  
  return addConstraint
  (
    Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_EQUAL_RADIUS
      , workPlane
      , 0.0
      , 0
      , 0
      , e1
      , e2
    )
  );
}

/*! @details Add an equal length constraint between 2 line segments.
 * 
 * @param l1 is a handle to a line segment.
 * @param l2 is a handle to a line segment.
 * @return The handle to the newly constructed constraint.
 */
Slvs_hConstraint Solver::addEqualLengthLines(Slvs_hEntity l1, Slvs_hEntity l2)
{
  assert(isEntityType(l1, SLVS_E_LINE_SEGMENT));
  assert(isEntityType(l2, SLVS_E_LINE_SEGMENT));
  return addConstraint
  (
    Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_EQUAL_LENGTH_LINES
      , workPlane
      , 0.0
      , 0
      , 0
      , l1
      , l2
    )
  );
}

/*! @details Add an equal length constraint between line segment and arc.
 * 
 * @param l is a handle to a line segment.
 * @param a is a handle to an arct.
 * @return The handle to the newly constructed constraint.
 * @note This doesn't work with a full circle.
 */
Slvs_hConstraint Solver::addEqualLengthLineArc(Slvs_hEntity l, Slvs_hEntity a)
{
  assert(isEntityType(l, SLVS_E_LINE_SEGMENT));
  assert(isEntityType(a, SLVS_E_ARC_OF_CIRCLE));
  return addConstraint
  (
    Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_EQUAL_LINE_ARC_LEN
      , workPlane
      , 0.0
      , 0
      , 0
      , l
      , a
    )
  );
}

/*! @details Add an equal length constraint between 2 line segments.
 * 
 * @param ratio is the ratio.
 * @param l1 is a handle to a line segment.
 * @param l2 is a handle to a line segment.
 * @return The handle to the newly constructed constraint.
 * @note l2.length = l1.length / ratio
 */
Slvs_hConstraint Solver::addLengthRatio(double ratio, Slvs_hEntity l1, Slvs_hEntity l2)
{
  assert(isEntityType(l1, SLVS_E_LINE_SEGMENT));
  assert(isEntityType(l2, SLVS_E_LINE_SEGMENT));
  
  return addConstraint
  (
    Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_LENGTH_RATIO
      , workPlane
      , ratio
      , 0
      , 0
      , l1
      , l2
    )
  );
}

/*! @details Add a length equal to the distance between point and line constraint.
 * 
 * @param p is a handle to a 2d or 3d point.
 * @param l1 is a handle to a line segment for length
 * @param l2 is a handle to a line segment for distance.
 * @return The handle to the newly constructed constraint.
 * @note This is a weird one. The length of l1 will be equal
 * to the distance between p and l2. Don't see how this is useful.
 */
Slvs_hConstraint Solver::addEqualLengthPointLineDistance(Slvs_hEntity l1, Slvs_hEntity p, Slvs_hEntity l2)
{
  assert(isEntityType(l1, SLVS_E_LINE_SEGMENT));
  assert(isEntityType(l2, SLVS_E_LINE_SEGMENT));
  assert(isEntityType(p, SLVS_E_POINT_IN_2D) || isEntityType(p, SLVS_E_POINT_IN_3D));
  
  return addConstraint
  (
    Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_EQ_LEN_PT_LINE_D
      , workPlane
      , 0
      , p
      , 0
      , l1
      , l2
    )
  );
}

/*! @details Add 'length from point to line is equal to length from other point to other line' constraint.
 * 
 * @param p1 is a handle to a 2d or 3d point.
 * @param p2 is a handle to a 2d or 3d point.
 * @param l1 is a handle to a line segment
 * @param l2 is a handle to a line segment
 * @return The handle to the newly constructed constraint.
 * @note see overload for 2 points and 1 line.
 */
Slvs_hConstraint Solver::addEqualPointLineDistances(Slvs_hEntity p1, Slvs_hEntity p2, Slvs_hEntity l1, Slvs_hEntity l2)
{
  assert(isEntityType(p1, SLVS_E_POINT_IN_2D) || isEntityType(p1, SLVS_E_POINT_IN_3D));
  assert(isEntityType(p2, SLVS_E_POINT_IN_2D) || isEntityType(p2, SLVS_E_POINT_IN_3D));
  assert(isEntityType(l1, SLVS_E_LINE_SEGMENT));
  assert(isEntityType(l2, SLVS_E_LINE_SEGMENT));
  
  return addConstraint
  (
    Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_EQ_PT_LN_DISTANCES
      , workPlane
      , 0
      , p1
      , p2
      , l1
      , l2
    )
  );
}

/*! @details Add 'length from point to line is equal to length from other point to same line' constraint.
 * 
 * @param p1 is a handle to a 2d or 3d point.
 * @param p2 is a handle to a 2d or 3d point.
 * @param l is a handle to a line segment
 * @return The handle to the newly constructed constraint.
 * @note see overload for 2 points and 2 lines.
 */
Slvs_hConstraint Solver::addEqualPointLineDistances(Slvs_hEntity p1, Slvs_hEntity p2, Slvs_hEntity l)
{
  assert(isEntityType(p1, SLVS_E_POINT_IN_2D) || isEntityType(p1, SLVS_E_POINT_IN_3D));
  assert(isEntityType(p2, SLVS_E_POINT_IN_2D) || isEntityType(p2, SLVS_E_POINT_IN_3D));
  assert(isEntityType(l, SLVS_E_LINE_SEGMENT));
  
  return addConstraint
  (
    Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_EQ_PT_LN_DISTANCES
      , workPlane
      , 0
      , p1
      , p2
      , l
      , l
    )
  );
}

/*! @details Add equal angle for 4 lines constraint.
 * 
 * @param l1 is a handle to a line segment
 * @param l2 is a handle to a line segment
 * @param l3 is a handle to a line segment
 * @param l4 is a handle to a line segment
 * @return The handle to the newly constructed constraint.
 * @note angle between l1 and l2 will be equal to angle between l3 and l4.
 */
Slvs_hConstraint Solver::addEqualAngle(Slvs_hEntity l1, Slvs_hEntity l2, Slvs_hEntity l3, Slvs_hEntity l4)
{
  assert(isEntityType(l1, SLVS_E_LINE_SEGMENT));
  assert(isEntityType(l2, SLVS_E_LINE_SEGMENT));
  assert(isEntityType(l3, SLVS_E_LINE_SEGMENT));
  assert(isEntityType(l4, SLVS_E_LINE_SEGMENT));
  
  //note: slvs.h:Slvs_MakeConstraint doesnt have parameters for entities c and d.
  //so we will pass zeros and fill in all entities.
  Slvs_Constraint c = Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_EQUAL_ANGLE
      , workPlane
      , 0
      , 0
      , 0
      , 0
      , 0
    );
  c.entityA = l1;
  c.entityB = l2;
  c.entityC = l3;
  c.entityD = l4;
    
  return addConstraint(c);
}

/*! @details Add equal angle for 4 lines constraint.
 * 
 * @param l1 is a handle to a line segment
 * @param l2 is a handle to a line segment
 * @param l3 is a handle to a line segment
 * @return The handle to the newly constructed constraint.
 * @note angle between l1 and l2 will be equal to angle between l2 and l3.
 */
Slvs_hConstraint Solver::addEqualAngle(Slvs_hEntity l1, Slvs_hEntity l2, Slvs_hEntity l3)
{
  assert(isEntityType(l1, SLVS_E_LINE_SEGMENT));
  assert(isEntityType(l2, SLVS_E_LINE_SEGMENT));
  assert(isEntityType(l3, SLVS_E_LINE_SEGMENT));
  
  //note: slvs.h:Slvs_MakeConstraint doesnt have parameters for entities c and d.
  //so we will pass zeros and fill in all entities.
  Slvs_Constraint c = Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_EQUAL_ANGLE
      , workPlane
      , 0
      , 0
      , 0
      , 0
      , 0
    );
  c.entityA = l1;
  c.entityB = l2;
  c.entityC = l2;
  c.entityD = l3;
    
  return addConstraint(c);
}

/*! @details Add symmetric horizontal constraint.
 * 
 * @param l is a handle to a line segment
 * @return The handle to the newly constructed constraint.
 */
Slvs_hConstraint Solver::addSymmetricHorizontal(Slvs_hEntity l)
{
  assert(isEntityType(l, SLVS_E_LINE_SEGMENT));
  auto le = findEntity(l);
  
  return addConstraint
  (
    Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_SYMMETRIC_HORIZ
      , workPlane
      , 0
      , le.get().point[0]
      , le.get().point[1]
      , 0
      , 0
    )
  );
}

/*! @details Add symmetric horizontal constraint.
 * 
 * @param p1 is a handle to a 2d or 3d point
 * @param p2 is a handle to a 2d or 3d point
 * @return The handle to the newly constructed constraint.
 */
Slvs_hConstraint Solver::addSymmetricHorizontal(Slvs_hEntity p1, Slvs_hEntity p2)
{
  assert(isEntityType(p1, SLVS_E_POINT_IN_2D) || isEntityType(p1, SLVS_E_POINT_IN_3D));
  assert(isEntityType(p2, SLVS_E_POINT_IN_2D) || isEntityType(p2, SLVS_E_POINT_IN_3D));
  
  return addConstraint
  (
    Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_SYMMETRIC_HORIZ
      , workPlane
      , 0
      , p1
      , p2
      , 0
      , 0
    )
  );
}

/*! @details Add symmetric vertical constraint.
 * 
 * @param l is a handle to a line segment
 * @return The handle to the newly constructed constraint.
 */
Slvs_hConstraint Solver::addSymmetricVertical(Slvs_hEntity l)
{
  assert(isEntityType(l, SLVS_E_LINE_SEGMENT));
  
  auto le = findEntity(l);
  
  return addConstraint
  (
    Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_SYMMETRIC_VERT
      , workPlane
      , 0
      , le.get().point[0]
      , le.get().point[1]
      , 0
      , 0
    )
  );
}

/*! @details Add symmetric vertical constraint.
 * 
 * @param p1 is a handle to a 2d or 3d point
 * @param p2 is a handle to a 2d or 3d point
 * @return The handle to the newly constructed constraint.
 */
Slvs_hConstraint Solver::addSymmetricVertical(Slvs_hEntity p1, Slvs_hEntity p2)
{
  assert(isEntityType(p1, SLVS_E_POINT_IN_2D) || isEntityType(p1, SLVS_E_POINT_IN_3D));
  assert(isEntityType(p2, SLVS_E_POINT_IN_2D) || isEntityType(p2, SLVS_E_POINT_IN_3D));
  
  return addConstraint
  (
    Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_SYMMETRIC_VERT
      , workPlane
      , 0
      , p1
      , p2
      , 0
      , 0
    )
  );
}

/*! @details Add a symmetric line constraint.
 * 
 * @param l1 is a handle to a line segment.
 * @param l2 is a handle to a line segment.
 * @return The handle to the newly constructed constraint.
 * @note the first line is centered and perpendicular to the second line.
 * Helps to have lengths established to keep from zero length line.
 */
Slvs_hConstraint Solver::addSymmetricLine(Slvs_hEntity l1, Slvs_hEntity l2)
{
  assert(isEntityType(l1, SLVS_E_LINE_SEGMENT));
  assert(isEntityType(l2, SLVS_E_LINE_SEGMENT));
  
  auto le = findEntity(l2);
  
  return addConstraint
  (
    Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_SYMMETRIC_LINE
      , workPlane
      , 0
      , le.get().point[0]
      , le.get().point[1]
      , l1
      , 0
    )
  );
}

/*! @details Add a symmetric line constraint.
 * 
 * @param p1 is a handle to a 2d or 3d point.
 * @param p2 is a handle to a 2d or 3d point.
 * @param l is a handle to a line segment.
 * @return The handle to the newly constructed constraint.
 * @note points are equal distance and perpendicular to the line.
 */
Slvs_hConstraint Solver::addSymmetricLine(Slvs_hEntity p1, Slvs_hEntity p2, Slvs_hEntity l)
{
  assert(isEntityType(p1, SLVS_E_POINT_IN_2D) || isEntityType(p1, SLVS_E_POINT_IN_3D));
  assert(isEntityType(p2, SLVS_E_POINT_IN_2D) || isEntityType(p2, SLVS_E_POINT_IN_3D));
  assert(isEntityType(l, SLVS_E_LINE_SEGMENT));
  
  return addConstraint
  (
    Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_SYMMETRIC_LINE
      , workPlane
      , 0
      , p1
      , p2
      , l
      , 0
    )
  );
}

/*! @details Add a line symmetric to a plane constraint.
 * 
 * @param wp is a handle to a work plane.
 * @param l is a handle to a line segment.
 * @return The handle to the newly constructed constraint.
 */
Slvs_hConstraint Solver::addSymmetric(Slvs_hEntity wp, Slvs_hEntity l)
{
  assert(isEntityType(wp, SLVS_E_WORKPLANE));
  assert(isEntityType(l, SLVS_E_LINE_SEGMENT));
  
  auto le = findEntity(l);
  
  return addConstraint
  (
    Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_SYMMETRIC
      , 0
      , 0
      , le.get().point[0]
      , le.get().point[1]
      , wp
      , 0
    )
  );
}

/*! @details Add a points symmetric to a plane constraint.
 * 
 * @param wp is a handle to a work plane.
 * @param p1 is a handle to a 2d or 3d point.
 * @param p2 is a handle to a 2d or 3d point.
 * @return The handle to the newly constructed constraint.
 */
Slvs_hConstraint Solver::addSymmetric(Slvs_hEntity wp, Slvs_hEntity p1, Slvs_hEntity p2)
{
  assert(isEntityType(wp, SLVS_E_WORKPLANE));
  assert(isEntityType(p1, SLVS_E_POINT_IN_2D) || isEntityType(p1, SLVS_E_POINT_IN_3D));
  assert(isEntityType(p2, SLVS_E_POINT_IN_2D) || isEntityType(p2, SLVS_E_POINT_IN_3D));
  
  return addConstraint
  (
    Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_SYMMETRIC
      , 0
      , 0
      , p1
      , p2
      , wp
      , 0
    )
  );
}

/*! @details Add a constraint for a point to lie on mid point of line
 * 
 * @param p is a handle for a 2d or 3d point.
 * @param l is a handle to a line segment.
 * @return The handle to the newly constructed constraint.
 */
Slvs_hConstraint Solver::addMidpointLine(Slvs_hEntity p, Slvs_hEntity l)
{
  assert(isEntityType(p, SLVS_E_POINT_IN_2D) || isEntityType(p, SLVS_E_POINT_IN_3D));
  assert(isEntityType(l, SLVS_E_LINE_SEGMENT));
  
  return addConstraint
  (
    Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_AT_MIDPOINT
      , workPlane
      , 0
      , p
      , 0
      , l
      , 0
    )
  );
}

/*! @details Add a constraint for a line center to lie on a work plane
 * 
 * @param l is a handle to a line segment.
 * @param p is a handle for a work plane.
 * @return The handle to the newly constructed constraint.
 */
Slvs_hConstraint Solver::addMidlinePlane(Slvs_hEntity l, Slvs_hEntity p)
{
  assert(isEntityType(l, SLVS_E_LINE_SEGMENT));
  assert(isEntityType(p, SLVS_E_WORKPLANE));
  
  return addConstraint
  (
    Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_AT_MIDPOINT
      , 0
      , 0
      , 0
      , 0
      , l
      , p
    )
  );
}

/*! @details Add a same orientation constraint.
 * 
 * @param e1 is a handle to a work plane or normal3d or normal2d.
 * @param e2 is a handle to a work plane or normal3d or normal2d.
 * @return The handle to the newly constructed constraint.
 * @note haven't test the implications mixing normal2d and normal3d.
 * I can't get this to behave the way I think it should. Needs more
 * work before it is useful.
 */
Slvs_hConstraint Solver::addSameOrientation(Slvs_hEntity e1, Slvs_hEntity e2)
{
  assert
  (
    isEntityType(e1, SLVS_E_WORKPLANE)
    || isEntityType(e1, SLVS_E_NORMAL_IN_3D)
    || isEntityType(e1, SLVS_E_NORMAL_IN_2D)
  );
  
  assert
  (
    isEntityType(e2, SLVS_E_WORKPLANE)
    || isEntityType(e2, SLVS_E_NORMAL_IN_3D)
    || isEntityType(e2, SLVS_E_NORMAL_IN_2D)
  );
  
  auto assign = [&](Slvs_hEntity e) -> Slvs_hEntity
  {
    if (!isEntityType(e, SLVS_E_WORKPLANE))
      return e;
    const Slvs_Entity &eref = findEntity(e).get();
    return eref.normal;
  };
  
  Slvs_hEntity o1 = assign(e1);
  Slvs_hEntity o2 = assign(e2);
  
  return addConstraint
  (
    Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_SAME_ORIENTATION
      , 0
      , 0
      , 0
      , 0
      , o1
      , o2
    )
  );
}

/*! @details Helper function for tangent constraint functions.
 * 
 * @param ent1 is a handle to an arc, line or cubic bezier. Not circle.
 * @param ent2 is a handle to an arc, line or cubic bezier. Not circle.
 * @return optional pair of handles to points respectively.
 * first member of pair will be referenced by e1
 * second member of pair will be referenced by e2.
 * @note
 */
boost::optional<std::pair<Slvs_hEntity, Slvs_hEntity> > Solver::getCoincidentPoints(const Slvs_Entity &ent1, const Slvs_Entity &ent2)
{
  assert
  (
    (ent1.type == SLVS_E_ARC_OF_CIRCLE)
    || (ent1.type == SLVS_E_LINE_SEGMENT)
    || (ent1.type == SLVS_E_CUBIC)
  );
  assert
  (
    (ent2.type == SLVS_E_ARC_OF_CIRCLE)
    || (ent2.type == SLVS_E_LINE_SEGMENT)
    || (ent2.type == SLVS_E_CUBIC)
  );
  
  auto fillPoints = [](const Slvs_Entity &entity) -> std::vector<Slvs_hEntity>
  {
    std::vector<Slvs_hEntity> out;
    
    out.push_back(entity.point[0]);
    out.push_back(entity.point[1]);
    out.push_back(entity.point[2]);
    out.push_back(entity.point[3]);
    uniquefy(out);
    
    return out;
  };
  
  std::vector<Slvs_hEntity> points1 = fillPoints(ent1);
  std::vector<Slvs_hEntity> points2 = fillPoints(ent2);
  std::vector<Slvs_hEntity> ap; //all points
  std::set_union(points1.begin(), points1.end(), points2.begin(), points2.end(), std::back_inserter(ap));
  
  boost::optional<Slvs_hEntity> v1, v2;
  for (const auto &c : constraints)
  {
    if (c.type != SLVS_C_POINTS_COINCIDENT)
      continue;
    std::vector<Slvs_hEntity> cPoints;
    cPoints.push_back(c.ptA);
    cPoints.push_back(c.ptB);
    uniquefy(cPoints);
    std::vector<Slvs_hEntity> intersect;
    std::set_intersection(ap.begin(), ap.end(), cPoints.begin(), cPoints.end(), std::back_inserter(intersect));
    if (intersect.size() != 2)
      continue;
    
    if (std::find(points1.begin(), points1.end(), intersect.front()) != points1.end())
      v1 = intersect.front();
    if (std::find(points1.begin(), points1.end(), intersect.back()) != points1.end())
      v1 = intersect.back();
    if (std::find(points2.begin(), points2.end(), intersect.front()) != points2.end())
      v2 = intersect.front();
    if (std::find(points2.begin(), points2.end(), intersect.back()) != points2.end())
      v2 = intersect.back();
    break;
  }
  
  if (!v1 || !v2)
    return boost::none;
  return std::make_pair(v1.get(), v2.get());
}

/*! @details Add arc line tangent constraint.
 * 
 * @param a is a handle to an arc. Not circle.
 * @param l is a handle to a line segment.
 * @return The handle to the newly constructed constraint or 0 if failed.
 * @note This constraint requires that there be a point to point coincident
 * constraint between the arc and line. If there isn't, no constraint will
 * be added and the handle return will be an invalid handle(0).
 */
Slvs_hConstraint Solver::addArcLineTangent(Slvs_hEntity a, Slvs_hEntity l)
{
  assert(isEntityType(a, SLVS_E_ARC_OF_CIRCLE));
  assert(isEntityType(l, SLVS_E_LINE_SEGMENT));
  
  const Slvs_Entity &arc = findEntity(a).get();
  const Slvs_Entity &line = findEntity(l).get();
  
  boost::optional<bool> other;
  auto pointsPair = getCoincidentPoints(arc, line);
  if(!pointsPair)
    return 0;
  if (arc.point[1] == pointsPair.get().first) //arc start.
    other = false;
  if (arc.point[2] == pointsPair.get().first) //arc finish.
    other = true;
 
  if (!other)
  {
    std::cout << "WARNING: couldn't establish common end point for line arc tangent" << std::endl;
    return 0;
  }
  
  //note: slvs.h:Slvs_MakeConstraint doesnt have parameter for 'other' that
  //arc line tangent uses.
  Slvs_Constraint c = Slvs_MakeConstraint
  (
    nch
    , group
    , SLVS_C_ARC_LINE_TANGENT
    , workPlane
    , 0
    , 0
    , 0
    , a
    , l
  );
  
  c.other = other.get();
  return addConstraint(c);
}

/*! @details Add a cubic bezier line tangent constraint.
 * 
 * @param cIn is a handle to a bezier curve.
 * @param lIn is a handle to a line segment.
 * @return The handle to the newly constructed constraint or 0 if failed.
 * @note This constraint requires that there be a point to point coincident
 * constraint between the bezier curve and the line. If there isn't, no constraint will
 * be added and the handle return will be an invalid handle(0). Asserts on
 * bezier point count != 4.
 */
Slvs_hConstraint Solver::addCubicLineTangent(Slvs_hEntity cIn, Slvs_hEntity lIn)
{
  assert(isEntityType(cIn, SLVS_E_CUBIC));
  assert(isEntityType(lIn, SLVS_E_LINE_SEGMENT));
  
  const Slvs_Entity &cubic = findEntity(cIn).get();
  const Slvs_Entity &line = findEntity(lIn).get();
  
  assert(cubic.point[0]);
  assert(cubic.point[1]);
  assert(cubic.point[2]);
  assert(cubic.point[3]);
  
  boost::optional<bool> other;
  auto pointsPair = getCoincidentPoints(cubic, line);
  if (!pointsPair)
    return 0;
  assert(pointsPair);
  if (cubic.point[0] == pointsPair.get().first) //cubic start.
    other = false;
  if (cubic.point[3] == pointsPair.get().first) //cubic finish.
    other = true;
  if (!other)
  {
    std::cout << "Error: couldn't establish common end point for cubic line tangent" << std::endl;
    return 0;
  }
  
  //note: slvs.h:Slvs_MakeConstraint doesnt have parameter for 'other' that
  //arc line tangent uses.
  Slvs_Constraint c = Slvs_MakeConstraint
  (
    nch
    , group
    , SLVS_C_CUBIC_LINE_TANGENT
    , workPlane
    , 0
    , 0
    , 0
    , cIn
    , lIn
  );

  c.other = other.get();
  return addConstraint(c);
}

/*! @details Add a cubic bezier line tangent constraint.
 * 
 * @param pIn is a handle to a 2d or 3d point.
 * @return The handle to the newly constructed constraint.
 * @note Not sure how useful this is outside of the solvespace gui.
 * Haven't tested.
 */
Slvs_hConstraint Solver::addWhereDragged(Slvs_hEntity pIn)
{
  assert(isEntityType(pIn, SLVS_E_POINT_IN_2D) || isEntityType(pIn, SLVS_E_POINT_IN_3D));
  
  return addConstraint
  (
    Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_WHERE_DRAGGED
      , workPlane
      , 0
      , pIn
      , 0
      , 0
      , 0
    )
  );
}

/*! @details Add a tangent constraint between 2 curves.
 * 
 * @param e1 is a handle to an arc or cubic bezier. Not circle
 * @param e2 is a handle to an arc or cubic bezier. Not circle
 * @return The handle to the newly constructed constraint.
 * @note This constraint requires that there be a point to point coincident
 * constraint between the 2 entities. If there isn't, no constraint will
 * be added and the handle return will be an invalid handle(0). Asserts on
 * bezier point count != 4.
 */
Slvs_hConstraint Solver::addCurveCurveTangent(Slvs_hEntity e1, Slvs_hEntity e2)
{
  assert(isEntityType(e1, SLVS_E_ARC_OF_CIRCLE) || isEntityType(e1, SLVS_E_CUBIC));
  assert(isEntityType(e2, SLVS_E_ARC_OF_CIRCLE) || isEntityType(e2, SLVS_E_CUBIC));
  
  const Slvs_Entity &ent1 = findEntity(e1).get();
  const Slvs_Entity &ent2 = findEntity(e2).get();
  
  auto getPoints = [](const Slvs_Entity &e) -> std::vector<Slvs_hEntity>
  {
    std::vector<Slvs_hEntity> out;
    if (e.type == SLVS_E_CUBIC)
    {
      out.push_back(e.point[0]);
      out.push_back(e.point[3]);
    }
    else
    {
      out.push_back(e.point[1]);
      out.push_back(e.point[2]);
    }
    return out;
  };
  
  auto pointsPair = getCoincidentPoints(ent1, ent2);
  if(!pointsPair)
    return 0;
  std::vector<Slvs_hEntity> eps1 = getPoints(ent1);
  std::vector<Slvs_hEntity> eps2 = getPoints(ent2);
  boost::optional<bool> other, other2;
  
  if (pointsPair.get().first == eps1.front())
    other = false;
  if (pointsPair.get().first == eps1.back())
    other = true;
  if (pointsPair.get().second == eps2.front())
    other2 = false;
  if (pointsPair.get().second == eps2.back())
    other2 = true;
  
  assert(other);
  assert(other2);
  
  //note: slvs.h:Slvs_MakeConstraint doesnt have parameter for
  //'other' and 'other2' that 'curve curve tangent' uses.
  Slvs_Constraint c = Slvs_MakeConstraint
  (
    nch
    , group
    , SLVS_C_CURVE_CURVE_TANGENT
    , workPlane
    , 0
    , 0
    , 0
    , e1
    , e2
  );
  c.other = other.get();
  c.other2 = other2.get();
  
  return addConstraint(c);
}

/*! @details Add a length difference constraint to 2 line segments.
 * 
 * @param value Is the distance between 2 line segments.
 * @param l1 is a handle to a line segment.
 * @param l2 is a handle to a line segment.
 * @return The handle to the newly constructed constraint.
 * @note The second line second with be 'value' units shorter than the first.
 */
Slvs_hConstraint Solver::addLengthDifference(double value, Slvs_hEntity l1, Slvs_hEntity l2)
{
  assert(isEntityType(l1, SLVS_E_LINE_SEGMENT));
  assert(isEntityType(l2, SLVS_E_LINE_SEGMENT));
  
  return addConstraint
  (
    Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_LENGTH_DIFFERENCE
      , workPlane
      , value
      , 0
      , 0
      , l1
      , l2
    )
  );
}

/*! @details Add a diameter constraint to an arc or circle.
 * 
 * @param value Is the value for the diameter.
 * @param e is a handle to an arc or circle.
 * @return The handle to the newly constructed constraint.
 */
Slvs_hConstraint Solver::addDiameter(double value, Slvs_hEntity e)
{
  assert(isEntityType(e, SLVS_E_CIRCLE) || isEntityType(e, SLVS_E_ARC_OF_CIRCLE));
  
  return addConstraint
  (
    Slvs_MakeConstraint
    (
      nch
      , group
      , SLVS_C_DIAMETER
      , workPlane
      , value
      , 0
      , 0
      , e
      , 0
    )
  );
}

/*! @details remove a constraint from the system.
 * 
 * @param ch is the handle of the constraint to remove.
 * @note This is just removal of the constraint. No reference validation is performed.
 * @see clean.
 */
void Solver::removeConstraint(Slvs_hConstraint ch)
{
  for (auto it = constraints.begin(); it != constraints.end(); ++it)
  {
    if (it->h == ch)
    {
      constraints.erase(it);
      break;
    }
  }
}

/*! @details Get all the orphaned constraints. An orphaned constraint is one 
 * that has references to a non-existent entity.
 * 
 * @return std::vector of orphaned constraint handles
 * @see removeOrpanedConstraints.
 */
std::vector<Slvs_hConstraint> Solver::getOrphanedConstraints() const
{
  std::vector<Slvs_hEntity> aeh; //all entity handles
  for (const auto &e : entities)
    aeh.push_back(e.h);
  uniquefy(aeh);
  
  std::vector<Slvs_hConstraint> out;
  for (const auto &c : constraints)
  {
    std::vector<Slvs_hEntity> ceh; //current entity handles
    ceh.push_back(c.ptA);
    ceh.push_back(c.ptB);
    ceh.push_back(c.entityA);
    ceh.push_back(c.entityB);
    ceh.push_back(c.entityC);
    ceh.push_back(c.entityD);
    uniquefy(ceh);
    std::vector<Slvs_hEntity> teh; //temp entity handles.
    std::set_intersection(ceh.begin(), ceh.end(), aeh.begin(), aeh.end(), std::back_inserter(teh));
    if (teh.size() != ceh.size())
      out.push_back(c.h);
  }
  return out;
}

/*! @details Scan and remove all the orphaned constraints in the system.
 * 
 * @return The number of constraints removed.
 * @see getOrphanedConstraints for definition of an 'orphaned constraint'.
 * @see clean is probably desired over this function.
 */
int Solver::removeOrphanedConstraints()
{
  int count = 0;
  for (auto ch : getOrphanedConstraints())
  {
    removeConstraint(ch);
    count++;
  }
  return count;
}

/*! @details Set the value of a constraint.
 * 
 * @param handle handle to the constraint to modify
 * @param value new value for the constraint
 */
void Solver::updateConstraintValue(Slvs_hConstraint handle, double value)
{
  for (auto &c : constraints)
  {
    if (c.h == handle)
    {
      c.valA = value;
      break;
    }
  }
}

/*! @details Clears the parameters marked for drag
 * 
 * @see dragSet.
 */
void Solver::dragClear()
{
  sys.dragged[0] = 0;
  sys.dragged[1] = 0;
  sys.dragged[2] = 0;
  sys.dragged[3] = 0;
}

/*! @details Mark parameters for drag.
 * Marking parameters for drag causes the solver to favor those parameters.
 * 
 * @param prms Is the parameters to be marked for drag. Max of 4 parameters.
 * @see dragClear. 
 * @note Any current parameters marked for drag will be removed.
 */
void Solver::dragSet(const std::vector<Slvs_hParam> &prms)
{
  std::vector<Slvs_hParam> copy(prms);
  copy.resize(4, 0);
  for (std::size_t i = 0; i < copy.size(); ++i)
    sys.dragged[i] = copy.at(i);
}

/*! @details Find the actual parameter from a handle.
 * 
 * @param pIn Is the handle of the parameter to find.
 * @return A boost optional containing a const Parameter& if handle is found. 
 * 
 */
boost::optional<const Slvs_Param&> Solver::findParameter(Slvs_hParam pIn) const
{
  for (const auto &p : parameters)
  {
    if (p.h == pIn)
      return p;
  }
  
  return boost::none;
}

/*! @details Find the actual entity from a handle.
 * 
 * @param eIn Is the handle of the entity to find.
 * @return A boost optional containing a const Enity& if handle is found. 
 * 
 */
boost::optional<const Slvs_Entity&> Solver::findEntity(Slvs_hEntity eIn) const
{
  for (const auto &e : entities)
  {
    if (e.h == eIn)
      return e;
  }
  
  return boost::none;
}

/*! @details Find the actual constraint from a handle.
 * 
 * @param cIn Is the handle of the constraint to find.
 * @return A boost optional containing a const Constraint& if handle is found. 
 * 
 */
boost::optional<const Slvs_Constraint&> Solver::findConstraint(Slvs_hConstraint cIn) const
{
  for (const auto &c : constraints)
  {
    if (c.h == cIn)
      return c;
  }
  
  return boost::none;
}

/*! @details Test for entity types.
 * 
 * @param eh Is the handle of the entity to test.
 * @param typeIn Is the type to match.
 * @return Whether it is a match.
 */
bool Solver::isEntityType(Slvs_hEntity eh, int typeIn)
{
  auto eo = findEntity(eh);
  if (!eo)
    return false;
  if (eo.get().type == typeIn)
    return true;
  
  return false;
}

/*! @details Get the value from a parameter.
 * 
 * @param pIn Is the handle of a parameter.
 * @return A boost optional containing a double if handle is found. 
 * 
 */
boost::optional<double> Solver::getParameterValue(Slvs_hParam pIn)
{
  if (auto parameter = findParameter(pIn))
    return parameter.get().val;
  return boost::none;
}

/*! @details Set the value from a parameter.
 * 
 * @param pIn Is the handle of a parameter.
 * @param freshValue Is the new value for the parameter.
 * 
 */
void Solver::setParameterValue(Slvs_hParam pIn, double freshValue)
{
  for (auto &p : parameters)
  {
    if (p.h == pIn)
    {
      p.val = freshValue;
      break;
    }
  }
}

/*! @details Get the next handle for a parameter.
 * 
 * @return The next parameter handle.
 * @note This increments the next parameter handle
 * to help ensure handles are unique. 
 */
Slvs_hParam Solver::getNextParameterHandle()
{
  Slvs_hParam out = nph;
  nph++;
  return out;
}

/*! @details Get the next handle for an entity.
 * 
 * @return The next entity handle.
 * @note This increments the next entity handle
 * to help ensure handles are unique. 
 */
Slvs_hEntity Solver::getNextEntityHandle()
{
  Slvs_hEntity out = neh;
  neh++;
  return out;
}

/*! @details Get the next handle for a constraint.
 * 
 * @return The next constraint handle.
 * @note This increments the next constraint handle
 * to help ensure handles are unique. 
 */
Slvs_hConstraint Solver::getNextConstraintHandle()
{
  Slvs_hConstraint out = nch;
  nch++;
  return out;
}

/*! @details Tries to ensure referencial integrity after removal of parameters, entities, constraints.
 * 
 * @param maxIterations Is the maximum loops. Warning to std::cout if max is reached.
 * 
 * @note The removal of any parameter, entity or constraint can trigger a 'chain reaction' of
 * invalid references. Going with this 'hack' while no real tree/graph structure.
 */
void Solver::clean(int maxIterations)
{
  for (int index = 0; index < maxIterations; ++index)
  {
    if
    (
      (
        removeOrphanedParameters()
        + removeOrphanedEntities()
        + removeOrphanedConstraints()
      )
      == 0
    )
    {
      return;
    }
  }
  std::cout << "WARNING: max iterations reached in: " << BOOST_CURRENT_FUNCTION << std::endl;
}

/*! @details Setup prior to the call to solve.
 * 
 * @note This is called from solve, so user doesn't normally need to use this function.
 */
void Solver::prepare()
{
  sys.param = &parameters[0];
  sys.params = parameters.size();
  
  sys.entity = &entities[0];
  sys.entities = entities.size();
  
  sys.constraint = &constraints[0];
  sys.constraints = constraints.size();
  
  //I am betting the failed count going into the actual solver
  //specifies the size of the array.
  failed.clear();
  failed.resize(constraints.size(), 0);
  sys.failed = &failed[0];
  sys.faileds = failed.size();
}

/*! @details solve the system
 * 
 * @param groupIn which group to solve. other groups will be constant
 * @param calculateFailures problem analysis. Didn't see any difference
 * in output for demo with purposefully redundant constraint. Looking at
 * system.cpp:System::Solve, it appears the parameter is used in a special
 * case and can be by passed. Long story short: don't expect much.
 */
void Solver::solve(const Slvs_hGroup &groupIn, bool calculateFailures)
{
  prepare();
  if (calculateFailures)
    sys.calculateFaileds = 1;
  else
    sys.calculateFaileds = 0;
  Slvs_Solve(&sys, groupIn);
  failed.resize(sys.faileds);
  dragClear();
}

/*! @details get the result code from last call to solve.
 * 
 * @return code from last call to solve.
 * @see getResultMessage
 */
int Solver::getResultCode()
{
  return sys.result;
}

/*! @details get the result message from last call to solve.
 * 
 * @return message from last call to solve.
 * @see getResultCode
 */
std::string Solver::getResultMessage()
{
  static const std::array<std::string, 4> messages =
  {
    "OK"
    , "Inconsistent"
    , "Didn't converge"
    , "Too many unknowns"
  };
  
  int result = sys.result;
  assert(result >= 0 && result <= 3);
  return messages.at(result);
}

//doxygen wasn't picking this up in the source file for some reason.
/*! @details Stream overload for solver. Helpful for debugging
* 
* @param stream To receive output.
* @param solver Target for output.
* @return The input stream.
*/
std::ostream& skt::operator<<(std::ostream &stream, const skt::Solver &solver)
{
  stream
    << "parameter count: " << solver.getParameters().size() << std::endl
    << "entity count: " << solver.getEntities().size() << std::endl
    << "constraint count: " << solver.getConstraints().size() << std::endl
    << "failed handle count: " << solver.getFailed().size() << std::endl
    << "next parameter handle: " << solver.nph << "    "
    << "next entity handle: " << solver.neh << "    "
    << "next constraint handle: " << solver.nch << std::endl
    << "current group handle: " << solver.group << std::endl
    << "current work plane handle: " << solver.workPlane << std::endl
    << "orphaned parameter count: " << solver.getOrphanedParameters().size() << std::endl
    << "orphaned entity count: " << solver.getOrphanedEntities().size() << std::endl
    << "orphaned constraint count: " << solver.getOrphanedConstraints().size() << std::endl
    ;
  
  return stream;
}

/*! @details Stream overload for parameter. Helpful for debugging
 * 
 * @param stream To receive output.
 * @param p Target for output.
 * @return The input stream.
 */
std::ostream& skt::operator<<(std::ostream &stream, const ::Slvs_Param &p)
{
  stream
    << "handle: " << p.h << std::endl
    << "group:  " << p.group << std::endl
    << "val:    " << p.val << std::endl
    ;
    
  return stream;
}

/*! @details Stream overload for entity. Helpful for debugging
 * 
 * @param stream To receive output.
 * @param e Target for output.
 * @return The input stream.
 */
std::ostream& skt::operator<<(std::ostream &stream, const ::Slvs_Entity &e)
{
  stream
    << "handle:      " << e.h << std::endl
    << "group:       " << e.group << std::endl
    << "type:        " << e.type << std::endl
    << "work plane:  " << e.wrkpl << std::endl
    << "point 0:     " << e.point[0] << std::endl
    << "point 1:     " << e.point[1] << std::endl
    << "point 2:     " << e.point[2] << std::endl
    << "point 3:     " << e.point[3] << std::endl
    << "normal:      " << e.normal << std::endl
    << "distance:    " << e.distance << std::endl
    << "parameter 0: " << e.param[0] << std::endl
    << "parameter 1: " << e.param[1] << std::endl
    << "parameter 2: " << e.param[2] << std::endl
    << "parameter 3: " << e.param[3] << std::endl
    ;
    
  return stream;
}

/*! @details Stream overload for entity. Helpful for debugging
 * 
 * @param stream To receive output.
 * @param c Target for output.
 * @return The input stream.
 */
std::ostream& skt::operator<<(std::ostream &stream, const ::Slvs_Constraint &c)
{
  stream
    << "handle:     " << c.h << std::endl
    << "group:      " << c.group << std::endl
    << "type:       " << c.type << std::endl
    << "work plane: " << c.wrkpl << std::endl
    << "valA:       " << c.valA << std::endl
    << "ptA:        " << c.ptA << std::endl
    << "ptB:        " << c.ptB << std::endl
    << "entityA:    " << c.entityA << std::endl
    << "entityB:    " << c.entityB << std::endl
    << "entityC:    " << c.entityC << std::endl
    << "entityD:    " << c.entityD << std::endl
    << "other:      " << c.other << std::endl
    << "other2:     " << c.other2 << std::endl
    ;
    
  return stream;
}
