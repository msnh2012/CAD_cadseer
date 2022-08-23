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

#include <TopoDS.hxx>
#include <BRep_Tool.hxx>

#include <QLabel>
#include <QAction>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsEllipseItem>
#include <QLineF>

#include "globalutilities.h"
#include "application/appapplication.h"
#include "project/prjproject.h"
#include "annex/annseershape.h"
#include "message/msgmessage.h"
#include "message/msgnode.h"
#include "message/msgsift.h"
#include "commandview/cmvselectioncue.h"
#include "commandview/cmvtable.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "tools/featuretools.h"
#include "tools/occtools.h"
#include "law/lwfvessel.h"
#include "law/lwfadapter.h"
#include "feature/ftrpick.h"
#include "feature/ftrlawspine.h"
#include "command/cmdlawspine.h"
#include "dialogs/dlgsplitterdecorated.h"
#include "commandview/cmvdraglist.h"
#include "commandview/cmvlawspine.h"

using boost::uuids::uuid;

namespace
{
  class GraphicsScene : public QGraphicsScene
  {
  public:
    const lwf::Vessel &vessel;
    
    GraphicsScene(QObject *parent, const lwf::Vessel &vIn)
    : QGraphicsScene(parent)
    , vessel(vIn)
    {}
    
    void generate(int steps)
    {
      //WTF. Qt y axis is inverted.
      //WTF. Ellipse item is defined by a rect. eyeroll.
      clear();
      
      /* I want the point size to be relative to scene size. We don't
       * know the scene size until we have looped through all points.
       * So we will build the point with unit values and set the scale
       * in another loop of the graphics items.
       */
      
      //build points
      std::vector<QGraphicsEllipseItem*> graphicPoints;
      auto buildPoint = [&]() -> QGraphicsEllipseItem*
      {
        QPen pointPen;
        pointPen.setWidthF(0.001);
        QBrush pointBrush(Qt::blue);
        return addEllipse(-0.5, -0.5, 1, 1, pointPen, pointBrush);
      };
      for (int index = 0; index < vessel.getPointCount(); ++index)
      {
        const prm::Parameters prms = vessel.getPointParameters(index); assert(prms.size() > 1);
        QGraphicsEllipseItem *point = buildPoint();
        double x = prms.front()->getDouble();
        double y = prms.at(1)->getDouble();
        point->setPos(x, y);
        graphicPoints.push_back(point);
      }
      qreal pointFactor = std::max(width(), height()) * 0.03;
      for (auto *gp : graphicPoints)
        gp->setScale(pointFactor);
      
      //build lines.
      auto drawLine = [&] (const osg::Vec3d &p0, const osg::Vec3d &p1)
      {
        QPen pen;
        pen.setColor(Qt::red);
        pen.setWidthF(std::max(width(), height()) * 0.01);
        auto *line = addLine(p0.x(), p0.y(), p1.x(), p1.y(), pen);
        line->setZValue(-0.1);
      };
      auto meshPoints = vessel.mesh(steps);
      if (meshPoints.size() < 2) return;
      for (int index = 0; index < static_cast<int>(meshPoints.size()) - 1; ++index)
        drawLine(meshPoints.at(index), meshPoints.at(index + 1));
    }
  };
  
  class GraphicsView : public QGraphicsView
  {
  public:
    GraphicsView(QGraphicsScene *scene, QWidget *parent) : QGraphicsView(scene, parent)
    {
      setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
      setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      connect
      (
        &resizeTimer,
        &QTimer::timeout,
        [&]()
        {
          fitToData();
          resizeTimer.stop();
        }
      );
      QTransform inverse;
      inverse.scale(1.0, -1.0);
      setTransform(inverse);
    }
    void fitToData()
    {
      fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
    }
  protected:
    void resizeEvent (QResizeEvent * event) override
    {
      QGraphicsView::resizeEvent(event);
      resizeTimer.start(1000);
    }
  private:
    QTimer resizeTimer;
  };
}

using namespace cmv;

