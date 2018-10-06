#ifndef SKT_VISUAL_H
#define SKT_VISUAL_H

#include <memory>
#include <utility>

#include <boost/uuid/uuid.hpp>
#include <boost/optional/optional_fwd.hpp>

#include <osg/ref_ptr>
#include <osgUtil/PolytopeIntersector>
#include <osgUtil/LineSegmentIntersector>

#include "nodemasks.h"

namespace osg
{
  class PositionAttitudeTransform;
  class MatrixTransform;
  class Geometry;
  class Vec3d;
  class Group;
  class Drawable;
}
namespace osgText
{
  class Text;
}

namespace skt
{
  typedef uint32_t SSHandle; //!< @brief SolveSpace handle.
  struct Solver;
  struct Map;
  struct Data;
  
  /*! @class Visual
   * @brief Manages visual interface to a sketch.
   * 
   * The sketch handles managing references between parameters, entities and constraints.
   * The visual should update itself to the state of the sketch. For the most
   * part visual will not change the sketch, but there will be cases it has to. i.e. dragging.
   */
  class Visual
  {
  public:
    Visual(Solver&);
    ~Visual();
    
    /** @anchor Visibility
     * @name Visibility
     * Functions used to control Visibility.
     */
    ///@{
    void update();
    void showPlane();
    void hidePlane();
    void showEntity();
    void hideEntity();
    void showConstraint();
    void hideConstraint();
    void showAll();
    void hideAll();
    void setActiveSketch();
    void clearActiveSketch();
    ///@}
    
    /** @anchor SelectionInteraction
     * @name Selection Interaction
     * These are instigated from the selection event handler.
     */
    ///@{
    void move(const osgUtil::PolytopeIntersector::Intersections&);
    void move(const osgUtil::LineSegmentIntersector::Intersections&);
    void pick(const osgUtil::PolytopeIntersector::Intersections&);
    void pick(const osgUtil::LineSegmentIntersector::Intersections&);
    void startDrag(const osgUtil::LineSegmentIntersector::Intersections&);
    void drag(const osgUtil::LineSegmentIntersector::Intersections&);
    void finishDrag(const osgUtil::LineSegmentIntersector::Intersections&);
    void clearSelection();
    ///@}
    
    /** @anchor Colors
     * @name Colors
     * Functions that control color.
     */
    ///@{
    void setPreHighlightColor(const osg::Vec4&);
    osg::Vec4 getPreHighlightColor();
    void setHighlightColor(const osg::Vec4&);
    osg::Vec4 getHighlightColor();
    ///@}
    
    /** @anchor ObjectControl
     * @name Object Control
     * Functions for adding and removing.
     */
    ///@{
    void addPoint();
    void addLine();
    void addArc();
    void addCircle();
    void addCoincident();
    void addHorizontal();
    void addVertical();
    void addTangent();
    void addDistance();
    void addEqual();
    void addEqualAngle();
    void addDiameter();
    void addSymmetric();
    void addAngle();
    void addParallel();
    void addPerpendicular();
    void addMidpoint();
    void addWhereDragged();
    void remove();
    void cancel();
    void toggleConstruction();
    ///@}
    
    /** @anchor Sizing
     * @name Sizing
     * Functions for controlling the visible plane size.
     * When autoSize is true, the visible plane size will be determined
     * when update is called. When autoSize is false, the plane size will
     * be determined by the value of size. @see autoSize size
     */
    ///@{
    void setAutoSize(bool);
    bool getAutoSize();
    void setSize(double);
    double getSize();
    ///@}
    
    /** @anchor VisualMisc
     * @name Misc
     */
    ///@{
    State getState();
    osg::MatrixTransform* getTransform();
    static osg::Vec3d projectPointLine(const osg::Vec3d&, const osg::Vec3d&, const osg::Vec3d&);
    static double distancePointLine(const osg::Vec3d&, const osg::Vec3d&, const osg::Vec3d&);
    bool isConstruction(SSHandle);
    boost::uuids::uuid getEntityId(SSHandle);
    ///@}
    
  private:
    Solver &solver; //!< Constraint Solver.
    std::unique_ptr<Data> data; //!< Private data. pimpl.
    
    bool autoSize; //!< @brief Size the visible plane automatically. @ref Sizing
    double size; //!< @brief Radius of the visible plane. @ref Sizing
    double lastSize; //!< @brief Last size used to update the visible plane.
    
    void buildPlane();
    osg::Geometry* buildGeometry();
    osg::Geometry* buildLine();
    osg::Geometry* buildPoint();
    osg::Geometry* buildArc();
    osg::Geometry* buildCircle();
    
    osgText::Text* buildConstraintText();
    osg::PositionAttitudeTransform* buildConstraint1(const std::string&, const std::string&);
    osg::Group* buildConstraint2(const std::string&, const std::string&);
    
    void updatePlaneSize();
    void updatePlane();
    void updateText();
    void updateArcGeometry(osg::Geometry*, const osg::Vec3d&, const osg::Vec3d&, const osg::Vec3d&);
    
    osg::Vec3d convert(SSHandle);
    boost::optional<osg::Vec3d> getPlanePoint(const osgUtil::LineSegmentIntersector::Intersections&);
    std::vector<SSHandle> getSelectedPoints(bool includeWork = true);
    std::vector<SSHandle> getSelectedLines(bool includeWork = true);
    std::vector<SSHandle> getSelectedCircles();
    osg::Vec3d parameterPoint(SSHandle, double);
    double vectorAngle(osg::Vec3d, osg::Vec3d);
    osg::Vec3d lineVector(SSHandle);
    std::pair<osg::Vec3d, osg::Vec3d> endPoints(SSHandle);
    osg::Vec3d boundingCenter(const std::vector<SSHandle>&);
    
    void goPreHighlight();
    void clearPreHighlight();
    void goHighlight(osg::Drawable*);
    void clearHighlight(osg::Drawable*);
    void setPointBig(osg::Geometry*);
    void setPointSmall(osg::Geometry*);
  };
}

#endif // SKT_VISUAL_H
