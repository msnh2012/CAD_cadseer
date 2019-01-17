/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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
#include <functional>
#include <stack>
#include <algorithm>

#include <boost/type_traits/is_volatile.hpp>
#include <boost/type_traits/is_assignable.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/current_function.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/composite_key.hpp>

#include <TopoDS_Shape.hxx>
#include <gp_Circ.hxx>
#include <gp_Elips.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopExp.hxx>
#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepTools.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <BRepBuilderAPI_Copy.hxx>

#include <osg/Vec3d>

#include <tools/idtools.h>
#include <tools/occtools.h>
#include <annex/shapeidhelper.h>
#include <project/serial/xsdcxxoutput/featurebase.h>
#include <feature/shapehistory.h>
#include <annex/seershape.h>

using boost::uuids::uuid;

namespace ann
{
  //mapping a set of ids to one id. this is for deriving an id from multiple parent shapes.
  typedef std::set<boost::uuids::uuid> IdSet;
  typedef std::map<IdSet, boost::uuids::uuid> DerivedContainer;
  
  typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS> Graph;
  typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
  typedef boost::graph_traits<Graph>::edge_descriptor Edge;
  typedef boost::graph_traits<Graph>::vertex_iterator VertexIterator;
  typedef boost::graph_traits<Graph>::edge_iterator EdgeIterator;
  typedef boost::graph_traits<Graph>::in_edge_iterator InEdgeIterator;
  typedef boost::graph_traits<Graph>::out_edge_iterator OutEdgeIterator;
  typedef boost::graph_traits<Graph>::adjacency_iterator VertexAdjacencyIterator;
  
  namespace BMI = boost::multi_index;
  
  struct ShapeIdRecord
  {
    uuid id;
    Vertex graphVertex;
    TopoDS_Shape shape;
    
    ShapeIdRecord() :
      id(gu::createNilId()),
      graphVertex(boost::graph_traits<Graph>::null_vertex()),
      shape(TopoDS_Shape())
      {}
    
    //@{
    //! for tags
    struct ById{};
    struct ByVertex{};
    struct ByShape{};
    //@}
  };
  
  struct ShapeIdKeyHash
  {
    std::size_t operator()(const TopoDS_Shape& shape)const
    {
      int hashOut;
      hashOut = shape.HashCode(std::numeric_limits<int>::max());
      return static_cast<std::size_t>(hashOut);
    }
  };
  
  struct ShapeIdShapeEquality
  {
    bool operator()(const TopoDS_Shape &shape1, const TopoDS_Shape &shape2) const
    {
      return shape1.IsSame(shape2);
    }
  };
  
  typedef boost::multi_index_container
  <
    ShapeIdRecord,
    BMI::indexed_by
    <
      BMI::ordered_non_unique
      <
        BMI::tag<ShapeIdRecord::ById>,
        BMI::member<ShapeIdRecord, BID::uuid, &ShapeIdRecord::id>
      >,
      BMI::ordered_non_unique
      <
        BMI::tag<ShapeIdRecord::ByVertex>,
        BMI::member<ShapeIdRecord, Vertex, &ShapeIdRecord::graphVertex>
      >,
      BMI::hashed_non_unique
      <
        BMI::tag<ShapeIdRecord::ByShape>,
        BMI::member<ShapeIdRecord, TopoDS_Shape, &ShapeIdRecord::shape>,
        ShapeIdKeyHash,
        ShapeIdShapeEquality
      >
    >
  > ShapeIdContainer;
  
  std::ostream& operator<<(std::ostream&, const ShapeIdRecord&);
  std::ostream& operator<<(std::ostream&, const ShapeIdContainer&);
  
  
  
  struct EvolveRecord
  {
    boost::uuids::uuid inId;
    boost::uuids::uuid outId;
    EvolveRecord() : inId(gu::createNilId()), outId(gu::createNilId()) {}
    EvolveRecord(const boost::uuids::uuid &inIdIn, const boost::uuids::uuid &outIdIn):
      inId(inIdIn), outId(outIdIn){}
    
    //@{
    //! used as tags.
    struct ByInId{};
    struct ByOutId{};
    struct ByInOutIds{};
    //@}
  };
  
  typedef boost::multi_index_container
  <
    EvolveRecord,
    BMI::indexed_by
    <
      BMI::ordered_non_unique
      <
        BMI::tag<EvolveRecord::ByInId>,
        BMI::member<EvolveRecord, boost::uuids::uuid, &EvolveRecord::inId>
      >,
      BMI::ordered_non_unique
      <
        BMI::tag<EvolveRecord::ByOutId>,
        BMI::member<EvolveRecord, boost::uuids::uuid, &EvolveRecord::outId>
      >,
      BMI::ordered_unique
      < 
        BMI::tag<EvolveRecord::ByInOutIds>,
        BMI::composite_key
        <
          EvolveRecord,
          BMI::member<EvolveRecord, boost::uuids::uuid, &EvolveRecord::inId>,
          BMI::member<EvolveRecord, boost::uuids::uuid, &EvolveRecord::outId>
        >
      >
    >
  > EvolveContainer;
  
  std::ostream& operator<<(std::ostream& os, const EvolveRecord& record);
  std::ostream& operator<<(std::ostream& os, const EvolveContainer& container);
  
  
  
  //! associate a string identifier to an id.
  struct FeatureTagRecord
  {
    boost::uuids::uuid id;
    std::string tag;
    
    FeatureTagRecord() : id(gu::createNilId()), tag() {}
    FeatureTagRecord(const uuid &idIn, const std::string &tagIn): id(idIn), tag(tagIn){}
    
    //@{
    //! used as tags.
    struct ById{};
    struct ByTag{};
    //@}
  };
  
  typedef boost::multi_index_container
  <
    FeatureTagRecord,
    BMI::indexed_by
    <
      BMI::ordered_unique
      <
        BMI::tag<FeatureTagRecord::ById>,
        BMI::member<FeatureTagRecord, boost::uuids::uuid, &FeatureTagRecord::id>
      >,
      BMI::ordered_unique
      <
        BMI::tag<FeatureTagRecord::ByTag>,
        BMI::member<FeatureTagRecord, std::string, &FeatureTagRecord::tag>
      >
    >
  > FeatureTagContainer;
  
  std::ostream& operator<<(std::ostream& os, const FeatureTagRecord& record);
  std::ostream& operator<<(std::ostream& os, const FeatureTagContainer& container);
  
  struct ShapeStow
  {
    ShapeIdContainer shapeIds;
    EvolveContainer evolves;
    FeatureTagContainer featureTags;
    DerivedContainer derives;
    Graph graph;
    Graph rGraph; //reversed graph.
    
    const ShapeIdRecord& findShapeIdRecord(const uuid& idIn) const
    {
      typedef ShapeIdContainer::index<ShapeIdRecord::ById>::type List;
      const List &list = shapeIds.get<ShapeIdRecord::ById>();
      List::const_iterator it = list.find(idIn);
      assert(it != list.end());
      return *it;
    }

    const ShapeIdRecord& findShapeIdRecord(const Vertex& vertexIn) const
    {
      typedef ShapeIdContainer::index<ShapeIdRecord::ByVertex>::type List;
      const List &list = shapeIds.get<ShapeIdRecord::ByVertex>();
      List::const_iterator it = list.find(vertexIn);
      assert(it != list.end());
      return *it;
    }

