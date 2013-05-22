#ifndef CONNECTORGRAPH_H
#define CONNECTORGRAPH_H

#include <map>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/reverse_graph.hpp>
#include <boost/graph/breadth_first_search.hpp>

#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS_Shape.hxx>

namespace ConnectorGraph
{
struct VertexProperty
{
    int hash;
    TopAbs_ShapeEnum shapeType;
    TopoDS_Shape shape;
};

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, VertexProperty> Graph;
typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
typedef boost::graph_traits<Graph>::edge_descriptor Edge;
typedef boost::graph_traits<Graph>::vertex_iterator VertexIterator;
typedef boost::graph_traits<Graph>::edge_iterator EdgeIterator;
typedef boost::graph_traits<Graph>::in_edge_iterator InEdgeIterator;
typedef boost::graph_traits<Graph>::out_edge_iterator OutEdgeIterator;
typedef boost::graph_traits<Graph>::adjacency_iterator VertexAdjacencyIterator;
typedef boost::reverse_graph<Graph, Graph&> GraphReversed;

typedef std::map<int, Vertex> HashVertexMap;

class TypeCollectionVisitor : public boost::default_bfs_visitor
{
public:
    TypeCollectionVisitor(const TopAbs_ShapeEnum &shapeTypeIn){shapeType = shapeTypeIn;}
    template <typename VisitorVertex, typename VisitorGraph>
    void discover_vertex(VisitorVertex u, const VisitorGraph &g)
    {
        if (g[u].shapeType == this->shapeType)
            typedVertices.push_back(u);
    }
    std::vector<Vertex> getVertices(){return typedVertices;}

private:
    TopAbs_ShapeEnum shapeType;
    std::vector<Vertex> typedVertices;
};

static std::vector<std::string> shapeStrings
({
     "Compound",
     "Compound Solid",
     "Solid",
     "Shell",
     "Face",
     "Wire",
     "Edge",
     "Vertex",
     "Shape",
 });


template <class Name>
class Node_writer {
public:
  Node_writer(Name _name) : name(_name) {}
  template <class NodeW>
  void operator()(std::ostream& out, const NodeW& v) const {
    out << "[label=\"";
    out << shapeStrings.at(static_cast<int>(name[v].shapeType));
    out << "\"]";
  }
private:
  Name name;
};

}

#endif // CONNECTORGRAPH_H