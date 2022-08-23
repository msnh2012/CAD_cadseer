/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2022 Thomas S. Anderson blobfish.at.gmx.com
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

#include <osg/Group>
#include <osg/Transform>

#include <Law_Constant.hxx>
#include <Law_Linear.hxx>
#include <Law_Interpol.hxx>
#include <Law_Composite.hxx>

#include "law/lwfnode.h"
#include "law/lwfgraph.h"
#include "law/lwfvessel.h"
#include "project/serial/generated/prjsrllwsplawspine.h"


/* Old law function was kept in the unit range [0,1]. This worked fine
 * for sweeps but blends expects the range to match spine. So we want
 * the range to be variable to match whatever the spine range is. That
 * probably means we will have to adapt positions of parameters from old
 * range to new range.
 * 
 * We want the user to be able to pick points along spine and assign values.
 * Old law function also let user add points described purely by a Vec3 with
 * no pick geometry. should we keep that? If all points are tied to a pick,
 * it might be cumbersome for user to change location having to re-pick?
 * Do we need 2 different kinds of points? One based off a pick and one
 * based off of a parameter. Maybe when a point is added it is a 'pick point'
 * then when dragged it is a parameter point? Maybe endpoint picks are tied
 * to geometry and nearest are tied to a parameter? Parameters are master and
 * will be used to tie values to spine. Any picks need to be resolved to spine parameters.
 * 
 * Law_S and Law_Interpol allow for assigning of slopes. that is one reason
 * we were using the Vec3 type for parameters, where x was the parameter,
 * y was the value, and z was the slope. Slope parameters are only applicable
 * for open ended versions of these 2. When these are connected to others
 * we will assign a slope from the connecting law. Note we were not even using
 * Law_S as it is the same thing as Law_Interpol with 2 points.
 * 
 * Law_composite presents some unique challenges as we want the final
 * curve to be at least c1. So we need some law_S or law_Interpol transitions
 * between constant and linear segments.
 * 
 * Law_Interpol is the only one that allows internal points. Everything else
 * just has start and end. Of course composite is a different animal all together.
 * 
 * Don't forget about periodic spines. Lets make last point value inactive.
 * 
 * Composite:
 * Probably shouldn't have Law_Interpol adjacent to another. What is the point?
 * Probably shouldn't have Linear or Constant connected to each other either.
 *   We will want to ensure c1.
 * 
 * We have points and we have a law or laws. Type of law has constraint on number of points.
 * constant: 2 points, 1 value
 * linear: 2 points, 2 values
 * interpolate: >= 2 points. 1 value for each point. end points get a slope.
 * composite: >= 2 points from sub-laws. 1 value for each point. Interpolate end points get a slope.
 *
 * updating:
 * we should scale parameter points first to reflect change in range
 * update the range. the first and last points.
 * sort the points in ascending order. this might alter the points that laws reference!! make invalid?
 * 
 * linked values:
 * In certain cases point values are linked together. List of known cases.
 * With a constant law both point values are the same.
 * With a periodic spine the first and last points have the same value.
 * Conditions can compound like, a periodic spine with a composite law that starts with a constant.
 *   In this case we have 3 points, first second and last, that share the same value
 * 
 * I originally thought linear laws required different values. I looked at the occt linear law
 * code and constructed some linear laws, and it appears linear law is fine with equal
 * values at the endpoints. So not going to have a not equal restriction on the linear law endpoint values.
 * 
 * What if we always have a composite law and other cases are handled with 1 sub-law of type?
 * 
 * periodic and C1:
 * constant is ok.
 * linear is ok, but it will be equal to a constant as the endpoints share the same value.
 * interpolate is ok. we have to have the same value and slope for the first and last points.
 * composite is ok. We have to watch first and last point value and slope like interpolate.
 *   restrictions for c1 that carry through have to extend to front and back. for example
 *   we cant' have a constant first law and a linear last law in a periodic condition.
 *   we should allow duplicate laws across period. Say, constants with same values as
 *   the first and last law.
 * 
 * I am having a hell of a time coming up with a data structure to encapsulate
 * the composite. It comes down to the topology of points being shared by
 * the laws. Thinking about getting boost::graph involved, yuck. Also
 * thinking about a variant of points and laws and then a vector of the variant.
 * 
 * Editing composite is going to be difficult trying to inform user of validity.
 * Maybe when user adds or remove points we just clear all laws and make them assign again?
 *   They won't like that but then we can verify law assigned validity as laws being added.
 * It is that or we try to 'solve' for valid law as changes are made. Not in love with that.
 * I am thinking we just let the user do what ever the hell they want and have a status
 *   message explaining valid state. So when the user adds a point in the middle of a constant
 *   we don't do any associating between the point and constant law. we just update status to
 *   reflect a point not contained in a law.
 * I do like the idea of a 2 step process where the points are defined and then laws are
 *   defined and associated to the points.
 * 
 * Can we set constraints on the parameter points(no picks) to prevent user from 
 * 'crossing the streams'?
 * 
 * Crazy? What if the user is presented with an ordered list of points and an ordered list of
 * laws and we 'solve'. Boy I really like this idea from a user perspective. Can I make this work?
 * Of the basic laws only interpolate can reference more than 2 points. So working forward or backward
 * constant and linear reference 2 points each. So the question would arise when they are more than
 * 2 interpolate laws that can reference more than 2 points. Maybe interpolate in the context of composite
 * Can have a point count preference parameter. Need to get into that. Another thing
 * I like is we have 2 simple lists for data structure containing parameters. We might still need 
 * a graph for solving but can be separate from the parameters. Without the graph we loose the nice
 * sharing of values and slopes that occurs in periodic spines. Maybe we keep the graph and blow away
 * reference edges and solve. Maybe what I am thinking about here is more of an interface not a
 * structural change?
 * 
 * Solving from list of laws and list of points.
 * quantities of laws and points:
 *   point count >= law count + 1;
 *   when point count == law count + 1, then type of laws don't matter. Every law references 2 points.
 *   when point cout > law count + 1:
 *     Then we need at least 1 interpolate to 'soak' up extra points.
 *     If we have only 1 interpolate that will 'soak' up extra points and we are solved.
 *     If we have more than 1 interpolate, then who gets extra point or points.
 *       this is where originally we had the concept of internal points.
 *       I don't like the user having to designate internal points.
 *       I think it will get overly complicated.
 * what to do?
 *   maybe have a 'forced count' variable on an interpolate law.
 *     interpolate will assume 2 points unless forced count is set.
 *     where to store that in the graph so it doesn't get lost?
 * make the user put multiple interpolates in law sequence to designate multiple points.
 *   Yes! this is exactly what we should do. then our laws to point ratio can be constant.
 * 
 * Maybe we need a state machine.
 *   Raw
 *   Points
 *   Laws
 *   Children
 *   Slopes
 *   Active
 * Then different alterations can set this state and then
 * update routines will have assurances that structure is ok
 * to operate on.
 */