    const ShapeIdRecord& findShapeIdRecord(const TopoDS_Shape& shapeIn) const
    {
      typedef ShapeIdContainer::index<ShapeIdRecord::ByShape>::type List;
      const List &list = shapeIds.get<ShapeIdRecord::ByShape>();
      List::const_iterator it = list.find(shapeIn);
      assert(it != list.end());
      return *it;
    }
  };
  
  template <class GraphTypeIn>
  class Node_writer {
  public:
    Node_writer(const GraphTypeIn &graphIn, const SeerShape &seerShapeIn):
      graph(graphIn), seerShape(seerShapeIn){}
    template <class NodeW>
    void operator()(std::ostream& out, const NodeW& v) const
    {
        out << 
            "[label=\"" <<
            occt::getShapeTypeString(seerShape.getStow().findShapeIdRecord(v).shape) << "\\n" <<
            gu::idToString(seerShape.getStow().findShapeIdRecord(v).id) <<
            "\"]";
    }
  private:
    const GraphTypeIn &graph;
    const SeerShape &seerShape;
  };
    
  class TypeCollectionVisitor : public boost::default_bfs_visitor
  {
  public:
    TypeCollectionVisitor(const TopAbs_ShapeEnum &shapeTypeIn, const SeerShape &seerShapeIn, std::vector<Vertex> &vertOut) :
      shapeType(shapeTypeIn), seerShape(seerShapeIn), graphVertices(vertOut){}
    template <typename VisitorVertex, typename VisitorGraph>
    void discover_vertex(VisitorVertex u, const VisitorGraph &)
    {
      if (seerShape.getStow().findShapeIdRecord(u).shape.ShapeType() == this->shapeType)
        graphVertices.push_back(u);
    }

  private:
    const TopAbs_ShapeEnum &shapeType;
    const SeerShape &seerShape;
    std::vector<Vertex> &graphVertices;
  };
}

using namespace ann;

std::ostream& ann::operator<<(std::ostream& os, const ShapeIdRecord& record)
{
  os << gu::idToString(record.id) << "      " << 
    ShapeIdKeyHash()(record.shape) << "      " <<
    record.graphVertex << "      " <<
    occt::getShapeTypeString(record.shape) << std::endl;
  return os;
}

std::ostream& ann::operator<<(std::ostream& os, const ShapeIdContainer& container)
{
  typedef ShapeIdContainer::index<ShapeIdRecord::ById>::type List;
  const List &list = container.get<ShapeIdRecord::ById>();
  for (List::const_iterator it = list.begin(); it != list.end(); ++it)
    os << *it;
  return os;
}

ostream& ann::operator<<(ostream& os, const EvolveRecord& record)
{
  os << gu::idToString(record.inId) << "      " << gu::idToString(record.outId) << std::endl;
  return os;
}

ostream& ann::operator<<(ostream& os, const EvolveContainer& container)
{
  typedef EvolveContainer::index<EvolveRecord::ByInId>::type List;
  const List &list = container.get<EvolveRecord::ByInId>();
  for (List::const_iterator it = list.begin(); it != list.end(); ++it)
    os << *it;
  return os;
}



ostream& ann::operator<<(ostream& os, const FeatureTagRecord& record)
{
  os << gu::idToString(record.id) << "      " << record.tag << std::endl;
  return os;
}

ostream& ann::operator<<(ostream& os, const FeatureTagContainer& container)
{
  typedef FeatureTagContainer::index<FeatureTagRecord::ById>::type List;
  const List &list = container.get<FeatureTagRecord::ById>();
  for (List::const_iterator it = list.begin(); it != list.end(); ++it)
    os << *it;
  return os;
}

SeerShape::SeerShape() : Base(), stow(new ShapeStow())
{
  reset();
}

SeerShape::~SeerShape(){}

void SeerShape::setOCCTShape(const TopoDS_Shape& shapeIn)
{
  reset();
  
  if (shapeIn.IsNull())
    return;
  
  //make sure we have a compound as root of shape.
  TopoDS_Shape workShape = occt::compoundWrap(shapeIn);
  
  //fill in container with shapes and nil ids and new vertices.
  for (const auto &s : occt::mapShapes(workShape))
  {
    ShapeIdRecord record;
    record.shape = s;
    record.graphVertex = boost::add_vertex(stow->graph);
    //id is nil. set by record constructor.
    
    stow->shapeIds.insert(record);
  }
  
  //root compound shape needs an id even though it maybe temp.
  rootShapeId = gu::createRandomId();
  updateId(workShape, rootShapeId);
  stow->rGraph = stow->graph; //now graph and rGraph are equal, edgeless graphs.
  
  updateGraphs();
}

void SeerShape::reset()
{
  rootShapeId = gu::createNilId();
  stow->shapeIds.get<ShapeIdRecord::ById>().clear();
  stow->graph = Graph();
  stow->rGraph = stow->graph;
}

void SeerShape::updateGraphs()
{
  //expects that graph and rGraph are equal and edgeless.
  //expects that the graph and rGraph contain vertices for all shapes in container.
  //expects that the rootShapeId is set even temporarily.
  
  //recursive function to build graph.
  std::stack<TopoDS_Shape> shapeStack;
  std::function <void (const TopoDS_Shape &)> recursion = [&](const TopoDS_Shape &shapeIn)
  {
    for (TopoDS_Iterator it(shapeIn); it.More(); it.Next())
    {
      const TopoDS_Shape &currentShape = it.Value();
      if (!(hasShape(currentShape)))
        continue; //topods_iterator doesn't ignore orientation like mapShapes does. probably seam edge.

      //add edge to previous graph vertex if stack is not empty.
      if (!shapeStack.empty())
      {
        Vertex pVertex = stow->findShapeIdRecord(shapeStack.top()).graphVertex;
        Vertex cVertex = stow->findShapeIdRecord(currentShape).graphVertex;
        
        if (!boost::edge(pVertex, cVertex, stow->graph).second)
        {
          boost::add_edge(pVertex, cVertex, stow->graph);
          boost::add_edge(cVertex, pVertex, stow->rGraph);
        }
      }
      shapeStack.push(currentShape);
      recursion(currentShape);
      shapeStack.pop();
    }
  };
  
  const TopoDS_Shape &rootWorkShape = getRootOCCTShape();
  shapeStack.push(rootWorkShape);
  recursion(rootWorkShape);
}

bool SeerShape::hasId(const uuid& idIn) const
{
  typedef ShapeIdContainer::index<ShapeIdRecord::ById>::type List;
  const List &list = stow->shapeIds.get<ShapeIdRecord::ById>();
  List::const_iterator it = list.find(idIn);
  return (it != list.end());
}

bool SeerShape::hasShape(const TopoDS_Shape& shapeIn) const
{
  typedef ShapeIdContainer::index<ShapeIdRecord::ByShape>::type List;
  const List &list = stow->shapeIds.get<ShapeIdRecord::ByShape>();
  List::const_iterator it = list.find(shapeIn);
  return (it != list.end());
}

const TopoDS_Shape& SeerShape::findShape(const uuid& idIn) const
{
  typedef ShapeIdContainer::index<ShapeIdRecord::ById>::type List;
  const List &list = stow->shapeIds.get<ShapeIdRecord::ById>();
  List::const_iterator it = list.find(idIn);
  assert(it != list.end());
  return it->shape;
}