struct LawSpine::Stow
{
  cmd::LawSpine *command = nullptr;
  cmv::LawSpine *view = nullptr;
  DragList *lawList = nullptr;
  QLabel *statusLabel = nullptr;
  dlg::SplitterDecorated *mainSplitter = nullptr;
  dlg::SplitterDecorated *pointSplitter = nullptr;
  QListWidget *pointList = nullptr;
  cmv::tbl::Model *prmModel = nullptr;
  cmv::tbl::View *prmView = nullptr;
  cmv::tbl::Model *pointPrmModel = nullptr;
  cmv::tbl::View *pointPrmView = nullptr;
  GraphicsScene *graphicsScene = nullptr;
  GraphicsView *graphicsView = nullptr;
  QAction *removePointAction = nullptr;
  QAction *togglePointTypeAction = nullptr;
  bool isGlobalPointSelection = false;
  std::optional<lwf::Adapter> adapter;
  
  Stow(cmd::LawSpine *cIn, cmv::LawSpine *vIn)
  : command(cIn)
  , view(vIn)
  {
    buildGui();
    loadFeatureData();
    glue();
    formAdapter();
    setPointListState();
    setGraphicsViewState();
    mainSplitter->restoreSettings("cmv::LawSpine::mainSplitter");
    pointSplitter->restoreSettings("cmv::LawSpine::pointSplitter");
  }
  
  void buildGui()
  {
    QVBoxLayout *mainLayout = new QVBoxLayout();
    view->setLayout(mainLayout);
    Base::clearContentMargins(view);
    view->setSizePolicy(view->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);
    
    statusLabel = new QLabel(view);
    statusLabel->setText("dummy text");
    mainLayout->addWidget(statusLabel);
    
    mainSplitter = new dlg::SplitterDecorated(view);
    mainSplitter->setOrientation(Qt::Vertical);
    mainSplitter->setChildrenCollapsible(false);
    mainLayout->addWidget(mainSplitter);
    
    auto *spinePick = command->feature->getParameter(ftr::LawSpine::Tags::spinePick);
    prmModel = new tbl::Model(view, command->feature,{spinePick});
    prmView = new tbl::View(view, prmModel, true);
    mainSplitter->addWidget(prmView);
    
    tbl::SelectionCue cue;
    cue.singleSelection = true;
    cue.mask = slc::EdgesBoth;
    cue.statusPrompt = tr("Select Edge");
    cue.accrueEnabled = false;
    prmModel->setCue(spinePick, cue);
    
    QStringList listOfLaws
    {
      QObject::tr("Constant")
      , QObject::tr("Linear")
      , QObject::tr("Interpolate")
    };
    lawList = new DragList(view, listOfLaws, "Laws");
    mainSplitter->addWidget(lawList);
    
    pointSplitter = new dlg::SplitterDecorated();
    pointSplitter->setOrientation(Qt::Horizontal);
    pointSplitter->setChildrenCollapsible(false);
    pointList = new QListWidget();
    pointList->setContextMenuPolicy(Qt::ActionsContextMenu);
    removePointAction = new QAction(QObject::tr("Remove Point"), pointList);
    pointList->addAction(removePointAction);
    togglePointTypeAction = new QAction(QObject::tr("Toggle Point Type"), pointList);
    pointList->addAction(togglePointTypeAction);
    pointSplitter->addWidget(pointList);
    mainSplitter->addWidget(pointSplitter);
    
    graphicsScene = new GraphicsScene(view, command->feature->getVessel());
    graphicsView = new GraphicsView(graphicsScene, view);
    graphicsView->setRenderHints(QPainter::Antialiasing);
    mainSplitter->addWidget(graphicsView);
  }
  
  void loadFeatureData()
  {
    const auto &v = command->feature->getVessel();
    statusLabel->setText(v.getValidationText());
    std::vector<int> laws;
    for (auto l :  v.getLaws())
      laws.push_back(static_cast<int>(l));
    lawList->setList(laws);
    for (int index = 0; index < v.getPointCount(); ++index)
      pointList->addItem(QObject::tr("Point"));
    
    const auto &msgs = prmModel->getMessages(command->feature->getParameter(ftr::LawSpine::Tags::spinePick));
    if (msgs.empty())
      QTimer::singleShot(0, [this](){this->prmView->openPersistent(prmModel->index(0, 1));});
    else
      QTimer::singleShot(0, [this](){this->goGlobalPointSelection();});
  }