namespace
{
  enum class State
  {
    Reset         //!< Nothing has been verified
    , Points    //!< 2 or more points are ordered in a single chain
    , Laws      //!< All laws have been set to a valid value
    , Children  //!< each point has 1 value, 1 slope and optionally 1 pick. Sharing is verified
    , Slopes    //!< each point slope has been set.
    , Active    //!< all parameters active state has been set.
  };
  
  bool fuzzyEqual(double d0, double d1)
  {
    if (std::fabs(d0 - d1) < std::numeric_limits<float>::epsilon())
      return true;
    return false;
  }
}

using namespace lwf;

struct Vessel::Stow
{
  bool periodic = false;
  State state = State::Reset;
  osg::ref_ptr<osg::Group> osgGroup = new osg::Group();
  Graph graph;
  QString validationText;
  Laws cachedLaws;
  
  prm::Parameters removedParameters;
  
  //fuck std::set
  std::function<bool(const Vertex&, const Vertex&)> pointPredicate = [&](const Vertex &first, const Vertex &second) -> bool
  {
    assert(graph[first].parameter.getTag() == Tags::parameter);
    assert(graph[second].parameter.getTag() == Tags::parameter);
    return graph[first].parameter.getDouble() < graph[second].parameter.getDouble();
  };
  void uniquefyPoints(Vertices &vsIn)
  {
    std::sort(vsIn.begin(), vsIn.end(), pointPredicate);
    auto last = std::unique(vsIn.begin(), vsIn.end());
    vsIn.erase(last, vsIn.end());
  }
  
  bool isPoint(Vertex v) const
  {
    if (graph[v].parameter.getTag() == Tags::parameter)
      return true;
    return false;
  }
  bool isValue(Vertex v) const
  {
    if (graph[v].parameter.getTag() == Tags::value)
      return true;
    return false;
  }
  bool isSlope(Vertex v) const
  {
    if (graph[v].parameter.getTag() == Tags::slope)
      return true;
    return false;
  }
  bool isPick(Vertex v) const
  {
    if (graph[v].parameter.getTag() == Tags::pick)
      return true;
    return false;
  }
  bool isLaw(Edge e) const
  {
    return (graph[e] & LawMask).any();
  }
  bool isChildren(Edge e) const
  {
    return (graph[e] & ChildMask).any();
  }
  
  bool mask(Edge e, EdgeMask mask) const
  {
    return (graph[e] & mask).any();
  }
  
  Vertices pointWalk()
  {
    //assumes a valid structure.
    JustPoints<Stow> justPoints(*this);
    auto justPointsGraph = boost::make_filtered_graph(graph, boost::keep_all(), justPoints);
    return std::get<Vertices>(simpleWalk<decltype(justPointsGraph)>(justPointsGraph));
  }
  
  Edges lawWalk()
  {
    JustPoints<Stow> justPoints(*this);
    auto justPointsGraph = boost::make_filtered_graph(graph, boost::keep_all(), justPoints);
    return std::get<Edges>(simpleWalk<decltype(justPointsGraph)>(justPointsGraph));
  }
  
  /* Interpolate laws are expressed in the graph by 2 or more points/vertices with 1 or more
   * edges with an InterpolateMask property. This makes iterating over laws a little cumbersome,
   * as the other laws are expressed with only 2 vertices and 1 edge.
   * This function removes all the edges of an interpolate sequence except the first edge.
   * Then we have a vector of edges that represents 1 law per edge. Users of this vector
   * must be aware of this. Functions like getLawPoints are aware of this graph anomaly
   * and behave accordingly.
   */
  Edges lawWalkCompacted()
  {
    Edges walked = lawWalk();
    
    Edges out;
    bool inInterpolate = false;
    for (auto e : walked)
    {
      if (inInterpolate)
      {
        if (graph[e] == InterpolateMask)
          continue;
        inInterpolate = false;
      }
      else if (graph[e] == InterpolateMask)
        inInterpolate = true;
      out.push_back(e);
    }
    return out;
  }
  
  Vertices getOrderedPoints()
  {
    JustPoints<Stow> justPoints(*this);
    auto jpg = boost::make_filtered_graph(graph, boost::keep_all(), justPoints);
    Vertices points;
    for (auto v : boost::make_iterator_range(boost::vertices(jpg)))
      points.push_back(v);
    uniquefyPoints(points);
    return points;
  }
  
  //empty return for invalid structure
  Vertices getLawPoints(Edge lawEdge)
  {
    assert(isLaw(lawEdge)); //don't set me up
    
    Vertices out;
    out.push_back(boost::source(lawEdge, graph));
    out.push_back(boost::target(lawEdge, graph));
    
    if (graph[lawEdge] == InterpolateMask)
    {
      JustPoints<Stow> justPoints(*this);
      auto justPointsGraph = boost::make_filtered_graph(graph, boost::keep_all(), justPoints);
      auto currentEdge = lawEdge;
      auto nextEdge = [&]() -> bool
      {
        if (boost::out_degree(out.back(), justPointsGraph) != 1)
          return false;
        auto its = boost::out_edges(out.back(), justPointsGraph);
        if (graph[*its.first] != InterpolateMask)
          return false;
        currentEdge = *its.first;
        return true;
      };
      
      while (nextEdge())
        out.push_back(boost::target(currentEdge, graph));
    }
    return out;
  }
  
  //doesn't consider periodic as that is encoded in sharing of children between end points.
  Edges getPointLaws(Vertex point)
  {
    assert(isPoint(point));
    Edges out;
    JustPoints<Stow> justPoints(*this);
    auto jpg = boost::make_filtered_graph(graph, boost::keep_all(), justPoints);
    for (auto e : boost::make_iterator_range(boost::in_edges(point, jpg)))
      out.push_back(e);
    for (auto e : boost::make_iterator_range(boost::out_edges(point, jpg)))
      out.push_back(e);
    return out;
  }
  
  Vertices getParentPoints(Vertex child)
  {
    assert(isValue(child) || isSlope(child) || isPick(child)); //need pick?
    Vertices out;
    for (auto e : boost::make_iterator_range(boost::in_edges(child, graph)))
      out.push_back(boost::source(e, graph));
    return out;
  }
  
  Vertex createPrmPoint(double prmIn)
  {
    auto npv = boost::add_vertex(Node::buildParameter(prmIn), graph);
    graph[npv].osgNode = lbr::buildGrid();
    graph[npv].osgNode->addChild(new lbr::PLabel(&graph[npv].parameter));
    return npv;
  }
  
  std::optional<Vertex> createPoint(double prmIn)
  {
    if (state < State::Points)
    {
      validationText = QObject::tr("Point not added. Bad State");
      return std::nullopt;
    }
    
    auto points = getOrderedPoints();
    auto firstParam = graph[points.front()].parameter.getDouble() + std::numeric_limits<float>::epsilon();
    auto lastParam = graph[points.back()].parameter.getDouble() - std::numeric_limits<float>::epsilon();
    if (prmIn <= firstParam || prmIn >= lastParam)
    {
      validationText = QObject::tr("Point not added. Parameter outside of range");
      return std::nullopt;
    }
    
    Vertex previous = points.front();
    for (auto v : points)
    {
      if (graph[v].parameter.getDouble() > prmIn)
        break;
      previous = v;
    }
    assert(previous != points.back());
    
    auto npv = createPrmPoint(prmIn);
    auto pv = getChildDouble(previous, Tags::value);
    if (!pv) pv = 1.0;
    auto nnv = createChildValue(npv, *pv);
    auto nsv = createChildSlope(npv, 0.0);
    
    osgGroup->addChild(graph[npv].osgNode);
    graph[npv].osgNode->addChild(graph[nnv].osgNode);
    graph[npv].osgNode->addChild(graph[nsv].osgNode);
    
    return npv;
  }
  
