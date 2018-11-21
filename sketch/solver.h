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


#ifndef SKT_SOLVER_H
#define SKT_SOLVER_H


#include <vector>
#include <cstring>

#include <boost/optional/optional.hpp>

#include <slvs.h>

namespace prj{namespace srl{class Solver;}}

/*! @namespace skt
 * @brief sketcher namespace
 */
namespace skt
{
  /*! @struct Solver
   * @brief Interface to sketch solving.
   * 
   * This class encapsulates the open source solver SolveSpace <http://solvespace.com>. 
   * SolveSpace has 3 main types of objects: @ref Parameters, @ref Entities and @ref Constraints.
   * All objects have handles that uniquely identify that object within its type. 
   * For example, notice the types 'Slvs_Param' and 'Slvs_hParam'.
   * The latter is the handle. Slvs_Param has a member named 'h' that is
   * it's handle. To get the Slvs_Param from the handle use @ref findParameter. This is also
   * true for entities and constraints. Be aware as most functions work with these handles, not the objects.
   * Also the handles are just typedefs for uint32_t, so there is no type safety between parameter,
   * entities and constraint handles.
   * SolveSpace organizes objects into groups and only solves for 1 group at time. To help ease
   * the burden, this class keeps an active group and active workplane so that add newly
   * added objects will reference these members. @ref Grouping and @ref WorkPlane. Here is
   * an example that adds a line segment on the work plane. Note that that because the work plane
   * and it's origin belong to the first group and we solve for the current, which is the second,
   * the origin is not free to move and only the second point can move.
   * 
   * @code
   * Solver solver;             //create the solver object.
   * solver.setWorkPlane();     //create and set the work plane to absolute
   * solver.groupIncrement();   //increase group for future objects. This isolates work plane from what we want to solve.
   * Slvs_hEntity p1 = solver.addPoint2d(1.0, 1.0);   //this will create 2 parameters and 'hook' the point to them.
   * Slvs_hEntity l1 = solver.addLineSegment(solver.getWPOrigin(), p1); //creates a line segment between origin and new point.
   * Slvs_hConstraint c1 = solver.addPointPointDistance(5.1, solver.getWPOrigin(), p1); //constrain the distance between points.
   * solver.solve(solver.getGroup(), true); //solve for second group.
   * if(solver.getSys().result == SLVS_RESULT_OKAY)
   *   std::cout << "solver success. " << solver.getSys().dof << " DOF" << std::endl;
   * else
   *   std::cout << "solver failed. " << std::endl;
   * @endcode
   * 
   */
  struct Solver
  {
  public:
    /** @anchor Grouping
     * @name Grouping
     * 
     * Because parameters, entities and constraints all belong to a group,
     * There is an active group member that is set at all times.
     */
    ///@{
    Slvs_hGroup getGroup() const {return group;}//!< @details return handle of current group
    void setGroup(Slvs_hGroup fresh) {group = fresh;}//!< @details set the current group to handle
    void groupIncrement() {group++;}//!< @details increment the current group by 1
    ///@}
    
    /** @anchor WorkPlane 
     * @name WorkPlane/Coordinate System
     * 
     * Because most entities reference a work plane, 
     * there is an active work plane member and should be set at all times.
     */
    ///@{
    Slvs_hEntity getWorkPlane() const {return workPlane;}//!< @details return handle of the current work plane.
    void setWorkPlane(Slvs_hEntity fresh) {workPlane = fresh;}//!< @details set the current work plane to handle
    void setWorkPlane();
    Slvs_hEntity getWPOrigin() const;
    Slvs_hEntity getWPNormal3d() const;
    ///@}
    
    /** @anchor Axes 
     * @name Axes of WorkPlane
     * 
     * Entities representing the axes of a work plane are needed for constraints.
     * For this, we create unit length line segments. These line segments are related
     * to a work plane, but it is users responsibility to keep the current workplane
     * and the current axes in sync.
     */
    ///@{
    Slvs_hEntity getXAxis() const {return xAxis;}//!< @details return the handle of the current x axis.
    Slvs_hEntity getYAxis() const {return yAxis;}//!< @details return the handle of the current y axis.
    void createXAxis();
    void createYAxis();
    ///@}
    
    /** @anchor Parameters
     * @name Parameters
     * Parameters are variables to be solved.
     */
    ///@{
    Slvs_hParam addParameter(double);
    void removeParameter(Slvs_hParam);
    std::vector<Slvs_hParam> getOrphanedParameters() const;
    int removeOrphanedParameters();
    const std::vector<Slvs_Param>& getParameters() const {return parameters;}//!< @details get const vector of actual parameters.
    ///@}
    
