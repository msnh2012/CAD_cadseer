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

#include <assert.h>
#include <limits>

#include <QHBoxLayout>
#include <QCloseEvent>
#include <QTabWidget>
#include <QToolBar>
#include <QMenu>
#include <QToolButton>
#include <QSurfaceFormat>
#include <QWidgetAction>

#include "dagview/dagmodel.h"
#include "dagview/dagview.h"
#include "expressions/exprwidget.h"
#include "application/appapplication.h"
#include "dialogs/dlgsplitterdecorated.h"
#include "viewer/vwrwidget.h"
#include "selection/slcmanager.h"
#include "message/msgnode.h"
#include "message/msgsift.h"
#include "commandview/cmvmessage.h"
#include "application/appincrementwidget.h"
#include "commandview/cmvpane.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "menu/mnuserial.h"
#include "menu/mnumanager.h"
#include "application/appinfowindow.h"
#include "application/appmainwindow.h"

using boost::uuids::uuid;
using namespace app;

struct MainWindow::Stow
{
  MainWindow *parent;
  QTabWidget *tabToolWidget;
  
  QToolBar *selectionBar;
  QAction *actionSelectObjects;
  QAction *actionSelectFeatures;
  QAction *actionSelectSolids;
  QAction *actionSelectShells;
  QAction *actionSelectFaces;
  QAction *actionSelectWires;
  QAction *actionSelectEdges;
  QAction *actionSelectVertices;
  QAction *actionSelectEndPoints;
  QAction *actionSelectMidPoints;
  QAction *actionSelectCenterPoints;
  QAction *actionSelectQuandrantPoints;
  QAction *actionSelectNearestPoints;
  QAction *actionSelectScreenPoints;
  IncrementWidget *translationEdit;
  IncrementWidget *rotationEdit;
  
  dag::Model *dagModel;
  dag::View *dagView;
  expr::Widget *expressionWidget;
  InfoDialog* infoDialog;
  vwr::Widget* viewWidget;
  slc::Manager *selectionManager;
  cmv::Pane *pane;
  dlg::SplitterDecorated *splitter;
  const mnu::srl::Cue &menuCue = mnu::manager().getCueRead();
  
  msg::Node node;
  msg::Sift sift;
  