const uuid& SeerShape::findId(const TopoDS_Shape& shapeIn) const
{
  typedef ShapeIdContainer::index<ShapeIdRecord::ByShape>::type List;
  const List &list = stow->shapeIds.get<ShapeIdRecord::ByShape>();
  List::const_iterator it = list.find(shapeIn);
  assert(it != list.end());
  return it->id;
}

//! updates the id by matching shape.
void SeerShape::updateId(const TopoDS_Shape& shapeIn, const uuid& idIn)
{
  typedef ShapeIdContainer::index<ShapeIdRecord::ByShape>::type List;
  List &list = stow->shapeIds.get<ShapeIdRecord::ByShape>();
  List::iterator it = list.find(shapeIn);
  assert(it != list.end());
  ShapeIdRecord record = *it;
  record.id = idIn;
  list.replace(it, record);
}

std::vector<uuid> SeerShape::getAllShapeIds() const
{
  std::vector<uuid> out;
  
  typedef ShapeIdContainer::index<ShapeIdRecord::ById>::type List;
  const List &list = stow->shapeIds.get<ShapeIdRecord::ById>();
  for (const auto &entry : list)
    out.push_back(entry.id);
  return out;
}

occt::ShapeVector SeerShape::getAllShapes() const
{
  occt::ShapeVector out;
  
  typedef ShapeIdContainer::index<ShapeIdRecord::ById>::type List;
  const List &list = stow->shapeIds.get<ShapeIdRecord::ById>();
  for (const auto &entry : list)
    out.push_back(entry.shape);
  return out;
}

occt::ShapeVector SeerShape::getAllNilShapes() const
{
  occt::ShapeVector out;
  typedef ShapeIdContainer::index<ShapeIdRecord::ById>::type List;
  const List &list = stow->shapeIds.get<ShapeIdRecord::ById>();
  auto rangeItPair = list.equal_range(gu::createNilId());
  for (; rangeItPair.first != rangeItPair.second; ++rangeItPair.first)
    out.push_back(rangeItPair.first->shape);
  
  return out;
}

bool SeerShape::hasEvolveRecordIn(const uuid &idIn) const
{
  typedef EvolveContainer::index<EvolveRecord::ByInId>::type List;
  const List &list = stow->evolves.get<EvolveRecord::ByInId>();
  List::const_iterator it = list.find(idIn);
  return (it != list.end());
}

bool SeerShape::hasEvolveRecordOut(const uuid &idOut) const
{
  typedef EvolveContainer::index<EvolveRecord::ByOutId>::type List;
  const List &list = stow->evolves.get<EvolveRecord::ByOutId>();
  List::const_iterator it = list.find(idOut);
  return (it != list.end());
}

bool SeerShape::hasEvolveRecord(const BID::uuid &inId, const BID::uuid &outId) const
{
  typedef EvolveContainer::index<EvolveRecord::ByInOutIds>::type List;
  const List &list = stow->evolves.get<EvolveRecord::ByInOutIds>();
  List::const_iterator it = list.find(std::make_tuple(inId, outId));
  return (it != list.end());
}

std::vector<uuid> SeerShape::evolve(const uuid &idIn) const
{
  std::vector<uuid> out;
  typedef EvolveContainer::index<EvolveRecord::ByInId>::type List;
  const List &list = stow->evolves.get<EvolveRecord::ByInId>();
  auto rangeItPair = list.equal_range(idIn);
  for (; rangeItPair.first != rangeItPair.second; ++rangeItPair.first)
    out.push_back(rangeItPair.first->outId);
  
  return out;
}

std::vector<uuid> SeerShape::devolve(const uuid &idOut) const
{
  std::vector<uuid> out;
  typedef EvolveContainer::index<EvolveRecord::ByOutId>::type List;
  const List &list = stow->evolves.get<EvolveRecord::ByOutId>();
  auto rangeItPair = list.equal_range(idOut);
  for (; rangeItPair.first != rangeItPair.second; ++rangeItPair.first)
    out.push_back(rangeItPair.first->inId);
  
  return out;
}

void SeerShape::insertEvolve(const uuid& idIn, const uuid& idOut)
{
  stow->evolves.insert(EvolveRecord(idIn, idOut));
}

void SeerShape::fillInHistory(ftr::ShapeHistory &historyIn, const BID::uuid &featureId) const
{
  /* we need to create ids in the history graph for this shape.
   * these would be the out ids. We need to remember that the evolution container
   * is not cleared each update and may contain a large number of entries not
   * relevant to the current update.
   */
  
  typedef EvolveContainer::index<EvolveRecord::ByInId>::type List;
  const List &list = stow->evolves.get<EvolveRecord::ByInId>();
  for (List::const_iterator it = list.begin(); it != list.end(); ++it)
  {
    //if the outid is nil, that means that a shape didn't make it through operation.
    //if the outid doesn't exist in the actually finished shape, that means this entry is from another update.
    //we don't need to worry about these conditions.
    if (it->outId.is_nil() || !hasId(it->outId))
      continue;
    
    if (!it->inId.is_nil())
    {
      //in this case we have a valid shape with a valid id in the out column, but the
      //in column id doesn't exist in the graph. A prior feature didn't update the history graph correctly.
      if(!historyIn.hasShape(it->inId))
        std::cout << "warning: shape id: " << gu::idToString(it->inId)
        << " should be in shape history in: " << BOOST_CURRENT_FUNCTION << std::endl;
    }
    
    if (!historyIn.hasShape(it->outId)) //might be there already, like a 'merge' situation.
      historyIn.addShape(featureId, it->outId);
    
    if (historyIn.hasShape(it->inId) && historyIn.hasShape(it->outId))
      historyIn.addConnection(it->outId, it->inId); //child points to parent.
  }
}

void SeerShape::replaceId(const uuid &staleId, const uuid &freshId, const ftr::ShapeHistory &)
{
  //not sure shape history should be passed into seershape?
  //don't see using this involving nil ids.
  assert(!staleId.is_nil());
  assert(!freshId.is_nil());
  
  typedef EvolveContainer::index<EvolveRecord::ByInId>::type ListIn;
  ListIn &listIn = stow->evolves.get<EvolveRecord::ByInId>();
  
  //iterator over equal_range was causing infinite loop. iterator invalidation I suspect.
  auto it = listIn.find(staleId);
  while (it != listIn.end())
  {
    EvolveRecord replacement = *it;
    replacement.inId = freshId;
    listIn.replace(it, replacement);
    it = listIn.find(staleId);
  }
}

uuid SeerShape::featureTagId(const std::string& tagIn)
{
  typedef FeatureTagContainer::index<FeatureTagRecord::ByTag>::type List;
  const List &list = stow->featureTags.get<FeatureTagRecord::ByTag>();
  List::const_iterator it = list.find(tagIn);
  assert(it != list.end());
  return it->id;
}

void SeerShape::insertFeatureTag(const uuid &idIn, const std::string &tagIn)
{
  stow->featureTags.insert(FeatureTagRecord(idIn, tagIn));
}

const TopoDS_Shape& SeerShape::getRootOCCTShape() const
{
  return findShape(rootShapeId);
}

const uuid& SeerShape::getRootShapeId() const
{
  return rootShapeId;
}

const TopoDS_Shape& SeerShape::getOCCTShape(const uuid& idIn) const
{
  return findShape(idIn);
}

