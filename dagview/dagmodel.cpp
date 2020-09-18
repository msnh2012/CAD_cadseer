/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2015  Thomas S. Anderson blobfish.at.gmx.com
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

#include <iostream>
#include <cassert>

#include <boost/graph/topological_sort.hpp>

#include <QString>
#include <QTextStream>
#include <QGraphicsTextItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsView>
#include <QInputDialog>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QPainter>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QMenu>
#include <QTimer>
#include <QUrl>
#include <QDesktopServices>

#include "application/appapplication.h"
#include "application/appmainwindow.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "project/prjproject.h"
#include "globalutilities.h"
#include "tools/idtools.h"
#include "feature/ftrbase.h"
#include "message/msgnode.h"
#include "message/msgsift.h"
#include "project/prjmessage.h"
#include "feature/ftrmessage.h"
#include "selection/slcmessage.h"
#include "viewer/vwrmessage.h"
#include "tools/graphtools.h"
#include "dagview/dagrectitem.h"
#include "dagview/dagstow.h"
#include "dagview/dagmodel.h"

using boost::uuids::uuid;

using namespace dag;

LineEdit::LineEdit(QWidget* parentIn): QLineEdit(parentIn)
{

}

void LineEdit::keyPressEvent(QKeyEvent *eventIn)
{
  if (eventIn->key() == Qt::Key_Escape)
  {
    Q_EMIT rejectedSignal();
    eventIn->accept();
    return;
  }
  if (
    (eventIn->key() == Qt::Key_Enter) ||
    (eventIn->key() == Qt::Key_Return)
  )
  {
    Q_EMIT acceptedSignal();
    eventIn->accept();
    return;
  }
  
  QLineEdit::keyPressEvent(eventIn);
}

void LineEdit::focusOutEvent(QFocusEvent *e)
{
  QLineEdit::focusOutEvent(e);
  
  Q_EMIT rejectedSignal();
}

//I dont think I should have to call invalidate
//and definitely not on the whole scene!
//if we have performance problems, this will definitely
//be something to re-visit. I am not wasting anymore time on
//this right now.
//   this->scene()->invalidate();
//   this->scene()->invalidate(this->sceneTransform().inverted().mapRect(this->boundingRect()));
//   update(boundingRect());
//note: I haven't tried this again since I turned BSP off.

namespace dag
{
  struct DragData
  {
    uuid featureId = gu::createNilId();
    std::vector<Vertex> acceptedVertices;
    std::vector<Edge> acceptedEdges;
    QGraphicsPathItem *lastHighlight = nullptr;
    bool isAcceptedEdge(const Edge &eIn)
    {
      return std::find(acceptedEdges.begin(), acceptedEdges.end(), eIn) != acceptedEdges.end();
    }
  };
}

Model::Model(QObject *parentIn) : QGraphicsScene(parentIn), stow(new Stow())
{
  node = std::make_unique<msg::Node>();
  node->connect(msg::hub());
  sift = std::make_unique<msg::Sift>();
  sift->name = "dag::Model";
  node->setHandler(std::bind(&msg::Sift::receive, sift.get(), std::placeholders::_1));
  setupDispatcher();
  
  //turned off BSP as it was giving inconsistent discovery of items
  //underneath cursor.
  this->setItemIndexMethod(QGraphicsScene::NoIndex);
  setupViewConstants();
  
//   setupFilters();
//   
  currentPrehighlight = nullptr;
  
  QIcon temp(":/resources/images/dagViewVisible.svg");
  visiblePixmapEnabled = temp.pixmap(iconSize, iconSize, QIcon::Normal, QIcon::On);
  visiblePixmapDisabled = temp.pixmap(iconSize, iconSize, QIcon::Disabled, QIcon::Off);
  QIcon tempOverlay(":/resources/images/dagViewOverlay.svg");
  overlayPixmapEnabled = tempOverlay.pixmap(iconSize, iconSize, QIcon::Normal, QIcon::On);
  overlayPixmapDisabled = tempOverlay.pixmap(iconSize, iconSize, QIcon::Disabled, QIcon::Off);
  QIcon tempSelectable(":/resources/images/dagViewSelectable.svg");
  selectablePixmapEnabled = tempSelectable.pixmap(iconSize, iconSize, QIcon::Normal, QIcon::On);
  selectablePixmapDisabled = tempSelectable.pixmap(iconSize, iconSize, QIcon::Disabled, QIcon::Off);
  QIcon passIcon(":/resources/images/dagViewPass.svg");
  passPixmap = passIcon.pixmap(iconSize, iconSize);
  QIcon failIcon(":/resources/images/dagViewFail.svg");
  failPixmap = failIcon.pixmap(iconSize, iconSize);
  QIcon pendingIcon(":/resources/images/dagViewPending.svg");
  pendingPixmap = pendingIcon.pixmap(iconSize, iconSize);
  QIcon inactiveIcon(":/resources/images/dagViewInactive.svg");
  inactivePixmap = inactiveIcon.pixmap(iconSize, iconSize);
  QIcon skippedIcon(":/resources/images/dagViewSkipped.svg");
  skippedPixmap = skippedIcon.pixmap(iconSize, iconSize);
  
  selectionMask = slc::All;
}

Model::~Model()
{
  //we used shared pointers in the graphics map to manage memory.
  //so we don't want qt deleting these objects. so remove from scene.
  removeAllItems();
}

void Model::setupViewConstants()
{
  //get direction. 1.0 = top to bottom.  -1.0 = bottom to top.
  direction = 1.0;
  if (direction != -1.0 && direction != 1.0)
    direction = 1.0;
  
  QFontMetrics fontMetric(this->font());
  fontHeight = fontMetric.height();
  verticalSpacing = 1.0;
  rowHeight = (fontHeight + 2.0 * verticalSpacing) * direction; //pixel space top and bottom.
  iconSize = fontHeight;
  pointSize = fontHeight * 0.6;
  pointSpacing = pointSize * 1.10;
  pointToIcon =  (pointSize / 2.0 + iconSize / 2.0) * 1.25;
  iconToIcon = iconSize * 1.25;
  iconToText = iconSize * 1.5;
  rowPadding = fontHeight / 2.0;
  backgroundBrushes = {this->palette().base(), this->palette().alternateBase()};
  forgroundBrushes = 
  {
    QBrush(Qt::red),
    QBrush(Qt::darkRed),
    QBrush(Qt::green),
    QBrush(Qt::darkGreen),
    QBrush(Qt::blue),
    QBrush(Qt::darkBlue),
    QBrush(Qt::cyan),
    QBrush(Qt::darkCyan),
    QBrush(Qt::magenta),
    QBrush(Qt::darkMagenta),
//     QBrush(Qt::yellow), can't read
    QBrush(Qt::darkYellow),
    QBrush(Qt::gray),
    QBrush(Qt::darkGray),
    QBrush(Qt::lightGray)
  }; //reserve some of the these for highlight stuff.
}