  Stow() = delete;
  Stow(MainWindow *parentIn) : parent(parentIn)
  {
    tabToolWidget = new QTabWidget(parent);
    tabToolWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    
    int tbSize = 32;
    bool showIcon = true;
    bool showIconText = false;
    if (menuCue.toolbarSettings())
    {
      tbSize = menuCue.toolbarSettings().get().iconSize();
      showIcon = menuCue.toolbarSettings().get().showIcon();
      showIconText = menuCue.toolbarSettings().get().showText();
    }
    selectionBar = new QToolBar(parent);
    selectionBar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    if (showIconText) selectionBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    selectionBar->setFloatable(false);
    selectionBar->setMovable(false);
    selectionBar->setIconSize(QSize(tbSize, tbSize));
    auto ssa = [&](const char *icon, const QString &text, const QString &iconText) -> QAction*
    {
      QAction *out = selectionBar->addAction(QString());
      if (showIcon) out->setIcon(QIcon(icon));
      if (showIconText) out->setIconText(iconText);
      out->setCheckable(true);
      out->setToolTip(text);
      out->setWhatsThis(text);
      
      return out;
    };
    actionSelectObjects = ssa(":/resources/images/selectObjects.svg", tr("Select Objects"), tr("Objects"));
    actionSelectFeatures = ssa(":/resources/images/selectFeatures.svg", tr("Select Features"), tr("Features"));
    actionSelectSolids = ssa(":/resources/images/selectSolids.svg", tr("Select Solids"), tr("Solids"));
    actionSelectShells = ssa(":/resources/images/selectShells.svg", tr("Select Shells"), tr("Shells"));
    actionSelectFaces = ssa(":/resources/images/selectFaces.svg", tr("Select Faces"), tr("Faces"));
    actionSelectWires = ssa(":/resources/images/selectWires.svg", tr("Select Wires"), tr("Wires"));
    actionSelectEdges = ssa(":/resources/images/selectEdges.svg", tr("Select Edges"), tr("Edges"));
    actionSelectVertices = ssa(":/resources/images/selectVertices.svg", tr("Select Vertices"), tr("Vertices"));
    actionSelectEndPoints = ssa(":/resources/images/selectPointsEnd.svg", tr("Select End Points"), tr("End"));
    actionSelectMidPoints = ssa(":/resources/images/selectPointsMid.svg", tr("Select Mid Points"), tr("Mid"));
    actionSelectCenterPoints = ssa(":/resources/images/selectPointsCenter.svg", tr("Select Center Points"), tr("Center"));
    actionSelectQuandrantPoints = ssa(":/resources/images/selectPointsQuandrant.svg", tr("Select Quadrant Points"), tr("Quadrant"));
    actionSelectNearestPoints = ssa(":/resources/images/selectPointsNearest.svg", tr("Select Nearest Points"), tr("Nearest"));
    actionSelectScreenPoints = ssa(":/resources/images/selectPointsScreen.svg", tr("Select Screen Points"), tr("Screen"));

    selectionBar->setContentsMargins(0, 0, 0, 0);
    selectionBar->layout()->setContentsMargins(0, 0, 0, 0); //yes both
    selectionBar->layout()->setSpacing(0);
    
    tabToolWidget->addTab(selectionBar, QIcon(":/resources/images/selectObjects.svg"), "");
    tabToolWidget->setContentsMargins(0, 0, 0, 0);
    tabToolWidget->setTabToolTip(0, tr("Selection"));
    if (menuCue.toolbarSettings())
    {
      if (!menuCue.toolbarSettings().get().showIcon())
        tabToolWidget->setTabIcon(0, QIcon());
      if (menuCue.toolbarSettings().get().showText())
        tabToolWidget->setTabText(0, tr("Selection"));
    }
    
    //add increment widgets to toolbar.
    selectionBar->addSeparator();
    translationEdit = new IncrementWidget(0, tr("Translation:"), prf::manager().rootPtr->dragger().linearIncrement());
    translationEdit->setToolTip(tr("Controls The Translation Dragger Increment"));
    QWidgetAction* translationAction = new QWidgetAction(parent);
    translationAction->setDefaultWidget(translationEdit);
    selectionBar->addAction(translationAction);
    
    rotationEdit = new IncrementWidget(0, tr("Rotation:"), prf::manager().rootPtr->dragger().angularIncrement());
    rotationEdit->setToolTip(tr("Controls The Rotation Dragger Increment"));
    QWidgetAction* rotationAction = new QWidgetAction(parent);
    rotationAction->setDefaultWidget(rotationEdit);
    selectionBar->addAction(rotationAction);
    
    node.connect(msg::hub());
    sift.name = "app::MainWindow";
    node.setHandler(std::bind(&msg::Sift::receive, &sift, std::placeholders::_1));
  }
  