void SeerShape::setRootShapeId(const uuid& idIn)
{
  rootShapeId = idIn;
}

bool SeerShape::isNull() const
{
  return rootShapeId.is_nil() || (!hasId(rootShapeId));
}

std::vector<uuid> SeerShape::useGetParentsOfType
  (const uuid &idIn, const TopAbs_ShapeEnum &shapeTypeIn) const
{
  assert(hasId(idIn));

  std::vector<Vertex> vertices;
  TypeCollectionVisitor vis(shapeTypeIn, *this, vertices);
  boost::breadth_first_search(stow->rGraph, stow->findShapeIdRecord(idIn).graphVertex, boost::visitor(vis));

  std::vector<Vertex>::const_iterator vit;
  std::vector<uuid> idsOut;
  for (vit = vertices.begin(); vit != vertices.end(); ++vit)
      idsOut.push_back(stow->findShapeIdRecord(*vit).id);
  return idsOut;
}

occt::ShapeVector SeerShape::useGetParentsOfType
  (const TopoDS_Shape &shapeIn, const TopAbs_ShapeEnum& shapeTypeIn) const
{
  assert(hasShape(shapeIn));
  
  std::vector<Vertex> vertices;
  TypeCollectionVisitor vis(shapeTypeIn, *this, vertices);
  boost::breadth_first_search(stow->rGraph, stow->findShapeIdRecord(shapeIn).graphVertex, boost::visitor(vis));

  occt::ShapeVector shapesOut;
  for (const auto &gVertex : vertices)
    shapesOut.push_back(stow->findShapeIdRecord(gVertex).shape);
  
  return shapesOut;
}

std::vector<uuid> SeerShape::useGetChildrenOfType
  (const uuid &idIn, const TopAbs_ShapeEnum &shapeTypeIn) const
{
  assert(hasId(idIn));

  std::vector<Vertex> vertices;
  TypeCollectionVisitor vis(shapeTypeIn, *this, vertices);
  boost::breadth_first_search(stow->graph, stow->findShapeIdRecord(idIn).graphVertex, boost::visitor(vis));

  std::vector<Vertex>::const_iterator vit;
  std::vector<uuid> idsOut;
  for (vit = vertices.begin(); vit != vertices.end(); ++vit)
      idsOut.push_back(stow->findShapeIdRecord(*vit).id);
  return idsOut;
}

occt::ShapeVector SeerShape::useGetChildrenOfType
  (const TopoDS_Shape &shapeIn, const TopAbs_ShapeEnum &shapeTypeIn) const
{
  assert(hasShape(shapeIn));

  std::vector<Vertex> vertices;
  TypeCollectionVisitor vis(shapeTypeIn, *this, vertices);
  boost::breadth_first_search(stow->graph, stow->findShapeIdRecord(shapeIn).graphVertex, boost::visitor(vis));

  std::vector<Vertex>::const_iterator vit;
  occt::ShapeVector out;
  for (vit = vertices.begin(); vit != vertices.end(); ++vit)
      out.push_back(stow->findShapeIdRecord(*vit).shape);
  return out;
}

uuid SeerShape::useGetWire
  (const uuid &edgeIdIn, const uuid &faceIdIn) const
{
  assert(hasId(edgeIdIn));
  Vertex edgeVertex = stow->findShapeIdRecord(edgeIdIn).graphVertex;

  assert(hasId(faceIdIn));
  Vertex faceVertex = stow->findShapeIdRecord(faceIdIn).graphVertex;

  VertexAdjacencyIterator wireIt, wireItEnd;
  for (boost::tie(wireIt, wireItEnd) = boost::adjacent_vertices(faceVertex, stow->graph); wireIt != wireItEnd; ++wireIt)
  {
    VertexAdjacencyIterator edgeIt, edgeItEnd;
    for (boost::tie(edgeIt, edgeItEnd) = boost::adjacent_vertices((*wireIt), stow->graph); edgeIt != edgeItEnd; ++edgeIt)
    {
      if (edgeVertex == (*edgeIt))
        return stow->findShapeIdRecord(*wireIt).id;
    }
  }
  return gu::createNilId();
}

uuid SeerShape::useGetClosestWire(const uuid& faceIn, const osg::Vec3d& pointIn) const
{
  //possible speed up. if face has only one wire, don't run the distance check.
  
  TopoDS_Vertex point = BRepBuilderAPI_MakeVertex(gp_Pnt(pointIn.x(), pointIn.y(), pointIn.z()));
  
  const ShapeIdRecord &faceInRecord = stow->findShapeIdRecord(faceIn);
  Vertex faceVertex = faceInRecord.graphVertex;
  assert(faceInRecord.shape.ShapeType() == TopAbs_FACE);
  VertexAdjacencyIterator it, itEnd;
  uuid wireOut = gu::createNilId();
  double distance = std::numeric_limits<double>::max();
  for (boost::tie(it, itEnd) = boost::adjacent_vertices(faceVertex, stow->graph); it != itEnd; ++it)
  {
    const ShapeIdRecord &cRecord = stow->findShapeIdRecord(*it);
    assert(cRecord.shape.ShapeType() == TopAbs_WIRE);
    double deflection = 0.1; //hopefully makes fast. factor of bounding box?
    BRepExtrema_DistShapeShape distanceCalc(point, cRecord.shape, deflection, Extrema_ExtFlag_MIN);
    if
    (
      (!distanceCalc.IsDone()) ||
      (distanceCalc.NbSolution() < 1)
    )
      continue;
    if (distanceCalc.Value() < distance)
    {
      distance = distanceCalc.Value();
      wireOut = cRecord.id;
    }
  }
  
  return wireOut;
}

std::vector<uuid> SeerShape::useGetFacelessWires(const uuid& edgeIn) const
{
  //get first faceless wire of edge.
  std::vector<uuid> out;
  auto wires = useGetParentsOfType(edgeIn, TopAbs_WIRE);
  for (const auto &wire : wires)
  {
    auto faces = useGetParentsOfType(wire, TopAbs_FACE);
    if (faces.empty())
      out.push_back(wire);
  }
  return out;
}

bool SeerShape::useIsEdgeOfFace(const uuid& edgeIn, const uuid& faceIn) const
{
  //note edge and face might belong to totally different solids.
  
  //we know the edge will be here. Not anymore. we are now selecting
  //wires with the face first. so 'this' maybe the face feature and not the edge.
//   assert(vertexMap.count(edgeIn) > 0);
  if(!hasId(edgeIn))
    return false;
  std::vector<uuid> faceParents = useGetParentsOfType(edgeIn, TopAbs_FACE);
  std::vector<uuid>::const_iterator it = std::find(faceParents.begin(), faceParents.end(), faceIn);
  return (it != faceParents.end());
}

std::vector<osg::Vec3d> SeerShape::useGetEndPoints(const uuid &edgeIdIn) const
{
  const TopoDS_Shape &shape = findShape(edgeIdIn);
  assert(!shape.IsNull());
  assert(shape.ShapeType() == TopAbs_EDGE);
  BRepAdaptor_Curve curveAdaptor(TopoDS::Edge(shape));
  
  std::vector<osg::Vec3d> out;
  
  gp_Pnt tempPoint;
  tempPoint = curveAdaptor.Value(curveAdaptor.FirstParameter());
  out.push_back(osg::Vec3d(tempPoint.X(), tempPoint.Y(), tempPoint.Z()));
  tempPoint = curveAdaptor.Value(curveAdaptor.LastParameter());
  out.push_back(osg::Vec3d(tempPoint.X(), tempPoint.Y(), tempPoint.Z()));
  
  //I don't believe curve adaptor respects orientation.
  if(shape.Orientation() == TopAbs_REVERSED)
    std::reverse(out.begin(), out.end());
  
  return out;
}

