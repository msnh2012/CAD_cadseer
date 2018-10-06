#include <iostream>
#include <sstream>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/optional/optional.hpp>

#include <osg/PositionAttitudeTransform>
#include <osg/MatrixTransform>
#include <osg/Node>
#include <osg/Geometry>
#include <osg/Point>
#include <osg/Switch>
#include <osg/Group>
#include <osg/LineWidth>
#include <osg/BlendFunc>
#include <osg/LineStipple>
#include <osg/Callback>
#include <osg/AutoTransform>
#include <osg/Depth>
#include <osg/Hint>
#include <osgText/Text>

#include "solver.h"
#include "visual.h"

using boost::uuids::uuid;

static thread_local boost::uuids::random_generator rGen;
static thread_local boost::uuids::string_generator sGen;
// static thread_local boost::uuids::nil_generator nGen;

namespace skt
{
  /*! @struct Map
   * @brief Associations between: solvespace handles, Geometry ids. openscenegraph nodes. 
   * @details I could use 1 osg::Geometry node for the whole sketch
   * or individual nodes for each entity. 1 node will
   * have performance, but be a pain to work with. See mdv::ShapeGeometryPrivate
   * Individual nodes will be much easier to work with but possible slow performance.
   * Going with individual for now. Lets don't premature optimize. Sketches should
   * be small to moderate on complexity. Not going to use boost multi index
   * for this.
   */
  struct Map
  {
    /*!@struct Record
     * @brief Associations between: solvespace handles, Geometry ids. openscenegraph nodes. 
     */
    struct Record
    {
      SSHandle handle; //!< solvespace handle
      uuid id; //!< id for output geometry where applicable.
      osg::ref_ptr<osg::Node> node; //!< geometry or a transform with child text
      bool referenced; //!< helps with finding entities automatically removed by solver.
      bool construction = false; //!< for construction geometry.
    };
    
    std::vector<Record> records; //!< all the records for mapping
    
    /*! @brief Get a record.
     * @param key Solvespace handle used for search.
     * @return Optional that may contain a Record reference.
     */
    boost::optional<Record&> getRecord(SSHandle key)
    {
      for (auto &r : records)
      {
        if (r.handle == key)
          return r;
      }
      return boost::none;
    }
    
    /*! @brief Get a record.
     * @param key id used for search.
     * @return Optional that may contain a Record reference.
     */
    boost::optional<Record&> getRecord(const uuid &key)
    {
      for (auto &r : records)
      {
        if (r.id == key)
          return r;
      }
      return boost::none;
    }
    
    /*! @brief Get a record.
     * @param key openscenegraph reference pointer used for search.
     * @return Optional that may contain a Record reference.
     */
    boost::optional<Record&> getRecord(const osg::ref_ptr<osg::Node> &key)
    {
      for (auto &r : records)
      {
        if (r.node == key)
          return r;
      }
      return boost::none;
    }
    
    /*! @brief Set all records as unreferenced.
     * @details Used at the beginning of an update to keep track
     * of objects still in the solver.
     */
    void setAllUnreferenced()
    {
      for (auto &r : records)
        r.referenced = false;
    }
    
    /*! @brief Clear records that are unreferenced.
     * @details Used at the ending of an update to keep
     * records in sync with the solver.
     */
    void removeUnreferenced()
    {
      for (auto it = records.begin(); it != records.end();)
      {
        if (!it->referenced)
          it = records.erase(it);
        else
          ++it;
      }
    }
  };
  
  /*! @struct Data
   * @brief Private data for visual
   */
  struct Data
  {
    osg::Vec4 preHighlightColor = osg::Vec4(1.0, 1.0, 0.0, 1.0); //!< Color used for prehighlighting.
    osg::Vec4 highlightColor = osg::Vec4(1.0, 1.0, 1.0, 1.0); //!< Color used for highlighting.
    osg::Vec4 entityColor = osg::Vec4(1.0f, 0.0f, 1.0f, 1.0f); //!< Color used for entities.
    osg::Vec4 constructionColor = osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f); //!< Color used for entities.
    
    osg::ref_ptr<osg::Drawable> preHighlight; //!< object currently prehighlighted or invalid.
    std::vector<osg::ref_ptr<osg::Drawable>> highlights;  //!< objects currently highlighted or empty.
    
    Map eMap; //!< Entity map.
    Map cMap; //!< Constraint map.
    
    State state = State::selection; //!< Current state 
    boost::optional<osg::Vec3d> previousPoint; //!< Used for 2 pick operation. i.e line
    osg::ref_ptr<osg::Drawable> previousPick; //!< What was prehighlighted during previous pick
    osg::ref_ptr<osg::Drawable> dynamic; //!< Dynamic geometry currently being created. Line, arc etc..
    
    osg::ref_ptr<osg::MatrixTransform> transform; //!< Transformation of sketch plane.
    osg::ref_ptr<osg::Switch> theSwitch; //!< Switch to control visibility of entities, constraints etc.
    osg::ref_ptr<osg::Group> planeGroup; //!< Visible plane, axes etc
    osg::ref_ptr<osg::Group> entityGroup; //!< Lines, arcs etc
    osg::ref_ptr<osg::Group> constraintGroup; //!< Constraint visuals.
    osg::ref_ptr<osg::PositionAttitudeTransform> statusTextTransform; //!< Status text into corner of visible plane.
    osg::ref_ptr<osgText::Text> statusText; //!< Solve info
    osg::ref_ptr<osg::Depth> planeDepth; //!< Depth offset for plane
    osg::ref_ptr<osg::Depth> curveDepth; //!< Depth offset for curve geometry
    osg::ref_ptr<osg::Depth> pointDepth; //!< Depth offset for point geometry
  };
}
  
/*! @class ForceCullCallback
 * @brief Assigned to selection plane to keep in invisible to user.
 */
class ForceCullCallback : public osg::DrawableCullCallback
{
public:
  //! Override cull
  virtual bool cull(osg::NodeVisitor*, osg::Drawable*, osg::State*) const override
  {
    return true;
  }
};

/*! @brief Make a drawable invisible to user.
 * @param drawable Object to make invisible.
 */
static void setDrawableToAlwaysCull(osg::Drawable& drawable)
{
  ForceCullCallback* cullCB = new ForceCullCallback;
  drawable.setCullCallback(cullCB);
}

/*! @class NoBoundCallback
 * @brief Assigned to selection plane to keep large selection plane from influencing bounding operations.
 */
class NoBoundCallback : public osg::Drawable::ComputeBoundingBoxCallback
{
public:
  //! Override computeBound
  virtual osg::BoundingBox computeBound (const osg::Drawable&) const override
  {
    return osg::BoundingBox();
  }
};

/*! @brief Make a drawable have no bounding box.
 * @param drawable Object to have an invalid bound.
 */
static void setDrawableToNoBound(osg::Drawable& drawable)
{
  NoBoundCallback* nb = new NoBoundCallback;
  drawable.setComputeBoundingBoxCallback(nb);
}


/*! @brief Find a named child.
 * @param parent Object to search.
 * @param nameIn Name of child to search.
 * @return optional that may contain templated pointer.
 * @note This searches one level. Not a recursive search.
 */
template <class T>
boost::optional<T*> getNamedChild(osg::Group *parent, const std::string &nameIn)
{
  for (unsigned int i = 0; i < parent->getNumChildren(); ++i)
  {
    if (parent->getChild(i)->getName() == nameIn)
    {
      T* out = dynamic_cast<T*>(parent->getChild(i));
      if (out)
        return out;
    }
  }
  return boost::none;
}

using namespace skt;

//! @brief construct visual referencing given solver.
Visual::Visual(Solver &sIn)
: solver(sIn)
, data(new Data())
, autoSize(true)
, size(1.0)
, lastSize(0.0)
{
  data->transform = new osg::MatrixTransform();
  data->theSwitch = new osg::Switch();
  data->planeGroup = new osg::Group();
  data->entityGroup = new osg::Group();
  data->constraintGroup = new osg::Group();
  data->statusTextTransform = new osg::PositionAttitudeTransform();
  data->statusText = new osgText::Text();
  data->planeDepth = new osg::Depth(osg::Depth::LESS, 0.03, 1.00);
  data->curveDepth = new osg::Depth(osg::Depth::LESS, 0.02, 1.00);
  data->pointDepth = new osg::Depth(osg::Depth::LESS, 0.01, 1.00);
  
  data->transform->addChild(data->theSwitch.get());
  data->theSwitch->addChild(data->planeGroup.get());
  data->theSwitch->addChild(data->entityGroup.get());
  data->theSwitch->addChild(data->constraintGroup.get());
  data->theSwitch->setAllChildrenOn();
  
  data->statusText->setName("statusText");
  data->statusText->setFont("fonts/arial.ttf");
  data->statusText->setColor(osg::Vec4(0.0, 0.0, 1.0, 1.0));
  data->statusText->setBackdropType(osgText::Text::OUTLINE);
  data->statusText->setBackdropColor(osg::Vec4(1.0, 1.0, 1.0, 1.0));
  data->statusText->setAlignment(osgText::Text::RIGHT_TOP);
  data->statusTextTransform->addChild(data->statusText);
  data->transform->addChild(data->statusTextTransform); //should we add underneath switch?
  
  //Some of this is for nice looking points. Didn't work.
  data->transform->getOrCreateStateSet()->setMode(GL_LINE_SMOOTH, osg::StateAttribute::ON);
  data->transform->getOrCreateStateSet()->setMode(GL_POINT_SMOOTH, osg::StateAttribute::ON);
  data->transform->getOrCreateStateSet()->setAttributeAndModes(new osg::Hint(GL_POINT_SMOOTH_HINT, GL_NICEST));
  osg::BlendFunc* bf = new osg::BlendFunc(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_SRC_ALPHA); 
  data->transform->getOrCreateStateSet()->setAttributeAndModes(bf);
}

//! @brief destroy object
Visual::~Visual(){}