  void glue()
  {
    connect(prmModel, &tbl::Model::dataChanged, view, &LawSpine::modelChanged);
    connect(prmView, &tbl::View::openingPersistent, [this](){this->stopGlobalPointSelection();});
    QObject::connect(lawList, &DragList::added, view, &LawSpine::lawsChanged);
    QObject::connect(lawList, &DragList::removed, view, &LawSpine::lawsChanged);
    QObject::connect(pointList, &QListWidget::itemSelectionChanged, view, &LawSpine::pointSelectionChanged);
    QObject::connect(removePointAction, &QAction::triggered, [this](){this->slcRemoved(msg::Message());});
    QObject::connect(togglePointTypeAction, &QAction::triggered, [this](){this->togglePoint();});
    
    lawList->getListView()->installEventFilter(view);
    lawList->getProtoView()->installEventFilter(view);
    prmView->installEventFilter(view);
    
    view->sift->insert
    ({
      std::make_pair(msg::Response | msg::Post | msg::Selection | msg::Add
      , std::bind(&Stow::slcAdded, this, std::placeholders::_1))
      , std::make_pair(msg::Response | msg::Post | msg::Selection | msg::Remove
      , std::bind(&Stow::slcRemoved, this, std::placeholders::_1))
    });
  }
  
  const ftr::Base& getSpineFeature()
  {
    const auto &msgs = prmModel->getMessages(command->feature->getParameter(ftr::LawSpine::Tags::spinePick));
    assert(!msgs.empty());
    const ftr::Base *f = view->project->findFeature(msgs.front().featureId);
    assert(f);
    return *f;
  }
  
  const ann::SeerShape& getSpineSeerShape()
  {
    const ftr::Base &f = getSpineFeature();
    assert(f.hasAnnex(ann::Type::SeerShape));
    const ann::SeerShape &ss = f.getAnnex<ann::SeerShape>();
    assert(!ss.isNull());
    return ss;
  }
  
  std::optional<ftr::Pick> pickFromSpineParameter(double spineParameter)
  {
    assert(adapter);
    TopoDS_Shape shape;
    double param;
    std::tie(shape, param) = adapter->fromSpineParameter(spineParameter); assert(!shape.IsNull());
    const auto &ss = getSpineSeerShape();
    assert(ss.hasShape(shape));
    const auto &id = ss.findId(shape); assert(!id.is_nil());
    ftr::Pick out; //don't need a tag.
    out.resolvedIds = {id};
    out.highlightIds = {id};
    out.shapeHistory = view->project->getShapeHistory().createDevolveHistory(id);
    if (shape.ShapeType() == TopAbs_VERTEX)
    {
      //convert vertex to end/start point selection. Man does this suck or what!
      auto edges = ss.useGetParentsOfType(shape, TopAbs_EDGE);
      std::optional<uuid> spEdge;
      for (const auto &e : edges)
      {
        if (adapter->isSpineEdge(TopoDS::Edge(e)))
        {
          spEdge = ss.findId(e);
          break;
        }
      }
      if (!spEdge) return std::nullopt;
      if (ss.useGetStartVertex(*spEdge) == ss.findId(shape))
        out.selectionType = slc::Type::StartPoint;
      if (ss.useGetEndVertex(*spEdge) == ss.findId(shape))
        out.selectionType = slc::Type::EndPoint;
      else {assert(0); return std::nullopt;} //WTF no vertex on spine shape?
    }
    else if (shape.ShapeType() == TopAbs_EDGE)
    {
      //convert parameterized point pick.
      osg::Vec3d point = gu::toOsg(adapter->location(spineParameter));
      out.setParameter(TopoDS::Edge(shape), point);
      out.selectionType = slc::Type::NearestPoint; //nearest will work for all parameters.
    }
    else assert(0); //unrecognized shape type.
    return out;
  }
  