std::vector<osg::Vec3d> SeerShape::useGetMidPoint(const uuid &edgeIdIn) const
{
  //all types of curves for mid point?
  const TopoDS_Shape &shape = findShape(edgeIdIn);
  assert(!shape.IsNull());
  assert(shape.ShapeType() == TopAbs_EDGE);
  BRepAdaptor_Curve curveAdaptor(TopoDS::Edge(shape));
  
  std::vector<osg::Vec3d> out;
  Standard_Real firstParameter = curveAdaptor.FirstParameter();
  Standard_Real lastParameter = curveAdaptor.LastParameter();
  Standard_Real midParameter = (lastParameter - firstParameter) / 2.0 + firstParameter;
  gp_Pnt tempPoint = curveAdaptor.Value(midParameter);
  out.push_back(osg::Vec3d(tempPoint.X(), tempPoint.Y(), tempPoint.Z()));
  
  return out;
}

std::vector< osg::Vec3d > SeerShape::useGetCenterPoint(const uuid& edgeIdIn) const
{
  const TopoDS_Shape &shape = findShape(edgeIdIn);
  assert(!shape.IsNull());
  assert(shape.ShapeType() == TopAbs_EDGE);
  BRepAdaptor_Curve curveAdaptor(TopoDS::Edge(shape));
  
  std::vector<osg::Vec3d> out;
  GeomAbs_CurveType curveType = curveAdaptor.GetType();
  if (curveType == GeomAbs_Circle)
  {
    gp_Circ circle = curveAdaptor.Circle();
    gp_Pnt tempPoint = circle.Location();
    out.push_back(osg::Vec3d(tempPoint.X(), tempPoint.Y(), tempPoint.Z()));
  }
  if (curveType == GeomAbs_Ellipse)
  {
    gp_Elips ellipse = curveAdaptor.Ellipse();
    gp_Pnt tempPoint = ellipse.Location();
    out.push_back(osg::Vec3d(tempPoint.X(), tempPoint.Y(), tempPoint.Z()));
  }
  
  return out;
}

std::vector< osg::Vec3d > SeerShape::useGetQuadrantPoints(const uuid &edgeIdIn) const
{
  const TopoDS_Shape &shape = findShape(edgeIdIn);
  assert(!shape.IsNull());
  assert(shape.ShapeType() == TopAbs_EDGE);
  BRepAdaptor_Curve curveAdaptor(TopoDS::Edge(shape));
  
  std::vector<osg::Vec3d> out;
  GeomAbs_CurveType curveType = curveAdaptor.GetType();
  if (curveType == GeomAbs_Circle || curveType == GeomAbs_Ellipse)
  {
    gp_Pnt tempPoint;
    tempPoint = curveAdaptor.Value(0.0);
    out.push_back(osg::Vec3d(tempPoint.X(), tempPoint.Y(), tempPoint.Z()));
    tempPoint = curveAdaptor.Value(M_PI / 2.0);
    out.push_back(osg::Vec3d(tempPoint.X(), tempPoint.Y(), tempPoint.Z()));
    tempPoint = curveAdaptor.Value(M_PI);
    out.push_back(osg::Vec3d(tempPoint.X(), tempPoint.Y(), tempPoint.Z()));
    tempPoint = curveAdaptor.Value(3.0 * M_PI / 2.0);
    out.push_back(osg::Vec3d(tempPoint.X(), tempPoint.Y(), tempPoint.Z()));
  }
  
  return out;
}

std::vector< osg::Vec3d > SeerShape::useGetNearestPoint(const uuid &shapeIn, const osg::Vec3d &pointIn) const
{
  const TopoDS_Shape &shape = findShape(shapeIn);
  assert(!shape.IsNull());
  TopAbs_ShapeEnum type = shape.ShapeType();
  assert((type == TopAbs_EDGE) || (type == TopAbs_FACE));
  if (type != TopAbs_EDGE && type != TopAbs_FACE)
    throw std::runtime_error("expection edge or face in SeerShape::useGetNearestPoint");
  
  TopoDS_Vertex vertex = BRepBuilderAPI_MakeVertex(gp_Pnt(pointIn.x(), pointIn.y(), pointIn.z()));
  std::vector<osg::Vec3d> out;
  
  BRepExtrema_DistShapeShape dist(shape, vertex, Extrema_ExtFlag_MIN);
  if (!dist.IsDone())
    return out;
  if (dist.NbSolution() < 1)
    return out;
  gp_Pnt tempPoint = dist.PointOnShape1(1);
  out.push_back(osg::Vec3d(tempPoint.X(), tempPoint.Y(), tempPoint.Z()));
  
  return out;
}

uuid SeerShape::useGetStartVertex(const uuid &edgeIdIn) const
{
  assert(hasId(edgeIdIn));
  const TopoDS_Shape& edgeShape = getOCCTShape(edgeIdIn);
  assert(edgeShape.ShapeType() == TopAbs_EDGE);
  TopoDS_Vertex v = TopExp::FirstVertex(TopoDS::Edge(edgeShape), Standard_True);
  assert(hasShape(v));
  return findId(v);
}

uuid SeerShape::useGetEndVertex(const uuid &edgeIdIn) const
{
  assert(hasId(edgeIdIn));
  const TopoDS_Shape& edgeShape = getOCCTShape(edgeIdIn);
  assert(edgeShape.ShapeType() == TopAbs_EDGE);
  TopoDS_Vertex v = TopExp::LastVertex(TopoDS::Edge(edgeShape), Standard_True);
  assert(hasShape(v));
  return findId(v);
}

occt::ShapeVector SeerShape::useGetNonCompoundChildren() const
{
  occt::ShapeVector out;
  for (auto its = boost::vertices(stow->graph); its.first != its.second; ++its.first)
  {
    const TopoDS_Shape &shape = stow->findShapeIdRecord(*its.first).shape;
    if (shape.ShapeType() != TopAbs_COMPOUND)
      continue;
    for (auto aits = boost::adjacent_vertices(*its.first, stow->graph); aits.first != aits.second; ++aits.first)
    {
      const TopoDS_Shape &subShape = stow->findShapeIdRecord(*aits.first).shape;
      if (subShape.ShapeType() == TopAbs_COMPOUND)
        continue;
      out.push_back(subShape);
    }
  }
  return out;
}

void SeerShape::shapeMatch(const SeerShape &source)
{
  typedef ShapeIdContainer::index<ShapeIdRecord::ByShape>::type List;
  const List &list = source.stow->shapeIds.get<ShapeIdRecord::ByShape>();
  
  //every feature shape has unique id even if it is the same topoDS_shape.
  //all tracking of shapes between feature will have to use evolve container.
  for (const auto &record : list)
  {
    if (!hasShape(record.shape))
      continue;
    if (!findId(record.shape).is_nil())
      continue;
    uuid freshId = gu::createNilId();
    if (hasEvolveRecordIn(record.id))
    {
      if (evolve(record.id).size() > 1)
      {
        std::cout << "WARNING: multiple evolution match in SeerShape::shapeMatch" << std::endl;
        continue;
      }
      freshId = evolve(record.id).front();
    }
    else
    {
      freshId = gu::createRandomId();
      insertEvolve(record.id, freshId);
    }
    updateId(record.shape, freshId);
  }
}