//! @brief update visual to new state of solver.
void Visual::update()
{
  data->eMap.setAllUnreferenced();
  for (const auto &e : solver.getEntities())
  {
    //understood that entites in group 1 are constant and don't need to be shown.
    if (e.group == 1)
      continue;
    auto record = data->eMap.getRecord(e.h);
    if (!record)
    {
      // add new entity
      data->eMap.records.push_back(Map::Record());
      record = data->eMap.records.back();
      record.get().handle = e.h;
      record.get().id = rGen();
      record.get().construction = false;
      if (e.type == SLVS_E_POINT_IN_2D)
      {
        record.get().node = buildPoint();
        record.get().node->setNodeMask(Entity.to_ulong());
        record.get().node->setName(boost::uuids::to_string(record.get().id)); //move outside once all types.
        data->entityGroup->addChild(record.get().node); //move outside once all types.
      }
      else if (e.type == SLVS_E_LINE_SEGMENT)
      {
        record.get().node = buildLine();
        record.get().node->setNodeMask(Entity.to_ulong());
        record.get().node->setName(boost::uuids::to_string(record.get().id)); //move outside once all types.
        data->entityGroup->addChild(record.get().node); //move outside once all types.
      }
      else if (e.type == SLVS_E_ARC_OF_CIRCLE)
      {
        record.get().node = buildArc();
        record.get().node->setNodeMask(Entity.to_ulong());
        record.get().node->setName(boost::uuids::to_string(record.get().id)); //move outside once all types.
        data->entityGroup->addChild(record.get().node); //move outside once all types.
      }
      else if (e.type == SLVS_E_CIRCLE)
      {
        record.get().node = buildCircle();
        record.get().node->setNodeMask(Entity.to_ulong());
        record.get().node->setName(boost::uuids::to_string(record.get().id)); //move outside once all types.
        data->entityGroup->addChild(record.get().node); //move outside once all types.
      }
    }
    //update entity, even new ones.
    if (e.type == SLVS_E_POINT_IN_2D)
    {
      osg::Geometry *geometry = dynamic_cast<osg::Geometry*>(record.get().node.get());
      assert(geometry);
      osg::Vec3Array *verts = dynamic_cast<osg::Vec3Array*>(geometry->getVertexArray());
      assert(verts);
      assert(verts->size() == 1);
      (*verts)[0] = convert(record.get().handle);
      verts->dirty();
      geometry->dirtyBound();
    }
    else if (e.type == SLVS_E_LINE_SEGMENT)
    {
      auto le = solver.findEntity(record.get().handle);
      assert(le);
      osg::Geometry *geometry = dynamic_cast<osg::Geometry*>(record.get().node.get());
      assert(geometry);
      osg::Vec3Array *verts = dynamic_cast<osg::Vec3Array*>(geometry->getVertexArray());
      assert(verts);
      assert(verts->size() == 2);
      (*verts)[0] = convert(le.get().point[0]);
      (*verts)[1] = convert(le.get().point[1]);
      verts->dirty();
      geometry->dirtyBound();
    }
    else if (e.type == SLVS_E_ARC_OF_CIRCLE)
    {
      auto ae = solver.findEntity(record.get().handle);
      assert(ae);
      osg::Geometry *geometry = dynamic_cast<osg::Geometry*>(record.get().node.get());
      assert(geometry);
      
      osg::Vec3d ac = convert(ae.get().point[0]);
      osg::Vec3d as = convert(ae.get().point[1]);
      osg::Vec3d af = convert(ae.get().point[2]);
      
      updateArcGeometry(geometry, ac, as, af);
      geometry->dirtyBound();
    }
    else if (e.type == SLVS_E_CIRCLE)
    {
      auto ce = solver.findEntity(record.get().handle);
      assert(ce);
      osg::Geometry *geometry = dynamic_cast<osg::Geometry*>(record.get().node.get());
      assert(geometry);
      
      osg::Vec3d center = convert(ce.get().point[0]);
      double radius = solver.getParameterValue(solver.findEntity(ce.get().distance).get().param[0]).get();
      osg::Vec3d se = center + osg::Vec3d(1.0, 0.0, 0.0) * radius;
      
      updateArcGeometry(geometry, center, se, se);
      geometry->dirtyBound();
    }
    record.get().referenced = true;
  }
  //clean up unreferenced.
  for (const auto &e : data->eMap.records)
  {
    if (!e.referenced)
      data->entityGroup->removeChild(e.node);
  }
  data->eMap.removeUnreferenced();
  
  //going to update plane before constraints.
  //plane update will change 'lastSize' to reflect boundind size of entities.
  //then we can use 'lastSize' to scale constraints.
  updatePlane();
  updateText();
  
  //constraints
  data->cMap.setAllUnreferenced();
  for (const auto &c : solver.getConstraints())
  {
    auto record = data->cMap.getRecord(c.h);
    if (!record)
    {
      // add new constraint
      data->cMap.records.push_back(Map::Record());
      record = data->cMap.records.back();
      record.get().handle = c.h;
      record.get().id = rGen();
      if (c.type == SLVS_C_HORIZONTAL)
        record.get().node = buildConstraint1("horizontal", "H");
      if (c.type == SLVS_C_VERTICAL)
        record.get().node = buildConstraint1("vertical", "V");
      if
      (
        c.type == SLVS_C_POINTS_COINCIDENT
        || c.type == SLVS_C_PT_ON_LINE
        || c.type == SLVS_C_PT_ON_CIRCLE
      )
        record.get().node = buildConstraint1("coincidence", "C");
      if (c.type == SLVS_C_ARC_LINE_TANGENT)
        record.get().node = buildConstraint1("tangent", "T");
      if (c.type == SLVS_C_PT_PT_DISTANCE)
        record.get().node = buildConstraint1("distance", "tempText");
      if (c.type == SLVS_C_PT_LINE_DISTANCE)
        record.get().node = buildConstraint1("distance", "tempText");
      if (c.type == SLVS_C_DIAMETER)
        record.get().node = buildConstraint1("diameter", "tempText");
      if (c.type == SLVS_C_EQUAL_LENGTH_LINES || c.type == SLVS_C_EQUAL_RADIUS || c.type == SLVS_C_EQUAL_ANGLE)
        record.get().node = buildConstraint2("equality", "E");
      if (c.type == SLVS_C_SYMMETRIC_HORIZ || c.type == SLVS_C_SYMMETRIC_VERT || c.type == SLVS_C_SYMMETRIC_LINE)
        record.get().node = buildConstraint2("symmetric", "S");
      if (c.type == SLVS_C_ANGLE)
        record.get().node = buildConstraint1("angle", "tempText");
      if (c.type == SLVS_C_PARALLEL)
        record.get().node = buildConstraint2("parallel", "P");
      if (c.type == SLVS_C_PERPENDICULAR)
        record.get().node = buildConstraint2("perpendicular", "PP");
      if (c.type == SLVS_C_AT_MIDPOINT)
        record.get().node = buildConstraint1("midpoint", "M");
      if (c.type == SLVS_C_WHERE_DRAGGED)
        record.get().node = buildConstraint1("whereDragged", "G");
      if (record.get().node.valid())
      {
        record.get().node->setNodeMask(Constraint.to_ulong());
        record.get().node->setName(boost::uuids::to_string(record.get().id));
        data->constraintGroup->addChild(record.get().node);
      }
      else
      {
        //error? didn't create osg::Node.
        data->cMap.records.pop_back();
        continue;
      }
    }
    //update constraints, even new ones.
    if (c.type == SLVS_C_HORIZONTAL || c.type == SLVS_C_VERTICAL || c.type == SLVS_C_AT_MIDPOINT)
    {
      osg::PositionAttitudeTransform *p = dynamic_cast<osg::PositionAttitudeTransform*>(record.get().node.get());
      assert(p);
      
      auto ls = solver.findEntity(solver.findConstraint(record.get().handle).get().entityA);
      assert(ls);
      osg::Vec3d point1 = convert(ls.get().point[0]);
      osg::Vec3d point2 = convert(ls.get().point[1]);
      osg::Vec3d proj = point2 - point1;
      if (proj.length() > std::numeric_limits<float>::epsilon())
        point1 += proj * 0.5;
      point1.z() = 0.002;
      p->setPosition(point1);
      double s = lastSize / 500.0;
      p->setScale(osg::Vec3d(s, s, s));
    }
    if
    (
      c.type == SLVS_C_POINTS_COINCIDENT
      || c.type == SLVS_C_PT_ON_LINE
      || c.type == SLVS_C_PT_ON_CIRCLE
      || c.type == SLVS_C_WHERE_DRAGGED
    )
    {
      osg::PositionAttitudeTransform *p = dynamic_cast<osg::PositionAttitudeTransform*>(record.get().node.get());
      assert(p);
      osg::Vec3d point1 = convert(solver.findConstraint(record.get().handle).get().ptA);
      point1.z() = 0.002;
      p->setPosition(point1);
      double s = lastSize / 500.0;
      p->setScale(osg::Vec3d(s, s, s));
    }
    if (c.type == SLVS_C_ARC_LINE_TANGENT)
    {
      osg::PositionAttitudeTransform *p = dynamic_cast<osg::PositionAttitudeTransform*>(record.get().node.get());
      assert(p);
      
      auto oc = solver.findConstraint(record.get().handle);
      assert(oc);
      auto oe = solver.findEntity(oc.get().entityA);
      assert(oe);
      
      osg::Vec3d point1 = convert(oe.get().point[1]);
      if (oc.get().other)
        point1 = convert(oe.get().point[2]);
      
      point1.z() = 0.002;
      p->setPosition(point1);
      double s = lastSize / 500.0;
      p->setScale(osg::Vec3d(s, s, s));
    }
    if (c.type == SLVS_C_PT_PT_DISTANCE)
    {
      osg::PositionAttitudeTransform *p = dynamic_cast<osg::PositionAttitudeTransform*>(record.get().node.get());
      assert(p);
      
      auto oc = solver.findConstraint(record.get().handle);
      assert(oc);
      osg::Vec3d point1 = convert(oc.get().ptA);
      osg::Vec3d point2 = convert(oc.get().ptB);
      
      osgText::Text *text = dynamic_cast<osgText::Text*>(p->getChild(0));
      assert(text);
      text->setText(std::to_string(oc.get().valA));
      
      point1 += (point2 - point1) * 0.5;
      point1.z() = 0.002;
      p->setPosition(point1);
      double s = lastSize / 1000.0;
      p->setScale(osg::Vec3d(s, s, s));
    }
    if (c.type == SLVS_C_PT_LINE_DISTANCE)
    {
      osg::PositionAttitudeTransform *p = dynamic_cast<osg::PositionAttitudeTransform*>(record.get().node.get());
      assert(p);
      auto oc = solver.findConstraint(record.get().handle);
      assert(oc);
      
      osg::Vec3d point = convert(oc.get().ptA);
      
      auto ols = solver.findEntity(oc.get().entityA);
      assert(ols);
      osg::Vec3d lp = convert(ols.get().point[0]);
      osg::Vec3d ld = convert(ols.get().point[1]) - lp;
      ld.normalize();
      osg::Vec3d pp = projectPointLine(lp, ld, point);
      osg::Vec3d mid = ((pp - point) * 0.5) + point;
      mid.z() = 0.002;
      
      osgText::Text *text = dynamic_cast<osgText::Text*>(p->getChild(0));
      assert(text);
      text->setText(std::to_string(oc.get().valA));
      
      p->setPosition(mid);
      double s = lastSize / 1000.0;
      p->setScale(osg::Vec3d(s, s, s));
    }
    if (c.type == SLVS_C_EQUAL_LENGTH_LINES || c.type == SLVS_C_EQUAL_RADIUS)
    {
      osg::Group *g = record.get().node->asGroup();
      assert(g);
      osg::PositionAttitudeTransform *t0 = g->getChild(0)->asTransform()->asPositionAttitudeTransform();
      assert(t0);
      osg::PositionAttitudeTransform *t1 = g->getChild(1)->asTransform()->asPositionAttitudeTransform();
      assert(t1);
      
      auto oc = solver.findConstraint(record.get().handle);
      assert(oc);
      
      double s = lastSize / 1000.0;
      osg::Vec3d scale(s, s, s);
      
      osg::Vec3d np0 = parameterPoint(oc.get().entityA, 0.9);
      np0.z() = 0.002;
      t0->setPosition(np0);
      t0->setScale(scale);
      
      osg::Vec3d np1 = parameterPoint(oc.get().entityB, 0.1);
      np1.z() = 0.002;
      t1->setPosition(np1);
      t1->setScale(scale);
    }
    if (c.type == SLVS_C_DIAMETER)
    {
      osg::PositionAttitudeTransform *p = dynamic_cast<osg::PositionAttitudeTransform*>(record.get().node.get());
      assert(p);
      
      double s = lastSize / 1000.0;
      osg::Vec3d scale(s, s, s);
      p->setScale(scale);
      
      auto oc = solver.findConstraint(record.get().handle);
      assert(oc);
      osgText::Text *text = dynamic_cast<osgText::Text*>(p->getChild(0));
      assert(text);
      text->setText(std::to_string(oc.get().valA));
      
      auto oe = solver.findEntity(oc.get().entityA);
      assert(oe);
      if (solver.isEntityType(oc.get().entityA, SLVS_E_CIRCLE))
      {
        osg::Vec3d projection(1.0, 0.0, 0.0);
        projection *= (oc.get().valA / 2.0);
        osg::Vec3d point = convert(oe.get().point[0]);
        point += projection;
        point.z() = 0.002;
        p->setPosition(point);
      }
      else if (solver.isEntityType(oc.get().entityA, SLVS_E_ARC_OF_CIRCLE))
      {
        osg::Vec3d pos = parameterPoint(oc.get().entityA, 0.5);
        pos.z() = 0.002;
        p->setPosition(pos);
      }
    }
    if (c.type == SLVS_C_SYMMETRIC_HORIZ || c.type == SLVS_C_SYMMETRIC_VERT || c.type == SLVS_C_SYMMETRIC_LINE)
    {
      osg::Group *g = record.get().node->asGroup();
      assert(g);
      osg::PositionAttitudeTransform *t0 = g->getChild(0)->asTransform()->asPositionAttitudeTransform();
      assert(t0);
      osg::PositionAttitudeTransform *t1 = g->getChild(1)->asTransform()->asPositionAttitudeTransform();
      assert(t1);
      
      auto oc = solver.findConstraint(record.get().handle);
      assert(oc);
      
      double s = lastSize / 1000.0;
      osg::Vec3d scale(s, s, s);
      
      osg::Vec3d np0 = convert(oc.get().ptA);
      np0.z() = 0.002;
      t0->setPosition(np0);
      t0->setScale(scale);
      
      osg::Vec3d np1 = convert(oc.get().ptB);
      np1.z() = 0.002;
      t1->setPosition(np1);
      t1->setScale(scale);
    }
    if (c.type == SLVS_C_ANGLE)
    {
      osg::PositionAttitudeTransform *p = dynamic_cast<osg::PositionAttitudeTransform*>(record.get().node.get());
      assert(p);
      
      double s = lastSize / 1000.0;
      osg::Vec3d scale(s, s, s);
      p->setScale(scale);
      
      auto oc = solver.findConstraint(record.get().handle);
      assert(oc);
      osgText::Text *text = dynamic_cast<osgText::Text*>(p->getChild(0));
      assert(text);
      text->setText(std::to_string(oc.get().valA));
      
      osg::Matrixd tf = osg::Matrixd::identity();
      bool results = data->transform->computeWorldToLocalMatrix(tf, nullptr);
      if (results)
      {
        std::vector<SSHandle> ehandles;
        ehandles.push_back(oc.get().entityA);
        ehandles.push_back(oc.get().entityB);
        osg::Vec3d np = boundingCenter(ehandles) * tf;
        p->setPosition(np);
      }
      else
        std::cout << "couldn't calculate matrix for SLVS_C_ANGLE in: " << BOOST_CURRENT_FUNCTION << std::endl;
    }
    if (c.type == SLVS_C_EQUAL_ANGLE)
    {
      //another hack.
      osg::Group *g = record.get().node->asGroup();
      assert(g);
      osg::PositionAttitudeTransform *t0 = g->getChild(0)->asTransform()->asPositionAttitudeTransform();
      assert(t0);
      osg::PositionAttitudeTransform *t1 = g->getChild(1)->asTransform()->asPositionAttitudeTransform();
      assert(t1);
      
      auto oc = solver.findConstraint(record.get().handle);
      assert(oc);
      
      double s = lastSize / 1000.0;
      osg::Vec3d scale(s, s, s);
      t0->setScale(scale);
      t1->setScale(scale);
      
      osg::Matrixd tf = osg::Matrixd::identity();
      bool results = data->transform->computeWorldToLocalMatrix(tf, nullptr);
      if (results)
      {
        {
          std::vector<SSHandle> ehandles;
          ehandles.push_back(oc.get().entityA);
          ehandles.push_back(oc.get().entityB);
          t0->setPosition(boundingCenter(ehandles) * tf);
        }
        {
          std::vector<SSHandle> ehandles;
          ehandles.push_back(oc.get().entityC);
          ehandles.push_back(oc.get().entityD);
          t1->setPosition(boundingCenter(ehandles) * tf);
        }
      }
      else
        std::cout << "couldn't calculate matrix for SLVS_C_EQUAL_ANGLE, in: " << BOOST_CURRENT_FUNCTION << std::endl;
    }
    
    if (c.type == SLVS_C_PARALLEL || c.type == SLVS_C_PERPENDICULAR)
    {
      //another hack.
      osg::Group *g = record.get().node->asGroup();
      assert(g);
      osg::PositionAttitudeTransform *t0 = g->getChild(0)->asTransform()->asPositionAttitudeTransform();
      assert(t0);
      osg::PositionAttitudeTransform *t1 = g->getChild(1)->asTransform()->asPositionAttitudeTransform();
      assert(t1);
      
      auto oc = solver.findConstraint(record.get().handle);
      assert(oc);
      
      double s = lastSize / 1000.0;
      osg::Vec3d scale(s, s, s);
      
      osg::Vec3d np0 = parameterPoint(oc.get().entityA, 0.8);
      np0.z() = 0.002;
      t0->setPosition(np0);
      t0->setScale(scale);
      
      osg::Vec3d np1 = parameterPoint(oc.get().entityB, 0.2);
      np1.z() = 0.002;
      t1->setPosition(np1);
      t1->setScale(scale);
    }
    
    record.get().referenced = true;
  }
  //clean up unreferenced.
  for (const auto &c : data->cMap.records)
  {
    if (!c.referenced)
      data->constraintGroup->removeChild(c.node);
  }
  data->cMap.removeUnreferenced();
}

