/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2021  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef PRJ_GRAPH_H
#define PRJ_GRAPH_H

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/reverse_graph.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/connected_components.hpp>

#include "globalutilities.h"
#include "tools/graphtools.h"
#include "feature/ftrstates.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrupdatepayload.h"

namespace ftr{class Base;}

namespace prj
{
  using boost::uuids::uuid;
  
  struct VertexProperty
  {
    bool alive = true;
    std::shared_ptr<ftr::Base> feature;
    ftr::State state;
  };
  struct EdgeProperty
  {
    ftr::InputType inputType;
  };
  typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, VertexProperty, EdgeProperty> Graph;
  typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
  typedef boost::graph_traits<Graph>::edge_descriptor Edge;
  typedef boost::graph_traits<Graph>::vertex_iterator VertexIterator;
  typedef boost::graph_traits<Graph>::edge_iterator EdgeIterator;
  typedef std::vector<Vertex> Vertices;
  typedef std::vector<Edge> Edges;
  inline Vertex NullVertex()
  {
    return boost::graph_traits<Graph>::null_vertex();
  }
  
  template <class GraphEW>
  class Edge_writer {
  public:
    Edge_writer(const GraphEW &graphEWIn) : graphEW(graphEWIn) {}
    template <class EdgeW>
    void operator()(std::ostream& out, const EdgeW& edgeW) const
    {
      out << "[label=\"";
      for (const auto &input : graphEW[edgeW].inputType.getTags())
        out << input << "\\n";
      out << "\"]";
    }
  private:
    const GraphEW &graphEW;
  };
  
  template <class GraphVW>
  class Vertex_writer {
  public:
    Vertex_writer(const GraphVW &graphVWIn) : graphVW(graphVWIn) {}
    template <class VertexW>
    void operator()(std::ostream& out, const VertexW& vertexW) const
    {
      out << 
      "[label=\"" <<
      graphVW[vertexW].feature->getName().toUtf8().data() << "\\n" <<
      gu::idToShortString(graphVW[vertexW].feature->getId()) << "\\n" <<
      ftr::getDescriptorString(graphVW[vertexW].feature->getDescriptor()) << 
      "\"]";
    }
  private:
    const GraphVW &graphVW;
  };
  
  template <class GraphIn>
  void outputGraphviz(const GraphIn &graphIn, const std::string &filePath)
  {
    std::ofstream file(filePath.c_str());
    boost::write_graphviz(file, graphIn, Vertex_writer<GraphIn>(graphIn),
                          Edge_writer<GraphIn>(graphIn));
  }
  
  //works with straight adjacency iterator. if parent map wanted, pass in reversed graph.
  //dont forget to filter out removed features.
  template<typename G, typename V>
  ftr::UpdatePayload::UpdateMap buildAjacentUpdateMap(const G &gIn, const V &vIn)
  {
    ftr::UpdatePayload::UpdateMap out;
    for (auto its = boost::adjacent_vertices(vIn, gIn); its.first != its.second; ++its.first)
    {
      auto e = boost::edge(vIn, *its.first, gIn);
      assert(e.second);
      for (const auto &tag : gIn[e.first].inputType.getTags())
        out.insert(std::make_pair(tag, gIn[*its.first].feature.get()));
    }
    return out;
  }
  
  template <typename G>
  struct ComboFilterVertex
  {
    ComboFilterVertex() : g(nullptr) { }
    ComboFilterVertex(const G &gIn) : g(&gIn) { }
    template <typename V>
    bool operator()(const V &vIn) const
    {
      if(!g)
        return false;
      
      bool localRemove = true;
      if (removed)
        localRemove = (*g)[vIn].alive;
      
      bool localActive = true;
      if (active)
        localActive = !(*g)[vIn].state.test(ftr::StateOffset::Inactive);
      
      bool localSubset = true;
      if (!vertexSubset.empty())
        localSubset = std::find(vertexSubset.begin(), vertexSubset.end(), vIn) != vertexSubset.end();
      
      return localRemove && localActive && localSubset;
    }
    const G *g = nullptr;
    bool removed = true; //filter for alive vertices. always yes.
    bool active = false; //filter for active vertices. no by default.
    std::vector<typename G::vertex_descriptor> vertexSubset; //vertices to keep. if empty keeping all.
  };
  