  Vertex createChildValue(Vertex point, double value)
  {
    assert(isPoint(point));
    auto vv = boost::add_vertex(Node::buildValue(value), graph);
    graph[vv].osgNode = new lbr::PLabel(&graph[vv].parameter);
    boost::add_edge(point, vv, ChildMask, graph);
    return vv;
  }
  
  Vertex createChildSlope(Vertex point, double value)
  {
    assert(isPoint(point));
    auto vv = boost::add_vertex(Node::buildSlope(value), graph);
    graph[vv].osgNode = new lbr::PLabel(&graph[vv].parameter);
    boost::add_edge(point, vv, ChildMask, graph);
    return vv;
  }
  
  Vertex createChildPick(Vertex point, const ftr::Pick &pick)
  {
    assert(isPoint(point));
    auto vv = boost::add_vertex(Node::buildPick(pick), graph);
    boost::add_edge(point, vv, ChildMask, graph);
    return vv;
  }
  
  void removeVertex(Vertex v)
  {
    if (!graph[v].isNew)
      removedParameters.push_back(&graph[v].parameter);
    boost::clear_vertex(v, graph);
    boost::remove_vertex(v, graph);
  }
  
  std::optional<Vertex> getChildParameterVertex(Vertex point, std::string_view tag)
  {
    assert(isPoint(point));
    for (auto c : boost::make_iterator_range(boost::adjacent_vertices(point, graph)))
    {
      if (graph[c].parameter.getTag() == tag)
        return c;
    }
    return std::nullopt;
  }
  
  std::optional<double> getChildDouble(Vertex point, std::string_view tag)
  {
    assert(isPoint(point));
    auto oc = getChildParameterVertex(point, tag);
    if (!oc)
      return std::nullopt;
    return graph[*oc].parameter.getDouble();
  }
  
  std::vector<double> getPointValues(Vertex point)
  {
    assert(isPoint(point));
    std::vector<double> out;
    out.push_back(graph[point].parameter.getDouble());
    auto ovv = getChildDouble(point, Tags::value); assert(ovv);
    out.push_back(*ovv);
    auto osv = getChildDouble(point, Tags::slope); assert(osv);
    out.push_back(*osv);
    return out;
  }
  
  //returns empty vector or size of 2 with value and slope only. no pick!
  Vertices getChildren(Vertex point)
  {
    assert(isPoint(point));
    Vertices out;
    auto ovv = getChildParameterVertex(point, Tags::value);
    if (ovv) out.push_back(*ovv);
    auto osv = getChildParameterVertex(point, Tags::slope);
    if (osv) out.push_back(*osv);
    assert(out.size() == 0 || out.size() == 2);
    return out;
  }
  
  //return the point parameter and pick parameter if the pick parameter exists. Otherwise empty vector.
  Vertices getPickPair(Vertex point)
  {
    assert(isPoint(point));
    Vertices out;
    auto opv = getChildParameterVertex(point, Tags::pick);
    if (!opv) return out;
    out.push_back(point);
    out.push_back(*opv);
    return out;
  }
  
  //makes sure each point has 1 value and 1 slope associated to it
  bool verifyChildren()
  {
    auto walkPoints = pointWalk(); assert(walkPoints.size() > 1);
    for (auto p : walkPoints)
    {
      if (getChildren(p).empty())
        return false;
    }
    return true;
  }
  
  void createShare(Vertices vsIn)
  {
    assert(vsIn.size() > 1);
    
    auto protoValue = getChildParameterVertex(vsIn.front(), Tags::value); assert(protoValue);
    auto protoSlope = getChildParameterVertex(vsIn.front(), Tags::slope); assert(protoSlope);
    vsIn.erase(vsIn.begin()); //first children are kept.
    for (auto v : vsIn)
    {
      auto vv = getChildParameterVertex(v, Tags::value); assert(vv);
      auto sv = getChildParameterVertex(v, Tags::slope); assert(sv);
      boost::add_edge(v, *protoValue, ChildMask, graph);
      boost::add_edge(v, *protoSlope, ChildMask, graph);
      removeVertex(*vv);
      removeVertex(*sv);
    }
  }
  
  //asserts all points share children
  void destroyShare(Vertices vsIn)
  {
    assert(vsIn.size() > 1);
    
    JustChild<Stow> edgeMask(*this, ChildMask);
    auto childGraph = boost::make_filtered_graph(graph, edgeMask, boost::keep_all());
    
    auto protoValue = getChildParameterVertex(vsIn.front(), Tags::value); assert(protoValue);
    auto protoSlope = getChildParameterVertex(vsIn.front(), Tags::slope); assert(protoSlope);
    double value = childGraph[*protoValue].parameter.getDouble();
    double slope = childGraph[*protoSlope].parameter.getDouble();
    //make sure this share is accurate.
    assert(boost::in_degree(*protoValue, childGraph) == vsIn.size());
    assert(boost::in_degree(*protoSlope, childGraph) == vsIn.size());
    
    vsIn.erase(vsIn.begin()); //first keeps the original
    for (auto v : vsIn)
    {
      auto ve = boost::edge(v, *protoValue, childGraph); assert(ve.second);
      boost::remove_edge(ve.first, graph);
      createChildValue(v, value);
      auto se = boost::edge(v, *protoSlope, childGraph); assert(ve.second);
      boost::remove_edge(se.first, graph);
      createChildSlope(v, slope);
    }
  }
  
  /* This only handles the situation where one point of a constant law
   * has a value and slope and the other point does not. All other
   * scenarios are ignored!
   */
  void constantShareConnect()
  {
    auto lawEdges = lawWalk(); assert(!lawEdges.empty());
    for (auto e : lawEdges)
    {
      if (graph[e] != ConstantMask)
        continue;
      auto firstPoint = boost::source(e, graph);
      auto secondPoint = boost::target(e, graph);
      //note getChildren asserts to return having size of 0 or 2.
      auto firstChildren = getChildren(firstPoint);
      auto secondChildren = getChildren(secondPoint);
      if (!firstChildren.empty() && secondChildren.empty())
      {
        //connect second point to first point children
        for (auto v : firstChildren)
          boost::add_edge(secondPoint, v, ChildMask,graph);
      }
      else if (firstChildren.empty() && !secondChildren.empty())
      {
        //connect first point to second point children
        for (auto v : secondChildren)
          boost::add_edge(firstPoint, v, ChildMask,graph);
      }
    }
  }
  