//! @brief show the visual sketch plane.
void Visual::showPlane()
{
  data->theSwitch->setChildValue(data->planeGroup.get(), true);
}

//! @brief hide the visual sketch plane.
void Visual::hidePlane()
{
  data->theSwitch->setChildValue(data->planeGroup.get(), false);
}

//! @brief show all entities.
void Visual::showEntity()
{
  data->theSwitch->setChildValue(data->entityGroup.get(), true);
}

//! @brief hide all entities.
void Visual::hideEntity()
{
  data->theSwitch->setChildValue(data->entityGroup.get(), false);
}

//! @brief show all constraints.
void Visual::showConstraint()
{
  data->theSwitch->setChildValue(data->constraintGroup.get(), true);
}

//! @brief hide all constraints.
void Visual::hideConstraint()
{
  data->theSwitch->setChildValue(data->constraintGroup.get(), false);
}

//! @brief show all entities and constraints.
void Visual::showAll()
{
  showPlane();
  showEntity();
  showConstraint();
}

//! @brief hide all entities and constraints.
void Visual::hideAll()
{
  hidePlane();
  hideEntity();
  hideConstraint();
}

//! @brief sets the node mask for active sketch.
void Visual::setActiveSketch()
{
  data->transform->setNodeMask(data->transform->getNodeMask() | ActiveSketch.to_ulong());
}

//! @brief clears the node mask for active sketch.
void Visual::clearActiveSketch()
{
  data->transform->setNodeMask(data->transform->getNodeMask() & ~ActiveSketch.to_ulong());
}

//! @brief Response to mouse move with a polytope intersector
void Visual::move(const osgUtil::PolytopeIntersector::Intersections &is)
{
  /* sketches have all geometry on top of each other, so the
    * sorting by distance that is done by osg intersector is really
    * worthless here. So we will sort to our preference. The intersector
    * struct is pretty substantial, so we will get crazy with some
    * ref wrappers. Be aware of lifetimes.
    */
  
  typedef std::vector<std::reference_wrapper<const osgUtil::PolytopeIntersector::Intersection>> IntersectionRefs;
  IntersectionRefs workPlane;
  IntersectionRefs workXAxis;
  IntersectionRefs workYAxis;
  IntersectionRefs workOrigin;
  IntersectionRefs points;
  IntersectionRefs curves;
  IntersectionRefs constraints;
  for (const auto &i : is)
  {
    if (std::find(data->highlights.begin(), data->highlights.end(), i.drawable) != data->highlights.end())
      continue;
    
    osg::Geometry *tg = dynamic_cast<osg::Geometry*>(i.drawable.get());
    if (tg)
    {
      if (tg->getName().empty())
        continue;
      if(tg->getName() == "selectionPlane")
      {
        continue;
      }
      if(tg->getName() == "planeQuad")
      {
        workPlane.push_back(i);
        continue;
      }
      else if (tg->getName() == "xAxis")
      {
        workXAxis.push_back(i);
        continue;
      }
      else if (tg->getName() == "yAxis")
      {
        workYAxis.push_back(i);
        continue;
      }
      else if (tg->getName() == "origin")
      {
        workOrigin.push_back(i);
        continue;
      }
      else if (tg->getName() == "planeLines")
        continue;
      
      uuid id = sGen(tg->getName());
      if (id.is_nil())
        continue;
      
      auto er = data->eMap.getRecord(id);
      if (er)
      {
        auto e = solver.findEntity(er.get().handle);
        assert(e);
        if (!e)
          continue;
        if (e.get().type == SLVS_E_POINT_IN_2D)
        {
          points.push_back(i);
          continue;
        }
        if
        (
          e.get().type == SLVS_E_LINE_SEGMENT
          || e.get().type == SLVS_E_ARC_OF_CIRCLE
          || e.get().type == SLVS_E_CIRCLE
        )
        {
          curves.push_back(i);
          continue;
        }
      }
    }
    
    //add constraints.
    osgText::Text *t = dynamic_cast<osgText::Text*>(i.drawable.get());
    if (t)
    {
      auto cr = data->cMap.getRecord(t->getParent(0));
      if (!cr) //equality, for example, has 2 texts with 2 transforms for parents then group. so try parent of parent.
        cr = data->cMap.getRecord(t->getParent(0)->getParent(0));
      if (cr) //make sure we have constraint text
      {
        constraints.push_back(i);
        continue;
      }
    }
  }
  
  IntersectionRefs all;
  std::copy(points.begin(), points.end(), std::back_inserter(all));
  std::copy(curves.begin(), curves.end(), std::back_inserter(all));
  std::copy(constraints.begin(), constraints.end(), std::back_inserter(all));
  std::copy(workOrigin.begin(), workOrigin.end(), std::back_inserter(all));
  std::copy(workXAxis.begin(), workXAxis.end(), std::back_inserter(all));
  std::copy(workYAxis.begin(), workYAxis.end(), std::back_inserter(all));
  std::copy(workPlane.begin(), workPlane.end(), std::back_inserter(all));
  
  if (!all.empty())
  {
    if (data->preHighlight != all.front().get().drawable.get())
    {
      clearPreHighlight();
      data->preHighlight = all.front().get().drawable.get();
      goPreHighlight();
    }
  }
  else
  {
    clearPreHighlight();
  }
}

//! @brief Response to mouse move with a line segment intersector
void Visual::move(const osgUtil::LineSegmentIntersector::Intersections &is)
{
  if (data->state == State::line)
  {
    if (!data->dynamic) //haven't picked first point yet.
      return;
    boost::optional<osg::Vec3d> point = getPlanePoint(is);
    if (!point)
      return;
    
    osg::Geometry *l = data->dynamic->asGeometry();
    osg::Vec3Array *v = dynamic_cast<osg::Vec3Array*>(l->getVertexArray());
    assert(v);
    (*v)[1] = point.get();
    v->dirty();
    
    return;
  }
  else if (data->state == State::arc)
  {
    if (!data->dynamic) //haven't picked first point yet.
      return;
    boost::optional<osg::Vec3d> point = getPlanePoint(is);
    if (!point)
      return;
    
    osg::Geometry *a = data->dynamic->asGeometry();
    osg::Vec3d as = data->previousPoint.get();
    osg::Vec3d af = point.get();
    osg::Vec3d ac = ((af - as) * 0.5) + as;
    updateArcGeometry(a, ac, as, af);
  }
  else if (data->state == State::circle)
  {
    if (!data->dynamic) //haven't picked first point yet.
      return;
    boost::optional<osg::Vec3d> point = getPlanePoint(is);
    if (!point)
      return;
    
    osg::Geometry *a = data->dynamic->asGeometry();
    osg::Vec3d cc = data->previousPoint.get();
    osg::Vec3d poc = point.get();
    updateArcGeometry(a, cc, poc, poc);
  }
}

//! @brief Response to mouse pick with a polytope intersector
void Visual::pick(const osgUtil::PolytopeIntersector::Intersections&)
{
  if (data->state == State::selection)
  {
    if (!data->preHighlight.valid())
      return;
    
    data->highlights.push_back(data->preHighlight);
    data->preHighlight.release();
    goHighlight(data->highlights.back().get());
  }
}