void SeerShape::uniqueTypeMatch(const SeerShape &source)
{
  static const std::vector<TopAbs_ShapeEnum> searchTypes
  {
    TopAbs_COMPOUND,
    TopAbs_COMPSOLID,
    TopAbs_SOLID,
    TopAbs_SHELL,
    TopAbs_FACE,
    TopAbs_WIRE,
    TopAbs_EDGE,
    TopAbs_VERTEX
  };
  
  //looks for a unique shape type.
  auto getUniqueRecord = [](const ShapeIdContainer &containerIn, ShapeIdRecord &recordOut, TopAbs_ShapeEnum shapeTypeIn) -> bool
  {
    std::size_t count = 0;
    
    for (const auto &record : containerIn)
    {
      if (record.shape.ShapeType() == shapeTypeIn)
      {
        ++count;
        recordOut = record;
      }
      if (count > 1)
        break;
    }
    
    if (count == 1)
      return true;
    else
      return false;
  };
  
  for (const auto &currentShapeType : searchTypes)
  {
    ShapeIdRecord sourceRecord;
    ShapeIdRecord targetRecord;
    if
    (
      (!getUniqueRecord(source.stow->shapeIds, sourceRecord, currentShapeType)) ||
      (!getUniqueRecord(stow->shapeIds, targetRecord, currentShapeType))
    )
      continue;
      
    uuid freshId = gu::createNilId();
    if (hasEvolveRecordIn(sourceRecord.id))
      freshId = evolve(sourceRecord.id).front(); //multiple returns?
    else
    {
      freshId = gu::createRandomId();
      insertEvolve(sourceRecord.id, freshId);
    }
    updateId(targetRecord.shape, freshId);
    if (currentShapeType == TopAbs_COMPOUND) //no compound of compounds? I think not.
      setRootShapeId(freshId);
  }
}

void SeerShape::outerWireMatch(const SeerShape &source)
{
  for (const auto &id : getAllShapeIds())
  {
    const ShapeIdRecord &faceRecord = stow->findShapeIdRecord(id);
    if (faceRecord.shape.ShapeType() != TopAbs_FACE)
      continue;
    const TopoDS_Shape &thisOuterWire = BRepTools::OuterWire(TopoDS::Face(faceRecord.shape));
    assert(hasShape(thisOuterWire)); //shouldn't have a result container with a face and no outer wire.
    const ShapeIdRecord &wireRecord = stow->findShapeIdRecord(thisOuterWire);
    if (!wireRecord.id.is_nil())
      continue; //only set id for nil wires.
      
    if (!hasEvolveRecordOut(faceRecord.id))
      continue;
    uuid sourceFaceId = devolve(faceRecord.id).front();
      
    //now find entries in source.
    if (!source.hasId(sourceFaceId))
      continue;
    const ShapeIdRecord &sourceFaceRecord = source.stow->findShapeIdRecord(sourceFaceId);
    const TopoDS_Shape &sourceOuterWire = BRepTools::OuterWire(TopoDS::Face(sourceFaceRecord.shape));
    assert(!sourceOuterWire.IsNull());
    auto sourceWireId = source.stow->findShapeIdRecord(sourceOuterWire).id;
    
    uuid freshId;
    if (hasEvolveRecordIn(sourceWireId))
      freshId = evolve(sourceWireId).front(); //multiple returns?
    else
    {
      freshId = gu::createRandomId();
      insertEvolve(sourceWireId, freshId);
    }
    updateId(thisOuterWire, freshId);
  }
}

void SeerShape::modifiedMatch
(
  BRepBuilderAPI_MakeShape &shapeMakerIn,
  const SeerShape &source
)
{
  typedef ShapeIdContainer::index<ShapeIdRecord::ById>::type List;
  const List &sourceList = source.stow->shapeIds.get<ShapeIdRecord::ById>();
  for (const auto &sourceRecord : sourceList)
  {
    const TopTools_ListOfShape &modifiedList = shapeMakerIn.Modified(sourceRecord.shape);
    if (modifiedList.IsEmpty())
      continue;
    TopTools_ListIteratorOfListOfShape it(modifiedList);
    //what happens here when we have more than one in the shape.
    //we end up with multiple entries in the target with the
    //same id? Evolution container? Need to consider this.
    for(;it.More(); it.Next())
    {
      //could be situations where members of the modified list
      //are not in the target container. Booleans operations have
      //this situation. In short, no assert on shape not present.
      if(!hasShape(it.Value()))
        continue;
      
      uuid freshId = gu::createNilId();
      if (hasEvolveRecordIn(sourceRecord.id))
        freshId = evolve(sourceRecord.id).front(); //multiple returns?
      else
      {
        freshId = gu::createRandomId();
        insertEvolve(sourceRecord.id, freshId);
      }
      updateId(it.Value(), freshId);
    }
  }
}

void SeerShape::derivedMatch()
{
  typedef ShapeIdContainer::index<ShapeIdRecord::ById>::type List;
  List &list = stow->shapeIds.get<ShapeIdRecord::ById>();
  occt::ShapeVector nilShapes;
  auto rangeItPair = list.equal_range(gu::createNilId());
  for (; rangeItPair.first != rangeItPair.second; ++rangeItPair.first)
    nilShapes.push_back(rangeItPair.first->shape);
  
  auto match = [&](TopAbs_ShapeEnum shapeType, TopAbs_ShapeEnum parentType)
  {
    for (const auto &shape : nilShapes)
    {
      if (shape.ShapeType() != shapeType)
        continue;
      
      bool bail = false;
      IdSet set;
      occt::ShapeVector parents = useGetParentsOfType(shape, parentType);
      for (const auto &parent : parents)
      {
        assert(hasShape(parent));
        boost::uuids::uuid id = findId(parent);
        if (id.is_nil())
        {
            std::cout << "empty parent Id in: " << BOOST_CURRENT_FUNCTION << std::endl;
            bail = true;
            break;
        }
        set.insert(id);
      }
      if (bail)
        continue;
      uuid id = gu::createNilId();
      DerivedContainer::iterator derivedIt = stow->derives.find(set);
      if (derivedIt == stow->derives.end())
      {
        id = gu::createRandomId();
        DerivedContainer::value_type newEntry(set, id);
        stow->derives.insert(newEntry);
        insertEvolve(gu::createNilId(), id);
      }
      else
      {
        id = derivedIt->second;
      }
      updateId(shape, id);
    }
  };
  
  match(TopAbs_EDGE, TopAbs_FACE);
  match(TopAbs_VERTEX, TopAbs_EDGE);
}

void SeerShape::dumpNils(const std::string &headerIn)
{
  std::ostringstream stream;
  
  typedef ShapeIdContainer::index<ShapeIdRecord::ById>::type List;
  const List &list = stow->shapeIds.get<ShapeIdRecord::ById>();
  auto rangeItPair = list.equal_range(gu::createNilId());
  for (; rangeItPair.first != rangeItPair.second; ++rangeItPair.first)
    stream << gu::idToString(rangeItPair.first->id) << "    "
    << rangeItPair.first->shape << std::endl;
  
  
  if (!stream.str().empty())
    std::cout << std::endl << headerIn << " nils" << std::endl << stream.str() << std::endl;
}