  void resetPrmView()
  {
    if (pointPrmView)
    {
      QWidget *old = pointSplitter->widget(1);
      if (old) delete old;
    }
    if (pointPrmModel)
      delete pointPrmModel;
    pointPrmView = nullptr;
    pointPrmModel = nullptr;
  }
  
  void closePersistent()
  {
    prmView->closePersistent();
    if (pointPrmView)
      pointPrmView->closePersistent();
    pointList->clearSelection();
  }
  
  void goLocalUpdate()
  {
    command->localUpdate();
    view->node->sendBlocked(msg::buildStatusMessage(command->getStatusMessage()));
    statusLabel->setText(command->feature->getVessel().getValidationText());
    QTimer::singleShot(0, [this](){this->goGlobalPointSelection();});
    setGraphicsViewState();
  }
  
  void goGlobalPointSelection()
  {
    if (!isGlobalPointSelection)
    {
      if (adapter)
      {
        isGlobalPointSelection = true;
        view->goMaskDefault(); // note: we don't remember any selection mask changes the user makes.
        view->node->sendBlocked(msg::buildStatusMessage(QObject::tr("Select New Points").toStdString()));
        view->goSelectionToolbar();
      }
      else
      {
        view->node->sendBlocked(msg::buildStatusMessage(QObject::tr("Set Valid Spine Edge").toStdString()));
        view->node->sendBlocked(msg::buildSelectionMask(slc::None));
      }
    }
  }
  
  void stopGlobalPointSelection()
  {
    if (isGlobalPointSelection)
    {
      isGlobalPointSelection = false;
      view->node->sendBlocked(msg::buildSelectionMask(slc::None));
    }
  }
  
  void formAdapter()
  {
    const auto &msgs = prmModel->getMessages(command->feature->getParameter(ftr::LawSpine::Tags::spinePick));
    if (msgs.empty())
    {
      adapter = std::nullopt;
      return;
    }
    else
    {
      const ann::SeerShape &ss = getSpineSeerShape();
      adapter = std::optional<lwf::Adapter>(lwf::Adapter(ss, msgs.front().shapeId));
      assert(adapter->isValid());
      if (!adapter->isValid()) adapter = std::nullopt;
    }
  }
  
  void addNewPoint(const slc::Message &mIn)
  {
    assert(adapter); //restrict interface so we don't get here with an invalid adapter.
    const ann::SeerShape &ss = getSpineSeerShape();
    
    ftr::Pick pick = tls::convertToPick(mIn, ss, view->project->getShapeHistory());
    if (pick.resolvedIds.size() != 1) return;
    const TopoDS_Shape &shape = ss.getOCCTShape(pick.resolvedIds.front()); assert(!shape.IsNull());
    std::optional<double> newPrm;
    
    if (shape.ShapeType() == TopAbs_EDGE)
      newPrm = adapter->toSpineParameter(TopoDS::Edge(shape), occt::deNormalize(TopoDS::Edge(shape), pick.u));
    else if (shape.ShapeType() == TopAbs_VERTEX)
      newPrm = adapter->toSpineParameter(TopoDS::Vertex(shape));
    if (!newPrm) return;
    
    if (!command->feature->getVessel().addPoint(pick, *newPrm)) return;
    pointList->addItem(QObject::tr("Point"));
    command->feature->setModelDirty();
  }
  
  void refreshPointList()
  {
    resetPrmView();
    pointList->clear();
    const auto &v = command->feature->getVessel();
    for (int index = 0; index < v.getPointCount(); ++index)
      pointList->addItem(QObject::tr("Point"));
  }
  
  void removePoint()
  {
    stopGlobalPointSelection();
    auto items = pointList->selectedItems();
    if (items.size() != 1) return;
    int row = pointList->row(items.front());
    //can't remove the first and last point
    if (row < 1 || row >= command->feature->getVessel().getPointCount() - 1) return;
    command->feature->getVessel().removePoint(row);
    refreshPointList();
    command->feature->setModelDirty();
    goLocalUpdate();
  }
  