//! @brief Response to mouse pick with a line segment intersector
void Visual::pick(const osgUtil::LineSegmentIntersector::Intersections &is)
{
  if (is.empty())
    return;
  
  auto getPointHandle = [&](const osg::ref_ptr<osg::Drawable> &d) -> boost::optional<SSHandle>
  {
    assert(d.valid()); //don't set me up!
    auto record = data->eMap.getRecord(d.get());
    if (!record)
      return boost::none;
    if (!solver.isEntityType(record.get().handle, SLVS_E_POINT_IN_2D))
      return boost::none;
    return record.get().handle;
  };
  
  if (data->state == State::point)
  {
    boost::optional<osg::Vec3d> point = getPlanePoint(is);
    if (!point)
    {
      std::cout << "couldn't find pick point on selectionPlane" << std::endl;
      return;
    }
    solver.addPoint2d(point.get().x(), point.get().y());
    data->state = State::selection;
    solver.solve(solver.getGroup(), true);
    update();
    return;
  }
  else if (data->state == State::line)
  {
    boost::optional<osg::Vec3d> point = getPlanePoint(is);
    if (!point)
    {
      std::cout << "couldn't find pick point on selectionPlane" << std::endl;
      return;
    }
    if (!data->previousPoint)
    {
      data->previousPoint = point.get();
      if (data->preHighlight.valid())
        data->previousPick = data->preHighlight;
      data->dynamic = buildLine();
      osg::Geometry *l = data->dynamic->asGeometry();
      osg::Vec3Array *v = dynamic_cast<osg::Vec3Array*>(l->getVertexArray());
      assert(v);
      (*v)[0] = point.get();
      (*v)[1] = point.get();
      v->dirty();
      data->transform->addChild(data->dynamic);
      return;
    }
    else
    {
      if ((point.get() - data->previousPoint.get()).length() < std::numeric_limits<float>::epsilon())
      {
        std::cout << "points to close, line not created" << std::endl;
        return;
      }
      
      SSHandle p1 = solver.addPoint2d(data->previousPoint.get().x(), data->previousPoint.get().y());
      SSHandle p2 = solver.addPoint2d(point.get().x(), point.get().y());
      solver.addLineSegment(p1, p2);
      if (data->previousPick.valid())
      {
        auto pp = getPointHandle(data->previousPick);
        if (pp)
          solver.addPointsCoincident(p1, pp.get());
      }
      if (data->preHighlight.valid())
      {
        auto cp = getPointHandle(data->preHighlight);
        if (cp)
          solver.addPointsCoincident(p2, cp.get());
      }
      
      data->state = State::selection;
      data->previousPoint = boost::none;
      data->previousPick.release();
      data->transform->removeChild(data->dynamic);
      data->dynamic.release();
      solver.solve(solver.getGroup(), true);
      update();
      return;
    }
  }
  else if (data->state == State::arc)
  {
    boost::optional<osg::Vec3d> point = getPlanePoint(is);
    if (!point)
    {
      std::cout << "couldn't find pick point on selectionPlane" << std::endl;
      return;
    }
    
    if (!data->previousPoint)
    {
      data->previousPoint = point.get();
      if (data->preHighlight.valid())
        data->previousPick = data->preHighlight;
      data->dynamic = buildArc();
      osg::Geometry *a = data->dynamic->asGeometry();
      osg::Vec3Array *v = dynamic_cast<osg::Vec3Array*>(a->getVertexArray());
      assert(v);
      (*v)[0] = point.get();
      (*v)[1] = point.get();
      v->dirty();
      data->transform->addChild(data->dynamic);
      return;
    }
    else
    {
      if ((point.get() - data->previousPoint.get()).length() < std::numeric_limits<float>::epsilon())
      {
        std::cout << "points to close, arc not created" << std::endl;
        return;
      }
      
      osg::Vec3d ac = ((point.get() - data->previousPoint.get()) * 0.5) + data->previousPoint.get();
      SSHandle ach = solver.addPoint2d(ac.x(), ac.y());
      SSHandle ash = solver.addPoint2d(data->previousPoint.get().x(), data->previousPoint.get().y());
      SSHandle afh = solver.addPoint2d(point.get().x(), point.get().y());
      solver.addArcOfCircle(solver.getWPNormal3d(), ach, ash, afh);
      
      if (data->previousPick.valid())
      {
        auto pp = getPointHandle(data->previousPick);
        if (pp)
          solver.addPointsCoincident(ash, pp.get());
      }
      if (data->preHighlight.valid())
      {
        auto cp = getPointHandle(data->preHighlight);
        if (cp)
          solver.addPointsCoincident(afh, cp.get());
      }
      
      data->state = State::selection;
      data->previousPoint = boost::none;
      data->previousPick.release();
      data->transform->removeChild(data->dynamic);
      data->dynamic.release();
      solver.solve(solver.getGroup(), true);
      update();
      return;
    }
  }
  else if (data->state == State::circle)
  {
    boost::optional<osg::Vec3d> point = getPlanePoint(is);
    if (!point)
    {
      std::cout << "couldn't find pick point on selectionPlane" << std::endl;
      return;
    }
    
    if (!data->previousPoint)
    {
      data->previousPoint = point.get();
      if (data->preHighlight.valid())
        data->previousPick = data->preHighlight;
      data->dynamic = buildCircle();
      osg::Geometry *a = data->dynamic->asGeometry();
      osg::Vec3Array *v = dynamic_cast<osg::Vec3Array*>(a->getVertexArray());
      assert(v);
      (*v)[0] = point.get();
      (*v)[1] = point.get();
      v->dirty();
      data->transform->addChild(data->dynamic);
      return;
    }
    else
    {
      double distance = (point.get() - data->previousPoint.get()).length();
      if (distance < std::numeric_limits<float>::epsilon())
      {
        std::cout << "points to close, arc not created" << std::endl;
        return;
      }
      SSHandle radius = solver.addDistance(distance);
      SSHandle cc = solver.addPoint2d(data->previousPoint.get().x(), data->previousPoint.get().y());
      solver.addCircle(cc, radius);
      
      if (data->previousPick.valid())
      {
        auto pp = getPointHandle(data->previousPick);
        if (pp)
          solver.addPointsCoincident(cc, pp.get());
      }
      //maybe point on circle?
//       if (data->preHighlight.valid())
//       {
//         auto cp = getPointHandle(data->preHighlight);
//         if (cp)
//           solver.addPointsCoincident(afh, cp.get());
//       }
      
      data->state = State::selection;
      data->previousPoint = boost::none;
      data->previousPick.release();
      data->transform->removeChild(data->dynamic);
      data->dynamic.release();
      solver.solve(solver.getGroup(), true);
      update();
      return;
    }
  }
}

//! @brief Response to starting of a mouse drag.
void Visual::startDrag(const osgUtil::LineSegmentIntersector::Intersections &is)
{
  if (data->preHighlight.valid())
  {
    auto op = getPlanePoint(is);
    if (!op)
      return;
    data->previousPick = data->preHighlight;
    data->previousPoint = op.get();
    data->state = State::drag;
  }
}

/*! @brief Response to a mouse drag.
 * 
 * @note I did some profiling to see if I can figure out why 
 * updating feels sluggish when dragging. The time to run solver
 * was similar to freecad, solvespace doesn't show time to solve by default.
 * The time to visual update looked insignificant. 
 * I tried different threading models for the viewer. didn't help.
 * I am convinced that the solver is working efficiently and the 
 * event system queue is being flooded. I tried clearing the osg event
 * queue, but that didn't help. Buffering must happen above. Perhaps
 * X11 for the prototype.
 */
void Visual::drag(const osgUtil::LineSegmentIntersector::Intersections &is)
{
//   std::cout << BOOST_CURRENT_FUNCTION << std::endl;
  assert(data->state == State::drag);
  assert(data->previousPick.valid());
  auto op = getPlanePoint(is);
  if (!op)
    return;
  osg::Vec3d projection = op.get() - data->previousPoint.get();
  if (projection.length() < std::numeric_limits<float>::epsilon())
    return;
  auto record = data->eMap.getRecord(data->previousPick);
  if (!record)
    return;
  SSHandle ph = record.get().handle;
  std::vector<Slvs_hParam> draggedParameters;
  auto projectPoint = [&](Slvs_hParam x, Slvs_hParam y)
  {
    osg::Vec3d cp(solver.getParameterValue(x).get(), solver.getParameterValue(y).get(), 0.0);
    cp += projection;
    solver.setParameterValue(x, cp.x());
    solver.setParameterValue(y, cp.y());
    draggedParameters.push_back(x);
    draggedParameters.push_back(y);
  };
  if (solver.isEntityType(ph, SLVS_E_POINT_IN_2D))
  {
    auto oe = solver.findEntity(ph);
    assert(oe);
    projectPoint(oe.get().param[0], oe.get().param[1]);
  }
  if (solver.isEntityType(ph, SLVS_E_LINE_SEGMENT))
  {
    auto ols = solver.findEntity(ph);
    assert(ols);
    auto p1 = solver.findEntity(ols.get().point[0]);
    assert(p1);
    projectPoint(p1.get().param[0], p1.get().param[1]);
    auto p2 = solver.findEntity(ols.get().point[1]);
    assert(p2);
    projectPoint(p2.get().param[0], p2.get().param[1]);
  }
  if (solver.isEntityType(ph, SLVS_E_ARC_OF_CIRCLE))
  {
    auto oac = solver.findEntity(ph);
    assert(oac);
    auto p1 = solver.findEntity(oac.get().point[1]);
    assert(p1);
    projectPoint(p1.get().param[0], p1.get().param[1]);
    auto p2 = solver.findEntity(oac.get().point[2]);
    assert(p2);
    projectPoint(p2.get().param[0], p2.get().param[1]);
  }
  if (solver.isEntityType(ph, SLVS_E_CIRCLE))
  {
    auto oc = solver.findEntity(ph);
    assert(oc);
    
    osg::Vec3d center = convert(oc.get().point[0]);
    osg::Vec3d rv = op.get() - center; //radius vector
    rv.normalize();
    osg::Vec3d pp = projectPointLine(center, rv, data->previousPoint.get());
    osg::Vec3d prv = op.get() - pp; // projected radius vector
    double adjustment = prv.length();
    prv.normalize();
    if ((prv - rv).length() > std::numeric_limits<float>::epsilon())
      adjustment *= -1.0;
    
    auto od = solver.findEntity(oc.get().distance);
    assert(od);
    auto opv = solver.getParameterValue(od.get().param[0]);
    solver.setParameterValue(od.get().param[0], opv.get() + adjustment);
    draggedParameters.push_back(od.get().param[0]);
  }
  data->previousPoint = op.get();
  solver.dragSet(draggedParameters);
  solver.solve(solver.getGroup(), true);
  update();
}

//! @brief Response to ending of a mouse drag.
void Visual::finishDrag(const osgUtil::LineSegmentIntersector::Intersections&)
{
  data->state = State::selection;
  data->previousPoint = boost::none;
  data->previousPick.release();
}

//! @brief Clear the current selection.
void Visual::clearSelection()
{
  for (auto d : data->highlights)
    clearHighlight(d.get());
  
  data->highlights.clear();
}

//! @brief Prehighlight the previously set preHighlight data member.
void Visual::goPreHighlight()
{
  //data prehighlight already set.
  if (data->preHighlight.valid())
  {
    osg::Geometry *e = dynamic_cast<osg::Geometry*>(data->preHighlight.get());
    if (e)
    {
      setPointBig(e);
      osg::Vec4Array *cs = dynamic_cast<osg::Vec4Array*>(e->getColorArray());
      assert(cs);
      assert(cs->size() == 1);
      osg::Vec4 c = data->preHighlightColor;
      c.w() = (*cs)[0].w();
      (*cs)[0] = c;
      cs->dirty();
    }
    osgText::Text *t = dynamic_cast<osgText::Text*>(data->preHighlight.get());
    if (t)
      t->setColor(data->preHighlightColor);
  }
}