    /** @anchor Entities
     * @name Entities
     * Entities are points, lines, arc etc.. New entities will be
     * added to the current group and will use the current workplane
     * when appropriate. @ref Grouping @ref WorkPlane
     * 
     */
    ///@{
    Slvs_hEntity addPoint3d(Slvs_hParam, Slvs_hParam, Slvs_hParam);
    Slvs_hEntity addPoint3d(double, double, double);
    Slvs_hEntity addPoint2d(Slvs_hParam, Slvs_hParam);
    Slvs_hEntity addPoint2d(double, double);
    Slvs_hEntity addNormal3d(Slvs_hParam, Slvs_hParam, Slvs_hParam, Slvs_hParam);
    Slvs_hEntity addNormal3d(double, double, double, double, double, double);
    Slvs_hEntity addWorkPlane(Slvs_hEntity, Slvs_hEntity);
    Slvs_hEntity addLineSegment(Slvs_hEntity, Slvs_hEntity);
    Slvs_hEntity addArcOfCircle(Slvs_hEntity, Slvs_hEntity, Slvs_hEntity, Slvs_hEntity);
    Slvs_hEntity addDistance(double);
    Slvs_hEntity addDistance(Slvs_hParam);
    Slvs_hEntity addCircle(Slvs_hEntity, Slvs_hEntity);
    Slvs_hEntity addCubicBezier(Slvs_hEntity, Slvs_hEntity, Slvs_hEntity, Slvs_hEntity);
    
    void removeEntity(Slvs_hEntity);
    std::vector<Slvs_hEntity> getOrphanedEntities() const;
    int removeOrphanedEntities();
    const std::vector<Slvs_Entity>& getEntities() const {return entities;}//!< @details get const vector of actual entities.
    ///@}
    
    /** @anchor Constraints
     * @name Constraints
     * Constraints are rules to tell the solver the characteristics of the Entities.
     * Constraints are added to the current group. @ref Grouping
     */
    ///@{
    Slvs_hConstraint addConstraint(const Slvs_Constraint&);
    Slvs_hConstraint addPointsCoincident(Slvs_hEntity, Slvs_hEntity);
    Slvs_hConstraint addPointOnCircle(Slvs_hEntity, Slvs_hEntity);
    Slvs_hConstraint addPointPointDistance(double, Slvs_hEntity, Slvs_hEntity);
    Slvs_hConstraint addPointLineDistance(double, Slvs_hEntity, Slvs_hEntity);
    Slvs_hConstraint addPointPlaneDistance(double, Slvs_hEntity, Slvs_hEntity);
    Slvs_hConstraint addPointInPlane(Slvs_hEntity, Slvs_hEntity);
    Slvs_hConstraint addPointOnLine(Slvs_hEntity, Slvs_hEntity);
    Slvs_hConstraint addVertical(Slvs_hEntity);
    Slvs_hConstraint addHorizontal(Slvs_hEntity);
    Slvs_hConstraint addParallel(Slvs_hEntity, Slvs_hEntity);
    Slvs_hConstraint addPerpendicular(Slvs_hEntity, Slvs_hEntity);
    Slvs_hConstraint addAngle(double, Slvs_hEntity, Slvs_hEntity, bool);
    Slvs_hConstraint addEqualRadius(Slvs_hEntity, Slvs_hEntity);
    Slvs_hConstraint addEqualLengthLines(Slvs_hEntity, Slvs_hEntity);
    Slvs_hConstraint addEqualLengthLineArc(Slvs_hEntity, Slvs_hEntity);
    Slvs_hConstraint addLengthRatio(double, Slvs_hEntity, Slvs_hEntity);
    Slvs_hConstraint addEqualLengthPointLineDistance(Slvs_hEntity, Slvs_hEntity, Slvs_hEntity);
    Slvs_hConstraint addEqualPointLineDistances(Slvs_hEntity, Slvs_hEntity, Slvs_hEntity, Slvs_hEntity);
    Slvs_hConstraint addEqualPointLineDistances(Slvs_hEntity, Slvs_hEntity, Slvs_hEntity);
    Slvs_hConstraint addEqualAngle(Slvs_hEntity, Slvs_hEntity, Slvs_hEntity, Slvs_hEntity);
    Slvs_hConstraint addEqualAngle(Slvs_hEntity, Slvs_hEntity, Slvs_hEntity);
    Slvs_hConstraint addSymmetricHorizontal(Slvs_hEntity);
    Slvs_hConstraint addSymmetricHorizontal(Slvs_hEntity, Slvs_hEntity);
    Slvs_hConstraint addSymmetricVertical(Slvs_hEntity);
    Slvs_hConstraint addSymmetricVertical(Slvs_hEntity, Slvs_hEntity);
    Slvs_hConstraint addSymmetricLine(Slvs_hEntity, Slvs_hEntity);
    Slvs_hConstraint addSymmetricLine(Slvs_hEntity, Slvs_hEntity, Slvs_hEntity);
    Slvs_hConstraint addSymmetric(Slvs_hEntity, Slvs_hEntity);
    Slvs_hConstraint addSymmetric(Slvs_hEntity, Slvs_hEntity, Slvs_hEntity);
    Slvs_hConstraint addMidpointLine(Slvs_hEntity, Slvs_hEntity);
    Slvs_hConstraint addMidlinePlane(Slvs_hEntity, Slvs_hEntity);
    Slvs_hConstraint addSameOrientation(Slvs_hEntity, Slvs_hEntity);
    Slvs_hConstraint addArcLineTangent(Slvs_hEntity, Slvs_hEntity);
    Slvs_hConstraint addCubicLineTangent(Slvs_hEntity, Slvs_hEntity);
    Slvs_hConstraint addWhereDragged(Slvs_hEntity);
    Slvs_hConstraint addCurveCurveTangent(Slvs_hEntity, Slvs_hEntity);
    Slvs_hConstraint addLengthDifference(double, Slvs_hEntity, Slvs_hEntity);
    