  /* This only handles the situation where one end point of a periodic spine
   * has a value and slope and the other end point does not. All other
   * scenarios are ignored!
   */
  void periodicShareConnect()
  {
    if (!periodic) return;
    auto walkPoints = pointWalk(); assert(walkPoints.size() > 1);
    //note getChildren asserts to return having size of 0 or 2.
    auto frontChildren = getChildren(walkPoints.front());
    auto backChildren = getChildren(walkPoints.back());
    
    if (!frontChildren.empty() && backChildren.empty())
    {
      //connect second point to first point children
      for (auto v : frontChildren)
        boost::add_edge(walkPoints.back(), v, ChildMask,graph);
    }
    else if (frontChildren.empty() && !backChildren.empty())
    {
      //connect first point to second point children
      for (auto v : backChildren)
        boost::add_edge(walkPoints.front(), v, ChildMask,graph);
    }
  }
  
  /* Ignores actually sharing and derives what should be shared
   * based upon points and laws and periodicity.
   */
  std::vector<Vertices> gleanSharing()
  {
    assert(state >= State::Laws);
    
    auto lawEdges = lawWalk(); assert(!lawEdges.empty());
    auto points = pointWalk(); assert(points.size() > 1);
    
    //accumulate pairs of points that should be shared.
    //std::set always seems like a good idea. Always end up fighting with it.
    std::vector<Vertices> groups;
    for (auto le : lawEdges) //get pairs of points of constant law points.
    {
      //don't need to sort because lawWalk respects point order.
      if ((graph[le] & ConstantMask).any())
        groups.push_back({boost::source(le, graph), boost::target(le, graph)});
    }
    if (periodic) //first and last should be shared.
      groups.push_back({points.front(), points.back()});// insert order obeys pointPredicate
    
    //combine groups.
    bool modified = true;
    while(modified && !groups.empty())
    {
      modified = false;
      for (auto it0 = groups.begin(); it0 != groups.end() - 1; ++it0)
      {
        for (auto it1 = it0 + 1; it1 < groups.end(); ++it1)
        {
          Vertices setUnion;
          std::set_union(it0->begin(), it0->end(), it1->begin(), it1->end(), std::back_inserter(setUnion));
          uniquefyPoints(setUnion);
          if (setUnion.size() != it0->size() + it1->size())
          {
            *it0 = setUnion;
            groups.erase(it1);
            modified = true;
            break;
          }
        }
        if (modified)
          break;
      }
    }
    std::sort(groups.begin(), groups.end());
    return groups;
  }
  
  /* get the current sharing.
   * going to walk the points in order to be consistent with
   * gleanSharing.
   */
  std::vector<Vertices> presentSharing()
  {
    std::vector<Vertices> out;
    auto points = pointWalk();
    JustChild<Stow> edgeMask(*this, ChildMask);
    auto childGraph = boost::make_filtered_graph(graph, edgeMask, boost::keep_all());
    std::set<Vertex> processed;
    for (auto p : points)
    {
      if (processed.count(p) != 0)
        continue;
      auto ovc = getChildParameterVertex(p, Tags::value); assert(ovc);
      Vertices parentPoints;
      for (auto ie : boost::make_iterator_range(boost::in_edges(*ovc, childGraph)))
        parentPoints.push_back(boost::source(ie, childGraph));
      if (parentPoints.size() > 1)
      {
        uniquefyPoints(parentPoints);
        for (auto pp : parentPoints)
          processed.insert(pp);
        out.push_back(parentPoints);
      }
    }
    std::sort(out.begin(), out.end());
    return out;
  }
  
  /* I think I have to do this all at once.
   * 2 conditions that affect sharing and they compound together.
   *   Constant law points
   *   first and last points of a periodic spine.
   * When splitting, who gets original and who gets new?
   *   point with lowest parameter has preference.
   * When merging, which children remove vs what children are removed?
   *   same as above. point with lowest parameter has preference.
   * note: Vertices are sorted by point parameters. see uniquefyPoints
   *   std::vector<Vertices> is sorted using Vertex/std::size_t
   */
  void resolveSharing()
  {
    assert(state >= State::Laws);
    auto gleanGroups = gleanSharing();
    auto presentGroups = presentSharing();
    
    if (gleanGroups == presentGroups) return;
    
    std::vector<Vertices> gleanCleaned, presentCleaned;
    std::set_difference(gleanGroups.begin(), gleanGroups.end(), presentGroups.begin(), presentGroups.end(), std::back_inserter(gleanCleaned));
    std::set_difference(presentGroups.begin(), presentGroups.end(), gleanGroups.begin(), gleanGroups.end(), std::back_inserter(presentCleaned));
    
//     outputGraphviz(graph, "aboutToResolveSharing.dot");
//     std::cout << std::endl;
//     for (const auto &vs : presentCleaned)
//     {
//       std::cout << "destroy share of: ";
//       for (auto v : vs)
//         std::cout << graph[v].parameter.getDouble() << "  ";
//     }
//     std::cout << std::endl;
    
    for (const auto &vs : presentCleaned)
      destroyShare(vs);
    
//     outputGraphviz(graph, "resolveSharingPostDestroy.dot");
//     for (const auto &vs : gleanCleaned)
//     {
//       std::cout << "create share of: ";
//       for (auto v : vs)
//         std::cout << graph[v].parameter.getDouble() << "  ";
//     }
//     std::cout << std::endl;
    
    for (const auto &vs : gleanCleaned)
      createShare(vs);
    
//     outputGraphviz(graph, "resolveSharingPostCreate.dot");
    
    return;
  }
  
  void clearLawEdges()
  {
    JustPoints<Stow> justPoints(*this);
    auto jpg = boost::make_filtered_graph(graph, boost::keep_all(), justPoints);
    Edges etr;
    for (auto e : boost::make_iterator_range(boost::edges(jpg)))
      etr.push_back(e);
    for (auto e : etr)
      boost::remove_edge(e, graph);
  }
  
  void reset()
  {
    validationText.clear();
    clearLawEdges();
    state = State::Reset;
  }
  
  /* clears all law edges.
   * makes sure we at least 2 points
   * makes sure we have unique parameters for points
   */
  void goPoints()
  {
    if (state != State::Reset)
      return;
    
    clearLawEdges();
    
    Vertices points;
    auto fuzzyHas = [&](Vertex v) -> bool
    {
      double vIn = graph[v].parameter.getDouble();
      for (auto pv : points)
      {
        double pvd = graph[pv].parameter.getDouble();
        if (fuzzyEqual(pvd, vIn))
          return true;
      }
      return false;
    };
    
    JustPoints<Stow> justPoints(*this);
    auto jpg = boost::make_filtered_graph(graph, boost::keep_all(), justPoints);
    for (auto v : boost::make_iterator_range(boost::vertices(jpg)))
    {
      if (fuzzyHas(v))
      {
        validationText = QObject::tr("Points are coincident");
        return;
      }
      points.push_back(v);
    }
    
    if (points.size() < 2)
    {
      validationText = QObject::tr("Not enough points defined");
      return;
    }
    
    state = State::Points;
  }
  