//! @brief Clear the preHighlight from the preHighlight data member.
void Visual::clearPreHighlight()
{
  if (data->preHighlight.valid())
  {
    osg::Geometry *e = dynamic_cast<osg::Geometry*>(data->preHighlight.get());
    if (e)
    {
      setPointSmall(e);
      osg::Vec4Array *cs = dynamic_cast<osg::Vec4Array*>(e->getColorArray());
      assert(cs);
      assert(cs->size() == 1);
      if (e->getName() == "planeQuad")
        (*cs)[0] = osg::Vec4(.4, .7, .75, .5);
      else if (e->getName() == "xAxis")
        (*cs)[0] = osg::Vec4(1.0, 0.0, 0.0, 1.0);
      else if (e->getName() == "yAxis")
        (*cs)[0] = osg::Vec4(0.0, 1.0, 0.0, 1.0);
      else if (e->getName() == "origin")
        (*cs)[0] = osg::Vec4(0.0, 0.0, 1.0, 1.0);
      else
      {
        auto r = data->eMap.getRecord(e);
        assert(r);
        if (r.get().construction)
          (*cs)[0] = data->constructionColor;
        else
          (*cs)[0] = data->entityColor;
      }
      cs->dirty();
    }
    osgText::Text *t = dynamic_cast<osgText::Text*>(data->preHighlight.get());
    if (t)
      t->setColor(osg::Vec4(1.0, 0.0, 0.0, 1.0));
    data->preHighlight.release();
  }
}

/*! @brief Highlight the openscenegraph drawable
 * 
 * @param dIn The drawable to highlight.
 */
void Visual::goHighlight(osg::Drawable *dIn)
{
  osg::Geometry *e = dynamic_cast<osg::Geometry*>(dIn);
  if (e)
  {
    setPointBig(e);
    osg::Vec4Array *cs = dynamic_cast<osg::Vec4Array*>(e->getColorArray());
    assert(cs);
    assert(cs->size() == 1);
    osg::Vec4 c = data->highlightColor;
    c.w() = (*cs)[0].w();
    (*cs)[0] = c;
    cs->dirty();
  }
  osgText::Text *t = dynamic_cast<osgText::Text*>(dIn);
  if (t)
    t->setColor(data->highlightColor);
}

/*! @brief Remove the highlight from the openscenegraph drawable
 * 
 * @param dIn The drawable to remove the highlight.
 */
void Visual::clearHighlight(osg::Drawable *dIn)
{
  osg::Geometry *e = dynamic_cast<osg::Geometry*>(dIn);
  if (e)
  {
    setPointSmall(e);
    osg::Vec4Array *cs = dynamic_cast<osg::Vec4Array*>(e->getColorArray());
    assert(cs);
    assert(cs->size() == 1);
    if (e->getName() == "planeQuad")
      (*cs)[0] = osg::Vec4(.4, .7, .75, .5);
    else if (e->getName() == "xAxis")
      (*cs)[0] = osg::Vec4(1.0, 0.0, 0.0, 1.0);
    else if (e->getName() == "yAxis")
      (*cs)[0] = osg::Vec4(0.0, 1.0, 0.0, 1.0);
    else if (e->getName() == "origin")
      (*cs)[0] = osg::Vec4(0.0, 0.0, 1.0, 1.0);
    else
    {
      auto r = data->eMap.getRecord(e);
      assert(r);
      if (r.get().construction)
        (*cs)[0] = data->constructionColor;
      else
        (*cs)[0] = data->entityColor;
    }
    cs->dirty();
  }
  osgText::Text *t = dynamic_cast<osgText::Text*>(dIn);
  if (t)
    t->setColor(osg::Vec4(1.0, 0.0, 0.0, 1.0));
}

/*! @brief Set the opengl point size to 10.0.
 * 
 * @param g Object to modify
 * @note Helps to see selection with coincident points.
 */
void Visual::setPointBig(osg::Geometry *g)
{
  osg::Point *p = dynamic_cast<osg::Point*>(g->getOrCreateStateSet()->getAttribute(osg::StateAttribute::Type::POINT));
  if (p)
    p->setSize(10.0);
}

/*! @brief Set the opengl point size to 5.0.
 * 
 * @param g Object to modify
 * @note Helps to see selection with coincident points.
 */
void Visual::setPointSmall(osg::Geometry *g)
{
  osg::Point *p = dynamic_cast<osg::Point*>(g->getOrCreateStateSet()->getAttribute(osg::StateAttribute::Type::POINT));
  if (p)
    p->setSize(5.0);
}

/*! @brief Set the color for preHighlighting.
 * 
 * @param cIn New color for preHighlight.
 */
void Visual::setPreHighlightColor(const osg::Vec4 &cIn)
{
  data->preHighlightColor = cIn;
}

/*! @brief Get the color for preHighlighting.
 * 
 * @return The color for preHighlight.
 */
osg::Vec4 Visual::getPreHighlightColor()
{
  return data->preHighlightColor;
}

/*! @brief Set the color for highlighting.
 * 
 * @param cIn New color for highlight.
 */
void Visual::setHighlightColor(const osg::Vec4 &cIn)
{
  data->highlightColor = cIn;
}

/*! @brief Get the color for highlighting.
 * 
 * @return The color for highlight.
 */
osg::Vec4 Visual::getHighlightColor()
{
  return data->highlightColor;
}

//! @brief Set state to add a point through a pick.
void Visual::addPoint()
{
  clearSelection();
  data->state = State::point;
  data->previousPoint = boost::none;
  data->previousPick.release();
  data->dynamic.release();
  
  data->statusText->setText("Point");
}

//! @brief Set state to add a line through picks.
void Visual::addLine()
{
  clearSelection();
  data->state = State::line;
  data->previousPoint = boost::none;
  data->previousPick.release();
  data->dynamic.release();
  
  data->statusText->setText("Line");
}

//! @brief Set state to add an arc through picks.
void Visual::addArc()
{
  clearSelection();
  data->state = State::arc;
  data->previousPoint = boost::none;
  data->previousPick.release();
  data->dynamic.release();
  
  data->statusText->setText("Arc");
}

//! @brief Set state to add a circle through picks.
void Visual::addCircle()
{
  clearSelection();
  data->state = State::circle;
  data->previousPoint = boost::none;
  data->previousPick.release();
  data->dynamic.release();
  
  data->statusText->setText("Circle");
}

//! @brief Add a 'points coincident' constraint from 2 currently entities, if possible.
void Visual::addCoincident()
{
  if (data->highlights.size() != 2)
  {
    std::cout << "incorrect selection count for coincident" << std::endl;
    return;
  }
  
  std::vector<SSHandle> points = getSelectedPoints();
  std::vector<SSHandle> lines = getSelectedLines();
  std::vector<SSHandle> circles = getSelectedCircles();
  
  if (points.size() == 2)
    solver.addPointsCoincident(points.front(), points.back());
  else if (points.size() == 1 && lines.size() == 1)
    solver.addPointOnLine(points.front(), lines.front());
  else if (points.size() == 1 && circles.size() == 1)
    solver.addPointOnCircle(points.front(), circles.front());
  else
  {
    std::cout << "unsupported combination for coincident constraint" << std::endl;
    return;
  }
  
  solver.solve(solver.getGroup(), true);
  update();
  clearSelection();
}

//! @brief Add a horizontal constraint to the 1 currently selected line segment.
void Visual::addHorizontal()
{
  std::vector<SSHandle> lines = getSelectedLines(false);
  for (const auto &l : lines)
    solver.addHorizontal(l);
  solver.solve(solver.getGroup(), true);
  update();
  clearSelection();
}

//! @brief Add a vertical constraint to the 1 currently selected line segment.
void Visual::addVertical()
{
  std::vector<SSHandle> lines = getSelectedLines(false);
  for (const auto &l : lines)
    solver.addVertical(l);
  solver.solve(solver.getGroup(), true);
  update();
  clearSelection();
}

//! @brief Add a tangent constraint to the 2 currently selected objects.
void Visual::addTangent()
{
  if (data->highlights.size() != 2)
  {
    std::cout << "wrong selection count for tangent" << std::endl;
    return;
  }
  std::vector<SSHandle> lines;
  std::vector<SSHandle> arcs;
  for (const auto &h : data->highlights)
  {
    auto e = data->eMap.getRecord(h);
    if (!e)
      continue;
    if (solver.isEntityType(e.get().handle, SLVS_E_LINE_SEGMENT))
      lines.push_back(e.get().handle);
    if (solver.isEntityType(e.get().handle, SLVS_E_ARC_OF_CIRCLE))
      arcs.push_back(e.get().handle);
  }
  if (lines.size() == 1 && arcs.size() == 1)
  {
    SSHandle tc = solver.addArcLineTangent(arcs.front(), lines.front());
    if (tc != 0)
    {
      solver.solve(solver.getGroup(), true);
      update();
      clearSelection();
    }
    // else warning about no common points coincidently constrained.
    return;
  }
}

//! @brief Add a distance constraint to the currently selected objects.
void Visual::addDistance()
{
  std::vector<SSHandle> points = getSelectedPoints();
  std::vector<SSHandle> lines = getSelectedLines();
  
  if (data->highlights.size() == 2 && points.size() == 2)
  {
    osg::Vec3d p1 = convert(points.front());
    osg::Vec3d p2 = convert(points.back());
    double length = (p2 - p1).length();
    solver.addPointPointDistance(length, points.front(), points.back());
  }
  else if (data->highlights.size() == 1 && lines.size() == 1)
  {
    auto ols = solver.findEntity(lines.front());
    assert(ols);
    
    osg::Vec3d p1 = convert(ols.get().point[0]);
    osg::Vec3d p2 = convert(ols.get().point[1]);
    double length = (p2 - p1).length();
    solver.addPointPointDistance(length, ols.get().point[0], ols.get().point[1]);
  }
  else if (data->highlights.size() == 2 && lines.size() == 1 && points.size() == 1)
  {
    osg::Vec3d p = convert(points.front());
    auto ols = solver.findEntity(lines.front());
    assert(ols);
    osg::Vec3d lp = convert(ols.get().point[0]);
    osg::Vec3d ld = convert(ols.get().point[1]) - lp;
    ld.normalize();
    double length = distancePointLine(lp, ld, p);
    solver.addPointLineDistance(length, points.front(), lines.front());
  }
  else
  {
    std::cout << "unsupported combination for distance" << std::endl;
    return;
  }
  solver.solve(solver.getGroup(), true);
  update();
  clearSelection();
}

//! @brief Add a equality constraint to the currently selected objects.
void Visual::addEqual()
{
  std::vector<SSHandle> lines = getSelectedLines(false);
  std::vector<SSHandle> arcs = getSelectedCircles();
  if (data->highlights.size() == lines.size() && lines.size() > 1)
  {
    for (std::size_t i = 0; i < lines.size() - 1; ++i)
      solver.addEqualLengthLines(lines.at(i), lines.at(i + 1));
  }
  else if (data->highlights.size() == arcs.size() && arcs.size() > 1)
  {
    for (std::size_t i = 0; i < arcs.size() - 1; ++i)
      solver.addEqualRadius(arcs.at(i), arcs.at(i + 1));
  }
  else
  {
    std::cout << "unsupported selection for equality" << std::endl;
    return;
  }
  solver.solve(solver.getGroup(), true);
  update();
  clearSelection();
}

/*! @brief Add a equal angle constraint to the currently selected objects.
 * 
 * @details The user needs to keep selection in a counter clockwise
 * to get desired results. Same as add angle.
 * 
 * @note this is done separate from addEqual because we want addEqual
 * to be able to apply equal length constraints to multiple line segments.
 * EqualAngle can take 3 or 4 segments, so there is no way to
 * differentiate between equal angle or multiple equal lengths from selection.
 */
void Visual::addEqualAngle()
{
  std::vector<SSHandle> lines = getSelectedLines();
  if (data->highlights.size() != lines.size())
  {
    std::cout << "wrong types selected. need just lines." << std::endl;
    return;
  }
  if (lines.size() == 3)
    solver.addEqualAngle(lines.at(0), lines.at(1), lines.at(1), lines.at(2));
  else if (lines.size() == 4)
    solver.addEqualAngle(lines.at(0), lines.at(1), lines.at(2), lines.at(3));
  else
  {
    std::cout << "invalid selection for adding equal angles." << std::endl;
    return;
  }
  solver.solve(solver.getGroup(), true);
  update();
  clearSelection();
}