  template <typename G>
  struct ComboFilterEdge
  {
    ComboFilterEdge() : g(nullptr) { }
    ComboFilterEdge(const G &gIn) : g(&gIn) { }
    template <typename E>
    bool operator()(const E &eIn) const
    {
      if(!g)
        return false;
      
      bool localSever = true;
      if (sever)
        localSever = !(*g)[eIn].inputType.has(ftr::InputType::sever);
      
      bool localSubset = true;
      if (!edgeSubset.empty())
        localSubset = std::find(edgeSubset.begin(), edgeSubset.end(), eIn) != edgeSubset.end();
      
      return localSever && localSubset;
    }
    const G *g = nullptr;
    bool sever = false; //filter sever edges out. no by default
    std::vector<typename G::edge_descriptor> edgeSubset; //edges to keep. if empty keeping all.
  };
  
  using RemovedGraph = boost::filtered_graph<Graph, boost::keep_all, ComboFilterVertex<Graph>>;
  inline RemovedGraph buildRemovedGraph(Graph& gIn)
  {
    ComboFilterVertex<Graph> f(gIn); //filters for alive vertices by default.
    RemovedGraph out(gIn, boost::keep_all(), f);
    return out;
  }
  
  using RemovedSeveredGraph = boost::filtered_graph<Graph, ComboFilterEdge<Graph>, ComboFilterVertex<Graph>>;
  inline RemovedSeveredGraph buildRemovedSeveredGraph(Graph &gIn)
  {
    ComboFilterVertex<Graph> vf(gIn);
    ComboFilterEdge<Graph> ef(gIn);
    ef.sever = true;
    RemovedSeveredGraph out(gIn, ef, vf);
    return out;
  }
  
  using ReversedGraph = boost::reverse_graph<RemovedGraph, RemovedGraph&>;
  
  
  template <typename G>
  std::vector<typename boost::graph_traits<G>::vertex_descriptor> getWeakComponents(const G& gIn, typename boost::graph_traits<G>::vertex_descriptor vIn)
  {
    //this sucks! the template nightmare that is boost graph
    //and they don't have an adapter for directed to undirected.
    //so we copy to a temp undirected graph so we can use connected_components.
    //should we map ids to vertex for fast lookup?
    boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, uuid> localGraph;
    auto findNewVertex = [&](const uuid &idIn) -> decltype(localGraph)::vertex_descriptor
    {
      auto dummy = decltype(localGraph)::null_vertex();
      for (auto nv : boost::make_iterator_range(boost::vertices(localGraph)))
      {
        if (localGraph[nv] == idIn)
          return nv;
      }
      //we shouldn't get here.
      assert(dummy != decltype(localGraph)::null_vertex());
      return dummy;
    };
    
    for (auto ov : boost::make_iterator_range(boost::vertices(gIn)))
    {
      auto nv = boost::add_vertex(localGraph);
      localGraph[nv] = gIn[ov].feature->getId();
    }
    for (auto oe : boost::make_iterator_range(boost::edges(gIn)))
    {
      uuid sourceId = gIn[boost::source(oe, gIn)].feature->getId();
      uuid targetId = gIn[boost::target(oe, gIn)].feature->getId();
      auto sourceVertex = findNewVertex(sourceId);
      auto targetVertex = findNewVertex(targetId);
      boost::add_edge(sourceVertex, targetVertex, localGraph);
    }
    
    //now we should have a duplicate undirected graph
    auto baseVertex = findNewVertex(gIn[vIn].feature->getId());
    //types with component map are kind of FUBAR.
    //vertex_descriptor are indexes of map and value is the group index.
    std::vector<std::size_t> component(boost::num_vertices(localGraph));
    std::size_t cc = connected_components(localGraph, &component[0]);
    std::vector<std::vector<std::size_t>> components(cc);
    boost::optional<std::size_t> theGroup;
    for (std::size_t index = 0; index < component.size(); ++index)
    {
      components[component[index]].push_back(index);
      if (index == baseVertex)
        theGroup = component[index];
    }
    assert(theGroup);
    
    std::vector<decltype(vIn)> out;
    auto findOldVertex = [&](const uuid& idIn)
    {
      boost::optional<decltype(vIn)> dummy;
      for (auto v : boost::make_iterator_range(boost::vertices(gIn)))
      {
        if (gIn[v].feature->getId() == idIn)
        {
          dummy = v;
          break;
        }
      }
      assert(dummy);
      out.push_back(dummy.get());
    };
    for (auto v : components[theGroup.get()])
      findOldVertex(localGraph[v]);
    
    return out;
  }
  
