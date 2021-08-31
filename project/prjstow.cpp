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

#include <boost/signals2/shared_connection_block.hpp>
#include <boost/filesystem.hpp>
#include <boost/graph/topological_sort.hpp>

#include <osg/Node> //yuck

#include <QDesktopServices>
#include <QUrl>

#include "application/appapplication.h"
#include "tools/idtools.h"
#include "annex/annseershape.h"
#include "feature/ftrbase.h"
#include "feature/ftrinert.h"
#include "feature/ftrmessage.h"
#include "message/msgnode.h"
#include "viewer/vwrmessage.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "project/prjmessage.h"
#include "project/prjstow.h"
#include "project/prjproject.h"

using boost::filesystem::path;
using namespace prj;

Stow::Stow(Project &pIn)
:project(pIn)
{
  node.connect(msg::hub());
  sift.name = "prj::Project";
  node.setHandler(std::bind(&msg::Sift::receive, &sift, std::placeholders::_1));
  setupDispatcher();
}

Stow::~Stow() = default;

Vertex Stow::addFeature(std::shared_ptr<ftr::Base> feature)
{
  Vertex newVertex = boost::add_vertex(graph);
  graph[newVertex].feature = feature;
  return newVertex;
}

void Stow::removeFeature(Vertex remove)
{
  prj::Message pMessage;
  pMessage.feature = graph[remove].feature;
  msg::Message preMessage
  (
    msg::Response | msg::Pre | msg::Remove | msg::Feature
    , pMessage
  );
  msg::hub().send(preMessage);
  
  //edges should already be cleared with appropriate messages. Just in case.
  boost::clear_vertex(remove, graph);
  graph[remove].alive = false;
}

Edge Stow::connect(const Vertex &parentIn, const Vertex &childIn, const ftr::InputType &type)
{
  assert(graph[parentIn].alive);
  assert(graph[childIn].alive);
  
  bool results;
  Edge newEdge;
  boost::tie(newEdge, results) = boost::edge(parentIn, childIn, graph);
  if (!results)
    boost::tie(newEdge, results) = boost::add_edge(parentIn, childIn, graph);
  assert(results);
  if (!results)
  {
    std::cout << "warning: couldn't add edge in prj::Stow::connect" << std::endl;
    return newEdge;
  }
  graph[newEdge].inputType += type;
  sendConnectMessage(parentIn, childIn, type);
  return newEdge;
}

void Stow::sendConnectMessage(const Vertex &parentIn, const Vertex &childIn, const ftr::InputType &type) const
{
  prj::Message pMessage;
  pMessage.featureIds.push_back(graph[parentIn].feature->getId());
  pMessage.featureIds.push_back(graph[childIn].feature->getId()); 
  pMessage.inputType = type;
  msg::Message postMessage
  (
    msg::Response | msg::Post | msg::Add | msg::Connection
    , pMessage
  );
  msg::hub().sendBlocked(postMessage);
}

void Stow::clearVertex(const Vertex &vIn)
{
  Edges etr; //edges to remove
  for (auto ie : boost::make_iterator_range(boost::in_edges(vIn, graph)))
    etr.push_back(ie);
  for (auto ie : boost::make_iterator_range(boost::out_edges(vIn, graph)))
    etr.push_back(ie);
  
  removeEdges(etr);
}

void Stow::disconnect(const Edge &eIn)
{
  sendDisconnectMessage(boost::source(eIn, graph), boost::target(eIn, graph), graph[eIn].inputType);
  boost::remove_edge(eIn, graph);
}

void Stow::sendDisconnectMessage(const Vertex &parentIn, const Vertex &childIn, const ftr::InputType &type) const
{
  prj::Message pMessage;
  pMessage.featureIds.push_back(graph[parentIn].feature->getId());
  pMessage.featureIds.push_back(graph[childIn].feature->getId());
  pMessage.inputType = type;
  msg::Message preMessage(msg::Response | msg::Pre | msg::Remove | msg::Connection, pMessage);
  msg::hub().send(preMessage);
}