  void buildToolbars()
  {
    int size = 32;
    bool showIcon = true;
    bool showIconText = false;
    if (menuCue.toolbarSettings())
    {
      size = menuCue.toolbarSettings().get().iconSize();
      showIcon = menuCue.toolbarSettings().get().showIcon();
      showIconText = menuCue.toolbarSettings().get().showText();
    }
    
    for (const auto &tb : menuCue.toolbars())
    {
      if (tb.entries().empty()) //don't build an empty toolbar
        continue;
      QToolBar *bar = new QToolBar(); //addTab reparents so no leak.
      bar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
      bar->setFloatable(false);
      bar->setMovable(false);
      bar->setIconSize(QSize(size, size));
      bar->setContentsMargins(0, 0, 0, 0);
      bar->layout()->setContentsMargins(0, 0, 0, 0);
      bar->layout()->setSpacing(0);
      tabToolWidget->addTab(bar, QIcon(QString::fromStdString(tb.icon())), "");
      if (!showIcon)
        tabToolWidget->setTabIcon(tabToolWidget->count() - 1, QIcon());
      if (showIconText)
      {
        bar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        tabToolWidget->setTabText(tabToolWidget->count() - 1, QString::fromStdString(tb.text()));
      }
      tabToolWidget->setTabToolTip(tabToolWidget->count() - 1, QString::fromStdString(tb.text()));
      for (const auto &entry : tb.entries())
      {
        if (entry.commandIds().empty())
          continue;
        
        auto setupAction = [&](QAction *action, const mnu::srl::Command &cIn)
        {
          action->setData(cIn.id());
          QObject::connect(action, &QAction::triggered, parent, &MainWindow::actionTriggeredSlot);
          
          if (!cIn.visual())
            return;
          const auto &v = cIn.visual().get();
          if (showIcon && v.icon())
            action->setIcon(QIcon(QString::fromStdString(v.icon().get())));
          if (showIconText && v.iconText())
            action->setIconText(QString::fromStdString(v.iconText().get()));
          if (v.toolTipText())
            action->setToolTip(QString::fromStdString(v.toolTipText().get()));
          if (v.whatThisText())
            action->setWhatsThis(QString::fromStdString(v.whatThisText().get()));
        };
        
        if (entry.commandIds().size() == 1)
        {
          //build single command action.
          const auto &srlCommand = mnu::manager().getCommand(entry.commandIds().front());
          if (srlCommand.id() == 0)
            continue;
          QAction *action = bar->addAction(QString());
          setupAction(action, srlCommand);
        }
        else
        {
          //build a menu of buttons
          QMenu *menu = new QMenu(bar);
          menu->setToolTipsVisible(true);
          for (const auto &cId : entry.commandIds())
          {
            const auto &srlCommand = mnu::manager().getCommand(cId);
            if (srlCommand.id() == 0)
              continue;
            QAction *action = new QAction(bar);
            setupAction(action, srlCommand);
            menu->addAction(action); //menu does not take ownership of action.
            if (!menu->defaultAction())
              menu->setDefaultAction(action);
          }
          QToolButton* toolButton = new QToolButton();
          toolButton->setMenu(menu);
          toolButton->setPopupMode(QToolButton::InstantPopup); // QToolButton::MenuButtonPopup QToolButton::DelayedPopup
          if (showIconText)
          {
            toolButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
            tabToolWidget->setTabText(tabToolWidget->count() - 1, QString::fromStdString(tb.text()));
          }
          if (!showIcon)
            toolButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
          
          QIcon icon;
          QString iconText;
          if (showIcon)
          {
            icon = menu->defaultAction()->icon();
            if (entry.visual() && entry.visual().get().icon())
              icon = QIcon(QString::fromStdString(entry.visual().get().icon().get()));
          }
          if (showIconText)
          {
            iconText = menu->defaultAction()->iconText();
            if (entry.visual() && entry.visual().get().iconText())
              iconText = QString::fromStdString(entry.visual().get().iconText().get());
          }
          QAction *action = new QAction(icon, iconText, bar);
          if (entry.visual())
          {
            if (entry.visual().get().toolTipText())
              action->setToolTip(QString::fromStdString(entry.visual().get().toolTipText().get()));
            if (entry.visual().get().whatThisText())
              action->setWhatsThis(QString::fromStdString(entry.visual().get().whatThisText().get()));
          }
          toolButton->setDefaultAction(action);
          bar->addWidget(toolButton); //bar takes ownership of widget.
        }
      }
    }
  }
};