//! @brief Add a diameter constraint to the currently selected objects.
void Visual::addDiameter()
{
  std::vector<SSHandle> circles = getSelectedCircles();
  if (circles.empty())
    return;
  for (const auto &c : circles)
  {
    auto oe = solver.findEntity(c);
    assert(oe);
    boost::optional<double> radius;
    if (solver.isEntityType(c, SLVS_E_CIRCLE))
    {
      //circles have a handle to a distance entity.
      auto od = solver.findEntity(oe.get().distance);
      assert(od);
      radius = solver.getParameterValue(od.get().param[0]).get();
    }
    else if (solver.isEntityType(c, SLVS_E_ARC_OF_CIRCLE))
    {
      osg::Vec3d ac = convert(oe.get().point[0]);
      osg::Vec3d as = convert(oe.get().point[1]);
      radius = (as - ac).length();
    }
    if (radius && (radius.get() > std::numeric_limits<float>::epsilon()))
      solver.addDiameter(radius.get() * 2.0, c);
  }
  solver.solve(solver.getGroup(), true);
  update();
  clearSelection();
}

//! @brief Add a symmetric constraint to the currently selected objects.
void Visual::addSymmetric()
{
  std::vector<SSHandle> lines = getSelectedLines();
  std::vector<SSHandle> notAxes;
  std::vector<SSHandle> points = getSelectedPoints(false);
  boost::optional<SSHandle> xAxis;
  boost::optional<SSHandle> yAxis;
  for (const auto &l : lines)
  {
    if (l == solver.getXAxis())
      xAxis = l;
    else if (l == solver.getYAxis())
      yAxis = l;
    else
      notAxes.push_back(l);
  }
  
  boost::optional<SSHandle> nc; //new constraint.
  
  if (data->highlights.size() == 2 && lines.size() == 2 && notAxes.size() == 1 && xAxis)
    nc = solver.addSymmetricVertical(notAxes.front());
  else if (data->highlights.size() == 2 && lines.size() == 2 && notAxes.size() == 1 && yAxis)
    nc = solver.addSymmetricHorizontal(notAxes.front());
  else if (data->highlights.size() == 3 && points.size() == 2 && xAxis)
    nc = solver.addSymmetricVertical(points.front(), points.back());
  else if (data->highlights.size() == 3 && points.size() == 2 && yAxis)
    nc = solver.addSymmetricHorizontal(points.front(), points.back());
  else if (data->highlights.size() == 2 && notAxes.size() == 2)
    nc = solver.addSymmetricLine(notAxes.front(), notAxes.back());
  else if (data->highlights.size() == 3 && notAxes.size() == 1 && points.size() == 2)
    nc = solver.addSymmetricLine(points.front(), points.back(), notAxes.front());
  
  if (nc)
  {
    solver.solve(solver.getGroup(), true);
    update();
    clearSelection();
  }
  else
    std::cout << "couldn't construct new symmetric constraint from selection" << std::endl;
}

//! @brief Add angle constraint to the currently selected objects.
void Visual::addAngle()
{
  std::vector<SSHandle> lines = getSelectedLines();
  if (data->highlights.size() == 2 && lines.size() == 2)
  {
    osg::Vec3d l0p0, l0p1, l1p0, l1p1;
    std::tie(l0p0, l0p1) = endPoints(lines.front());
    std::tie(l1p0, l1p1) = endPoints(lines.back());
    
    //initial to closest tails
    osg::Vec3d cv0 = l0p1 - l0p0; // current vectors
    osg::Vec3d cv1 = l1p1 - l1p0; // current vectors
    double distance = (l1p0 - l0p0).length();
    bool reversedSense = false;
    
    //try closest heads
    if ((l1p1 - l0p1).length() < distance)
    {
      cv0 = l0p0 - l0p1; //reverse both line vectors
      cv1 = l1p0 - l1p1;
      distance = (l1p1 - l0p1).length();
      //reversed sense is still false.
    }
    //try 1st head is closest to second tail
    if ((l1p0 - l0p1).length() < distance)
    {
      cv0 = l0p0 - l0p1; //reverse just first vector
      cv1 = l1p1 - l1p0; // assign to original in case reversed in previous
      distance = (l1p0 - l0p1).length();
      reversedSense = true;
    }
    //try 2nd head is closest to first tail
    if ((l1p1 - l0p0).length() < distance)
    {
      cv0 = l0p1 - l0p0; // assign to original in case reversed in previous
      cv1 = l1p0 - l1p1; //reverse just second vector
      //don't need to assign distance, no more tests.
      reversedSense = true;
    }
    
    
    double angle = vectorAngle(cv0, cv1);
    if 
    (
      (std::fabs(osg::PI - angle) < std::numeric_limits<float>::epsilon())
      || (std::fabs(osg::PI * 2.0 - angle) < std::numeric_limits<float>::epsilon())
    )
    {
      std::cout << "angle to close to parallel" << std::endl;
      return;
    }
    solver.addAngle(osg::RadiansToDegrees(angle), lines.front(), lines.back(), reversedSense);
    solver.solve(solver.getGroup(), true);
    update();
    clearSelection();
  }
}

//! @brief Add parallel constraint to the currently selected lines.
void Visual::addParallel()
{
  std::vector<SSHandle> lines = getSelectedLines();
  if (lines.size() != 2)
  {
    std::cout << "invalid selection for parallel" << std::endl;
    return;
  }
  solver.addParallel(lines.front(), lines.back());
  solver.solve(solver.getGroup(), true);
  update();
  clearSelection();
}

//! @brief Add perpendicular constraint to the currently selected lines.
void Visual::addPerpendicular()
{
  std::vector<SSHandle> lines = getSelectedLines();
  if (lines.size() != 2)
  {
    std::cout << "invalid selection for perpendicualr" << std::endl;
    return;
  }
  solver.addPerpendicular(lines.front(), lines.back());
  solver.solve(solver.getGroup(), true);
  update();
  clearSelection();
}

//! @brief Add midpoint constraint to the currently selected line and point.
void Visual::addMidpoint()
{
  std::vector<SSHandle> lines = getSelectedLines();
  std::vector<SSHandle> points = getSelectedPoints();
  
  if (data->highlights.size() != 2)
  {
    std::cout << "invalid selection count for midpoint" << std::endl;
    return;
  }
  if (lines.size() != 1 || points.size() != 1)
  {
    std::cout << "invalid selection for midpoint" << std::endl;
    return;
  }
  
  solver.addMidpointLine(points.front(), lines.front());
  solver.solve(solver.getGroup(), true);
  update();
  clearSelection();
}

//! @brief Add where dragged constraint to the currently selected points.
void Visual::addWhereDragged()
{
  std::vector<SSHandle> points = getSelectedPoints(false);
  if (points.empty())
  {
    std::cout << "invalid selection for where dragged" << std::endl;
    return;
  }
  
  for (const auto &h : points)
    solver.addWhereDragged(h);
  solver.solve(solver.getGroup(), true);
  update();
  clearSelection();
}

//! @brief Remove currently selected objects.
void Visual::remove()
{
  for (const auto &h : data->highlights)
  {
    osgText::Text *t = dynamic_cast<osgText::Text*>(h.get());
    if (t)
    {
      auto c = data->cMap.getRecord(h.get()->getParent(0));
      if (!c)
        c = data->cMap.getRecord(h.get()->getParent(0)->getParent(0));
      if (c)
      {
        solver.removeConstraint(c.get().handle);
        continue;
      }
    }
    
    auto e = data->eMap.getRecord(h.get());
    if (e)
    {
      solver.removeEntity(e.get().handle);
      continue;
    }
  }
  solver.clean();
  solver.solve(solver.getGroup(), true);
  update();
  data->highlights.clear(); //items gone so no need to unhighlight, just remove.
}

//! @brief Cancel the current command.
void Visual::cancel()
{
  if (data->state == State::point)
  {
    data->state = State::selection;
  }
  else if (data->state == State::line || data->state == State::arc || data->state == State::circle)
  {
    data->state = State::selection;
    data->previousPoint = boost::none;
    data->previousPick.release();
    data->transform->removeChild(data->dynamic);
    data->dynamic.release();
  }
  updateText();
}

//! @brief Make current selected entities construction or not.
void Visual::toggleConstruction()
{
  for (const auto &h : data->highlights)
  {
    auto e = data->eMap.getRecord(h.get());
    if (e)
    {
      if (e.get().construction)
        e.get().construction = false;
      else
        e.get().construction = true;
    }
  }
  clearSelection();
}

/*! @brief Set auto sizing of visible plane on or off.
 * @param value Is the new value for autoSize.
 * @ref Sizing
 */
void Visual::setAutoSize(bool value)
{
  autoSize = value;
}

/*! @brief Get the current value for auto size.
 * @return Current value for autoSize
 * @ref Sizing
 */
bool Visual::getAutoSize()
{
  return autoSize;
}

/*! @brief Set the current value for size.
 * @param value New value for size
 * @ref Sizing
 * @note This sets autoSize to false. Call @ref update for
 * display to reflect this change.
 */
void Visual::setSize(double value)
{
  setAutoSize(false);
  size = value;
}

/*! @brief Get the current value of size.
 * @return Current value of size
 * @ref Sizing
 */
double Visual::getSize()
{
  return size;
}

//! @brief get current state.
State Visual::getState()
{
  return data->state;
}

/*! @brief Get the main transform.
 * @return The main transform that contains all sketch visualization.
 */
osg::MatrixTransform* Visual::getTransform()
{
  return data->transform.get();
}

/*! @brief Project a point onto a line.
 * 
 * @param lp Is a point on the line.
 * @param ld Is a unit vector for the line direction.
 * @param p Is the point to project.
 * @return The projected point.
 */
osg::Vec3d Visual::projectPointLine(const osg::Vec3d &lp, const osg::Vec3d &ld, const osg::Vec3d &p)
{
  osg::Vec3d b(lp - p);
  osg::Vec3d proj(ld * (b * ld));
  return b - proj + p;
}

/*! @brief Get the distance between a point and a line.
 * 
 * @param lp Is a point on the line.
 * @param ld Is a unit vector for the line direction.
 * @param p Is a point.
 * @return The distance.
 * @note This function uses the line sense and returns a distance that
 * can be positive or negative.
 */
double Visual::distancePointLine(const osg::Vec3d &lp, const osg::Vec3d &ld, const osg::Vec3d &p)
{
  osg::Vec3d projectedPoint = projectPointLine(lp, ld, p);
  double distance = (projectedPoint - p).length();
  
  osg::Vec3d aux = p - lp;
  aux.normalize(); //probably don't need.
  osg::Vec3d cross = aux ^ ld;
  if (cross.z() < 0.0)
    distance *= -1.0;
  
  return distance;
}

/*! @brief Return construction state of entity.
 * 
 * @return the construction state.
 */
bool Visual::isConstruction(SSHandle h)
{
  auto ro = data->eMap.getRecord(h);
  assert(ro);
  return ro.get().construction;
}

boost::uuids::uuid Visual::getEntityId(SSHandle h)
{
  auto ro = data->eMap.getRecord(h);
  assert(ro);
  return ro.get().id;
}

/*! @brief Build the openscenegraph plane objects.
 * 
 * @details selection plane, visible plane, x and y axis lines
 * and origin point.
 */