void Stow::removeEdges(Edges esIn)
{
  //don't invalidate.
  gu::uniquefy(esIn);
  for (auto it = esIn.rbegin(); it != esIn.rend(); ++it)
    disconnect(*it);
}

bool Stow::hasFeature(const boost::uuids::uuid &idIn) const
{
  return findVertex(idIn) != NullVertex();
}

ftr::Base* Stow::findFeature(const boost::uuids::uuid &idIn) const
{
  Vertex v = findVertex(idIn);
  if (v != NullVertex())
    return graph[v].feature.get();
  assert(0); //no feature with id in prj::Stow::findFeature
  std::cout << "warning: no feature with id in prj::Stow::findFeature" << std::endl;
  return nullptr;
}

Vertex Stow::findVertex(const boost::uuids::uuid &idIn) const
{
  for (auto its = boost::vertices(graph); its.first != its.second; ++its.first)
  {
    if (graph[*its.first].feature->getId() == idIn)
    {
      assert(graph[*its.first].alive);
      return *its.first;
    }
  }
  assert(0); //no vertex with id in prj::Stow::findVertex
  std::cout << "warning: no vertex with id in prj::Stow::findVertex" << std::endl;
  return NullVertex();
}

std::vector<boost::uuids::uuid> Stow::getAllFeatureIds() const
{
  std::vector<boost::uuids::uuid> out;
  
  for (auto its = boost::vertices(graph); its.first != its.second; ++its.first)
  {
    if (graph[*its.first].alive)
      out.push_back(graph[*its.first].feature->getId());
  }
  
  return out;
}

prm::Parameter* Stow::findParameter(const boost::uuids::uuid &idIn) const
{
  for (auto its = boost::vertices(graph); its.first != its.second; ++its.first)
  {
    if (!graph[*its.first].feature->hasParameter(idIn))
      continue;
    return graph[*its.first].feature->getParameter(idIn);
  }

  assert(0); //no such parameter.
  std::cout << "warning: no such parameter in prj::Stow::findParameter" << std::endl;
  return nullptr;
}

void Stow::setFeatureActive(Vertex vIn)
{
  if (isFeatureActive(vIn))
    return; //already active.
  graph[vIn].state.set(ftr::StateOffset::Inactive, false);
  sendStateMessage(vIn, ftr::StateOffset::Inactive);
}

void Stow::setFeatureInactive(Vertex vIn)
{
  if (isFeatureInactive(vIn))
    return;
  graph[vIn].state.set(ftr::StateOffset::Inactive, true);
  sendStateMessage(vIn, ftr::StateOffset::Inactive);
}

bool Stow::isFeatureActive(Vertex vIn)
{
  return !graph[vIn].state.test(ftr::StateOffset::Inactive);
}

bool Stow::isFeatureInactive(Vertex vIn)
{
  return graph[vIn].state.test(ftr::StateOffset::Inactive);
}

void Stow::setFeatureLeaf(Vertex vIn)
{
  if (isFeatureLeaf(vIn))
    return;
  graph[vIn].state.set(ftr::StateOffset::NonLeaf, false);
  sendStateMessage(vIn, ftr::StateOffset::NonLeaf);
}

void Stow::setFeatureNonLeaf(Vertex vIn)
{
  if (isFeatureNonLeaf(vIn))
    return;
  graph[vIn].state.set(ftr::StateOffset::NonLeaf, true);
  sendStateMessage(vIn, ftr::StateOffset::NonLeaf);
}

bool Stow::isFeatureLeaf(Vertex vIn)
{
  return !graph[vIn].state.test(ftr::StateOffset::NonLeaf);
}

bool Stow::isFeatureNonLeaf(Vertex vIn)
{
  return graph[vIn].state.test(ftr::StateOffset::NonLeaf);
}