  void goLaws()
  {
    if (state != State::Points)
      return;
    
    clearLawEdges();
    
    if (cachedLaws.empty())
    {
      validationText = QObject::tr("No law assigned");
      return;
    }
    if (cachedLaws.size() > 1)
    {
      //detect c0
      auto isC0 = [](Law l0, Law l1) -> bool
      {
        if
        (
          (l0 == Law::Constant && l1 == Law::Linear)
          || (l0 == Law::Linear && l1 == Law::Constant)
          || (l0 == Law::Linear && l1 == Law::Linear)
        )
          return true;
        return false;
      };
      auto previousLaw = cachedLaws.front();
      for (auto currentIt = cachedLaws.begin() + 1; currentIt != cachedLaws.end(); ++currentIt)
      {
        if (isC0(previousLaw, *currentIt))
        {
          validationText = QObject::tr("C0 condition detected");
          return;
        }
        if ((previousLaw == *currentIt) && (previousLaw != Law::Interpolate))
        {
          validationText = QObject::tr("Duplicate law in sequence");
          return;
        }
        previousLaw = *currentIt;
      }
      if (periodic && isC0(cachedLaws.back(), cachedLaws.front()))
      {
        validationText = QObject::tr("C0 condition detected at period");
        return;
      }
    }
    
    Vertices points = getOrderedPoints();
    if (points.size() != cachedLaws.size() + 1)
    {
      validationText = QObject::tr("Wrong Point Law Count");
      return;
    }
    
    auto currentIt = points.begin();
    auto nextIt = currentIt + 1;
    auto lawIt = cachedLaws.begin();
    while (nextIt != points.end() && lawIt != cachedLaws.end())
    {
      boost::add_edge(*currentIt, *nextIt, lawToMask(*lawIt), graph);
      currentIt = nextIt;
      nextIt++;
      lawIt++;
    }
    
    state = State::Laws;
  }
  
  void goChildren()
  {
    if (state != State::Laws)
      return;
    
    constantShareConnect();
    periodicShareConnect();
    
    if (!verifyChildren())
    {
      validationText = QObject::tr("Programmer error: Point to value/slope association is invalid.");
      return;
    }
    //now we can be sure that every point has a value and slope associated to it.
    resolveSharing();
    if (!verifyChildren())
    {
      validationText = QObject::tr("Programmer error: Point to value/slope association is invalid after sharing resolution");
      return;
    }
    
    //rework osg.
    osgGroup->removeChildren(0, osgGroup->getNumChildren());
    auto points = pointWalk();
    for (auto v : points)
    {
      auto grid = graph[v].osgNode;
      grid->removeChildren(1, grid->getNumChildren() - 1);
      auto ovv = getChildParameterVertex(v, Tags::value); assert(ovv);
      grid->addChild(graph[*ovv].osgNode);
      auto osv = getChildParameterVertex(v, Tags::slope); assert(osv);
      grid->addChild(graph[*osv].osgNode);
      auto *cb = dynamic_cast<lbr::PLabelGridCallback*>(grid->getUpdateCallback()); assert(cb);
      cb->setDirtyParameters();
      osgGroup->addChild(grid);
    }
    
    state = State::Children;
  }
  
  //walk laws and assign slopes for constant and linear.
  //ignores c0 condition and last law overwrites
  void goSlopes()
  {
    if (state != State::Children)
      return;
    
    for (auto lv : lawWalk())
    {
      Vertices points;
      double slope = 0.0;
      if (graph[lv] == ConstantMask)
      {
        points = getLawPoints(lv);
        slope = 0.0;
      }
      else if (graph[lv] == LinearMask)
      {
        points = getLawPoints(lv);
        double dx = graph[points.back()].parameter.getDouble() - graph[points.front()].parameter.getDouble();
        auto ov0 = getChildDouble(points.front(), Tags::value);
        auto ov1 = getChildDouble(points.back(), Tags::value);
        assert(ov0 && ov1);
        double dy = *ov1 - *ov0;
        if (dy != 0.0)
          slope = dy / dx;
      }
      for (auto p : points)
      {
        auto os = getChildParameterVertex(p, Tags::slope);
        if (os) graph[*os].parameter.setValue(slope);
      }
    }
    state = State::Slopes;
  }
  
  //walk points and determine the active state for all parameters.
  void goActiveStates()
  {
    if (state != State::Slopes)
      return;
    
    //parameters
    auto points = pointWalk();
    for (auto p : points)
    {
      auto opv = getChildParameterVertex(p, Tags::pick);
      if (opv)
        graph[p].parameter.setActive(false);
      else
        graph[p].parameter.setActive(true);
    }
    //first and last point parameters are always inactive. They are controlled by spine ends.
    graph[points.front()].parameter.setActive(false);
    graph[points.back()].parameter.setActive(false);
    
    //values always active
    for (auto v : boost::make_iterator_range(boost::vertices(graph)))
    {
      if (isValue(v))
        graph[v].parameter.setActive(true);
    }
    
    //slope is more complicated.
    for (auto v : boost::make_iterator_range(boost::vertices(graph)))
    {
      if (!isSlope(v))
        continue;
      
      //get laws associated to a point
      auto parentPoints = getParentPoints(v);
      Edges laws;
      for (auto p : parentPoints)
      {
        auto tls = getPointLaws(p);
        laws.insert(laws.end(), tls.begin(), tls.end());
      }
      auto lit = std::unique(laws.begin(), laws.end());
      laws.erase(lit, laws.end());
      
      //tally types
      int constantCount = 0;
      int linearCount = 0;
      int interpolateCount = 0;
      for (auto e : laws)
      {
        if (graph[e] == ConstantMask)
          constantCount++;
        else if (graph[e] == LinearMask)
          linearCount++;
        else if (graph[e] == InterpolateMask)
          interpolateCount++;
      }
      
      //any slope associated to constant or linear law, is inactive.
      if (constantCount != 0 || linearCount != 0)
      {
        graph[v].parameter.setActive(false);
        continue;
      }
      
      //interpolate at beginning or end of spine, is active.
      if (interpolateCount == 1)
      {
        graph[v].parameter.setActive(true);
        continue;
      }
      
      //internal interpolate point is inactive.
      if (interpolateCount == 2 && parentPoints.size() == 1)
      {
        graph[v].parameter.setActive(false);
        continue;
      }
      
      //interpolate at front and back of periodic spine, is active.
      if (interpolateCount == 2 && parentPoints.size() == 2)
      {
        graph[v].parameter.setActive(true);
        continue;
      }
      
      //at this point I am lost.
      validationText = QObject::tr("Programmer Error: Baffled by slope state");
      return;
    }
    validationText = QObject::tr("OK");
    state = State::Active;
  }
  
  void process()
  {
    goPoints();
    goLaws();
    goChildren();
    goSlopes();
    goActiveStates();
  }
};

Vessel::Vessel() : stow(std::make_unique<Stow>())
{
  //setup a dummy constant law from 0.0 to 1.0 with a value of 1.0;
//   resetConstant(0.0, 1.0, 1.0);
}

Vessel::~Vessel() = default;

//returns true if periodicity changed.
bool Vessel::setPeriodic(bool pIn)
{
  if (pIn == stow->periodic)
    return false;
  stow->periodic = pIn;
  stow->state = std::min(stow->state, State::Laws);
  stow->process();
  return true;
}