  void slcAdded(const msg::Message &mIn)
  {
    assert(mIn.isSLC());
    if
    (
      view->isHidden()
      || !isGlobalPointSelection
      || (!slc::isPointType(mIn.getSLC().type))
    )
      return;
    
    stopGlobalPointSelection();
    const auto &sm = mIn.getSLC();
    
    //we shouldn't get here without a valid spine selection. So lets make sure this
    //point selection is on the same feature as the spine.
    const auto spineMessages = prmModel->getMessages(prmModel->index(0,1));
    assert(!spineMessages.empty());
    if (spineMessages.front().featureId != sm.featureId)
    {
      view->node->sendBlocked(msg::buildStatusMessage("Invalid Point", 2.0));
      view->node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
      goGlobalPointSelection();
      return;
    }
    addNewPoint(sm);
    view->node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    goLocalUpdate();
  }
  
  void slcRemoved(const msg::Message&)
  {
    if(view->isHidden() || !isGlobalPointSelection)
      return;
    removePoint();
    goLocalUpdate();
  }
  
  //point list view should only be enabled when we have a valid spine edge selected.
  void setPointListState()
  {
    //use adapter as a sign of a valid spine
    if (adapter)
      pointList->setEnabled(true);
    else
      pointList->setEnabled(false);
  }
  
  void setGraphicsViewState()
  {
    
    if (command->feature->getVessel().isValid() && adapter)
    {
      int steps = 5;
      if (graphicsView->width() >= graphicsView->height())
        steps = std::max(steps, static_cast<int>(graphicsView->width() / 5.0));
      else
        steps = std::max(steps, static_cast<int>(graphicsView->height() / 5.0));
      
      graphicsScene->generate(steps);
      graphicsView->setEnabled(true);
      graphicsView->fitToData();
    }
    else
      graphicsView->setEnabled(false);
  }
  
  void togglePoint()
  {
    if(view->isHidden() || !isGlobalPointSelection)
      return;
    stopGlobalPointSelection(); //probably not needed, but to be safe.
    assert(adapter); //gui should prevent this condition.
    auto items = pointList->selectedItems();
    if (items.size() != 1) return;
    int row = pointList->row(items.front());
    if (row < 0 || row >= command->feature->getVessel().getPointCount()) return;
    
    //going to build ftr::Pick even if not needed and let vessel determine if pick is used.
    double param = command->feature->getVessel().getPointParameters(row).front()->getDouble();
    auto pick = pickFromSpineParameter(param);
    if (!pick)
    {
      view->node->sendBlocked(msg::buildStatusMessage("Unable to make pick from parameter", 2.0));
      return;
    }
    if (!command->feature->getVessel().togglePoint(row, *pick))
      return;
    QTimer::singleShot(0, [this](){this->view->pointSelectionChanged();});
    command->feature->setModelDirty();
    goLocalUpdate();
  }
  
  void showPoint3d(const prm::Parameter *prmIn)
  {
    view->node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    slc::Message out;
    const ftr::Base &feature = getSpineFeature();
    const ann::SeerShape &sShape = getSpineSeerShape();
    out.featureId = feature.getId();
    out.featureType = feature.getType();
    auto pair = adapter->fromSpineParameter(prmIn->getDouble());
    if (pair.first.IsNull()) return;
    if (!sShape.hasShape(pair.first)) return;
    out.type = slc::Type::EndPoint; //type of point doesn't matter
    out.shapeId = sShape.findId(pair.first);
    out.pointLocation = gu::toOsg(adapter->location(prmIn->getDouble()));
    view->node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Add, out));
  }
};

LawSpine::LawSpine(cmd::LawSpine *cIn)
: Base("cmv::LawSpine")
, stow(std::make_unique<Stow>(cIn, this))
{
  maskDefault = slc::AllPointsEnabled | slc::EndPointsSelectable;
  goSelectionToolbar();
}

LawSpine::~LawSpine() = default;

bool LawSpine::eventFilter(QObject *watched, QEvent *event)
{
  //installed on child widgets so we can dismiss persistent editors
  if(event->type() == QEvent::FocusIn) stow->closePersistent();
  return QObject::eventFilter(watched, event);
}

