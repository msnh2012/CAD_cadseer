/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef PRJ_STOW_H
#define PRJ_STOW_H

#include <memory>
#include <vector>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/reverse_graph.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/uuid/uuid.hpp>

#include "feature/ftrstates.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrupdatepayload.h"
#include "tools/graphtools.h"
#include "globalutilities.h"

namespace prm{class Parameter;}
namespace ftr{class Base;}

namespace prj
{
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
  
  class Stow
  {
  public:
    Stow();
    ~Stow();
    
    Vertex addFeature(std::shared_ptr<ftr::Base> feature);
    Edge connect(const Vertex &parentIn, const Vertex &childIn, const ftr::InputType &type);
    void sendConnectMessage(const Vertex&, const Vertex&, const ftr::InputType&) const;
    void disconnect(const Edge&);
    void sendDisconnectMessage(const Vertex&, const Vertex&, const ftr::InputType&) const;
    void removeEdges(Edges);
    
    bool hasFeature(const boost::uuids::uuid &idIn) const;
    ftr::Base* findFeature(const boost::uuids::uuid &) const;
    Vertex findVertex(const boost::uuids::uuid&) const;
    std::vector<boost::uuids::uuid> getAllFeatureIds() const;
    prm::Parameter* findParameter(const boost::uuids::uuid &idIn) const;
    
    void writeGraphViz(const std::string &fileName);
    
    void setFeatureActive(Vertex);
    void setFeatureInactive(Vertex);
    bool isFeatureActive(Vertex);
    bool isFeatureInactive(Vertex);
    void setFeatureLeaf(Vertex);
    void setFeatureNonLeaf(Vertex);
    bool isFeatureLeaf(Vertex);
    bool isFeatureNonLeaf(Vertex);
    
    Graph graph;
  private:
    void sendStateMessage(const Vertex&, std::size_t);
  };
  
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
  struct RemovedFilter
  {
    RemovedFilter() : g(nullptr) { }
    RemovedFilter(const G &gIn) : g(&gIn) { }
    template <typename V>
    bool operator()(const V &vIn) const
    {
      if(!g)
        return false;
      return (*g)[vIn].alive;
    }
    const G *g = nullptr;
  };
  typedef boost::filtered_graph<Graph, boost::keep_all, RemovedFilter<Graph>>RemovedGraph;
  inline RemovedGraph buildRemovedGraph(Graph& gIn)
  {
    RemovedFilter<Graph> f(gIn);
    return RemovedGraph(gIn, boost::keep_all(), f);
  }
  
  typedef boost::reverse_graph<RemovedGraph, RemovedGraph&> ReversedGraph;
  
  inline std::vector<Vertex> weakComponents(Graph& gIn, Vertex v)
  {
    RemovedGraph baseGraph = buildRemovedGraph(gIn);
    ReversedGraph rBaseGraph = boost::make_reverse_graph(baseGraph);
    std::vector<Vertex> vertices;
    gu::BFSLimitVisitor<Vertex> limitVisitor(vertices);
    boost::breadth_first_search(rBaseGraph, v, boost::visitor(limitVisitor));
    
    //find the leafs in the reversed graph = roots in forward graph. 
    std::vector<Vertex> roots;
    for (const auto &v : vertices)
    {
      if (boost::out_degree(v, rBaseGraph) == 0)
        roots.push_back(v);
    }
    
    //travel all forward roots to get all components
    std::vector<Vertex> all;
    for (const auto &v : roots)
    {
      std::vector<Vertex> current;
      gu::BFSLimitVisitor<Vertex> visitor(current);
      boost::breadth_first_search(baseGraph, v, boost::visitor(visitor));
      all.insert(all.end(), current.begin(), current.end());
    }
    gu::uniquefy(all);
    return all;
  }
  
  
  template <typename GraphTypeIn>
  struct ActiveFilter
  {
    ActiveFilter() : graph(nullptr) { }
    ActiveFilter(const GraphTypeIn &graphIn) : graph(&graphIn) { }
    template <typename VertexType>
    bool operator()(const VertexType& vertexIn) const
    {
      if(!graph)
        return false;
      return !(*graph)[vertexIn].state.test(ftr::StateOffset::Inactive);
    }
    const GraphTypeIn *graph;
  };
  
  template <typename GraphTypeIn>
  struct TargetEdgeFilter
  {
    TargetEdgeFilter() : graph(nullptr) {}
    TargetEdgeFilter(const GraphTypeIn &graphIn) : graph(&graphIn) {}
    template <typename EdgeType>
    bool operator()(const EdgeType& edgeIn) const
    {
      if (!graph)
        return false;
      return (*graph)[edgeIn].inputType.has(ftr::InputType::target);
    }
    const GraphTypeIn *graph;
  };
  
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

#endif // PRJ_STOW_H
