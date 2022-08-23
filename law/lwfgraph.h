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

#ifndef LWF_GRAPH_H
#define LWF_GRAPH_H

#include <variant>
#include <bitset>
#include <fstream>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/graphviz.hpp>

#include "law/lwfnode.h"
#include "law/lwflaw.h"

/* I didn't want to resort to a graph, but modeling a composite law
 * is just too ugly otherwise.
 */

namespace lwf
{
  using EdgeMask = std::bitset<8>;
  constexpr EdgeMask NoneMask = 1; //No assigned law but connects points
  constexpr EdgeMask ConstantMask = 2; //Constant law between points
  constexpr EdgeMask LinearMask = 4; //Linear law between points
  constexpr EdgeMask InterpolateMask = 8; //Interpolate law between points
  constexpr EdgeMask LawMask = 15; //All laws
  constexpr EdgeMask ChildMask = 16; //connection from points to value and slope
  
  EdgeMask lawToMask(Law l)
  {
    switch (l)
    {
      case Law::Constant:{return ConstantMask; break;}
      case Law::Linear:{return LinearMask; break;}
      case Law::Interpolate:{return InterpolateMask; break;}
      case Law::Composite:{return NoneMask; break;}
    }
    return NoneMask;
  }
  
  using Graph = boost::adjacency_list
  <
    boost::setS
    , boost::listS
    , boost::bidirectionalS
    , Node
    , EdgeMask
  >;
  using Vertex = boost::graph_traits<Graph>::vertex_descriptor; //!< @brief A graph vertex.
  using Edge = boost::graph_traits<Graph>::edge_descriptor; //!< @brief A graph edge.
  using Vertices = std::vector<Vertex>; //!< @brief A vector of graph vertices.
  using Edges = std::vector<Edge>; //!< @brief A vector of graph edges.
  
  template <typename S>
  struct JustPoints
  {
    const S *s = nullptr;
    JustPoints(const S &sIn) : s(&sIn) { }
    JustPoints() = default;
    
    template <typename V>
    bool operator()(const V &vIn) const
    {
      assert(s);
      return s->isPoint(vIn);
    }
  };
  
  template <typename S>
  struct JustChild
  {
    const S *s = nullptr;
    EdgeMask mask;
    JustChild(const S &sIn, EdgeMask mIn) : s(&sIn), mask(mIn) { }
    JustChild() = default;
    
    template <typename E>
    bool operator()(const E &eIn) const
    {
      assert(s);
      return s->mask(eIn, mask);
    }
  };
  
  template <class G>
  std::size_t numVertices(const G &g)
  {
    auto its = boost::vertices(g);
    return std::distance(its.first, its.second);
  }

  template <class G>
  std::size_t numEdges(const G &g)
  {
    auto its = boost::edges(g);
    return std::distance(its.first, its.second);
  }
  
  template <class G>
  Vertices getRoots(const G &g)
  {
    Vertices out;
    for (auto v : boost::make_iterator_range(boost::vertices(g)))
    {
      if (boost::in_degree(v, g) == 0)
        out.push_back(v);
    }
    return out;
  }
  
  template <class G>
  std::tuple<Vertices, Edges> simpleWalk(const G &g)
  {
    Vertices verticesOut = getRoots<G>(g);
    assert(verticesOut.size() == 1);
    Edges edgesOut;
    while (boost::out_degree(verticesOut.back(), g) == 1)
    {
      auto e = boost::out_edges(verticesOut.back(), g).first;
      verticesOut.push_back(boost::target(*e, g));
      edgesOut.push_back(*e);
    }
    return std::make_tuple(verticesOut, edgesOut);
  }
  
  template <class G>
  class EdgeWriter {
  public:
    EdgeWriter(const G &gIn) : g(gIn) {}
    template <class E>
    void operator()(std::ostream& out, const E &e) const
    {
      auto getMaskText = [](EdgeMask em) -> std::string
      {
        if (em == ConstantMask) return "Constant";
        else if (em == LinearMask) return "Linear";
        else if (em == InterpolateMask) return "Interpolate";
        else if (em == ChildMask) return "Child";
        return "None";
      };
      
      out << "[label=\"";
      out << getMaskText(g[e]);
      out << "\"]";
    }
  private:
    const G &g;
  };
  
  template <class G>
  class VertexWriter {
  public:
    VertexWriter(const G &gIn) : g(gIn) {}
    template <class V>
    void operator()(std::ostream& out, const V &v) const
    {
      out <<
      "[label=\""
      << g[v].parameter.getTag();
      if (g[v].parameter.getTag() != lwf::Tags::pick)
        out << "\\n" << g[v].parameter.getDouble();
      out << "\"]";
    }
  private:
    const G &g;
  };
  
  template <class G>
  void outputGraphviz(const G &g, std::string_view filePath)
  {
    std::map<Vertex, std::size_t> pmap;
    boost::associative_property_map<decltype(pmap)> pMap(pmap);
    std::size_t index = 0;
    for (auto v : boost::make_iterator_range(boost::vertices(g)))
      boost::put(pMap, v, index++);
    
    std::ofstream file(filePath.data());
    boost::write_graphviz(file, g, VertexWriter<G>(g), EdgeWriter<G>(g), boost::default_writer(), pMap);
  }
  
}

#endif //LWF_GRAPH_H