//returns true if range is changed. false otherwise.
bool Vessel::setRange(double start, double finish)
{
  //We are going to project the parameter values from old to new.
  //We are going to ignore whether the points are established from picks or not.
  //  That will ensure this never makes points out of order.
  //So we will want to call this before we resolve picks and establish parameter values from that.
  //We are assuming a valid structure.
  assert(start < finish);
  
  if (stow->state < State::Points)
  {
    stow->validationText = QObject::tr("Bad input state for set range");
    return false;
  }
  Vertices points = stow->getOrderedPoints();
  assert(points.size() > 1);
  double oldStart = stow->graph[points.front()].parameter.getDouble();
  double oldFinish = stow->graph[points.back()].parameter.getDouble();
  
  //don't do anything if range is the same.
  if (fuzzyEqual(start, oldStart) && fuzzyEqual(finish, oldFinish))
    return false;
  
  //I think start will always be zero ... right? pretending it is not.
  double offset = start - oldStart;
  double scale = (finish - start) / (oldFinish - oldStart);
  for (auto p : points)
  {
    auto &param = stow->graph[p].parameter;
    param.setValue(param.getDouble() * scale + offset);
  }
  //we don't alter any structure so no need to set state
  return true;
}

void Vessel::setLaws(const Laws &lawsIn)
{
  stow->cachedLaws = lawsIn;
  stow->state = std::min(stow->state, State::Points);
  stow->process();
}

bool Vessel::addPoint(double prmIn)
{
  auto opv = stow->createPoint(prmIn);
  if (!opv) return false; //createPoint sets validation text if error.
  
  stow->reset();
  stow->process();
  return true;
}

/*! @brief Add a new point based upon a pick
 * @return true = point added, false otherwise.
 * @details Add a new point based upon a pick.
 * pick and parameter expected to be in sync.
 */
bool Vessel::addPoint(const ftr::Pick &pIn, double prmIn)
{
  auto opv = stow->createPoint(prmIn);
  if (!opv) return false; //createPoint sets validation text if error.
  
  stow->createChildPick(*opv, pIn);
  
  stow->reset();
  stow->process();
  return true;
}

bool Vessel::removePoint(int index)
{
  auto points = stow->getOrderedPoints();
  if (index < 0 || index >= static_cast<int>(points.size()))
  {
    stow->validationText = QObject::tr("Point not removed. Out of range");
    return false;
  }
  
  JustChild<Stow> edgeMask(*stow, ChildMask);
  auto childGraph = boost::make_filtered_graph(stow->graph, edgeMask, boost::keep_all());
  
  Vertices verticesToRemove;
  Edges edgesToRemove;
  for (auto e : boost::make_iterator_range(boost::out_edges(points.at(index), childGraph)))
  {
    auto child = boost::target(e, childGraph);
    if (boost::in_degree(child, childGraph) == 1)
      verticesToRemove.push_back(child);
    else
      edgesToRemove.push_back(e);
  }
  
  stow->osgGroup->removeChild(stow->graph[points.at(index)].osgNode);
  for (auto e : edgesToRemove)
    boost::remove_edge(e, stow->graph);
  for (auto v : verticesToRemove)
    stow->removeVertex(v);
  stow->removeVertex(points.at(index));
  stow->reset();
  stow->process();
  return true;
}

bool Vessel::togglePoint(int index, const ftr::Pick &pIn)
{
  auto points = stow->getOrderedPoints();
  if (index < 0 || index >= static_cast<int>(points.size()))
    return false;
  auto opv = stow->getChildParameterVertex(points.at(index), Tags::pick);
  if (opv)
    stow->removeVertex(*opv);
  else
    stow->createChildPick(points.at(index), pIn);
  stow->state = std::min(stow->state, State::Laws);
  stow->process();
  return true;
}

/*! @brief reset to constant with given range and value.
 * @details reset to constant with given range and value.
 * @param[in] rangeBegin is the lowest parameter.
 * @param[in] rangeEnd is the highest parameter.
 * @param[in] value is the constant value.
 * @note periodic is not relevant.
 */
void Vessel::resetConstant(double rangeBegin, double rangeEnd, double value)
{
  stow->graph.clear(); //I think that is ok as we are adding all new stuff.
  
  auto pn0v = stow->createPrmPoint(rangeBegin);
  auto pn1v = stow->createPrmPoint(rangeEnd);
  boost::add_edge(pn0v, pn1v, ConstantMask, stow->graph);
  stow->cachedLaws.push_back(Law::Constant);
  
  //add 1 value node and 1 slope node that are shared to parameter nodes.
  auto vv = stow->createChildValue(pn0v, value);
  boost::add_edge(pn1v, vv, ChildMask, stow->graph);
  auto sv = stow->createChildSlope(pn0v, 0.0);
  boost::add_edge(pn1v, sv, ChildMask, stow->graph);
  
  stow->reset();
  stow->process();
}

void Vessel::forceReprocess()
{
  stow->reset();
  stow->process();
}

Law Vessel::getLawType() const
{
  assert(stow->state >= State::Laws);
  
  auto compacted = stow->lawWalkCompacted(); assert(!compacted.empty());
  if (compacted.size() == 1)
  {
    if (stow->graph[compacted.front()] == ConstantMask)
      return Law::Constant;
    else if (stow->graph[compacted.front()] == LinearMask)
      return Law::Linear;
    else if (stow->graph[compacted.front()] == InterpolateMask)
      return Law::Interpolate;
    else
      assert(0); //undefined single law.
  }
  else
    return Law::Composite;
}


int Vessel::getPointCount() const
{
  int out = 0;
  for (auto v : boost::make_iterator_range(boost::vertices(stow->graph)))
  {
    if (stow->isPoint(v))
      out++;
  }
  return out;
}

prm::Parameters Vessel::getPointParameters(int index) const
{
  prm::Parameters out;
  if (stow->state < State::Children)
  {
    stow->validationText = QObject::tr("Invalid state to get point parameters");
    return out;
  }
  auto points = stow->pointWalk();
  for (auto it = 0; it < static_cast<int>(points.size()); ++it)
  {
    if (it == index)
    {
      //doing it this way to ensure consistent ordering of the parameters.
      out.push_back(&stow->graph[points.at(it)].parameter);
      auto ovv = stow->getChildParameterVertex(points.at(it), Tags::value); assert(ovv);
      out.push_back(&stow->graph[*ovv].parameter);
      auto osv = stow->getChildParameterVertex(points.at(it), Tags::slope); assert(osv);
      out.push_back(&stow->graph[*osv].parameter);
      auto opv = stow->getChildParameterVertex(points.at(it), Tags::pick);
      if (opv)
        out.push_back(&stow->graph[*opv].parameter);
      break;
    }
  }
  return out;
}

/*! @brief get pairs of parameters and picks for updating parameter from pick.
 * @details get pairs of parameters and picks for updating parameter from pick.
 * @return vector of parameter pairs. first will be parameter, second is pick.
 * Resets state, if any pairs, to handle points crossing laws. Call 'forceReprocess' after
 * updating parameters to ensure state.
 */