MainWindow::MainWindow(QWidget *parent)
: QMainWindow(parent)
, stow(std::make_unique<MainWindow::Stow>(this))
{
  this->setWindowIcon(QIcon(":/resources/images/cadSeer.svg"));
  
  QVBoxLayout *mainLayout = new QVBoxLayout();
  mainLayout->addWidget(stow->tabToolWidget);
  
  stow->dagModel = new dag::Model(this);
  stow->dagView = new dag::View(this);
  stow->dagView->setScene(stow->dagModel);
  stow->expressionWidget = new expr::Widget(this);
  stow->expressionWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  dlg::SplitterDecorated *subSplitter = new dlg::SplitterDecorated(this);
  subSplitter->setOrientation(Qt::Vertical);
  subSplitter->addWidget(stow->expressionWidget);
  subSplitter->addWidget(stow->dagView);
  subSplitter->setSizes(QList<int>({250, 1000}));
  subSplitter->restoreSettings("mainWindowSubSplitter");
  
  QSurfaceFormat format = QSurfaceFormat::defaultFormat();
  
//   format.setVersion(3, 2);
//   format.setProfile(QSurfaceFormat::CoreProfile);
//   format.setRenderableType(QSurfaceFormat::OpenGL);
//   format.setOption(QSurfaceFormat::DebugContext);
  
  format.setVersion(2, 0);
  format.setProfile(QSurfaceFormat::CompatibilityProfile);
  format.setRenderableType(QSurfaceFormat::OpenGL);
  format.setOption(QSurfaceFormat::DebugContext);
  
  format.setDepthBufferSize(24);
  //format.setAlphaBufferSize(8);
  format.setSamples(8);
  format.setStencilBufferSize(8);
  format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
  int samples = prf::manager().rootPtr->visual().display().samples().get();
  if (samples > 0)
    format.setSamples(samples); //big slowdown.
  QSurfaceFormat::setDefaultFormat(format);
  osg::DisplaySettings::instance()->setTextShaderTechnique("SIGNED_DISTANCE_FUNCTION");
  stow->viewWidget = new vwr::Widget(this);
  stow->viewWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  stow->viewWidget->setGeometry(100, 100, 800, 600);
  stow->viewWidget->setMinimumSize(QSize(100, 100)); //don't collapse view widget. osg nan erros.
  
  stow->pane = new cmv::Pane();
  
  stow->splitter = new dlg::SplitterDecorated(this);
  dlg::SplitterDecorated *splitter = stow->splitter;
  splitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  splitter->setOpaqueResize(Qt::Horizontal);
  splitter->addWidget(stow->pane);
  splitter->addWidget(stow->viewWidget);
  splitter->addWidget(subSplitter);
  //size setup temp.
  QList<int> sizes;
  sizes.append(0); //start collasped
  sizes.append(1000);
  sizes.append(300);
  splitter->setSizes(sizes);
  splitter->setCollapsible(1, false); //don't collapse view widget. osg nan erros.
  splitter->restoreSettings("mainWindowSplitter");
  connect(stow->splitter, &dlg::SplitterDecorated::splitterMoved, this, &MainWindow::commandViewWidth);
  
  QWidget *centralWidget = new QWidget(this);
  centralWidget->setContentsMargins(0, 0, 0, 0);
  QVBoxLayout *cwl = new QVBoxLayout();
  cwl->setSpacing(0);
  cwl->setContentsMargins(0, 0, 0, 0);
  cwl->addWidget(stow->tabToolWidget);
  cwl->addWidget(splitter);
  centralWidget->setLayout(cwl);
  this->setCentralWidget(centralWidget);
  
  stow->selectionManager = new slc::Manager(this);
  
  //build the info window.
  stow->infoDialog = new InfoDialog(this);
  stow->infoDialog->restoreSettings();
  stow->infoDialog->hide();

  setupDispatcher();
  setupSelectionToolbar();
  stow->buildToolbars();
}

MainWindow::~MainWindow() = default;

vwr::Widget* MainWindow::getViewer()
{
  return stow->viewWidget;
}

slc::Manager* MainWindow::getSelectionManager()
{
  return stow->selectionManager;
}

void MainWindow::closeEvent (QCloseEvent *event)
{
  stow->node.send(msg::Message(msg::Request | msg::Command | msg::Clear));
  QMainWindow::closeEvent(event);
}

void MainWindow::actionTriggeredSlot()
{
  QAction *action = dynamic_cast<QAction *>(QObject::sender());
  assert(action);
  if (!action)
    return;
  std::size_t commandId = action->data().value<std::size_t>();
  assert(commandId != 0);
  if (commandId == 0)
    return;
  
  auto mask = mnu::manager().getMask(commandId);
  assert(!mask.none());
  if (mask.none())
    return;
  stow->node.send(msg::Message(mask));
}