void SeerShape::dumpDuplicates(const std::string &headerIn)
{
  std::ostringstream stream;
  
  std::set<boost::uuids::uuid> processed;
  typedef ShapeIdContainer::index<ShapeIdRecord::ById>::type List;
  const List &list = stow->shapeIds.get<ShapeIdRecord::ById>();
  for (const auto &record : list)
  {
    if (processed.count(record.id) > 0)
      continue;
    std::size_t count = stow->shapeIds.count(record.id);
    if (count > 1)
    {
      processed.insert(record.id);
      stream << gu::idToString(record.id) << "    " << count << "    " << record.shape << std::endl;
    }
  }
  if (!stream.str().empty())
    std::cout << std::endl << headerIn << " duplicates" << std::endl << stream.str() << std::endl;
}

void SeerShape::ensureNoNils()
{
  occt::ShapeVector nilShapes;
  
  typedef ShapeIdContainer::index<ShapeIdRecord::ById>::type List;
  List &list = stow->shapeIds.get<ShapeIdRecord::ById>();
  auto rangeItPair = list.equal_range(gu::createNilId());
  for (; rangeItPair.first != rangeItPair.second; ++rangeItPair.first)
    nilShapes.push_back(rangeItPair.first->shape);
  
  for (const auto &shape : nilShapes)
  {
    auto freshId = gu::createRandomId();
    updateId(shape, freshId);
    insertEvolve(gu::createNilId(), freshId);
//     std::cout << "nil id of " << occt::getShapeTypeString(shape) << ", assigned to: " << gu::idToString(freshId) << std::endl;
  }
}

void SeerShape::ensureNoDuplicates()
{
  std::set<boost::uuids::uuid> processed;
  occt::ShapeVector shapes;
  typedef ShapeIdContainer::index<ShapeIdRecord::ById>::type List;
  const List &list = stow->shapeIds.get<ShapeIdRecord::ById>();
  for (const auto &record : list)
  {
    if (processed.count(record.id) > 0)
      shapes.push_back(record.shape);
    else
      processed.insert(record.id);
  }
  for (const auto &shape : shapes)
  {
    auto freshId = gu::createRandomId();
    updateId(shape, freshId);
    insertEvolve(gu::createNilId(), freshId);
  }
}

void SeerShape::faceEdgeMatch(const SeerShape &source)
{
  occt::ShapeVector nilEdges;
  typedef ShapeIdContainer::index<ShapeIdRecord::ById>::type List;
  List &list = stow->shapeIds.get<ShapeIdRecord::ById>();
  auto rangeItPair = list.equal_range(gu::createNilId());
  for (; rangeItPair.first != rangeItPair.second; ++rangeItPair.first)
  {
    if (rangeItPair.first->shape.ShapeType() != TopAbs_EDGE)
      continue;
    nilEdges.push_back(rangeItPair.first->shape);
  }
  
  for (const auto &nilEdge : nilEdges)
  {
    //get 2 parent face ids. need 2 to make edge unique.
    occt::ShapeVector parentFaces = useGetParentsOfType(nilEdge, TopAbs_FACE);
    if (parentFaces.size() != 2)
      continue;
    std::vector<uuid> parentFacesIds;
    parentFacesIds.push_back(findId(parentFaces.front()));
    parentFacesIds.push_back(findId(parentFaces.back()));
    if
    (
      (parentFacesIds.front().is_nil()) ||
      (parentFacesIds.back().is_nil())
    )
      continue;
    
    if
    (!(
      (source.hasId(parentFacesIds.front())) &&
      (source.hasId(parentFacesIds.back()))
    ))
      continue;
    
    std::vector<uuid> face1Edges = source.useGetChildrenOfType(parentFacesIds.front(), TopAbs_EDGE);
    std::vector<uuid> face2Edges = source.useGetChildrenOfType(parentFacesIds.back(), TopAbs_EDGE);
    std::vector<uuid> commonEdges;
    std::sort(face1Edges.begin(), face1Edges.end());
    std::sort(face2Edges.begin(), face2Edges.end());
    std::set_intersection
    (
      face1Edges.begin(), face1Edges.end(),
      face2Edges.begin(), face2Edges.end(), 
      std::back_inserter(commonEdges)
    );

    if (commonEdges.size() == 1)
    {
      uuid freshId = gu::createNilId();
      if (hasEvolveRecordIn(commonEdges.front()))
        freshId = evolve(commonEdges.front()).front(); //multiple returns?
      else
      {
        freshId = gu::createRandomId();
        insertEvolve(commonEdges.front(), freshId);
      }
      updateId(nilEdge, freshId);
    }
  }
}

void SeerShape::edgeVertexMatch(const SeerShape &source)
{
  using boost::uuids::uuid;
  
  occt::ShapeVector nilVertices;
  typedef ShapeIdContainer::index<ShapeIdRecord::ById>::type List;
  List &list = stow->shapeIds.get<ShapeIdRecord::ById>();
  auto rangeItPair = list.equal_range(gu::createNilId());
  for (; rangeItPair.first != rangeItPair.second; ++rangeItPair.first)
  {
    if (rangeItPair.first->shape.ShapeType() != TopAbs_VERTEX)
      continue;
    nilVertices.push_back(rangeItPair.first->shape);
  }
  
  for (const auto &nilVertex : nilVertices)
  {
    bool nilDetected = false;
    bool missingInSource = false;
    occt::ShapeVector parentEdges = useGetParentsOfType(nilVertex, TopAbs_EDGE);
    std::vector<uuid> parentEdgeIds;
    for (const auto &pEdge : parentEdges)
      parentEdgeIds.push_back(findId(pEdge));
    std::vector<std::vector<uuid> >sourceVertices;
    
    for (const auto parentEdgeId : parentEdgeIds)
    {
      if (parentEdgeId.is_nil())
      {
        nilDetected = true;
        break;
      }
      if (!source.hasId(parentEdgeId))
      {
        missingInSource = true;
        break;
      }
      
      std::vector<uuid> tempVertexIds = source.useGetChildrenOfType(parentEdgeId, TopAbs_VERTEX);
      std::sort(tempVertexIds.begin(), tempVertexIds.end());
      sourceVertices.push_back(tempVertexIds);
    }
    if (nilDetected || missingInSource || sourceVertices.size() < 2)
      continue;
    
    std::vector<uuid> intersectedVertices = sourceVertices.back();
    sourceVertices.pop_back();
    for (const auto &sourceGroup : sourceVertices)
    {
      std::vector<uuid> temp;
      std::set_intersection
      (
        sourceGroup.begin(), sourceGroup.end(),
        intersectedVertices.begin(), intersectedVertices.end(), 
        std::back_inserter(temp)
      );
      intersectedVertices = temp;
    }
    
    if (intersectedVertices.size() == 1)
    {
      uuid freshId = gu::createNilId();
      if (hasEvolveRecordIn(intersectedVertices.front()))
        freshId = evolve(intersectedVertices.front()).front(); //multiple returns?
      else
      {
        freshId = gu::createRandomId();
        insertEvolve(intersectedVertices.front(), freshId);
      }
      updateId(nilVertex, freshId);
    }
  }
}