std::vector<prm::Parameters> Vessel::getPickPairs() const
{
  std::vector<prm::Parameters> out;
  if (stow->state < State::Children)
  {
    stow->validationText = QObject::tr("Invalid state to retrieve pick pairs");
    return out;
  }
  
  auto points = stow->pointWalk();
  for (auto p : points)
  {
    auto pairs = stow->getPickPair(p);
    if (pairs.empty()) continue; assert(pairs.size() == 2);
    out.push_back({&stow->graph[pairs.front()].parameter, &stow->graph[pairs.back()].parameter});
  }
  
  if (!out.empty()) stow->reset();
  return out;
}

std::vector<std::pair<prm::Parameter*, osg::Transform*>> Vessel::getGridPairs() const
{
  std::vector<std::pair<prm::Parameter*, osg::Transform*>> out;
  if (stow->state < State::Children)
  {
    stow->validationText = QObject::tr("Invalid state to retrieve grid pairs");
    return out;
  }
  
  auto points = stow->pointWalk();
  for (auto p : points)
    out.push_back(std::make_pair(&stow->graph[p].parameter, stow->graph[p].osgNode.get()));
  
  return out;
}

const Laws& Vessel::getLaws() const
{
  return stow->cachedLaws;
}

/* Interpol and composite take a periodic boolean value. An Interpol
 * as an entry in a composite should never be set to periodic. The
 * composite will be set to periodic if applicable in that case. An
 * Interpol, by itself, will be set to periodic if applicable. Structure
 * prevents the case of a composite law containing a single law. So
 * we don't need to worry about setting periodic in that case.
 */
opencascade::handle<Law_Function> Vessel::buildLawFunction() const
{
  assert(isValid());
  
  auto buildConstant = [&](Vertex p0, Vertex p1) -> opencascade::handle<Law_Constant>
  {
    double x0 = stow->graph[p0].parameter.getDouble();
    double x1 = stow->graph[p1].parameter.getDouble();
    auto children = stow->getChildren(p0); assert(children.size() == 2);
    double y0 = stow->graph[children.front()].parameter.getDouble();
    opencascade::handle<Law_Constant> t = new Law_Constant();
    t->Set(y0, x0, x1);
    return t;
  };
  
  auto buildLinear = [&](Vertex p0, Vertex p1) -> opencascade::handle<Law_Linear>
  {
    double x0 = stow->graph[p0].parameter.getDouble();
    auto c0 = stow->getChildren(p0); assert(c0.size() == 2);
    double y0 = stow->graph[c0.front()].parameter.getDouble();
    
    double x1 = stow->graph[p1].parameter.getDouble();
    auto c1 = stow->getChildren(p1); assert(c1.size() == 2);
    double y1 = stow->graph[c1.front()].parameter.getDouble();
    
    opencascade::handle<Law_Linear> t = new Law_Linear();
    t->Set(x0, y0, x1, y1);
    return t;
  };
  
  auto buildInterpolate = [&] (const Vertices &vertices, bool periodic) -> opencascade::handle<Law_Interpol>
  {
    TColgp_Array1OfPnt2d points(1, vertices.size());
    int index = 1;
    for (auto v : vertices)
    {
      double x = stow->graph[v].parameter.getDouble();
      auto oy = stow->getChildDouble(v, Tags::value); assert(oy);
      points.ChangeValue(index) = gp_Pnt2d(x, *oy);
      ++index;
    }
    auto os0 = stow->getChildDouble(vertices.front(), Tags::slope); assert(os0);
    auto os1 = stow->getChildDouble(vertices.back(), Tags::slope); assert(os1);
    
    opencascade::handle<Law_Interpol> t = new Law_Interpol();
    t->Set(points, *os0, *os1, periodic);
    return t;
  };
  
  opencascade::handle<Law_Function> out;
  auto lawEdges = stow->lawWalkCompacted();
  switch(getLawType())
  {
    case Law::Constant:
    {
      assert(lawEdges.size() == 1);
      auto points = stow->getLawPoints(lawEdges.front()); assert(points.size() == 2);
      out = buildConstant(points.front(), points.back());
      break;
    }
    case Law::Linear:
    {
      assert(lawEdges.size() == 1);
      auto points = stow->getLawPoints(lawEdges.front()); assert(points.size() == 2);
      out = buildLinear(points.front(), points.back());
      break;
    }
    case Law::Interpolate:
    {
      assert(lawEdges.size() == 1);
      auto points = stow->getLawPoints(lawEdges.front()); assert(points.size() >= 2);
      out = buildInterpolate(points, stow->periodic);
      break;
    }
    case Law::Composite:
    {
      assert(lawEdges.size() > 1);
      opencascade::handle<Law_Composite> t = new Law_Composite();
      for (auto le : lawEdges)
      {
        auto points = stow->getLawPoints(le); assert(points.size() >= 2);
        if (stow->graph[le] == ConstantMask)
          t->ChangeLaws().Append(buildConstant(points.front(), points.back()));
        else if (stow->graph[le] == LinearMask)
          t->ChangeLaws().Append(buildLinear(points.front(), points.back()));
        else if (stow->graph[le] == InterpolateMask)
          t->ChangeLaws().Append(buildInterpolate(points, false));
      }
      
      //call value so composite is initialized or bounds don't get calculated and we get 'infinite boundary'
      auto points = stow->pointWalk();
      double p0 = stow->graph[points.front()].parameter.getDouble();
      double p1 = stow->graph[points.back()].parameter.getDouble();
      double mid = (p1 - p0) / 2.0 + p0;
      assert(mid > std::numeric_limits<float>::epsilon());
      t->Value(mid);
      
      out = t;
      break;
    }
  }
  
  return out;
}

std::vector<osg::Vec3d> Vessel::mesh(int steps) const
{
  std::vector<osg::Vec3d> out;
  auto addConstantLinear = [&](Edge e)
  {
    auto points = stow->getLawPoints(e); assert(points.size() == 2);
    auto p0 = stow->getPointValues(points.front());
    auto p1 = stow->getPointValues(points.back());
    out.push_back(osg::Vec3d(p0.at(0), p0.at(1), 0.0));
    out.push_back(osg::Vec3d(p1.at(0), p1.at(1), 0.0));
  };
  auto addInterpolate = [&](Edge e)
  {
    //going to actual build the law so we can use.
    auto vertices = stow->getLawPoints(e); assert(vertices.size() >= 2);
    
    TColgp_Array1OfPnt2d points(1, vertices.size());
    int index = 1;
    for (auto v : vertices)
    {
      double x = stow->graph[v].parameter.getDouble();
      auto oy = stow->getChildDouble(v, Tags::value); assert(oy);
      points.ChangeValue(index) = gp_Pnt2d(x, *oy);
      ++index;
    }
    auto os0 = stow->getChildDouble(vertices.front(), Tags::slope); assert(os0);
    auto os1 = stow->getChildDouble(vertices.back(), Tags::slope); assert(os1);
    
    opencascade::handle<Law_Interpol> t = new Law_Interpol();
    t->Set(points, *os0, *os1, false);
    
    double pf, pl;
    t->Bounds(pf, pl);
    double stepDistance = (pl - pf) / steps;
    //number of points = steps + 1. Notice '<='
    for (int index = 0; index <= steps; ++index)
    {
      double cx = pf + stepDistance * static_cast<double>(index);
      double cy = t->Value(cx);
      out.push_back(osg::Vec3d(cx, cy, 0.0));
    }
  };
  
  auto lawEdges = stow->lawWalkCompacted();
  switch(getLawType())
  {
    case Law::Constant:
    case Law::Linear:
    {
      assert(lawEdges.size() == 1);
      addConstantLinear(lawEdges.front());
      break;
    }
    case Law::Interpolate:
    {
      assert(lawEdges.size() == 1);
      auto points = stow->getLawPoints(lawEdges.front()); assert(points.size() >= 2);
      addInterpolate(lawEdges.front());
      break;
    }
    case Law::Composite:
    {
      assert(lawEdges.size() > 1);
      for (auto le : lawEdges)
      {
        if (stow->graph[le] == ConstantMask || stow->graph[le] == LinearMask)
        {
          if (!out.empty()) out.pop_back();
          addConstantLinear(le);
        }
        else if (stow->graph[le] == InterpolateMask)
        {
          if (!out.empty()) out.pop_back();
          addInterpolate(le);
        }
        else
          assert(0); //WTF: unrecognized edge property mask
      }
      break;
    }
  }
  
  return out;
}