void Stow::setupDispatcher()
{
  sift.insert
  (
    {
      std::make_pair
      (
        msg::Request | msg::SetCurrentLeaf
        , std::bind(&Stow::setCurrentLeafDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Remove | msg::Feature
        , std::bind(&Stow::removeFeatureDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Project | msg::Update
        , std::bind(&Stow::updateDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Force | msg::Update
        , std::bind(&Stow::forceUpdateDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Project | msg::Update | msg::Model
        , std::bind(&Stow::updateModelDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Project | msg::Update | msg::Visual
        , std::bind(&Stow::updateVisualDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Save | msg::Project
        , std::bind(&Stow::saveProjectRequestDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::CheckShapeIds
        , std::bind(&Stow::checkShapeIdsDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Feature | msg::Status
        , std::bind(&Stow::featureStateChangedDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Project | msg::Graph | msg::Dump
        , std::bind(&Stow::dumpProjectGraphDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::View | msg::Show | msg::ThreeD
        , std::bind(&Stow::shownThreeDDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Project | msg::Feature | msg::Reorder
        , std::bind(&Stow::reorderFeatureDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Feature | msg::Skipped | msg::Toggle
        , std::bind(&Stow::toggleSkippedDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Project | msg::Feature | msg::Dissolve
        , std::bind(&Stow::dissolveFeatureDispatched, this, std::placeholders::_1)
      )
    }
  );
}

void Stow::setCurrentLeafDispatched(const msg::Message &messageIn)
{
  prj::Message message = messageIn.getPRJ();
  //send response signal out 'pre set current feature'.
  assert(message.featureIds.size() == 1);
  project.setCurrentLeaf(message.featureIds.front());
  //send response signal out 'post set current feature'.
}

void Stow::removeFeatureDispatched(const msg::Message &messageIn)
{
  prj::Message message = messageIn.getPRJ();
  assert(message.featureIds.size() == 1);
  project.removeFeature(message.featureIds.front());
}

void Stow::updateDispatched(const msg::Message&)
{
  app::WaitCursor waitCursor;
  project.updateModel();
  project.updateVisual();
  
  node.sendBlocked(msg::buildStatusMessage(std::string()));
}

void Stow::forceUpdateDispatched(const msg::Message&)
{
  RemovedGraph rg = buildRemovedGraph(graph);
  for (auto its = boost::vertices(rg); its.first != its.second; ++its.first)
    rg[*its.first].feature->setModelDirty();
  
  app::WaitCursor waitCursor;
  project.updateModel();
  project.updateVisual();
  
  node.sendBlocked(msg::buildStatusMessage(std::string()));
}

void Stow::updateModelDispatched(const msg::Message&)
{
  project.updateModel();
}

void Stow::updateVisualDispatched(const msg::Message&)
{
  project.updateVisual();
}

void Stow::saveProjectRequestDispatched(const msg::Message&)
{
  project.save();
}

void Stow::checkShapeIdsDispatched(const msg::Message&)
{
  /* initially shapeIds were being copied from parent feature to child feature.
   * decided to make each shape unique to feature and use evolution container
   * to map shapes between related features. This command visits graph and checks 
   * for duplicated ids, which we shouldn't have anymore.
   */
  
  
  // this has not been updated to the new graph.
  std::cout << "command is out of date" << std::endl;
  
  /*
   * using boost::uuids::uuid;
   * 
   * typedef std::vector<uuid> FeaturesIds;
   * typedef std::map<uuid, FeaturesIds> IdMap;
   * IdMap idMap;
   * 
   * BGL_FORALL_VERTICES(currentVertex, projectGraph, Graph)
   * {
   *   const ftr::Base *feature = projectGraph[currentVertex].feature.get();
   *   if (!feature->hasSeerShape())
   *     continue;
   *   for (const auto &id : feature->getSeerShape().getAllShapeIds())
   *   {
   *     IdMap::iterator it = idMap.find(id);
   *     if (it == idMap.end())
   *     {
   *       std::vector<uuid> freshFeatureIds;
   *       freshFeatureIds.push_back(feature->getId());
   *       idMap.insert(std::make_pair(id, freshFeatureIds));
}
else
{
it->second.push_back(id);
}
}
}

std::cout << std::endl << std::endl << "Check shape ids:";

bool foundDuplicate = false;
for (const auto &entry : idMap)
{
if (entry.second.size() < 2)
  continue;
foundDuplicate = true;
std::cout << std::endl << "shape id of: " << gu::idToString(entry.first) << "    is in feature id of:";
for (const auto &it : entry.second)
{
std::cout << " " << gu::idToString(it);
}
}

if (!foundDuplicate)
  std::cout << std::endl << "No duplicate ids found. Test passed" << std::endl;

*/
}

void Stow::featureStateChangedDispatched(const msg::Message &messageIn)
{
  //here we want to dirty depenedent features when applicable.
  ftr::Message fMessage = messageIn.getFTR();
  std::size_t co = fMessage.stateOffset; //changed offset.
  
  //we only care about dirty and skipped states.
  if ((co != ftr::StateOffset::ModelDirty) && (co != ftr::StateOffset::Skipped))
    return;
  
  //we don't care about feature set to clean.
  if ((co == ftr::StateOffset::ModelDirty) && (!fMessage.state.test(ftr::StateOffset::ModelDirty)))
    return;
  
  //this code blocks all incoming messages to the project while it
  //executes. This prevents the cycles from setting a dependent fetures dirty.
  auto block = node.createBlocker();
  
  Vertex vertex = findVertex(fMessage.featureId);
  Vertices vertices;
  gu::BFSLimitVisitor<Vertex> visitor(vertices);
  boost::breadth_first_search(graph, vertex, boost::visitor(visitor));
  
  //we don't need to set the feature responsible for getting here.
  vertices.erase(std::remove_if(vertices.begin(), vertices.end(), [&](Vertex v){return v == vertex;}), vertices.end());
  for (const auto &v : vertices)
    graph[v].feature->setModelDirty();
}

void Stow::dumpProjectGraphDispatched(const msg::Message &)
{
  //   indexVerticesEdges();
  
  path filePath = app::instance()->getApplicationDirectory() / "project.dot";
  writeGraphViz(filePath.string());
  
  QDesktopServices::openUrl(QUrl(QString::fromStdString(filePath.string())));
}

void Stow::shownThreeDDispatched(const msg::Message &mIn)
{
  uuid id = mIn.getVWR().featureId;
  auto feature = findFeature(id);
  if
    (
      feature->isModelClean() &&
      feature->isVisible3D() &&
      project.isFeatureActive(id) &&
      feature->isVisualDirty()
    )
    feature->updateVisual();
}

void Stow::reorderFeatureDispatched(const msg::Message &mIn)
{
  const prj::Message &pm = mIn.getPRJ();
  Vertices fvs; //feature vertices.
  for (const auto &id : pm.featureIds)
  {
    fvs.push_back(findVertex(id));
    graph[fvs.back()].feature->setModelDirty();
  }
  assert(fvs.size() == 2 || fvs.size() == 3);
  if (fvs.size() !=2 && fvs.size() != 3)
  {
    std::cout << "Warning: wrong vertex count in Stow::reorderFeatureDispatched" << std::endl;
    return;
  }
  
  //shouldn't need anymore messages into project.
  auto block = node.createBlocker();
  
  /* reorder operation. Real simple For now. This only supports
   * an edge as the drop target. source node can only have 1
   * or less in-edge. same is true for out-edge.
   */
  struct Reorder
  {
    struct DragNode
    {
      boost::optional<Vertex> parent;
      Vertex v;
      boost::optional<Vertex> child;
      boost::optional<Edge> oldInEdge;
      boost::optional<Edge> oldOutEdge;
    };
    struct DropNode
    {
      Vertex parent;
      Vertex child;
      Edge edge;
    };
    
    Reorder() = delete;
    Reorder(Stow &stowIn, Vertex drag, Edge drop) : stow(stowIn)
    {
      if
        (
          (boost::in_degree(drag, stow.graph) > 1)
          || (boost::out_degree(drag, stow.graph) > 1)
        )
        {
          std::cout << "unsupported reorder" << std::endl;
          return;
        }
        
        dragNode.v = drag;
      for (auto its = boost::in_edges(drag, stow.graph); its.first != its.second; ++its.first)
      {
        dragNode.oldInEdge = *its.first;
        dragNode.parent = boost::source(*its.first, stow.graph);
      }
      for (auto its = boost::out_edges(drag, stow.graph); its.first != its.second; ++its.first)
      {
        dragNode.oldOutEdge = *its.first;
        dragNode.child = boost::target(*its.first, stow.graph);
      }
      
      dropNode.edge = drop;
      dropNode.parent = boost::source(drop, stow.graph);
      dropNode.child = boost::target(drop, stow.graph);
      
      go();
    }
    
    void go()
    {
      //make all new connections.
      ftr::InputType ph = (dragNode.oldInEdge) ? (stow.graph[*dragNode.oldInEdge].inputType) : (ftr::InputType()); // place holder
      stow.connect(dropNode.parent, dragNode.v, ph);
      stow.connect(dragNode.v, dropNode.child, stow.graph[dropNode.edge].inputType);
      if (dragNode.parent && dragNode.oldOutEdge) //if an out edge then the child vertex is present also.
        stow.connect(*dragNode.parent, *dragNode.child, stow.graph[*dragNode.oldOutEdge].inputType);
      //collect old connections for erasure.
      oldEdges.push_back(dropNode.edge);
      if (dragNode.oldInEdge)
        oldEdges.push_back(*dragNode.oldInEdge);
      if (dragNode.oldOutEdge)
        oldEdges.push_back(*dragNode.oldOutEdge);
    }
    
    Stow &stow;
    DragNode dragNode;
    DropNode dropNode;
    Edges oldEdges;
  };
  
  if (pm.featureIds.size() == 2) //drop onto vertex.
  {
    //first is the feature to move and the last is feature to land.
    //forward and reverse dictate whether the feature is moved before or after the landing vertex.
    std::vector<Vertices> fps = getAllPaths<Graph>(fvs.front(), fvs.back(), graph);
    if (!fps.empty())
      std::cout << "forward drag with vertex drop" << std::endl;
    
    std::vector<Vertices> rps = getAllPaths<Graph>(fvs.back(), fvs.front(), graph);
    if (!rps.empty())
      std::cout << "reverse drag with vertex drop" << std::endl;
  }
  if (pm.featureIds.size() == 3) //drop onto edge.
  {
    //first is the feature to move and the last two are the source and target of edge, respectively.
    //we don't care about forward and reverse when landing on edge.
    bool results;
    Edge ce; //connecting edge.
    std::tie(ce, results) = boost::edge(fvs.at(1), fvs.back(), graph);
    assert(results);
    if (!results)
    {
      std::cout << "Warning: couldn't find edge in Stow::reorderFeatureDispatched" << std::endl;
      return;
    }
    std::cout << "drag with edge drop" << std::endl;
    Reorder re(*this, fvs.front(), ce);
    removeEdges(re.oldEdges);
  }
  
  //assuming everything went well.
  //should I call these here? maybe let source of reorder request call update on the project?
  project.updateModel();
  project.updateVisual();
}

void Stow::toggleSkippedDispatched(const msg::Message &mIn)
{
  //log action to git.
  std::ostringstream gitMessage;
  gitMessage << QObject::tr("Toggling skip status for: ").toStdString();
  
  const prj::Message &pm = mIn.getPRJ();
  for (const auto& id : pm.featureIds)
  {
    ftr::Base *f = findFeature(id);
    if (f->isSkipped())
      f->setNotSkipped();
    else
      f->setSkipped();
    gitMessage << f->getName().toStdString() << " " << gu::idToShortString(id) << "    ";
  }
  gitManager.appendGitMessage(gitMessage.str());
}

void Stow::dissolveFeatureDispatched(const msg::Message &mIn)
{
  const prj::Message &pm = mIn.getPRJ();
  assert(pm.featureIds.size() == 1);
  assert(hasFeature(pm.featureIds.front()));
  if (!hasFeature(pm.featureIds.front()))
    return;
  
  ftr::Base *fb = findFeature(pm.featureIds.front());
  if (!fb->hasAnnex(ann::Type::SeerShape))
    return; //for now we only care about features with shape.
    //should dissolve be a virtual member of ftr::Base?
    
  const ann::SeerShape &oss = fb->getAnnex<ann::SeerShape>();
  if (oss.isNull())
    return; //do we really care about this here?
    
  fb->setModelDirty(); //this will make all children dirty.
  auto block = node.createBlocker();
  
  node.send
  (
    msg::Message(msg::Response | msg::Pre | msg::Project | msg::Feature | msg::Dissolve, pm)
  );
  
  //create new inert feature.
  std::shared_ptr<ftr::Inert::Feature> nf;
  auto csysPrms = fb->getParameters(prm::Tags::CSys);
  if (!csysPrms.empty())
    nf = std::make_shared<ftr::Inert::Feature>(oss.getRootOCCTShape(), csysPrms.front()->getMatrix());
  else
    nf = std::make_shared<ftr::Inert::Feature>(oss.getRootOCCTShape());
  nf->setColor(fb->getColor());
  ann::SeerShape &nss = nf->getAnnex<ann::SeerShape>();
  occt::ShapeVector oshapes = oss.getAllShapes(); //original shapes
  for (const auto &s : oshapes)
  {
    assert(nss.hasShape(s));
    if (!nss.hasShape(s))
    {
      std::cerr << "WARNING: new dissolved inert feature doesn't have original shape in Stow::dissolveFeatureDispatched" << std::endl;
      continue;
    }
    auto id = oss.findId(s);
    nss.updateId(s, id);
    nss.insertEvolve(gu::createNilId(), id);
  }
  nss.setRootShapeId(oss.getRootShapeId());
  Vertex nfv = addFeature(nf); //new feature vertex.
  prj::Message addPMessage;
  addPMessage.feature = nf;
  msg::Message postMessage
  (
    msg::Response | msg::Post | msg::Add | msg::Feature
    , addPMessage
  );
  node.send(postMessage);
  
  //give new feature same visualization status as old.
  if (fb->isHiddenOverlay())
    node.send(msg::buildHideOverlay(nf->getId()));
  if (fb->isHidden3D())
    node.send(msg::buildHideThreeD(nf->getId()));
  
  //get children of original feature and setup graph edges from new inert feature.
  Vertex ov = findVertex(pm.featureIds.front());
  RemovedGraph removedGraph = buildRemovedGraph(graph);
  for (auto its = boost::adjacent_vertices(ov, removedGraph); its.first != its.second; ++its.first)
  {
    auto e = boost::edge(ov, *its.first, removedGraph);
    assert(e.second);
    connect(nfv, *its.first, removedGraph[e.first].inputType);
  }
  
  //now we can remove original feature and applicable parents.
  Vertices vertsToRemove;
  vertsToRemove.push_back(ov);
  ReversedGraph reversedGraph = boost::make_reverse_graph(removedGraph);
  if (fb->getDescriptor() != ftr::Descriptor::Create)
  {
    //walk parent paths to first create.
    Vertices vertexes;
    gu::BFSLimitVisitor<Vertex> rVis(vertexes);
    boost::breadth_first_search(reversedGraph, ov, visitor(rVis));
    
    //filter on the accumulated vertexes.
    gu::SubsetFilter<ReversedGraph> vertexFilter(reversedGraph, vertexes);
    typedef boost::filtered_graph<ReversedGraph, boost::keep_all, gu::SubsetFilter<ReversedGraph> > FilteredGraph;
    FilteredGraph filteredGraph(reversedGraph, boost::keep_all(), vertexFilter);
    
    AlterVisitor apv(vertsToRemove, AlterVisitor::Create::Inclusion);
    boost::depth_first_search(filteredGraph, visitor(apv).root_vertex(ov));
  }
  
  //remove edges from vertices that are to be removed
  Edges etr; //edges to remove
  for (const auto &v : vertsToRemove)
  {
    for (auto its = boost::adjacent_vertices(v, reversedGraph); its.first != its.second; ++its.first)
    {
      //notice we are using the removedGraph with vertices reversed.
      auto ce = boost::edge(*its.first, v, removedGraph);
      assert(ce.second);
      etr.push_back(ce.first);
    }
  }
  for (auto its = boost::adjacent_vertices(ov, removedGraph); its.first != its.second; ++its.first)
  {
    auto ce = boost::edge(ov, *its.first, removedGraph);
    assert(ce.second);
    etr.push_back(ce.first);
  }
  removeEdges(etr);
  
  //now finally remove the actual vertices.
  for (const auto &v : vertsToRemove)
  {
    assert (boost::degree(v, graph) == 0);
    
    prj::Message pMessage;
    pMessage.feature = graph[v].feature;
    msg::Message preMessage
    (
      msg::Response | msg::Pre | msg::Remove | msg::Feature
      , pMessage
    );
    node.send(preMessage);
    
    //remove file if exists.
    assert(boost::filesystem::exists(saveDirectory));
    path filePath = saveDirectory / fb->getFileName();
    if (boost::filesystem::exists(filePath))
      boost::filesystem::remove(filePath);
    
    boost::clear_vertex(v, graph); //should be redundent.
    graph[v].alive = false;
  }
  
  node.send
  (
    msg::Message(msg::Response | msg::Post | msg::Project | msg::Feature | msg::Dissolve, pm)
  );
  
  std::ostringstream gitMessage;
  gitMessage << QObject::tr("Disolving feature: ").toStdString() << gu::idToShortString(fb->getId());
  gitManager.appendGitMessage(gitMessage.str());
}

void Stow::sendStateMessage(const Vertex &v, std::size_t stateOffset)
{
  ftr::Message fMessage(graph[v].feature->getId(), graph[v].state, stateOffset);
  msg::Message mMessage(msg::Response | msg::Project | msg::Feature | msg::Status, fMessage);
  msg::hub().sendBlocked(mMessage);
}

void Stow::writeGraphViz(const std::string &fileName)
{
  outputGraphviz(graph, fileName);
}

void Stow::updateLeafStatus()
{
  //we end up in here twice when set current leaf from dag view.
  //once when make the change and once when we call an update.
  
  //first set all features to non leaf.
  RemovedGraph removedGraph = buildRemovedGraph(graph);
  for (auto v : boost::make_iterator_range(boost::vertices(removedGraph)))
    setFeatureNonLeaf(v);
  
  //build filtered graph for active features and no sever edges.
  ComboFilterVertex<Graph> vf(graph); //filters for alive vertices by default.
  vf.active = true;
  ComboFilterEdge<Graph> ef(graph);
  ef.sever = true;
  boost::filtered_graph<Graph, decltype(ef), decltype(vf)> filteredGraph(graph, ef, vf);
  
  for (auto v : boost::make_iterator_range(boost::vertices(filteredGraph)))
  {
    if (boost::out_degree(v, filteredGraph) == 0)
      setFeatureLeaf(v);
    else
    {
      //if all children are of the type 'create' then it is also considered leaf.
      bool allCreate = true;
      for (auto av : boost::make_iterator_range(boost::adjacent_vertices(v, filteredGraph)))
      {
        if (filteredGraph[av].feature->getDescriptor() != ftr::Descriptor::Create)
        {
          allCreate = false;
          break;
        }
      }
      if (allCreate)
        setFeatureLeaf(v);
    }
  }
}

void Stow::buildShapeHistory()
{
  //used for loading so don't worry about 'dead' vertices
  shapeHistory.clear();
  Vertices sorted;
  try
  {
    boost::topological_sort(graph, std::back_inserter(sorted));
  }
  catch(const boost::not_a_dag &)
  {
    std::cout << std::endl << "Graph is not a dag exception in " << BOOST_CURRENT_FUNCTION << std::endl << std::endl;
    return;
  }
  
  for (auto it = sorted.rbegin(); it != sorted.rend(); ++it)
    graph[*it].feature->fillInHistory(shapeHistory);
}