void MainWindow::setupSelectionToolbar()
{
  stow->selectionManager->actionSelectObjects = stow->actionSelectObjects;
  stow->selectionManager->actionSelectFeatures = stow->actionSelectFeatures;
  stow->selectionManager->actionSelectSolids = stow->actionSelectSolids;
  stow->selectionManager->actionSelectShells = stow->actionSelectShells;
  stow->selectionManager->actionSelectFaces = stow->actionSelectFaces;
  stow->selectionManager->actionSelectWires = stow->actionSelectWires;
  stow->selectionManager->actionSelectEdges = stow->actionSelectEdges;
  stow->selectionManager->actionSelectVertices = stow->actionSelectVertices;
  stow->selectionManager->actionSelectEndPoints = stow->actionSelectEndPoints;
  stow->selectionManager->actionSelectMidPoints = stow->actionSelectMidPoints;
  stow->selectionManager->actionSelectCenterPoints = stow->actionSelectCenterPoints;
  stow->selectionManager->actionSelectQuadrantPoints = stow->actionSelectQuandrantPoints;
  stow->selectionManager->actionSelectNearestPoints = stow->actionSelectNearestPoints;
  stow->selectionManager->actionSelectScreenPoints = stow->actionSelectScreenPoints;

  connect(stow->actionSelectObjects, SIGNAL(triggered(bool)), stow->selectionManager, SLOT(triggeredObjects(bool)));
  connect(stow->actionSelectFeatures, SIGNAL(triggered(bool)), stow->selectionManager, SLOT(triggeredFeatures(bool)));
  connect(stow->actionSelectSolids, SIGNAL(triggered(bool)), stow->selectionManager, SLOT(triggeredSolids(bool)));
  connect(stow->actionSelectShells, SIGNAL(triggered(bool)), stow->selectionManager, SLOT(triggeredShells(bool)));
  connect(stow->actionSelectFaces, SIGNAL(triggered(bool)), stow->selectionManager, SLOT(triggeredFaces(bool)));
  connect(stow->actionSelectWires, SIGNAL(triggered(bool)), stow->selectionManager, SLOT(triggeredWires(bool)));
  connect(stow->actionSelectEdges, SIGNAL(triggered(bool)), stow->selectionManager, SLOT(triggeredEdges(bool)));
  connect(stow->actionSelectVertices, SIGNAL(triggered(bool)), stow->selectionManager, SLOT(triggeredVertices(bool)));
  connect(stow->actionSelectEndPoints, SIGNAL(triggered(bool)), stow->selectionManager, SLOT(triggeredEndPoints(bool)));
  connect(stow->actionSelectMidPoints, SIGNAL(triggered(bool)), stow->selectionManager, SLOT(triggeredMidPoints(bool)));
  connect(stow->actionSelectCenterPoints, SIGNAL(triggered(bool)), stow->selectionManager, SLOT(triggeredCenterPoints(bool)));
  connect(stow->actionSelectQuandrantPoints, SIGNAL(triggered(bool)), stow->selectionManager, SLOT(triggeredQuadrantPoints(bool)));
  connect(stow->actionSelectNearestPoints, SIGNAL(triggered(bool)), stow->selectionManager, SLOT(triggeredNearestPoints(bool)));
  connect(stow->actionSelectScreenPoints, SIGNAL(triggered(bool)), stow->selectionManager, SLOT(triggeredScreenPoints(bool)));
}

void MainWindow::setupDispatcher()
{
  stow->sift.insert
  (
    {
      std::make_pair
      (
        msg::Request | msg::Info | msg::Text
        , std::bind(&MainWindow::infoTextDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Preferences
        , std::bind(&MainWindow::preferencesChanged, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Command | msg::View | msg::Show
        , std::bind(&MainWindow::showCommandViewDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Command | msg::View | msg::Hide
        , std::bind(&MainWindow::hideCommandViewDispatched, this, std::placeholders::_1)
      )
    }
  );
}

void MainWindow::preferencesChanged(const msg::Message&)
{
  stow->translationEdit->externalUpdate();
  stow->rotationEdit->externalUpdate();
}

void MainWindow::infoTextDispatched(const msg::Message&)
{
  stow->infoDialog->show();
  stow->infoDialog->raise();
  stow->infoDialog->activateWindow();
}

void MainWindow::showCommandViewDispatched(const msg::Message &mIn)
{
  assert(mIn.isCMV());
  if (!mIn.isCMV())
    return;
  const cmv::Message &m = mIn.getCMV();
  
  //this sucks. This should be done for me...like everything.
  auto sizes = stow->splitter->sizes();
  int difference = sizes.at(0) - m.paneWidth;
  sizes[0] = m.paneWidth;
  sizes[1] = sizes.at(1) + difference;
  stow->splitter->setSizes(sizes);
}

void MainWindow::hideCommandViewDispatched(const msg::Message&)
{
  auto sizes = stow->splitter->sizes();
  sizes[1] = sizes.at(0) + sizes.at(1);
  sizes[0] = 0;
  stow->splitter->setSizes(sizes);
}

void MainWindow::commandViewWidth(int, int)
{
  cmv::Message vm(nullptr, stow->splitter->sizes().front());
  msg::Message out(msg::Response | msg::Command | msg::View | msg::Update, vm);
  stow->node.sendBlocked(out);
}