    Slvs_hConstraint addDiameter(double, Slvs_hEntity);
    
    void removeConstraint(Slvs_hConstraint);
    std::vector<Slvs_hConstraint> getOrphanedConstraints() const;
    int removeOrphanedConstraints();
    const std::vector<Slvs_Constraint>& getConstraints() const {return constraints;}//!< @details get const vector of actual constraints.
    void updateConstraintValue(Slvs_hConstraint, double);
    ///@}
    
    /** @anchor Drag
     * @name Drag
     * Marking parameters for drag causes the solver to favor the current
     * values of those parameters over other parameters. Drag parameters
     * will be cleared upon a call to solve.
     */
    ///@{
    void dragClear();
    void dragSet(const std::vector<Slvs_hParam>&); //clears and sets. max of 4.
    ///@}
    
    /** @anchor Misc
     * @name Misc
     */
    ///@{
    const Slvs_System& getSys(){return sys;} //!< @details Return the read only solvespace object.
    friend std::ostream& operator<<(std::ostream&, const skt::Solver&);
    
    boost::optional<const Slvs_Param&> findParameter(Slvs_hParam) const;
    boost::optional<const Slvs_Entity&> findEntity(Slvs_hEntity) const;
    boost::optional<const Slvs_Constraint&> findConstraint(Slvs_hConstraint) const;
    bool isEntityType(Slvs_hEntity, int);
    boost::optional<double> getParameterValue(Slvs_hParam);
    void setParameterValue(Slvs_hParam, double);
    
    Slvs_hParam getNextParameterHandle();
    Slvs_hEntity getNextEntityHandle();
    Slvs_hConstraint getNextConstraintHandle();
    
    void clean(int maxIterations = 10000);
    void prepare();
    void solve(const Slvs_hGroup&, bool);
    int getResultCode();
    std::string getResultMessage();
    const std::vector<Slvs_hConstraint>& getFailed() const {return failed;}//!< @details get const vector of failing constraints.
    ///@}
    
    /** @anchor Serial
     * @name Serial
     */
    ///@{
    prj::srl::Solver serialOut() const;
    void serialIn(const prj::srl::Solver&);
    ///@}
    
  private:
    Slvs_System sys;//!< @details SolveSpace system.
    
    std::vector<Slvs_Param> parameters;//!< @details All the parameters for the system.
    std::vector<Slvs_Entity> entities;//!< @details All the entities for the system.
    std::vector<Slvs_Constraint> constraints;//!< @details All the constraints for the system.
    
    std::vector<Slvs_hConstraint> failed; //!< @details Failing constraint handles for a failed solve.
    
    Slvs_hParam nph = 1; //!< @details Next parameter handle.
    Slvs_hEntity neh = 1; //!< @details Next entity handle.
    Slvs_hConstraint nch = 1; //!< @details Next constraint handle.
    Slvs_hGroup group = 1; //!< @details Group for all new parameters, entities and constraints.
    Slvs_hEntity workPlane = 0; //!< @details Workplane for all new entities that need one.
    Slvs_hEntity xAxis = 0; //!< @details Unit line segment representing x axis on the current workplane.
    Slvs_hEntity yAxis = 0; //!< @details Unit line segment representing y axis on the current workplane.
    
    boost::optional<std::pair<Slvs_hEntity, Slvs_hEntity> > getCoincidentPoints(const Slvs_Entity&, const Slvs_Entity&);
  };
  
  /*! @brief Make sure unique, sorted, and non zero
   * 
   * @note used for vectors of handles.
   * Values of zero are removed as SolveSpace
   * considers them to be a null handle.
   */
  template<typename T>
  void uniquefy(T &t)
  {
    std::sort(t.begin(), t.end());
    auto last = std::unique(t.begin(), t.end());
    t.erase(last, t.end());
    last = std::remove(t.begin(), t.end(), 0);
    t.erase(last, t.end());
  }
  
  /** @anchor StreamOverloads
  * @name Stream Overloads
  * Useful for debugging
  */
  ///@{
  std::ostream& operator<<(std::ostream&, const Solver&);
  std::ostream& operator<<(std::ostream&, const ::Slvs_Param&);
  std::ostream& operator<<(std::ostream&, const ::Slvs_Entity&);
  std::ostream& operator<<(std::ostream&, const ::Slvs_Constraint&);
  ///@}
}

#endif // SKT_SOLVER_H