void SeerShape::dumpGraph(const std::string &filePathIn) const
{
  std::ofstream file(filePathIn.c_str());
  boost::write_graphviz(file, stow->graph, Node_writer<Graph>(stow->graph, *this), boost::default_writer());
}

void SeerShape::dumpReverseGraph(const std::string &filePathIn) const
{
  std::ofstream file(filePathIn.c_str());
  boost::write_graphviz(file, stow->rGraph, Node_writer<Graph>(stow->rGraph, *this), boost::default_writer());
}

void SeerShape::dumpShapeIdContainer(std::ostream &streamIn) const
{
  streamIn << stow->shapeIds << std::endl;
}

void SeerShape::dumpEvolveContainer(std::ostream &streamIn) const
{
  streamIn << stow->evolves << std::endl;
}

void SeerShape::dumpFeatureTagContainer(std::ostream &streamIn) const
{
  streamIn << stow->featureTags << std::endl;
}

prj::srl::SeerShape SeerShape::serialOut()
{
  prj::srl::ShapeIdContainer shapeIdContainerOut;
  if (!rootShapeId.is_nil())
  {
    const TopoDS_Shape &shape = getRootOCCTShape();
    if (!shape.IsNull())
    {
      //if mapShapes only contains subshapes, like the doc says, we don't write out compound?
      occt::ShapeVector shapes = occt::mapShapes(shape);
      std::size_t count = 0;
      for (const auto &s : shapes)
      {
        if (!hasShape(s))
        {
          std::cerr << "WARNING: ShapeId Container doesn't have shape in SeerShape::serialOut" << std::endl;
          count++;
          continue;
        }
        prj::srl::ShapeIdRecord rRecord
        (
          gu::idToString(findId(s)),
          count
        );
        shapeIdContainerOut.shapeIdRecord().push_back(rRecord);
        count++;
      }
    }
  }
  
  prj::srl::EvolveContainer eContainerOut;
  typedef EvolveContainer::index<EvolveRecord::ByInId>::type EList;
  const EList &eList = stow->evolves.get<EvolveRecord::ByInId>();
  for (EList::const_iterator it = eList.begin(); it != eList.end(); ++it)
  {
    prj::srl::EvolveRecord eRecord
    (
      gu::idToString(it->inId),
      gu::idToString(it->outId)
    );
    eContainerOut.evolveRecord().push_back(eRecord);
  }
  
  prj::srl::FeatureTagContainer fContainerOut;
  typedef FeatureTagContainer::index<FeatureTagRecord::ById>::type FList;
  const FList &fList = stow->featureTags.get<FeatureTagRecord::ById>();
  for (FList::const_iterator it = fList.begin(); it != fList.end(); ++it)
  {
    prj::srl::FeatureTagRecord fRecord
    (
      gu::idToString(it->id),
      it->tag
    );
    fContainerOut.featureTagRecord().push_back(fRecord);
  }
  
  prj::srl::DerivedContainer dContainerOut;
  for (DerivedContainer::const_iterator dIt = stow->derives.begin(); dIt != stow->derives.end(); ++dIt)
  {
    prj::srl::IdSet setIn;
    for (IdSet::const_iterator sIt = dIt->first.begin(); sIt != dIt->first.end(); ++sIt)
      setIn.id().push_back(gu::idToString(*sIt));
    prj::srl::DerivedRecord::IdType mId(gu::idToString(dIt->second));
    prj::srl::DerivedRecord record
    (
      setIn,
      mId
    );
    
    dContainerOut.derivedRecord().push_back(record);
  }
  
  return prj::srl::SeerShape
  (
    gu::idToString(rootShapeId),
    shapeIdContainerOut,
    eContainerOut,
    fContainerOut,
    dContainerOut
  ); 
}

void SeerShape::serialIn(const prj::srl::SeerShape &sSeerShapeIn)
{
  //note: shape has already been set through setOCCTShape, so shapeIdContainer has been populated and don't clear.
  //setOCCTShape assigns an id for the root, so that is valid.
  
  //fill in shapeId container.
  occt::ShapeVector shapes = occt::mapShapes(getRootOCCTShape());
  for (const prj::srl::ShapeIdRecord &sRRecord : sSeerShapeIn.shapeIdContainer().shapeIdRecord())
  {
    std::size_t offset = sRRecord.shapeOffset();
    assert(offset < shapes.size());
    if (offset >= shapes.size())
    {
      std::cerr << "WARNING: invalid shape offset in SeerShape::serialIn" << std::endl;
      continue;
    }
    updateId(shapes.at(sRRecord.shapeOffset()), gu::stringToId(sRRecord.id()));
  }
  
  stow->evolves.get<EvolveRecord::ByInId>().clear();
  for (const prj::srl::EvolveRecord &sERecord : sSeerShapeIn.evolveContainer().evolveRecord())
  {
    EvolveRecord record;
    record.inId = gu::stringToId(sERecord.idIn());
    record.outId = gu::stringToId(sERecord.idOut());
    stow->evolves.insert(record);
  }
  
  stow->featureTags.get<FeatureTagRecord::ById>().clear();
  for (const prj::srl::FeatureTagRecord &sFRecord : sSeerShapeIn.featureTagContainer().featureTagRecord())
  {
    FeatureTagRecord record;
    record.id = gu::stringToId(sFRecord.id());
    record.tag = sFRecord.tag();
    stow->featureTags.insert(record);
  }
  
  stow->derives.clear();
  for (const prj::srl::DerivedRecord &sDRecord : sSeerShapeIn.derivedContainer().derivedRecord())
  {
    IdSet setIn;
    for (const auto &idSet : sDRecord.idSet().id())
      setIn.insert(gu::stringToId(idSet));
    boost::uuids::uuid mId = gu::stringToId(sDRecord.id());
    stow->derives.insert(std::make_pair(setIn, mId));
  }
  
  rootShapeId = gu::stringToId(sSeerShapeIn.rootShapeId());
}

std::unique_ptr<SeerShape> SeerShape::createWorkCopy() const
{
  std::unique_ptr<SeerShape> target(new SeerShape());
  
  BRepBuilderAPI_Copy copier;
  copier.Perform(getRootOCCTShape());
  target->setOCCTShape(copier.Shape());
  target->ensureNoNils(); //give all shapes a new id.
  //ensureNoNils fills in the evolve container also. we have to clear it.
  target->stow->evolves.get<EvolveRecord::ByInId>().clear();
  
  for (const auto &sourceId : getAllShapeIds())
  {
    const TopoDS_Shape &sourceShape = findShape(sourceId);
    TopoDS_Shape targetShape = copier.ModifiedShape(sourceShape);
    assert(target->hasShape(targetShape));
    uuid targetId = target->findId(targetShape);
    target->insertEvolve(sourceId, targetId);
  }
  
  return target;
}

ShapeIdHelper SeerShape::buildHelper() const
{
  //make sure we store in occt::mapshapes order
  //that is the order the external viz generation will use.
  ShapeIdHelper out;
  occt::ShapeVector shapes = occt::mapShapes(getRootOCCTShape());
  std::cout << std::endl;
  for (const auto &shape : shapes)
  {
    uuid id = findId(shape);
    out.add(id, shape);
  }
  
  return out;
}