void LawSpine::lawsChanged(int)
{
  auto indexes = stow->lawList->getIndexList();
  lwf::Laws theLaws;
  for (auto i : indexes)
    theLaws.push_back(static_cast<lwf::Law>(i));
  stow->command->feature->getVessel().setLaws(theLaws);
  stow->command->feature->setModelDirty();
  stow->goLocalUpdate();
}

void LawSpine::modelChanged(const QModelIndex &index, const QModelIndex&)
{
  //we only have 1 parameter hooked to here so we know what it is.
  const auto &spineMessages = stow->prmModel->getMessages(stow->prmModel->getParameter(index));
  stow->command->setSelections(spineMessages);
  stow->formAdapter();
  stow->goLocalUpdate();
  stow->setPointListState();
}

/* we have a little bit of a dilemma. Normally we have all pick parameters
 * resolved into messages and stored through the entire life of the view.
 * Because points are so volatile, we have decided to make point parameter
 * tables dynamic so they are always up to date with the vessel. That decision
 * causes the problem of updating graph picks to project graph edges. Messages
 * for only 1 pick are present at any one time. So do we go through all vessel
 * points and get ftr::Picks and convert them to messages so command can
 * convert them back to picks for normal like operation of command set selections?
 * I don't think so, kind of ugly. No project graph edges for vessel picks.
 */
void LawSpine::pointModelChanged(const QModelIndex &index, const QModelIndex&)
{
  if (!index.isValid())
    return;
  
  // Note: we are not using graph edge tags and ftr::Pick tags for the point picks.
  if (stow->pointPrmModel)
  {
    auto *param = stow->pointPrmModel->getParameter(index);
    if (param->getTag() == "pick")
    {
      const auto &pickMessages = stow->pointPrmModel->getMessages(param);
      ftr::Picks picks;
      if (!pickMessages.empty())
      {
        const ftr::Base *lf = project->findFeature(pickMessages.front().featureId);
        picks.push_back(tls::convertToPick(pickMessages.front(), *lf, project->getShapeHistory()));
        picks.back().tag = "pick";
      }
      param->setValue(picks);
      QTimer::singleShot(0, [this](){this->pointSelectionChanged();});
    }
    else if (param->getTag() == "parameter")
    {
      stow->showPoint3d(param);
    }
  }
  
  stow->goLocalUpdate();
}

void LawSpine::pointSelectionChanged()
{
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  stow->resetPrmView();
  stow->removePointAction->setEnabled(false);
  stow->togglePointTypeAction->setEnabled(false);
  auto selected = stow->pointList->selectedItems();
  if (selected.isEmpty()) return;
  
  int row = stow->pointList->row(selected.front());
  assert(row >= 0 && row < stow->pointList->count());
  
  //we only show the point parameters if vessel is valid.
  if (!stow->command->feature->getVessel().isValid())
  {
    if (row != 0 && row != stow->pointList->count() - 1)
      stow->removePointAction->setEnabled(true);
    return;
  }
  
  if (row != 0 && row != stow->pointList->count() - 1)
  {
    stow->removePointAction->setEnabled(true);
    stow->togglePointTypeAction->setEnabled(true);
  }
  auto parameters = stow->command->feature->getVessel().getPointParameters(row);
  assert(!parameters.empty());
  
  stow->pointPrmModel = new tbl::Model(this, stow->command->feature, parameters);
  stow->pointPrmView = new tbl::View(this, stow->pointPrmModel, false);
  stow->pointSplitter->addWidget(stow->pointPrmView);
  connect(stow->pointPrmModel, &tbl::Model::dataChanged, this, &LawSpine::pointModelChanged);
  connect(stow->pointPrmView, &tbl::View::openingPersistent, [this](){this->stow->stopGlobalPointSelection();});
  
  //find pick parameter if it exists.
  for (auto *p : parameters)
  {
    if (p->getTag() == "pick")
    {
      tbl::SelectionCue cue;
      cue.singleSelection = true;
      cue.mask = slc::AllPointsEnabled | slc::EndPointsSelectable | slc::NearestPointsSelectable;
      cue.statusPrompt = tr("Select Point");
      cue.accrueEnabled = false;
      stow->pointPrmModel->setCue(p, cue);
      break;
    }
  }
  
  stow->showPoint3d(parameters.front());
}