void Visual::buildPlane()
{
  data->planeGroup->removeChildren(0, data->planeGroup->getNumChildren());
  
  //common items
  osg::LineWidth *lw = new osg::LineWidth(4.0f);
  osg::LineStipple *stipple = new osg::LineStipple(2, 0xf0f0);
  
  osg::Geometry *planeQuad = buildGeometry();
  planeQuad->getOrCreateStateSet()->setAttribute(data->planeDepth.get());
  planeQuad->setNodeMask(WorkPlane.to_ulong());
  planeQuad->setName("planeQuad");
  planeQuad->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
  (*(dynamic_cast<osg::Vec4Array*>(planeQuad->getColorArray())))[0] = osg::Vec4(.4, .7, .75, .5);
  osg::Vec3Array *normals = new osg::Vec3Array();
  normals->push_back(osg::Vec3d(0.0, 0.0, 1.0));
  planeQuad->setNormalArray(normals);
  planeQuad->setNormalBinding(osg::Geometry::BIND_OVERALL);
  osg::Vec3Array *verts = new osg::Vec3Array();
  verts->resizeArray(4);
  (*verts)[0] = osg::Vec3(size, size, 0.0);
  (*verts)[1] = osg::Vec3(-size, size, 0.0);
  (*verts)[2] = osg::Vec3(-size, -size, 0.0);
  (*verts)[3] = osg::Vec3(size, -size, 0.0);
  planeQuad->setVertexArray(verts);
  planeQuad->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0 , verts->size()));
  data->planeGroup->addChild(planeQuad);
  
  osg::Geometry *planeLines = buildGeometry();
  planeLines->setName("planeLines");
  planeLines->getOrCreateStateSet()->setAttribute(lw);
  (*(dynamic_cast<osg::Vec4Array*>(planeLines->getColorArray())))[0] = osg::Vec4(0.0, 0.0, 0.0, 1.0);
  planeLines->setVertexArray(verts);
  planeLines->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_LOOP, 0 , verts->size()));
  data->planeGroup->addChild(planeLines);
  
  //selection plane so we can intersect something for drawing entities.
  osg::Geometry *selectionPlane = buildGeometry();
  selectionPlane->setNodeMask(SelectionPlane.to_ulong());
  selectionPlane->setName("selectionPlane");
  (*(dynamic_cast<osg::Vec4Array*>(selectionPlane->getColorArray())))[0] = osg::Vec4(1.0, 0.0, 0.0, 1.0);
  osg::Vec3Array *spVerts = new osg::Vec3Array();
  spVerts->resizeArray(4);
  double sf = 10000.0;
  (*spVerts)[0] = osg::Vec3(sf, sf, 0.0);
  (*spVerts)[1] = osg::Vec3(-sf, sf, 0.0);
  (*spVerts)[2] = osg::Vec3(-sf, -sf, 0.0);
  (*spVerts)[3] = osg::Vec3(sf, -sf, 0.0);
  selectionPlane->setVertexArray(spVerts);
  selectionPlane->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0 , spVerts->size()));
  setDrawableToAlwaysCull(*selectionPlane);
  setDrawableToNoBound(*selectionPlane);
  osg::AutoTransform *autoScale = new osg::AutoTransform();
  autoScale->setAutoScaleToScreen(true);
  autoScale->addChild(selectionPlane);
  data->planeGroup->addChild(autoScale);
  
  osg::Geometry *xAxis = buildGeometry();
  xAxis->setNodeMask(WorkPlaneAxis.to_ulong());
  xAxis->setName("xAxis");
  xAxis->getOrCreateStateSet()->setAttribute(lw);
  xAxis->getOrCreateStateSet()->setAttributeAndModes(stipple);
  (*(dynamic_cast<osg::Vec4Array*>(xAxis->getColorArray())))[0] = osg::Vec4(1.0, 0.0, 0.0, 1.0);
  osg::Vec3Array *xVerts = new osg::Vec3Array();
  xVerts->resizeArray(2);
  (*xVerts)[0] = osg::Vec3(-size, 0.0, 0.0);
  (*xVerts)[1] = osg::Vec3(size, 0.0, 0.0);
  xAxis->setVertexArray(xVerts);
  xAxis->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0 , xVerts->size()));
  data->planeGroup->addChild(xAxis);
  
  osg::Geometry *yAxis = buildGeometry();
  yAxis->setNodeMask(WorkPlaneAxis.to_ulong());
  yAxis->setName("yAxis");
  yAxis->getOrCreateStateSet()->setAttribute(lw);
  yAxis->getOrCreateStateSet()->setAttributeAndModes(stipple);
  (*(dynamic_cast<osg::Vec4Array*>(yAxis->getColorArray())))[0] = osg::Vec4(0.0, 1.0, 0.0, 1.0);
  osg::Vec3Array *yVerts = new osg::Vec3Array();
  yVerts->resizeArray(2);
  (*yVerts)[0] = osg::Vec3(0.0, -size, 0.0);
  (*yVerts)[1] = osg::Vec3(0.0, size, 0.0);
  yAxis->setVertexArray(yVerts);
  yAxis->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0 , yVerts->size()));
  data->planeGroup->addChild(yAxis);
  
  osg::Geometry *origin = buildPoint();
  origin->setNodeMask(WorkPlaneOrigin.to_ulong());
  origin->setName("origin");
  (*(dynamic_cast<osg::Vec4Array*>(origin->getColorArray())))[0] = osg::Vec4(0.0, 0.0, 1.0, 1.0);
  data->planeGroup->addChild(origin);
}

/*! @brief General openscenegraph geometry construction.
 * 
 * @return General purpose openscenegraph dynamically allocated Geometry pointer. 
 */
osg::Geometry* Visual::buildGeometry()
{
  osg::Geometry *out = new osg::Geometry();
//   out->setNodeMask(mdv::edge);
//   out->setName("edges");
//   out->getOrCreateStateSet()->setAttribute(lineWidth.get());
  out->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
  out->setDataVariance(osg::Object::DYNAMIC);
  out->setUseDisplayList(false);
  
  osg::Vec4Array *ca = new osg::Vec4Array();
  ca->push_back(data->entityColor);
  out->setColorArray(ca);
  out->setColorBinding(osg::Geometry::BIND_OVERALL);
  
  return out;
}

/*! @brief build openscenegraph geometry for a line segment.
 * 
 * @return Dynamically allocated Geometry pointer for a line.
 */
osg::Geometry* Visual::buildLine()
{
  osg::Geometry *out = buildGeometry();
  out->getOrCreateStateSet()->setAttribute(data->curveDepth.get());
  osg::Vec3Array *verts = new osg::Vec3Array();
  verts->push_back(osg::Vec3(0.0, 0.0, 0.0));
  verts->push_back(osg::Vec3(1.0, 0.0, 0.0));
  out->setVertexArray(verts);
  
  out->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, 2));
  
  return out;
}

/*! @brief build openscenegraph geometry for a point.
 * 
 * @return Dynamically allocated Geometry pointer for a point.
 */
osg::Geometry* Visual::buildPoint()
{
  osg::Geometry *out = buildGeometry();
  out->getOrCreateStateSet()->setAttribute(data->pointDepth.get());
  osg::Vec3Array *verts = new osg::Vec3Array();
  verts->push_back(osg::Vec3(0.0, 0.0, 0.0));
  out->setVertexArray(verts);
  
  out->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, 1));
  
  out->getOrCreateStateSet()->setAttribute(new osg::Point(5.0));
  
  return out;
}

/*! @brief build openscenegraph geometry for an arc.
 * 
 * @return Dynamically allocated Geometry pointer for an arc.
 */
osg::Geometry* Visual::buildArc()
{
  osg::Geometry *out = buildGeometry();
  osg::Vec3Array *verts = new osg::Vec3Array();
  verts->push_back(osg::Vec3(0.0, 0.0, 0.0));
  verts->push_back(osg::Vec3(1.0, 0.0, 0.0));
  out->setVertexArray(verts);
  
  out->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, 2));
  
  return out;
}

/*! @brief build openscenegraph geometry for a circle.
 * 
 * @return Dynamically allocated Geometry pointer for a circle.
 */
osg::Geometry* Visual::buildCircle()
{
  osg::Geometry *out = buildGeometry();
  osg::Vec3Array *verts = new osg::Vec3Array();
  verts->push_back(osg::Vec3(0.0, 0.0, 0.0));
  verts->push_back(osg::Vec3(1.0, 0.0, 0.0));
  out->setVertexArray(verts);
  
  out->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, 2));
  
  return out;
}

/*! @brief General openscenegraph constraint text construction.
 * 
 * @return Dynamically allocated Text pointer.
 */
osgText::Text* Visual::buildConstraintText()
{
  osgText::Text *out = new osgText::Text();
  out->setName("text");
  out->setFont("fonts/arial.ttf");
  out->setColor(osg::Vec4(1.0, 0.0, 0.0, 1.0));
//   out->setBackdropType(osgText::Text::OUTLINE);
//   out->setBackdropColor(osg::Vec4(1.0, 1.0, 1.0, 1.0));
  out->setAlignment(osgText::Text::CENTER_CENTER);
//   out->setShaderTechnique(osgText::ShaderTechnique::GREYSCALE);
  
  return out;
}

/*! @brief General openscenegraph constraint construction.
 * @param nodeName Is the name  assigned to transform node.
 * @param text Is the text displayed.
 * @return Dynamically allocated transform pointer.
 * @note For one visible text item.
 */
osg::PositionAttitudeTransform* Visual::buildConstraint1(const std::string &nodeName, const std::string &text)
{
  osg::PositionAttitudeTransform *out = new osg::PositionAttitudeTransform();
  out->setPosition(osg::Vec3d(0.0, 0.0, 0.0));
  out->setName(nodeName);
  osgText::Text *hText = buildConstraintText();
  hText->setText(text);
  out->addChild(hText);
  
  return out;
}

/*! @brief General openscenegraph constraint construction.
 * @param nodeName Is the name  assigned to transform node.
 * @param textIn Is the text displayed.
 * @return Dynamically allocated transform pointer.
 * @note For two visible text items.
 */
osg::Group* Visual::buildConstraint2(const std::string &nodeName, const std::string &textIn)
{
  osg::Group *out = new osg::Group();
  out->setName(nodeName);
  auto build = [&]()
  {
    osg::PositionAttitudeTransform *t = new osg::PositionAttitudeTransform();
    t->setPosition(osg::Vec3d(0.0, 0.0, 0.0));
    osgText::Text *text = buildConstraintText();
    text->setText(textIn);
    t->addChild(text);
    out->addChild(t);
  };
  build();
  build();
  return out;
}

//! @brief Calculate the visual plane @ref size to contain the current sketch.
void Visual::updatePlaneSize()
{
  if (!autoSize)
    return;
  
  osg::Matrixd m(osg::Matrixd::identity());
  osg::MatrixList ms = data->planeGroup->getWorldMatrices();
  if (!ms.empty())
    m = ms.front();
  
  const osg::BoundingSphere &ebs = data->entityGroup->getBound();
  if (!ebs.valid())
  {
    size = 1.0;
    return;
  }
  
  //build a sphere at the transform origin and expand it to encapsulate entity bounding sphere.
  osg::BoundingSphere bs;
  bs.radius() = std::numeric_limits<float>::epsilon(); //otherwise bs is invalid and different behaviour.
  bs.center() = m.getTrans();
  bs.expandRadiusBy(ebs);
  size = bs.radius();
}