  template <typename G, typename V>
  std::vector<std::vector<V>> getAllPaths(V source, V target, G graph)
  {
    //filter
    std::vector<V> slvs; //source limit vertices 
    gu::BFSLimitVisitor<V> slv(slvs); //source limit visitor
    boost::breadth_first_search(graph, source, boost::visitor(slv));
    
    typedef boost::reverse_graph<G, G&> RG;
    RG rGraph = boost::make_reverse_graph(graph);
    std::vector<V> tlvs; //target limit vertices
    gu::BFSLimitVisitor<V> tlv(tlvs); //target limit visitor
    boost::breadth_first_search(rGraph, target, boost::visitor(tlv));
    
    std::vector<V> lvs; //limit vertices
    gu::uniquefy(slvs); //need sort for set intersection
    gu::uniquefy(tlvs); //need sort for set intersection
    std::set_intersection(slvs.begin(), slvs.end(), tlvs.begin(), tlvs.end(), std::back_inserter(lvs));
    
    //path search algorithm expects at 1 root and 1 leaf.
    if (lvs.size() < 2)
      return std::vector<std::vector<V>>(); 
    
    //filter on the accumulated vertexes.
    gu::SubsetFilter<G> vertexFilter(graph, lvs);
    typedef boost::filtered_graph<G, boost::keep_all, gu::SubsetFilter<G> > FilteredGraph;
    FilteredGraph fg(graph, boost::keep_all(), vertexFilter);
    
    gu::PathSearch<FilteredGraph> ps(fg);
    return ps.getPaths();
  }
  
  class AlterVisitor : public boost::default_dfs_visitor
  {
  public:
    enum class Create
    {
      Inclusion, //!< the output vertices will include the 1st create path vertex
      Exclusion //!< the output vertices will NOT include the 1st create path vertex
    };
    
    AlterVisitor(Vertices &vsIn, Create cIn) : vs(vsIn), create(cIn)
    {
      assert(!vs.empty()); //need at least root vertex in.
    }
    template<typename V, typename G>
    void discover_vertex(V vertex, G &graph)
    {
      stack.push_back(vertex);
      if (vertex == vs.front())// understood vector has source added so we can ignore it
        return;
      if (firstCreateIndex != -1) //found first create so ignore rest
        return;
      vs.push_back(vertex);
      if (graph[vertex].feature->getDescriptor() == ftr::Descriptor::Create)
      {
        firstCreateIndex = static_cast<int>(stack.size()) - 1;
        if (create == Create::Exclusion)
          vs.pop_back();
      }
    }
    //       template<typename E, typename G>
    //       void examine_edge(E edge, G &graph)
    //       {
    // 
    //       }
    template<typename V, typename G>
    void finish_vertex(V, G)
    {
      stack.pop_back();
      if ((static_cast<int>(stack.size()) == firstCreateIndex) && (firstCreateIndex != -1))
      {
        firstCreateIndex = -1;
      }
    }
  private:
    Vertices &vs;
    int firstCreateIndex = -1; // index into stack
    Vertices stack;
    Create create;
  };
}

#endif // PRJ_GRAPH_H