void Model::featureAddedDispatched(const msg::Message &messageIn)
{
  prj::Message message = messageIn.getPRJ();
  
  Vertex virginVertex = boost::add_vertex(stow->graph);
  stow->graph[virginVertex].featureId = message.feature->getId();
  stow->graph[virginVertex].state = message.feature->getState();
  stow->graph[virginVertex].hasSeerShape = message.feature->hasAnnex(ann::Type::SeerShape);
  
  if (message.feature->isVisible3D())
    stow->graph[virginVertex].visibleIconShared->setPixmap(visiblePixmapEnabled);
  else
    stow->graph[virginVertex].visibleIconShared->setPixmap(visiblePixmapDisabled);
  
  if (message.feature->isVisibleOverlay())
    stow->graph[virginVertex].overlayIconShared->setPixmap(overlayPixmapEnabled);
  else
    stow->graph[virginVertex].overlayIconShared->setPixmap(overlayPixmapDisabled);
  
  if (message.feature->isSelectable())
    stow->graph[virginVertex].selectableIconShared->setPixmap(selectablePixmapEnabled);
  else
    stow->graph[virginVertex].selectableIconShared->setPixmap(selectablePixmapDisabled);
  
  stow->graph[virginVertex].featureIconShared->setPixmap(message.feature->getIcon().pixmap(iconSize, iconSize));
  stow->graph[virginVertex].textShared->setText(message.feature->getName());
  stow->graph[virginVertex].textShared->setFont(this->font());
  
  addItemsToScene(stow->getAllSceneItems(virginVertex));
  this->invalidate(); //temp.
}

void Model::featureRemovedDispatched(const msg::Message &messageIn)
{
  prj::Message message = messageIn.getPRJ();
  
  Vertex vertex = stow->findVertex(message.feature->getId());
  if (vertex == NullVertex())
    return;
  removeItemsFromScene(stow->getAllSceneItems(vertex));
  //connections should already removed
  assert(boost::in_degree(vertex, stow->graph) == 0);
  assert(boost::out_degree(vertex, stow->graph) == 0);
  stow->graph[vertex].alive = false;
}


void Model::connectionAddedDispatched(const msg::Message &messageIn)
{
  prj::Message message = messageIn.getPRJ();
  
  assert(message.featureIds.size() == 2);
  Vertex parentVertex = stow->findVertex(message.featureIds.front());
  Vertex childVertex = stow->findVertex(message.featureIds.back());
  if (parentVertex == NullVertex() || childVertex == NullVertex())
    return;
  
  bool results;
  Edge edge;
  
  std::tie(edge, results) = boost::edge(parentVertex, childVertex, stow->graph);
  if (results)
  {
    //connection already exists. just add input type and get out.
    stow->graph[edge].inputType += message.inputType;
    return;
  }
  
  std::tie(edge, results) = boost::add_edge(parentVertex, childVertex, stow->graph);
  assert(results);
  if (!results)
    return;
  stow->graph[edge].inputType = message.inputType;
  
  QPainterPath path;
  path.moveTo(0.0, 0.0);
  path.lineTo(0.0, 1.0);
  stow->graph[edge].connector->setPath(path);
  
  if (!stow->graph[edge].connector->scene())
    this->addItem(stow->graph[edge].connector.get());
}

void Model::connectionRemovedDispatched(const msg::Message &messageIn)
{
  prj::Message message = messageIn.getPRJ();
  
  assert(message.featureIds.size() == 2);
  Vertex parentVertex = stow->findVertex(message.featureIds.front());
  Vertex childVertex = stow->findVertex(message.featureIds.back());
  if (parentVertex == NullVertex() || childVertex == NullVertex())
    return;
  
  bool results;
  Edge edge;
  std::tie(edge, results) = boost::edge(parentVertex, childVertex, stow->graph);
  assert(results);
  if (!results)
    return;
  
  if (stow->graph[edge].connector->scene())
    this->removeItem(stow->graph[edge].connector.get());
  boost::remove_edge(edge, stow->graph);
}