//! @brief Update the visual plane to @ref size.
void Visual::updatePlane()
{
  if (data->planeGroup->getNumChildren() == 0) //change to != 5
    buildPlane();
  updatePlaneSize();
  if (size == lastSize)
    return;
  
  auto planeQuad = getNamedChild<osg::Geometry>(data->planeGroup, "planeQuad");
  assert(planeQuad);
  if (!planeQuad)
  {
    std::cout << "ERROR: can't find plane quad in: " << BOOST_CURRENT_FUNCTION << std::endl;
    return;
  }
  osg::Vec3Array *pVerts = dynamic_cast<osg::Vec3Array*>(planeQuad.get()->getVertexArray());
  assert(pVerts);
  assert(pVerts->size() == 4);
  (*pVerts)[0] = osg::Vec3(size, size, 0.0);
  (*pVerts)[1] = osg::Vec3(-size, size, 0.0);
  (*pVerts)[2] = osg::Vec3(-size, -size, 0.0);
  (*pVerts)[3] = osg::Vec3(size, -size, 0.0);
  pVerts->dirty();
  planeQuad.get()->dirtyBound();
  
  auto xAxis = getNamedChild<osg::Geometry>(data->planeGroup, "xAxis");
  assert(xAxis);
  if (!xAxis)
  {
    std::cout << "ERROR: can't find xAxis in: " << BOOST_CURRENT_FUNCTION << std::endl;
    return;
  }
  osg::Vec3Array *xVerts = dynamic_cast<osg::Vec3Array*>(xAxis.get()->getVertexArray());
  assert(xVerts);
  assert(xVerts->size() == 2);
  (*xVerts)[0] = osg::Vec3(-size, 0.0, 0.0);
  (*xVerts)[1] = osg::Vec3(size, 0.0, 0.0);
  xVerts->dirty();
  xAxis.get()->dirtyBound();
  
  auto yAxis = getNamedChild<osg::Geometry>(data->planeGroup, "yAxis");
  assert(yAxis);
  if (!yAxis)
  {
    std::cout << "ERROR: can't find yAxis in: " << BOOST_CURRENT_FUNCTION << std::endl;
    return;
  }
  osg::Vec3Array *yVerts = dynamic_cast<osg::Vec3Array*>(yAxis.get()->getVertexArray());
  assert(yVerts);
  assert(yVerts->size() == 2);
  (*yVerts)[0] = osg::Vec3(0.0, -size, 0.0);
  (*yVerts)[1] = osg::Vec3(0.0, size, 0.0);
  yVerts->dirty();
  yAxis.get()->dirtyBound();
  
  //shouldn't need to update point;
  
  lastSize = size;
}

//! @brief Update the solver status text.
void Visual::updateText()
{
  std::ostringstream t;
  
  if(solver.getSys().result == SLVS_RESULT_OKAY)
  {
    t << "solver success. " << solver.getSys().dof << " DOF";
  }
  else
  {
    t << "solver failed. ";
  }
  data->statusText->setText(t.str());
  
  double corner = size - size * 0.005;
  double scale = size * 0.002;
  
  data->statusTextTransform->setPosition(osg::Vec3d(corner, corner, 0.001));
  data->statusTextTransform->setScale(osg::Vec3d(scale, scale, scale));
}

//! @brief Update polygon representation of an arc. Not very smart.
void Visual::updateArcGeometry(osg::Geometry *geometry, const osg::Vec3d &ac, const osg::Vec3d &as, const osg::Vec3d &af)
{
  assert(geometry);
  osg::Vec3Array *verts = dynamic_cast<osg::Vec3Array*>(geometry->getVertexArray());
  assert(verts);
  
  double angle = 0.0;
  osg::Vec3d r1 = as - ac;
  osg::Vec3d r2 = af - ac;
  double radius = r1.length();
  if (std::fabs(r2.length() - r1.length()) > std::numeric_limits<float>::epsilon())
    std::cout << "WARNING! radei are different for arc points" << std::endl;
  
  r1.normalize();
  r2.normalize();
  osg::Vec3d cross = r1 ^ r2;
  double dot = r1 * r2;
  if (std::fabs(cross.z()) < std::numeric_limits<float>::epsilon())
  {
    if (dot > 0.0)
      angle = osg::PI * 2.0;
    else
      angle = osg::PI;
  }
  else
  {
    angle = std::acos(dot);
    if (cross.z() < 0.0)
      angle = (osg::PI * 2.0) - angle;
  }
  double sides = 16.0;
  if (angle > osg::PI)
    sides = 32.0;
  verts->resizeArray(sides + 1);
  dynamic_cast<osg::DrawArrays*>(geometry->getPrimitiveSet(0))->setCount(verts->size());
  (*verts)[0] = as;
  double angleIncrement = angle / sides;
  osg::Quat rotation(angleIncrement, osg::Vec3d(0.0, 0.0, 1.0));
  osg::Vec3d currentPoint = r1;
  for (int index = 1; index < sides; ++index)
  {
    currentPoint = rotation * currentPoint;
    (*verts)[index] = currentPoint * radius + ac;
  }
  (*verts)[sides] = af;
  verts->dirty();
}

/*! @brief Convert a solvespace point into an openscenegraph one.
 * 
 * @param ph A handle to a solvespace point.
 * @return an openscenegraph vector for the solve space point.
 */
osg::Vec3d Visual::convert(SSHandle ph)
{
  assert(solver.isEntityType(ph, SLVS_E_POINT_IN_2D) || solver.isEntityType(ph, SLVS_E_POINT_IN_3D));
  
  auto pe = solver.findEntity(ph);
  assert(pe);
  
  osg::Vec3d out(solver.getParameterValue(pe.get().param[0]).get(), solver.getParameterValue(pe.get().param[1]).get(), 0.0);
  if (solver.isEntityType(ph, SLVS_E_POINT_IN_3D))
    out.z() = solver.getParameterValue(pe.get().param[2]).get();
  
  return out;
}

/*! @brief Filter for selection plane intersection.
 * 
 * @param is Is line segment intersections.
 * @return a boost optional possibly containing the point.
 */
boost::optional<osg::Vec3d> Visual::getPlanePoint(const osgUtil::LineSegmentIntersector::Intersections &is)
{
  boost::optional<osg::Vec3d> out;
  for (const auto &i : is)
  {
    osg::Geometry *tg = dynamic_cast<osg::Geometry*>(i.drawable.get());
    if (tg)
    {
      if(tg->getName() == "selectionPlane")
      {
        assert(i.nodePath.size() > 1);
        osg::Node *noi = i.nodePath.at(i.nodePath.size() - 2);
        osg::AutoTransform *autoScale = dynamic_cast<osg::AutoTransform*>(noi);
        assert(autoScale);
        out = i.localIntersectionPoint * osg::Matrixd::scale(autoScale->getScale());
        break;
      }
    }
  }
  return out;
}

/*! @brief Filter current selection for all points.
 * @param includeWork True means: include the work plane origin.
 * @return A vector of handles for the currently selected points.
 * @note this includes the workplane origin and both 2d and 3d points.
 */
std::vector<SSHandle> Visual::getSelectedPoints(bool includeWork)
{
  std::vector<SSHandle> out;
  
  for (const auto &h : data->highlights)
  {
    const std::string &n = h->getName();
    if (includeWork && n == "origin")
    {
      out.push_back(solver.getWPOrigin());
      continue;
    }
    
    auto e = data->eMap.getRecord(h);
    if (!e)
      continue;
    if
    (
      solver.isEntityType(e.get().handle, SLVS_E_POINT_IN_3D)
      || solver.isEntityType(e.get().handle, SLVS_E_POINT_IN_2D)
    )
      out.push_back(e.get().handle);
  }
  
  return out;
}

/*! @brief Filter current selection for all lines.
 * @param includeWork True means: include the work plane axes.
 * @return A vector of handles for the currently selected lines
 * @note this includes the workplane axes.
 */
std::vector<SSHandle> Visual::getSelectedLines(bool includeWork)
{
  std::vector<SSHandle> out;
  
  for (const auto &h : data->highlights)
  {
    const std::string &n = h->getName();
    if (includeWork && n == "xAxis")
    {
      out.push_back(solver.getXAxis());
      continue;
    }
    if (includeWork && n == "yAxis")
    {
      out.push_back(solver.getYAxis());
      continue;
    }
    auto e = data->eMap.getRecord(h);
    if (!e)
      continue;
    if (solver.isEntityType(e.get().handle, SLVS_E_LINE_SEGMENT))
      out.push_back(e.get().handle);
  }
  
  return out;
}

/*! @brief Filter current selection for all circles and arcs.
 * 
 * @return A vector of handles for the currently selected circles and arcs
 */
std::vector<SSHandle> Visual::getSelectedCircles()
{
  std::vector<SSHandle> out;
  
  for (const auto &h : data->highlights)
  {
    auto e = data->eMap.getRecord(h);
    if (!e)
      continue;
    if
    (
      solver.isEntityType(e.get().handle, SLVS_E_CIRCLE)
      || solver.isEntityType(e.get().handle, SLVS_E_ARC_OF_CIRCLE)
    )
      out.push_back(e.get().handle);
  }
  
  return out;
}

/*! @brief Get a point along a line or arc.
 * @param h Is a handle to a line or arc. Not circle.
 * @param u Is a parameter along curve.
 * @return A point on the curve.
 * @note for an arc: u is clamped to a closed range of 0.0 to 1.0.
 * for a line: u of 0.0 start of line and u of 1.0 will be end of line.
 * However values less than 0.0 and greater than 1.0 can be used.
 */
osg::Vec3d Visual::parameterPoint(SSHandle h, double u)
{
  auto e = solver.findEntity(h);
  assert(e);
  
  if (solver.isEntityType(h, SLVS_E_LINE_SEGMENT))
  {
    osg::Vec3d s = convert(e.get().point[0]);
    osg::Vec3d f = convert(e.get().point[1]);
    return s + ((f - s) * u);
  }
  else if (solver.isEntityType(h, SLVS_E_ARC_OF_CIRCLE))
  {
    u = std::max(u, 0.0);
    u = std::min(1.0, u);
    osg::Vec3d ac = convert(e.get().point[0]);
    osg::Vec3d as = convert(e.get().point[1]);
    osg::Vec3d af = convert(e.get().point[2]);
    
    osg::Vec3d r1 = as - ac;
    osg::Vec3d r2 = af - ac;
    double angle = vectorAngle(r1, r2);
    
    angle *= u;
    osg::Quat rot(angle, osg::Vec3d(0.0, 0.0, 1.0));
    return (rot * (as - ac)) + ac;
  }
  else
  {
    assert(0); //unsupported type
    std::cout << "unsupported type " << BOOST_CURRENT_FUNCTION << std::endl;
  }
  
  return osg::Vec3d(0.0, 0.0, 0.0);
}

/*! @brief Get rotation angle between to vectors.
 * @param v1 Start vector. Will be normalized.
 * @param v2 Finish vector. Will be normalized.
 * @return Angle in radians. [0, 2PI]
 */
double Visual::vectorAngle(osg::Vec3d v1, osg::Vec3d v2)
{
  v1.normalize();
  v2.normalize();
  double angle;
  osg::Vec3d cross = v1 ^ v2;
  double dot = v1 * v2;
  if (std::fabs(cross.z()) < std::numeric_limits<float>::epsilon())
  {
    if (dot > 0.0)
      angle = osg::PI * 2.0;
    else
      angle = osg::PI;
  }
  else
  {
    angle = std::acos(dot);
    if (cross.z() < 0.0)
      angle = (osg::PI * 2.0) - angle;
  }
  return angle;
}

/*! @brief Get direction vector of line.
 * @param h Handle to line segment.
 * @return A non-unit vector of the line.
 */
osg::Vec3d Visual::lineVector(SSHandle h)
{
  assert(solver.isEntityType(h, SLVS_E_LINE_SEGMENT));
  auto oe = solver.findEntity(h);
  assert(oe);
  
  return convert(oe.get().point[1]) - convert(oe.get().point[0]);
}

/*! @brief Get end points of a line segment.
 * @param h Handle to line segment.
 * @return A pair of ordered vectors.
 */
std::pair<osg::Vec3d, osg::Vec3d> Visual::endPoints(SSHandle h)
{
  assert(solver.isEntityType(h, SLVS_E_LINE_SEGMENT));
  auto oe = solver.findEntity(h);
  assert(oe);
  
  return std::make_pair(convert(oe.get().point[0]), convert(oe.get().point[1]));
}

/*! @brief Get the center of the bounding sphere of entities.
 * @param handles Array of entity handles.
 * @return Bounding center.
 * @note this is used as a hack to place constraints
 */
osg::Vec3d Visual::boundingCenter(const std::vector<SSHandle> &handles)
{
  assert(!handles.empty());
  osg::BoundingSphere bs;
  
  for (const auto &h : handles)
  {
    auto r = data->eMap.getRecord(h);
    assert(r);
    
    osg::Drawable *d = dynamic_cast<osg::Drawable*>(r.get().node.get());
    assert(d);
    if (!bs.valid())
      bs = d->getBound();
    else
      bs.expandBy(d->getBound());
  };
  assert(bs.valid());
  return bs.center();
}