osg::Group* Vessel::getOsgGroup() const
{
  return stow->osgGroup.get();
}

bool Vessel::isValid() const
{
  return stow->state == State::Active;
}

int Vessel::getInternalState() const
{
  return static_cast<int>(stow->state);
}

void Vessel::outputGraphviz(std::string_view filePath) const
{
  lwf::outputGraphviz<decltype(stow->graph)>(stow->graph, filePath);
}

const QString& Vessel::getValidationText() const
{
  return stow->validationText;
}

prm::Parameters Vessel::getNewParameters() const
{
  prm::Parameters out;
  for (auto v : boost::make_iterator_range(boost::vertices(stow->graph)))
  {
    if (stow->graph[v].isNew)
      out.push_back(&stow->graph[v].parameter);
  }
  return out;
}

void Vessel::resetNew()
{
  for (auto v : boost::make_iterator_range(boost::vertices(stow->graph)))
    stow->graph[v].isNew = false;
}

/*! @brief get list of obsolete parameters.
 * @return vector of parameter pointers.
 * @details get list of obsolete parameters.
 * Watch out parameters are removed from internal structure and
 * pointers point to freed memory.
 */
const prm::Parameters& Vessel::getRemovedParameters() const
{
  return stow->removedParameters;
}

void Vessel::resetRemoved()
{
  stow->removedParameters.clear();
}

prj::srl::lwsp::Vessel Vessel::serialOut()
{
  prj::srl::lwsp::Graph graphOut;
  int vIndex = 0;
  std::map<Vertex, int> vertexIndexMap;
  for (auto v : boost::make_iterator_range(boost::vertices(stow->graph)))
  {
    auto serialParameter = stow->graph[v].parameter.serialOut();
    prj::srl::lwsp::GraphNode::LocationOptional location;
    prj::srl::lwsp::GraphNode::PLabelOptional pLabel;
    if (stow->isPoint(v)) //first child of point lbr::Grid is parameter plabel.
    {
      osg::Vec3d tl = stow->graph[v].osgNode->asAutoTransform()->getPosition();
      location = prj::srl::lwsp::GraphNode::LocationType(tl.x(), tl.y(), tl.z());
      pLabel = dynamic_cast<const lbr::PLabel*>(stow->graph[v].osgNode->getChild(0))->serialOut();
    }
    else if (stow->graph[v].osgNode)
      pLabel = dynamic_cast<const lbr::PLabel*>(stow->graph[v].osgNode.get())->serialOut();
    prj::srl::lwsp::GraphNode outNode(stow->graph[v].parameter.serialOut());
    outNode.location() = location;
    outNode.pLabel() = pLabel;
    graphOut.nodes().push_back(outNode);
    vertexIndexMap.insert(std::make_pair(v, vIndex));
    vIndex++;
  }
  
  for (auto e : boost::make_iterator_range(boost::edges(stow->graph)))
  {
    auto sourceIndex = vertexIndexMap[boost::source(e, stow->graph)];
    auto targetIndex = vertexIndexMap[boost::target(e, stow->graph)];
    auto mask = static_cast<int>(stow->graph[e].to_ulong());
    graphOut.edges().push_back(prj::srl::lwsp::GraphEdge(sourceIndex, targetIndex, mask));
  }
  
  prj::srl::lwsp::Vessel out
  (
    stow->periodic
    , static_cast<int>(stow->state)
    , graphOut
  );
  
  for (auto l : stow->cachedLaws)
    out.laws().push_back(static_cast<int>(l));
  
  return out;
}

void Vessel::serialIn(const prj::srl::lwsp::Vessel &vIn)
{
  stow->periodic = vIn.periodic();
  stow->state = static_cast<State>(vIn.state());
  
  stow->graph.clear();
  int vIndex = 0;
  std::map<int, Vertex> vertexIndexMap;
  for (const auto &sn : vIn.graph().nodes())
  {
    //what about putting together osg nodes?
    auto v = boost::add_vertex(Node(sn.parameter()), stow->graph);
    if (stow->isPoint(v))
    {
      assert(sn.location());
      auto *grid = lbr::buildGrid();
      grid->setPosition(osg::Vec3d(sn.location().get().x(), sn.location().get().y(), sn.location().get().z()));
      stow->graph[v].osgNode = grid;
      
      assert(sn.pLabel());
      lbr::PLabel *label = new lbr::PLabel(&stow->graph[v].parameter);
      label->serialIn(sn.pLabel().get());
      stow->graph[v].osgNode->addChild(label);
    }
    else if (stow->isValue(v))
    {
      assert(!sn.location());
      assert(sn.pLabel());
      lbr::PLabel *label = new lbr::PLabel(&stow->graph[v].parameter);
      label->serialIn(sn.pLabel().get());
      stow->graph[v].osgNode = label;
    }
    else if (stow->isSlope(v))
    {
      assert(!sn.location());
      assert(sn.pLabel());
      lbr::PLabel *label = new lbr::PLabel(&stow->graph[v].parameter);
      label->serialIn(sn.pLabel().get());
      stow->graph[v].osgNode = label;
    }
    else if (stow->isPick(v))
    {
      assert(!sn.location());
      assert(!sn.pLabel());
    }
    else
      assert(0); //wtf don't understand node type.
    
    vertexIndexMap.insert(std::make_pair(vIndex, v));
    vIndex++;
  }
  
  for (const auto &se : vIn.graph().edges())
  {
    auto edgeMask = static_cast<EdgeMask>(se.mask());
    auto source = vertexIndexMap[se.source()];
    auto target = vertexIndexMap[se.target()];
    boost::add_edge(source, target, edgeMask, stow->graph);
  }
  
  for (auto li : vIn.laws())
    stow->cachedLaws.push_back(static_cast<Law>(li));
  
  /* just going to reset and process.
   * this should set the status message that we don't serialize.
   * this should 'compile' osgNodes together.
   */
  stow->reset();
  stow->process();
}