void Model::setupDispatcher()
{
  sift->insert
  (
    {
      std::make_pair
      (
        msg::Response | msg::Post | msg::Add | msg::Feature
      , std::bind(&Model::featureAddedDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Pre | msg::Remove | msg::Feature
      , std::bind(&Model::featureRemovedDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Post | msg::Add | msg::Connection
      , std::bind(&Model::connectionAddedDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Pre | msg::Remove | msg::Connection
      , std::bind(&Model::connectionRemovedDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Post | msg::Project | msg::Update | msg::Model
      , std::bind(&Model::projectUpdatedDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Post | msg::Preselection | msg::Add
      , std::bind(&Model::preselectionAdditionDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Pre | msg::Preselection | msg::Remove
      , std::bind(&Model::preselectionSubtractionDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Post | msg::Selection | msg::Add
      , std::bind(&Model::selectionAdditionDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Pre | msg::Selection | msg::Remove
      , std::bind(&Model::selectionSubtractionDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Pre | msg::Close | msg::Project
      , std::bind(&Model::closeProjectDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Feature | msg::Status
      , std::bind(&Model::featureStateChangedDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Project | msg::Feature | msg::Status
      , std::bind(&Model::projectFeatureStateChangedDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Edit | msg::Feature | msg::Name
      , std::bind(&Model::featureRenamedDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::DAG | msg::Graph | msg::Dump
      , std::bind(&Model::dumpDAGViewGraphDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::View | msg::Show | msg::ThreeD
      , std::bind(&Model::threeDShowDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::View | msg::Hide | msg::ThreeD
      , std::bind(&Model::threeDHideDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::DAG | msg::View | msg::Update
      , std::bind(&Model::projectUpdatedDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::View | msg::Show | msg::Overlay
      , std::bind(&Model::overlayShowDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::View | msg::Hide | msg::Overlay
      , std::bind(&Model::overlayHideDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Selection | msg::Feature | msg::Thaw
        , std::bind(&Model::featureSelectionThawDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Selection | msg::Feature | msg::Freeze
        , std::bind(&Model::featureSelectionFreezeDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Selection | msg::SetMask
      , std::bind(&Model::selectionMaskDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Command | msg::Active
      , std::bind(&Model::commandActiveDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Command | msg::Inactive
      , std::bind(&Model::commandInactiveDispatched, this, std::placeholders::_1)
      )
    }
  );
  
}

void Model::featureStateChangedDispatched(const msg::Message &messageIn)
{
  ftr::Message fMessage = messageIn.getFTR();
  Vertex vertex = stow->findVertex(fMessage.featureId);
  if (vertex == NullVertex())
    return;
  
  //this is the feature state change from the actual feature.
  stow->graph[vertex].state &= ftr::StateOffset::ProjectMask;
  stow->graph[vertex].state |= (ftr::StateOffset::FeatureMask & fMessage.state);
  
  stateUpdate(vertex);
}

void Model::projectFeatureStateChangedDispatched(const msg::Message &mIn)
{
  ftr::Message fMessage = mIn.getFTR();
  Vertex vertex = stow->findVertex(fMessage.featureId);
  if (vertex == NullVertex())
    return;
  
  //this is the feature state change from the PROJECT.
  stow->graph[vertex].state &= ftr::StateOffset::FeatureMask;
  stow->graph[vertex].state |= (ftr::StateOffset::ProjectMask & fMessage.state);
  
  stateUpdate(vertex);
}

void Model::stateUpdate(Vertex vIn)
{
  if (vIn == NullVertex())
    return;
  
  ftr::State cState = stow->graph[vIn].state;

  //from highest to lowest priority.
  if (cState.test(ftr::StateOffset::Inactive))
    stow->graph[vIn].stateIconShared->setPixmap(inactivePixmap);
  else if (cState.test(ftr::StateOffset::Skipped))
    stow->graph[vIn].stateIconShared->setPixmap(skippedPixmap);
  else if (cState.test(ftr::StateOffset::ModelDirty))
    stow->graph[vIn].stateIconShared->setPixmap(pendingPixmap);
  else if (cState.test(ftr::StateOffset::Failure))
    stow->graph[vIn].stateIconShared->setPixmap(failPixmap);
  else
    stow->graph[vIn].stateIconShared->setPixmap(passPixmap);

  if (cState.test(ftr::StateOffset::NonLeaf) && (!cState.test(ftr::StateOffset::Editing)))
  {
    if (stow->graph[vIn].visibleIconShared->scene())
      removeItem(stow->graph[vIn].visibleIconShared.get());
    if (stow->graph[vIn].selectableIconShared->scene())
      removeItem(stow->graph[vIn].selectableIconShared.get());
  }
  else
  {
    if (!stow->graph[vIn].visibleIconShared->scene())
      addItem(stow->graph[vIn].visibleIconShared.get());
    if (!stow->graph[vIn].selectableIconShared->scene())
      addItem(stow->graph[vIn].selectableIconShared.get());
  }
  
  //set tool tip to current state.
  QString ts = tr("True");
  QString fs = tr("False");
  QString toolTip;
  QTextStream stream(&toolTip);
  stream <<
  "<table border=\"1\" cellpadding=\"6\">" << 
  "<tr>" <<
  "<td>" << tr("Model Dirty") << "</td><td>" << ((cState.test(ftr::StateOffset::ModelDirty)) ? ts : fs) << "</td>" <<
  "</tr><tr>" <<
  "<td>" << tr("Visual Dirty") << "</td><td>" << ((cState.test(ftr::StateOffset::VisualDirty)) ? ts : fs) << "</td>" <<
  "</tr><tr>" <<
  "<td>" << tr("Failure") << "</td><td>" << ((cState.test(ftr::StateOffset::Failure)) ? ts : fs) << "</td>" <<
  "</tr><tr>" <<
  "<td>" << tr("Skipped") << "</td><td>" << ((cState.test(ftr::StateOffset::Skipped)) ? ts : fs) << "</td>" <<
  "</tr><tr>" <<
  "<td>" << tr("Inactive") << "</td><td>" << ((cState.test(ftr::StateOffset::Inactive)) ? ts : fs) << "</td>" <<
  "</tr><tr>" <<
  "<td>" << tr("Non-Leaf") << "</td><td>" << ((cState.test(ftr::StateOffset::NonLeaf)) ? ts : fs) << "</td>" <<
  "</tr>" <<
  "</table>";
  stow->graph[vIn].stateIconShared->setToolTip(toolTip);
}

void Model::featureRenamedDispatched(const msg::Message &messageIn)
{
  ftr::Message fMessage = messageIn.getFTR();
  Vertex vertex = stow->findVertex(fMessage.featureId);
  if (vertex == NullVertex())
    return;
  stow->graph[vertex].textShared->setText(fMessage.string);
}

void Model::preselectionAdditionDispatched(const msg::Message &messageIn)
{
  slc::Message sMessage = messageIn.getSLC();
  if (sMessage.type != slc::Type::Object)
    return;
  Vertex vertex = stow->findVertex(sMessage.featureId);
  if (vertex == NullVertex())
    return;
  assert(!currentPrehighlight); //trying to set prehighlight when something is already set.
  if (currentPrehighlight)
    return;
  currentPrehighlight = stow->graph[vertex].rectShared.get();
  currentPrehighlight->preHighlightOn();
  
  invalidate();
}

void Model::preselectionSubtractionDispatched(const msg::Message &messageIn)
{
  slc::Message sMessage = messageIn.getSLC();
  if (sMessage.type != slc::Type::Object)
    return;
  Vertex vertex = stow->findVertex(sMessage.featureId);
  if (vertex == NullVertex())
    return;
  assert(currentPrehighlight); //trying to clear prehighlight when already empty.
  if (!currentPrehighlight)
    return;
  stow->graph[vertex].rectShared->preHighlightOff();
  currentPrehighlight = nullptr;
  
  invalidate();
}

void Model::selectionAdditionDispatched(const msg::Message &messageIn)
{
  slc::Message sMessage = messageIn.getSLC();
  if (sMessage.type != slc::Type::Object)
    return;
  Vertex vertex = stow->findVertex(sMessage.featureId);
  if (vertex == NullVertex())
    return;
  stow->graph[vertex].rectShared->selectionOn();
  for (const auto &e : stow->getAllEdges(vertex))
    stow->highlightConnectorOn(e, stow->graph[vertex].textShared->brush().color());
  
  lastPickValid = true;
  lastPick = stow->graph[vertex].rectShared->mapToScene(stow->graph[vertex].rectShared->rect().center());
  
  invalidate();
}

void Model::selectionSubtractionDispatched(const msg::Message &messageIn)
{
  slc::Message sMessage = messageIn.getSLC();
  if (sMessage.type != slc::Type::Object)
    return;
  Vertex vertex = stow->findVertex(sMessage.featureId);
  if (vertex == NullVertex())
    return;
  stow->graph[vertex].rectShared->selectionOff();
  for (const auto &e : stow->getAllEdges(vertex))
    stow->highlightConnectorOff(e);
  
  lastPickValid = false;
  
  invalidate();
}

void Model::selectionMaskDispatched(const msg::Message &mIn)
{
  selectionMask = mIn.getSLC().selectionMask;
}

void Model::closeProjectDispatched(const msg::Message&)
{
  removeAllItems();
  stow->graph.clear();
  invalidate();
}

void Model::projectUpdatedDispatched(const msg::Message &)
{
  //create a filtered graph on what is alive and visible.
  std::vector<Vertex> filterVertices;
  for (auto its = boost::vertices(stow->graph); its.first != its.second; ++its.first)
  {
    if (stow->graph[*its.first].dagVisible && stow->graph[*its.first].alive)
      filterVertices.push_back(*its.first);
  }
  gu::SubsetFilter<decltype(stow->graph)> filter(stow->graph, filterVertices);
  boost::filtered_graph<decltype(stow->graph), boost::keep_all, decltype(filter)> filteredGraph(stow->graph, boost::keep_all(), filter);
  
  //topo sort vertices.
  Path sorted;
  boost::topological_sort(filteredGraph, std::back_inserter(sorted));
  std::reverse(sorted.begin(), sorted.end()); //topo sort is reversed.
  
  
  /* I see no other way:
   * I have to loop vertices to set the sorted index on the vertices.
   * Then I have to loop edges to set the distance on the edges.
   * Then I have to loop vertices to set the receptacle offsets, so I can ignore edges with a distance of 1.
   * Then I have to loop vertices to set the background rectangle.
   * Hope it scales. 
   */
  
  //layout constant items
  std::size_t currentRow = 0;
  QRectF maxTextRect;
  QRectF maxConnectorRect;
  for (auto currentVertex : sorted)
  {
    //I can't calculate receptacle locations here. See note above.
    stow->graph[currentVertex].sortedIndex = currentRow;
    QBrush currentBrush(forgroundBrushes.at(currentRow % forgroundBrushes.size()));
    
    auto point = stow->graph[currentVertex].pointShared;
    point->setRect(pointSize / -2.0, 0, pointSize, pointSize);
    point->setTransform(QTransform::fromTranslate(0, rowHeight * currentRow + rowHeight / 2.0 - pointSize / 2.0));
    point->setBrush(currentBrush);
    
    float cheat = 0.0;
    if (direction == -1)
      cheat = rowHeight;
    
    float yValue = rowHeight * currentRow + cheat;
    stow->graph[currentVertex].visibleIconShared->setTransform(QTransform::fromTranslate(pointToIcon, yValue));
    stow->graph[currentVertex].overlayIconShared->setTransform(QTransform::fromTranslate(pointToIcon + iconToIcon, yValue));
    stow->graph[currentVertex].selectableIconShared->setTransform(QTransform::fromTranslate(pointToIcon + iconToIcon * 2, yValue));
    stow->graph[currentVertex].stateIconShared->setTransform(QTransform::fromTranslate(pointToIcon + iconToIcon * 3, yValue));
    stow->graph[currentVertex].featureIconShared->setTransform(QTransform::fromTranslate(pointToIcon + iconToIcon * 4, yValue));
    
    auto text = stow->graph[currentVertex].textShared;
    text->setBrush(currentBrush.color());
    maxTextLength = std::max(maxTextLength, static_cast<float>(text->boundingRect().width()));
    text->setTransform(QTransform::fromTranslate (pointToIcon + iconToIcon * 4 + iconToText, rowHeight * currentRow - verticalSpacing * 2.0 + cheat));
    QRectF textRect = text->boundingRect();
    textRect.translate(text->transform().dx(), text->transform().dy());
    
    if (textRect.width() > maxTextRect.width())
      maxTextRect = textRect;
    
    //we can have a graph with no edges. That will cause maxConnectorRect to be left in its null state
    //and will mess up the background rect size. So here we will set it to a point as a fallback
    if (maxConnectorRect.isNull())
      maxConnectorRect = point->rect();
    
    currentRow++;
  }
  
  //set sorted distance on edges and then sort
  //this should process the shorted distance edges first and help
  //minimize edge crossings.
  std::vector<Edge> sortedEdges;
  for (auto its = boost::edges(filteredGraph); its.first != its.second; ++its.first)
  {
    Edge e = *its.first;
    sortedEdges.emplace_back(e);
    decltype(filteredGraph) &g = filteredGraph;
    g[e].sortedDistance = static_cast<int>(g[boost::target(e, g)].sortedIndex) - static_cast<int>(g[boost::source(e, g)].sortedIndex);
  }
  std::sort(sortedEdges.begin(), sortedEdges.end(), [&filteredGraph](Edge e0, Edge e1){return filteredGraph[e0].sortedDistance < filteredGraph[e1].sortedDistance;});
  
  //use for drawing connectors
  struct Receptacle
  {
    qreal yOffset = 0.0;
    bool used = false;
  };
  
  struct VertexState
  {
    Vertex vertex;
    int maxColumnOut = 0;
    int maxColumnIn = 0;
    int maxColumnPassing = 0;
    std::vector<Receptacle> receptacles;
    
    VertexState() = delete;
    VertexState(Vertex vIn) : vertex(vIn){}
    qreal nextInput()
    {
      for (auto &receptacle : receptacles)
      {
        if (!receptacle.used)
        {
          receptacle.used = true;
          return receptacle.yOffset;
        }
      }
      assert(0); //couldn't find an unused receptacle. Shouldn't happen.
      return 0.0;
    }
    qreal nextOutput()
    {
      for (auto it = receptacles.rbegin(); it != receptacles.rend(); ++it)
      {
        if (!it->used)
        {
          it->used = true;
          return it->yOffset;
        }
      }
      assert(0); //couldn't find an unused receptacle. Shouldn't happen.
      return 0.0;
    }
  };
  std::vector<VertexState> vertexStates;
  
  for (auto currentVertex : sorted)
  {
    vertexStates.emplace_back(currentVertex);
    int degree = boost::degree(currentVertex, filteredGraph);
    
    //we ignore any edges that have a distance of 1. they
    //get connected into a straight line between the two
    //vertices and won't need any receptacles.
    for (auto its = boost::in_edges(currentVertex, filteredGraph); its.first != its.second; ++its.first)
    {
      if (filteredGraph[*its.first].sortedDistance == 1)
        degree--;
    }
    for (auto its = boost::out_edges(currentVertex, filteredGraph); its.first != its.second; ++its.first)
    {
      if (filteredGraph[*its.first].sortedDistance == 1)
        degree--;
    }
    
    if (degree == 1)
    {
      vertexStates.back().receptacles.emplace_back();
      vertexStates.back().receptacles.back().yOffset = pointSize / 2.0;
    }
    else if (degree != 0)
    {
      qreal spacing = pointSize / (degree - 1);
      for (int index = 0 ; index < degree ; ++index)
      {
        vertexStates.back().receptacles.emplace_back();
        vertexStates.back().receptacles.back().yOffset = spacing * index;
      }
    }
  }
  
  auto findStateVertex = [&](Vertex vIn) -> std::vector<VertexState>::iterator
  {
    for (auto it = vertexStates.begin(); it != vertexStates.end(); ++it)
    {
      if (vIn == it->vertex)
        return it;
    }
    assert(0); //couldn't find vertex state
    return vertexStates.end();
  };

  for (auto edge : sortedEdges)
  {
    auto sv = boost::source(edge, filteredGraph);
    auto tv = boost::target(edge, filteredGraph);
    QGraphicsPathItem *pathItem = filteredGraph[edge].connector.get();
    pathItem->setBrush(Qt::NoBrush);
    QPainterPath path;
    
    if (filteredGraph[edge].sortedDistance == 1)
    {
      //update with straight line in y at x 0, but don't count against vertex and edge states
      path.moveTo(0.0, filteredGraph[sv].pointShared->transform().dy() + pointSize / 2.0);
      path.lineTo(0.0, filteredGraph[tv].pointShared->transform().dy() + pointSize / 2.0);
      pathItem->setPath(path);
      if (pathItem->boundingRect().width() > maxConnectorRect.width())
        maxConnectorRect = pathItem->boundingRect();
      continue;
    }
    
    /* now the tricky part.
      calculating line offset:
        ignore how many lines are coming in to the source.
        ignore how many lines are going out of target.
        consider both in and out of the sorted vertices between source and target.
      in and out of vertex:
        we do the closest edges first, so we have to consider the number of
        total in and out edges to calculate the y offset with minimal crossing.
    */
    auto sourceIterator = findStateVertex(sv);
    auto targetIterator = findStateVertex(tv);
    assert(std::distance(sourceIterator, targetIterator) > 1);
    
    //find greatest offset relative to range source to vertex
    int offset = std::max(sourceIterator->maxColumnOut, targetIterator->maxColumnIn);
    offset = std::max(offset, sourceIterator->maxColumnPassing);
    offset = std::max(offset, targetIterator->maxColumnPassing);
    auto middleIterator = sourceIterator + 1;
    while(middleIterator != targetIterator)
    {
      offset = std::max(offset, middleIterator->maxColumnIn);
      offset = std::max(offset, middleIterator->maxColumnOut);
      offset = std::max(offset, middleIterator->maxColumnPassing);
      middleIterator++;
    }
    offset++; //get one increment beyond
    
    //now that we have found the value we have to update
    sourceIterator->maxColumnOut = offset;
    targetIterator->maxColumnIn = offset;
    middleIterator = sourceIterator + 1;
    while(middleIterator != targetIterator)
    {
      middleIterator->maxColumnPassing = offset;
      middleIterator++;
    }
    
    //now we can use offset for drawing
    qreal edgeSpacing = pointSize / 4.0;
    qreal xPosition = edgeSpacing * offset + iconSize / 2.0;
    qreal yOut = filteredGraph[sv].pointShared->transform().dy() + sourceIterator->nextOutput();
    qreal yIn = filteredGraph[tv].pointShared->transform().dy() + targetIterator->nextInput();
    path.moveTo(0.0, yOut);
    path.lineTo(-xPosition, yOut);
    path.lineTo(-xPosition, yIn);
    path.lineTo(0.0, yIn);
    pathItem->setPath(path);
    if (pathItem->boundingRect().width() > maxConnectorRect.width())
        maxConnectorRect = pathItem->boundingRect();
  }
  
  //now set the the rectangle background items.
  QRectF rowSizeRect = maxTextRect | maxConnectorRect;
  qreal rowWidth = rowSizeRect.width() + 2 * rowPadding;
  qreal rowX = -maxConnectorRect.width() - rowPadding;
  for (auto v : sorted)
  {
    qreal yStart = rowHeight * filteredGraph[v].sortedIndex;
    auto rectangle = filteredGraph[v].rectShared;
    rectangle->setRect(rowX, yStart, rowWidth, rowHeight); //calculate actual size later.
    rectangle->setBackgroundBrush(backgroundBrushes[filteredGraph[v].sortedIndex % backgroundBrushes.size()]);
  }
  
  this->setSceneRect(this->itemsBoundingRect());
}

void Model::dumpDAGViewGraphDispatched(const msg::Message &)
{
  //something here to check preferences about writing this out.
  boost::filesystem::path path = app::instance()->getApplicationDirectory();
  path /= "dagView.dot";
  stow->writeGraphViz(path.string());
  
  QDesktopServices::openUrl(QUrl(QString::fromStdString(path.string())));
}

void Model::threeDShowDispatched(const msg::Message &msgIn)
{
  Vertex vertex = stow->findVertex(msgIn.getVWR().featureId);
  if (vertex == NullVertex())
    return;
  stow->graph[vertex].visibleIconShared->setPixmap(visiblePixmapEnabled);
}

void Model::threeDHideDispatched(const msg::Message &msgIn)
{
  Vertex vertex = stow->findVertex(msgIn.getVWR().featureId);
  if (vertex == NullVertex())
    return;
  stow->graph[vertex].visibleIconShared->setPixmap(visiblePixmapDisabled);
}

void Model::overlayShowDispatched(const msg::Message &msgIn)
{
  Vertex vertex = stow->findVertex(msgIn.getVWR().featureId);
  if (vertex == NullVertex())
    return;
  stow->graph[vertex].overlayIconShared->setPixmap(overlayPixmapEnabled);
}

void Model::overlayHideDispatched(const msg::Message &msgIn)
{
  Vertex vertex = stow->findVertex(msgIn.getVWR().featureId);
  if (vertex == NullVertex())
    return;
  stow->graph[vertex].overlayIconShared->setPixmap(overlayPixmapDisabled);
}

void Model::featureSelectionThawDispatched(const msg::Message &mIn)
{
  assert(mIn.isSLC());
  Vertex vertex = stow->findVertex(mIn.getSLC().featureId);
  if (vertex == NullVertex())
    return;
  stow->graph[vertex].selectableIconShared->setPixmap(selectablePixmapEnabled);
}

void Model::featureSelectionFreezeDispatched(const msg::Message &mIn)
{
  assert(mIn.isSLC());
  Vertex vertex = stow->findVertex(mIn.getSLC().featureId);
  if (vertex == NullVertex())
    return;
  stow->graph[vertex].selectableIconShared->setPixmap(selectablePixmapDisabled);
}

void Model::commandActiveDispatched(const msg::Message &)
{
  commandActive = true;
}

void Model::commandInactiveDispatched(const msg::Message &)
{
  commandActive = false;
}

void Model::removeAllItems()
{
  for (auto its = boost::vertices(stow->graph); its.first != its.second; ++its.first)
  {
    if (!stow->graph[*its.first].alive)
      continue;
    removeItemsFromScene(stow->getAllSceneItems(*its.first));
  }
  
  for (auto its = boost::edges(stow->graph); its.first != its.second; ++its.first)
  {
    if (stow->graph[*its.first].connector->scene())
      this->removeItem(stow->graph[*its.first].connector.get());
  }
}

void Model::addItemsToScene(std::vector<QGraphicsItem*> items)
{
  for (auto *item : items)
  {
    if (!item->scene())
      this->addItem(item);
  }
}

void Model::removeItemsFromScene(std::vector<QGraphicsItem*> items)
{
  for (auto *item : items)
  {
    if (item->scene())
      this->removeItem(item);
  }
}

RectItem* Model::getRectFromPosition(const QPointF& position)
{
  auto theItems = this->items(position, Qt::IntersectsItemBoundingRect, Qt::DescendingOrder);
  if (theItems.isEmpty())
    return nullptr;
  return dynamic_cast<RectItem *>(theItems.back());
}

void Model::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
  QGraphicsScene::mouseMoveEvent(event);
  
  if (dragData)
  {
    RectItem *rect = getRectFromPosition(event->scenePos());
    QGraphicsView *qgv = this->views().front();
    
    //search for edge first as it is more explicit.
    Vertex dv = stow->findVertex(dragData->featureId); //drag vertex
    auto pi = stow->graph[dv].pointShared.get(); //point item.
    QList<QGraphicsItem *> cis = collidingItems(pi);
    bool foundIntersection = false;
    for (QGraphicsItem* gi : cis)
    {
      if (gi->data(qtd::key).toInt() != qtd::connector)
        continue;
      if (pi->collidesWithItem(gi))
      {
        QGraphicsPathItem *npi = static_cast<QGraphicsPathItem *>(gi); //new path item
        Edge edge = stow->findEdge(npi);
        if (dragData->isAcceptedEdge(edge))
        {
          foundIntersection = true;
          if (dragData->lastHighlight)
            stow->highlightConnectorOff(dragData->lastHighlight);
          stow->highlightConnectorOn(npi, Qt::darkYellow);
          dragData->lastHighlight = npi;
          node->send(msg::buildStatusMessage(QObject::tr("Drop accepted on edge").toStdString()));
          break;
        }
      }
    }
    if (!foundIntersection)
    {
      if (dragData->lastHighlight)
      {
        stow->highlightConnectorOff(dragData->lastHighlight);
        dragData->lastHighlight = nullptr;
      }
    }
    
    //now search for a vertex.
    if (!foundIntersection && rect)
    {
      Vertex rv = stow->findVertex(rect);
      if (rv != NullVertex())
      {
        const auto &av = dragData->acceptedVertices; //just makes next line readable
        if (std::find(av.begin(), av.end(), rv) != av.end())
        {
          foundIntersection = true;
          node->send(msg::buildStatusMessage(QObject::tr("Drop accepted on vertex").toStdString()));
        }
      }
    }
    if (!foundIntersection)
    {
      qgv->setCursor(Qt::ForbiddenCursor);
      node->send(msg::buildStatusMessage(QObject::tr("Drop rejected").toStdString()));
    }
    else
      qgv->setCursor(Qt::DragMoveCursor);
  }
  else //not in a drag operation
  {
    auto buildMessage = [&](Vertex vertex)
    {
      slc::Message sMessage;
      sMessage.type = slc::Type::Object;
      sMessage.featureId = stow->graph[vertex].featureId;
      sMessage.shapeId = gu::createNilId();
      return sMessage;
    };
    
    auto clearPrehighlight = [&]()
    {
      if (!currentPrehighlight)
        return;
      Vertex vertex = stow->findVertex(currentPrehighlight);
      if (vertex == NullVertex())
        return;
      node->send(msg::Message(msg::Request | msg::Preselection | msg::Remove, buildMessage(vertex)));
    };
    
    auto setPrehighlight = [&](RectItem *rectIn)
    {
      Vertex vertex = stow->findVertex(rectIn);
      if (vertex == NullVertex())
        return;
      
      if (commandActive)
      {
        //only consider the selection mask when a command is active.
        if
        (
          (selectionMask & slc::ObjectsEnabled).any()
          && (selectionMask & slc::ObjectsSelectable).any()
        )
        {
          node->send(msg::Message(msg::Request | msg::Preselection | msg::Add, buildMessage(vertex)));
        }
      }
      else
        node->send(msg::Message(msg::Request | msg::Preselection | msg::Add, buildMessage(vertex)));
    };
    
    
    //don't prehighlight if we are on a interactive control
    RectItem *rect = nullptr;
    auto theItems = this->items(event->scenePos(), Qt::IntersectsItemBoundingRect, Qt::DescendingOrder);
    if (!theItems.empty())
    {
      int t = theItems.front()->data(qtd::key).toInt();
      if (t != qtd::point && t != qtd::visibleIcon && t != qtd::overlayIcon && t != qtd::selectableIcon && t != qtd::connector)
        rect = dynamic_cast<RectItem *>(theItems.back());
    }
    
    if (rect == currentPrehighlight)
      return;
    else
    {
      clearPrehighlight();
      if (rect)
        setPrehighlight(rect);
    }
  }
}

void Model::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
  QGraphicsScene::mousePressEvent(event); //make sure we don't skip this.
  
  auto select = [this](const boost::uuids::uuid &featureIdIn, msg::Mask actionIn)
  {
    assert((actionIn == msg::Add) || (actionIn == msg::Remove));
    if (featureIdIn.is_nil())
      return;
    if
    (
      (
        (selectionMask & slc::ObjectsEnabled).none()
        || (selectionMask & slc::ObjectsSelectable).none()
      )
      && commandActive
    )
      return;
    slc::Message sMessage;
    sMessage.type = slc::Type::Object;
    sMessage.featureId = featureIdIn;
    sMessage.shapeId = gu::createNilId();
    msg::Message message
    (
      msg::Request | msg::Selection | actionIn
      , sMessage
    );
    node->send(message);
  };
  
  auto getFeatureIdFromRect = [this](RectItem *rectIn)
  {
    assert(rectIn);
    Vertex vertex = stow->findVertex(rectIn);
    if (vertex == NullVertex())
      return gu::createNilId();
    return stow->graph[vertex].featureId;
  };
  
  auto goShiftSelect = [this, event, select, getFeatureIdFromRect]()
  {
    QPointF currentPickPoint = event->scenePos();
    QGraphicsLineItem intersectionLine(QLineF(lastPick, currentPickPoint));
    QList<QGraphicsItem *>selection = collidingItems(&intersectionLine);
    for (auto currentItem = selection.begin(); currentItem != selection.end(); ++currentItem)
    {
      RectItem *rect = dynamic_cast<RectItem *>(*currentItem);
      if (!rect || rect->isSelected())
        continue;
      select(getFeatureIdFromRect(rect), msg::Add);
    }
  };
  
  auto toggleSelect = [select, getFeatureIdFromRect](RectItem *rectIn)
  {
    if (rectIn->isSelected())
      select(getFeatureIdFromRect(rectIn), msg::Remove);
    else
      select(getFeatureIdFromRect(rectIn), msg::Add);
  };
  
  if (event->button() == Qt::LeftButton)
  {
    auto theItems = this->items(event->scenePos(), Qt::IntersectsItemBoundingRect, Qt::DescendingOrder);
    if (theItems.isEmpty())
      return;

    int currentType = theItems.front()->data(qtd::key).toInt();
    if (currentType == qtd::visibleIcon)
    {
      QGraphicsPixmapItem *currentPixmap = dynamic_cast<QGraphicsPixmapItem *>(theItems.front());
      assert(currentPixmap);
      
      Vertex vertex = stow->findVisibleVertex(currentPixmap);
      if (vertex == NullVertex())
        return;
      msg::Message msg
      (
        msg::Request | msg::View | msg::Toggle | msg::ThreeD
        , vwr::Message(stow->graph[vertex].featureId)
      );
      node->send(msg);
      
      return;
    }
    else if (currentType == qtd::overlayIcon)
    {
      QGraphicsPixmapItem *currentPixmap = dynamic_cast<QGraphicsPixmapItem *>(theItems.front());
      assert(currentPixmap);
      
      Vertex vertex = stow->findOverlayVertex(currentPixmap);
      if (vertex == NullVertex())
        return;
      msg::Message msg
      (
        msg::Request | msg::View | msg::Toggle | msg::Overlay
        , vwr::Message(stow->graph[vertex].featureId)
      );
      node->send(msg);
      
      return;
    }
    else if (currentType == qtd::selectableIcon)
    {
      QGraphicsPixmapItem *currentPixmap = dynamic_cast<QGraphicsPixmapItem *>(theItems.front());
      assert(currentPixmap);
      
      Vertex vertex = stow->findSelectableVertex(currentPixmap);
      if (vertex == NullVertex())
        return;
      
      slc::Message sMsg;
      sMsg.featureId = stow->graph[vertex].featureId;
      msg::Message msg(msg::Request | msg::Selection | msg::Feature | msg::Toggle, sMsg);
      node->send(msg);
      
      return;
    }
    else if (currentType == qtd::point)
    {
      QGraphicsEllipseItem *ellipse = dynamic_cast<QGraphicsEllipseItem *>(theItems.front());
      assert(ellipse);
      
      Vertex vertex = stow->findVertex(ellipse);
      if (vertex == NullVertex())
        return;
      
      assert(!dragData);
      dragData = std::shared_ptr<DragData>(new DragData());
      dragData->featureId = stow->graph[vertex].featureId;
      std::tie(dragData->acceptedVertices, dragData->acceptedEdges) = stow->getDropAccepted(vertex);
      
      node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
      
      return;
    }
    
    //now going with selection.
    RectItem *rect = getRectFromPosition(event->scenePos());
    if (rect)
    {
      if((event->modifiers() & Qt::ShiftModifier) && lastPickValid)
      {
        goShiftSelect();
      }
      else
      {
        toggleSelect(rect);
      }
    }
  }
  else if (event->button() == Qt::MiddleButton)
  {
    node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  }
}

void Model::mouseReleaseEvent(QGraphicsSceneMouseEvent *e)
{
  QGraphicsScene::mouseReleaseEvent(e); //make sure we don't skip this.
  
  if ((e->button() == Qt::LeftButton) && dragData)
  {
    //drag operation sets the ellipse xy location. set it back.
    Vertex dv = stow->findVertex(dragData->featureId);
    stow->graph[dv].pointShared->setX(0.0);
    stow->graph[dv].pointShared->setY(0.0);
    
    bool sentMessage = false;
    
    //make sure no edges are left highlighted.
    if (dragData->lastHighlight)
    {
      Edge edge = stow->findEdge(dragData->lastHighlight);
      stow->highlightConnectorOff(dragData->lastHighlight);
      dragData->lastHighlight = nullptr;
      prj::Message pmOut;
      pmOut.featureIds.push_back(dragData->featureId);
      pmOut.featureIds.push_back(stow->graph[boost::source(edge, stow->graph)].featureId);
      pmOut.featureIds.push_back(stow->graph[boost::target(edge, stow->graph)].featureId);
      msg::Message mOut(msg::Mask(msg::Request | msg::Project | msg::Feature | msg::Reorder), pmOut);
      node->send(mOut);
      sentMessage = true;
    }
    else
    {
      RectItem *rect = getRectFromPosition(e->scenePos());
      if (rect)
      {
        Vertex rv = stow->findVertex(rect);
        if (rv != NullVertex())
        {
          if (std::find(dragData->acceptedVertices.begin(), dragData->acceptedVertices.end(), rv) != dragData->acceptedVertices.end())
          {
            prj::Message pmOut;
            pmOut.featureIds.push_back(dragData->featureId);
            pmOut.featureIds.push_back(stow->graph[rv].featureId);
            msg::Message mOut(msg::Mask(msg::Request | msg::Project | msg::Feature | msg::Reorder), pmOut);
            node->send(mOut);
            sentMessage = true;
          }
        }
      }
    }
    
    if (!sentMessage)
      projectUpdatedDispatched(msg::Message());
    
    //take action.
    QGraphicsView *qgv = this->views().front();
    qgv->setCursor(Qt::ArrowCursor);
    dragData.reset();
  }
}

void Model::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
  if (event->button() == Qt::LeftButton)
    node->send(msg::Message(msg::Request | msg::Edit | msg::Feature));
  
  QGraphicsScene::mouseDoubleClickEvent(event);
}

void Model::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
  auto theItems = this->items(event->scenePos(), Qt::IntersectsItemBoundingRect, Qt::DescendingOrder);
  if (!theItems.isEmpty())
  {
    int currentType = theItems.front()->data(qtd::key).toInt();
    if (currentType == qtd::visibleIcon)
    {
      Vertex v = stow->findVisibleVertex(dynamic_cast<QGraphicsPixmapItem*>(theItems.front()));
      node->send(msg::Message(msg::Request | msg::View | msg::ThreeD | msg::Isolate, vwr::Message(stow->graph[v].featureId)));
      return;
    }
    else if (currentType == qtd::overlayIcon)
    {
      Vertex v = stow->findOverlayVertex(dynamic_cast<QGraphicsPixmapItem*>(theItems.front()));
      node->send(msg::Message(msg::Request | msg::View | msg::Overlay | msg::Isolate, vwr::Message(stow->graph[v].featureId)));
      return;
    }
  }
  
  RectItem *rect = getRectFromPosition(event->scenePos());
  if (rect)
  {
    Vertex vertex = stow->findVertex(rect);
    if (vertex == NullVertex())
      return;
    if (!rect->isSelected())
    {
      auto localSend = [&]()
      {
        slc::Message sMessage;
        sMessage.type = slc::Type::Object;
        sMessage.featureId = stow->graph[vertex].featureId;
        msg::Message message
        (
          msg::Request | msg::Selection | msg::Add
          , sMessage
        );
        node->send(message);
      };
      
      if (commandActive)
      {
        //consider selection mask only when command active
        if
        (
          (selectionMask & slc::ObjectsEnabled).any()
          && (selectionMask & slc::ObjectsSelectable).any()
        )
        {
          localSend();
        }
      }
      else
        localSend();
    }
    
    QMenu contextMenu;
    
    static QIcon overlayIcon(":/resources/images/dagViewOverlay.svg");
    QAction* toggleOverlayAction = contextMenu.addAction(overlayIcon, tr("Toggle Overlay"));
    connect(toggleOverlayAction, SIGNAL(triggered()), this, SLOT(toggleOverlaySlot()));
    
    static QIcon selectableIcon(":/resources/images/dagViewSelectable.svg");
    QAction* toggleSelectableAction = contextMenu.addAction(selectableIcon, tr("Toggle Selectable"));
    connect(toggleSelectableAction, SIGNAL(triggered()), this, SLOT(toggleSelectableSlot()));
    
    static QIcon viewIsolateIcon(":/resources/images/viewIsolate.svg");
    QAction* viewIsolateAction = contextMenu.addAction(viewIsolateIcon, tr("View Isolate"));
    connect(viewIsolateAction, SIGNAL(triggered()), this, SLOT(viewIsolateSlot()));
    
    contextMenu.addSeparator();
    
    static QIcon infoFeatureIcon(":/resources/images/inspectInfo.svg");
    QAction* infoFeatureAction = contextMenu.addAction(infoFeatureIcon, tr("Info Feature"));
    connect(infoFeatureAction, SIGNAL(triggered()), this, SLOT(infoFeatureSlot()));
    
    static QIcon checkGeometryIcon(":/resources/images/inspectCheckGeometry.svg");
    QAction* checkGeometryAction = contextMenu.addAction(checkGeometryIcon, tr("Check Geometry"));
    connect(checkGeometryAction, SIGNAL(triggered()), this, SLOT(checkGeometrySlot()));
    
    static QIcon editFeatureIcon(":/resources/images/editFeature.svg");
    QAction* editFeatureAction = contextMenu.addAction(editFeatureIcon, tr("Edit Feature"));
    connect(editFeatureAction, SIGNAL(triggered()), this, SLOT(editFeatureSlot()));
    
    static QIcon dissolveIcon(":/resources/images/editFeatureDissolve.svg");
    QAction* dissolveAction = contextMenu.addAction(dissolveIcon, tr("Dissolve Feature"));
    connect(dissolveAction, SIGNAL(triggered()), this, SLOT(dissolveSlot()));
    
    static QIcon editColorIcon(":/resources/images/editColor.svg");
    QAction* editColorAction = contextMenu.addAction(editColorIcon, tr("Edit Color"));
    connect(editColorAction, SIGNAL(triggered()), this, SLOT(editColorSlot()));
    
    static QIcon editRenameIcon(":/resources/images/editRename.svg");
    QAction* editRenameAction = contextMenu.addAction(editRenameIcon, tr("Rename"));
    connect(editRenameAction, SIGNAL(triggered()), this, SLOT(editRenameSlot()));
    
    contextMenu.addSeparator();
    
    static QIcon leafIcon(":/resources/images/dagViewLeaf.svg");
    QAction* setCurrentLeafAction = contextMenu.addAction(leafIcon, tr("Set Current Leaf"));
    connect(setCurrentLeafAction, SIGNAL(triggered()), this, SLOT(setCurrentLeafSlot()));
    
    static QIcon skippedIcon(":/resources/images/dagViewSkipped.svg");
    QAction* toggleSkippedAction = contextMenu.addAction(skippedIcon, tr("Toggle Skipped"));
    connect(toggleSkippedAction, SIGNAL(triggered()), this, SLOT(toggleSkippedSlot()));
    
    static QIcon removeIcon(":/resources/images/editRemove.svg");
    QAction* removeFeatureAction = contextMenu.addAction(removeIcon, tr("Remove Feature"));
    connect(removeFeatureAction, SIGNAL(triggered()), this, SLOT(removeFeatureSlot()));
    
    std::vector<Vertex> selected = stow->getAllSelected();
    
    //disable actions that work for only 1 feature at a time.
    if (selected.size() != 1)
    {
      editRenameAction->setDisabled(true);
      editFeatureAction->setDisabled(true);
      setCurrentLeafAction->setDisabled(true);
    }
    
    //disable entries if no seer shape present.
    bool hasSeerShape = true;
    for (const auto &v : selected)
    {
      if (!stow->graph[v].hasSeerShape)
      {
        hasSeerShape = false;
        break;
      }
    }
    if (!hasSeerShape)
    {
      checkGeometryAction->setDisabled(true);
      dissolveAction->setDisabled(true);
    }
    
    contextMenu.exec(event->screenPos());
    return;
  }
  
  //passes event to graphics item at position or ignores.
  QGraphicsScene::contextMenuEvent(event);
}

void Model::setCurrentLeafSlot()
{
  auto currentSelections = stow->getAllSelected();
  assert(currentSelections.size() == 1);
  
  prj::Message prjMessageOut;
  prjMessageOut.featureIds.push_back(stow->graph[currentSelections.front()].featureId);
  msg::Message messageOut(msg::Request | msg::SetCurrentLeaf, prjMessageOut);
  node->send(messageOut);
  
  if (prf::manager().rootPtr->dragger().triggerUpdateOnFinish())
    node->send(msg::Mask(msg::Request | msg::Project | msg::Update));
}

void Model::removeFeatureSlot()
{
  msg::Message message(msg::Request | msg::Remove);
  node->send(message);
}

void Model::toggleOverlaySlot()
{
  auto currentSelections = stow->getAllSelected();
  
  msg::Message message(msg::Request | msg::Selection | msg::Clear);
  node->send(message);
  
  for (auto v : currentSelections)
  {
    msg::Message mOut
    (
      msg::Request | msg::View | msg::Toggle | msg::Overlay
      , vwr::Message(stow->graph[v].featureId)
    );
    node->send(mOut);
  }
}

void Model::toggleSelectableSlot()
{
  auto currentSelections = stow->getAllSelected();
  
  msg::Message message(msg::Request | msg::Selection | msg::Clear);
  node->send(message);
  
  for (auto v : currentSelections)
  {
    //this should only be relative to leafs
    if (!stow->graph[v].visibleIconShared->scene())
      continue;
    
    slc::Message sm;
    sm.featureId = stow->graph[v].featureId;
    node->send(msg::Message(msg::Request | msg::Selection | msg::Feature | msg::Toggle, sm));
  }
}

void Model::viewIsolateSlot()
{
  node->send(msg::Message(msg::Request | msg::View | msg::ThreeD | msg::Overlay | msg::Isolate));
}

void Model::editColorSlot()
{
  node->send(msg::Message(msg::Request | msg::Edit | msg::Feature | msg::Color));
}

void Model::editRenameSlot()
{
  std::vector<Vertex> selections = stow->getAllSelected();
  assert(selections.size() == 1);
  
  auto text = stow->graph[selections.front()].textShared;
  bool results;
  QString freshName = QInputDialog::getText(app::instance()->getMainWindow(), tr("Feature Rename"), tr("New Name"), QLineEdit::Normal, text->text(), &results);
  if (results && !freshName.isEmpty())
  {
    ftr::Message fm(stow->graph[selections.front()].featureId, freshName);
    msg::Message m(msg::Request | msg::Edit | msg::Feature | msg::Name, fm);
    node->send(m); //don't block rename makes it back here.
    projectUpdatedDispatched(msg::Message()); //this will size the background rects to match.
  }
}

void Model::editFeatureSlot()
{
  node->send(msg::Message(msg::Request | msg::Edit | msg::Feature));
}

void Model::dissolveSlot()
{
  node->send(msg::Message(msg::Request | msg::Feature | msg::Dissolve));
}

void Model::infoFeatureSlot()
{
  node->send(msg::Message(msg::Request | msg::Info));
}

void Model::checkGeometrySlot()
{
  node->send(msg::Message(msg::Request | msg::CheckGeometry));
}

void Model::toggleSkippedSlot()
{
  std::vector<Vertex> selections = stow->getAllSelected();
  if (selections.empty())
    return;
  prj::Message pm;
  for (const auto &v : selections)
    pm.featureIds.push_back(stow->graph[v].featureId);
  
  node->send(msg::Message(msg::Request | msg::Feature | msg::Skipped | msg::Toggle, pm));
  
  if (prf::manager().rootPtr->dragger().triggerUpdateOnFinish())
    node->send(msg::Mask(msg::Request | msg::Project | msg::Update));
}
