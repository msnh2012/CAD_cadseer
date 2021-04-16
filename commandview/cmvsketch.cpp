/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2020 Thomas S. Anderson blobfish.at.gmx.com
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

#include <osgViewer/Viewer>

#include <QSettings>
#include <QRadioButton>
#include <QButtonGroup>
#include <QGroupBox>
#include <QToolBar>
#include <QTimer>
#include <QLineEdit>
#include <QVBoxLayout>

#include "application/appapplication.h"
#include "application/appmainwindow.h"
#include "project/prjproject.h"
#include "viewer/vwrwidget.h"
#include "message/msgmessage.h"
#include "message/msgnode.h"
#include "message/msgsift.h"
#include "commandview/cmvcsyswidget.h"
#include "commandview/cmvexpressionedit.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrsketch.h"
#include "sketch/sktvisual.h"
#include "sketch/sktselection.h"
#include "sketch/sktsolver.h"
#include "command/cmdsketch.h"
#include "commandview/cmvsketch.h"

using boost::uuids::uuid;

using namespace cmv;

struct Sketch::Stow
{
  cmv::Sketch *view = nullptr;
  cmd::Sketch *command = nullptr;
  ftr::Sketch *feature = nullptr;
  skt::Visual *visual = nullptr;
  osg::ref_ptr<skt::Selection> selection;
  prm::Parameter *parameter = nullptr; //!< currently selected parameter or nullptr
  cmv::ParameterEdit *pEdit = nullptr;
  CSysWidget *csysWidget = nullptr;
  
  QRadioButton *sketchRadio = nullptr;
  QRadioButton *csysRadio = nullptr;
  QButtonGroup *radioGroup = nullptr;
  QGroupBox *sketchGroup = nullptr;
  QGroupBox *csysGroup = nullptr;
  QButtonGroup *geoGroup = nullptr;
  QVBoxLayout *sgLayout = nullptr; //so I can add parameter edit widget.
  QAction *pointAction = nullptr;
  QAction *lineAction = nullptr;
  QAction *arcAction = nullptr;
  QAction *circleAction = nullptr;
  QAction *bezierAction = nullptr;
  QAction *coincidentAction = nullptr;
  QAction *horizontalAction = nullptr;
  QAction *verticalAction = nullptr;
  QAction *tangentAction = nullptr;
  QAction *symmetryAction = nullptr;
  QAction *parallelAction = nullptr;
  QAction *perpendicularAction = nullptr;
  QAction *equalAction = nullptr;
  QAction *equalAngleAction = nullptr;
  QAction *midPointAction = nullptr;
  QAction *whereDraggedAction = nullptr;
  QAction *distanceAction = nullptr;
  QAction *diameterAction = nullptr;
  QAction *angleAction = nullptr;
  QAction *commandCancelAction = nullptr;
  QAction *toggleConstructionAction = nullptr;
  QAction *removeAction = nullptr;
  QAction *dummyAction = nullptr; //see uncheck function.
  
  Stow(cmd::Sketch *cIn, cmv::Sketch *vIn)
  : view(vIn)
  , command(cIn)
  , feature(command->feature)
  , visual(feature->getVisual())
  , selection(new skt::Selection(visual))
  {
    buildGui();
    
    QSettings &settings = app::instance()->getUserSettings();
    settings.beginGroup("cmv::Sketch");
    //load settings
    settings.endGroup();
    
    view->sift->insert
    ({
      std::make_pair
      (
        msg::Response | msg::Sketch | msg::Selection | msg::Add
        , std::bind(&Stow::selectParameterDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Sketch | msg::Shape | msg::Done
        , std::bind(&Stow::unCheck, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Sketch | msg::Selection | msg::Clear
        , std::bind(&Stow::selectionCleared, this, std::placeholders::_1)
      )
    });
  }
  
  void buildGui()
  {
    QSizePolicy adjust = view->sizePolicy();
    adjust.setVerticalPolicy(QSizePolicy::Maximum);
    view->setSizePolicy(adjust);
    
    QVBoxLayout *mainLayout = new QVBoxLayout();
    view->setLayout(mainLayout);
    
    sketchRadio = new QRadioButton(tr("Sketch"), view);
    csysRadio = new QRadioButton(tr("CSys"), view);
    radioGroup = new QButtonGroup(view);
    radioGroup->addButton(sketchRadio, 0);
    radioGroup->addButton(csysRadio, 1);
    
    QToolBar *etb = new QToolBar(view); //entity tool bar
    etb->setOrientation(Qt::Vertical);
    geoGroup = new QButtonGroup(view); //defaults to exclusive.
    QAction *chainAction = new QAction(QIcon(":resources/images/sketchChain.svg"), tr("Chain"), view);
    chainAction->setCheckable(true);
    connect(chainAction, &QAction::toggled, view, &Sketch::chainToggled);
    etb->addAction(chainAction);
    pointAction = new QAction(QIcon(":resources/images/sketchPoint.svg"), tr("Point"), view);
    pointAction->setCheckable(true);
    connect(pointAction, &QAction::triggered, view, &Sketch::addPoint);
    etb->addAction(pointAction);
    geoGroup->addButton(static_cast<QAbstractButton*>(etb->widgetForAction(pointAction)));
    lineAction = new QAction(QIcon(":resources/images/sketchLine.svg"), tr("Line"), view);
    lineAction->setCheckable(true);
    connect(lineAction, &QAction::triggered, view, &Sketch::addLine);
    etb->addAction(lineAction);
    geoGroup->addButton(static_cast<QAbstractButton*>(etb->widgetForAction(lineAction)));
    arcAction = new QAction(QIcon(":resources/images/sketchArc.svg"), tr("Arc"), view);
    arcAction->setCheckable(true);
    connect(arcAction, &QAction::triggered, view, &Sketch::addArc);
    etb->addAction(arcAction);
    geoGroup->addButton(static_cast<QAbstractButton*>(etb->widgetForAction(arcAction)));
    circleAction = new QAction(QIcon(":resources/images/sketchCircle.svg"), tr("Circle"), view);
    circleAction->setCheckable(true);
    connect(circleAction, &QAction::triggered, view, &Sketch::addCircle);
    etb->addAction(circleAction);
    geoGroup->addButton(static_cast<QAbstractButton*>(etb->widgetForAction(circleAction)));
    bezierAction = new QAction(QIcon(":resources/images/sketchBezeir.svg"), tr("Bezeir"), view);
    bezierAction->setCheckable(true);
    connect(bezierAction, &QAction::triggered, view, &Sketch::addBezier);
    etb->addAction(bezierAction);
    etb->addSeparator();
    geoGroup->addButton(static_cast<QAbstractButton*>(etb->widgetForAction(bezierAction)));
    dummyAction = new QAction(QIcon(":resources/images/start.svg"), tr("Dummy"), view); //see uncheck
    dummyAction->setCheckable(true);
    dummyAction->setVisible(false);
    etb->addAction(dummyAction);
    geoGroup->addButton(static_cast<QAbstractButton*>(etb->widgetForAction(dummyAction)));
    
    commandCancelAction = new QAction(QIcon(":resources/images/editCommandCancel.svg"), tr("Cancel Command"), view);
    commandCancelAction->setDisabled(true);
    connect(commandCancelAction, &QAction::triggered, view, &Sketch::cancel);
    etb->addAction(commandCancelAction);
    toggleConstructionAction = new QAction(QIcon(":resources/images/constructionBase.svg"), tr("Toggle Construction"), view);
    toggleConstructionAction->setDisabled(true);
    connect(toggleConstructionAction, &QAction::triggered, view, &Sketch::toggleConstruction);
    etb->addAction(toggleConstructionAction);
    removeAction = new QAction(QIcon(":resources/images/editRemove.svg"), tr("Remove"), view);
    removeAction->setDisabled(true);
    connect(removeAction, &QAction::triggered, view, &Sketch::remove);
    etb->addAction(removeAction);
    etb->addSeparator();

    distanceAction = new QAction(QIcon(":resources/images/sketchDistance.svg"), tr("Distance"), view);
    distanceAction->setDisabled(true);
    connect(distanceAction, &QAction::triggered, view, &Sketch::addDistance);
    etb->addAction(distanceAction);
    diameterAction = new QAction(QIcon(":resources/images/sketchDiameter.svg"), tr("Diameter"), view);
    diameterAction->setDisabled(true);
    connect(diameterAction, &QAction::triggered, view, &Sketch::addDiameter);
    etb->addAction(diameterAction);
    angleAction = new QAction(QIcon(":resources/images/sketchAngle.svg"), tr("Angle"), view);
    angleAction->setDisabled(true);
    connect(angleAction, &QAction::triggered, view, &Sketch::addAngle);
    etb->addAction(angleAction);
    
    QToolBar *ctb = new QToolBar(view); //constraint tool bar
    ctb->setOrientation(Qt::Vertical);
    coincidentAction = new QAction(QIcon(":resources/images/sketchCoincident.svg"), tr("Coincident"), view);
    coincidentAction->setDisabled(true);
    connect(coincidentAction, &QAction::triggered, view, &Sketch::addCoincident);
    ctb->addAction(coincidentAction);
    horizontalAction = new QAction(QIcon(":resources/images/sketchHorizontal.svg"), tr("Horizontal"), view);
    horizontalAction->setDisabled(true);
    connect(horizontalAction, &QAction::triggered, view, &Sketch::addHorizontal);
    ctb->addAction(horizontalAction);
    verticalAction = new QAction(QIcon(":resources/images/sketchVertical.svg"), tr("Vertical"), view);
    verticalAction->setDisabled(true);
    connect(verticalAction, &QAction::triggered, view, &Sketch::addVertical);
    ctb->addAction(verticalAction);
    tangentAction = new QAction(QIcon(":resources/images/sketchTangent.svg"), tr("Tangent"), view);
    tangentAction->setDisabled(true);
    connect(tangentAction, &QAction::triggered, view, &Sketch::addTangent);
    ctb->addAction(tangentAction);
    symmetryAction = new QAction(QIcon(":resources/images/sketchSymmetry.svg"), tr("Symmetry"), view);
    symmetryAction->setDisabled(true);
    connect(symmetryAction, &QAction::triggered, view, &Sketch::addSymmetric);
    ctb->addAction(symmetryAction);
    parallelAction = new QAction(QIcon(":resources/images/sketchParallel.svg"), tr("Parallel"), view);
    parallelAction->setDisabled(true);
    connect(parallelAction, &QAction::triggered, view, &Sketch::addParallel);
    ctb->addAction(parallelAction);
    perpendicularAction = new QAction(QIcon(":resources/images/sketchPerpendicular.svg"), tr("Perpendicular"), view);
    perpendicularAction->setDisabled(true);
    connect(perpendicularAction, &QAction::triggered, view, &Sketch::addPerpendicular);
    ctb->addAction(perpendicularAction);
    equalAction = new QAction(QIcon(":resources/images/sketchEqual.svg"), tr("Equal"), view);
    equalAction->setDisabled(true);
    connect(equalAction, &QAction::triggered, view, &Sketch::addEqual);
    ctb->addAction(equalAction);
    equalAngleAction = new QAction(QIcon(":resources/images/sketchEqualAngle.svg"), tr("Equal Angle"), view);
    equalAngleAction->setDisabled(true);
    connect(equalAngleAction, &QAction::triggered, view, &Sketch::addEqualAngle);
    ctb->addAction(equalAngleAction);
    midPointAction = new QAction(QIcon(":resources/images/sketchMidPoint.svg"), tr("Mid Point"), view);
    midPointAction->setDisabled(true);
    connect(midPointAction, &QAction::triggered, view, &Sketch::addMidpoint);
    ctb->addAction(midPointAction);
    whereDraggedAction = new QAction(QIcon(":resources/images/sketchDragged.svg"), tr("Dragged"), view);
    whereDraggedAction->setDisabled(true);
    connect(whereDraggedAction, &QAction::triggered, view, &Sketch::addWhereDragged);
    ctb->addAction(whereDraggedAction);
    
    QToolBar *vtb = new QToolBar(view); //view tool bar
    vtb->setOrientation(Qt::Vertical);
    QAction *viewEntitiesAction = new QAction(QIcon(":resources/images/sketchViewEntities.svg"), tr("View Entities"), view);
    viewEntitiesAction->setCheckable(true);
    viewEntitiesAction->setChecked(true);
    connect(viewEntitiesAction, &QAction::toggled, view, &Sketch::viewEntitiesToggled);
    vtb->addAction(viewEntitiesAction);
    QAction *viewConstraintsAction = new QAction(QIcon(":resources/images/sketchViewConstraints.svg"), tr("View Constraints"), view);
    viewConstraintsAction->setCheckable(true);
    viewConstraintsAction->setChecked(true);
    connect(viewConstraintsAction, &QAction::toggled, view, &Sketch::viewConstraintsToggled);
    vtb->addAction(viewConstraintsAction);
    QAction *viewWorkAction = new QAction(QIcon(":resources/images/sketchViewWork.svg"), tr("View Work"), view);
    viewWorkAction->setCheckable(true);
    viewWorkAction->setChecked(true);
    connect(viewWorkAction, &QAction::toggled, view, &Sketch::viewWorkToggled);
    vtb->addAction(viewWorkAction);
    
    QToolBar *stb = new QToolBar(view); //select tool bar
    stb->setOrientation(Qt::Vertical);
    QAction *selectEntitiesAction = new QAction(QIcon(":resources/images/sketchSelectEntities.svg"), tr("Select Entities"), view);
    selectEntitiesAction->setCheckable(true);
    selectEntitiesAction->setChecked(true);
    connect(selectEntitiesAction, &QAction::toggled, view, &Sketch::selectEntitiesToggled);
    stb->addAction(selectEntitiesAction);
    QAction *selectConstraintsAction = new QAction(QIcon(":resources/images/sketchSelectConstraints.svg"), tr("Select Constraints"), view);
    selectConstraintsAction->setCheckable(true);
    selectConstraintsAction->setChecked(true);
    connect(selectConstraintsAction, &QAction::toggled, view, &Sketch::selectConstraintsToggled);
    stb->addAction(selectConstraintsAction);
    QAction *selectWorkAction = new QAction(QIcon(":resources/images/sketchSelectWork.svg"), tr("Select Work"), view);
    selectWorkAction->setCheckable(true);
    selectWorkAction->setChecked(true);
    connect(selectWorkAction, &QAction::toggled, view, &Sketch::selectWorkToggled);
    stb->addAction(selectWorkAction);

    auto *tbLayout = new QHBoxLayout();
    tbLayout->addWidget(ctb);
    tbLayout->addWidget(etb);
    tbLayout->addWidget(vtb);
    tbLayout->addWidget(stb);
    tbLayout->addStretch();
    
    sgLayout = new QVBoxLayout();
    sgLayout->addLayout(tbLayout);
    
    sketchGroup = new QGroupBox(view);
    sketchGroup->setLayout(sgLayout);
    auto *siLayout = new QHBoxLayout(); //sketch indent layout.
    siLayout->addSpacing(20);
    siLayout->addWidget(sketchGroup);
    
    csysGroup = new QGroupBox(view);
    csysGroup->setDisabled(true);
    auto systemParameters = feature->getParameters(prm::Tags::CSys);
    if (!systemParameters.empty())
    {
      csysWidget = new CSysWidget(view, systemParameters.front());
      ftr::UpdatePayload payload = view->project->getPayload(feature->getId());
      std::vector<const ftr::Base*> inputs = payload.getFeatures(ftr::InputType::linkCSys);
      if (inputs.empty())
        csysWidget->setCSysLinkId(gu::createNilId());
      else
        csysWidget->setCSysLinkId(inputs.front()->getId());
      QObject::connect(csysWidget, &CSysWidget::dirty, view, &Sketch::linkedCSysChanged);
      QHBoxLayout *csysWrapLayout = new QHBoxLayout();
      csysWrapLayout->addWidget(csysWidget);
      csysGroup->setLayout(csysWrapLayout);
    }
    auto *ciLayout = new QHBoxLayout(); //csys indent layout.
    ciLayout->addSpacing(20);
    ciLayout->addWidget(csysGroup);

    mainLayout->addWidget(sketchRadio);
    mainLayout->addLayout(siLayout);
    mainLayout->addWidget(csysRadio);
    mainLayout->addLayout(ciLayout);
  }
  
  void sketchSelectionGo()
  {
    osgViewer::View* view = app::instance()->getMainWindow()->getViewer()->getOsgViewer();
    assert(view);
    view->addEventHandler(selection.get());
    visual->setActiveSketch();
    feature->getOverlaySwitch()->setNodeMask(feature->getOverlaySwitch()->getNodeMask() | skt::ActiveSketch.to_ulong());
  }

  void sketchSelectionStop()
  {
    osgViewer::View* view = app::instance()->getMainWindow()->getViewer()->getOsgViewer();
    assert(view);
    view->removeEventHandler(selection.get());
    visual->clearActiveSketch();
    feature->getOverlaySwitch()->setNodeMask(feature->getOverlaySwitch()->getNodeMask() & (~skt::ActiveSketch).to_ulong());
  }

  void selectionGo()
  {
    view->node->sendBlocked(msg::buildSelectionMask(slc::AllEnabled));
  }

  void selectionStop()
  {
    view->node->sendBlocked(msg::buildSelectionMask(~slc::All));
  }
  
  void selectParameterDispatched(const msg::Message &mIn)
  {
    evaluateActiveControls();
    
    boost::uuids::uuid pId = mIn.getSLC().shapeId;
    if (pId.is_nil() || (!feature->hasParameter(pId)))
    {
      if (pEdit)
      {
        delete pEdit;
        pEdit = nullptr;
      }
      parameter = nullptr;
      return;
    }
    parameter = feature->getParameter(pId);
    pEdit = new cmv::ParameterEdit(view, parameter);
    sgLayout->addWidget(pEdit);
    connect(pEdit, &cmv::ParameterEdit::prmValueChanged, view, &Sketch::prmValueChanged);
    connect(pEdit, &cmv::ParameterEdit::prmConstantChanged, view, &Sketch::prmConstantChanged);
    pEdit->setFocus();
    view->activateWindow();
  }
  
  //https://forum.qt.io/topic/6419/how-to-uncheck-button-in-qbuttongroup/8
  //I tried turning off exclusive and unchecking and turning exclusive back on.
  //  it worked, but had an annoying side effect of user having to hit a button twice that had been unchecked.
  //  now trying the hidden button method. That works without side effect
  void unCheck(const msg::Message&)
  {
    dummyAction->setChecked(true);
    evaluateActiveControls();
  }
  
  void selectionCleared(const msg::Message&)
  {
    evaluateActiveControls();
  }
  
  void evaluateActiveControls()
  {
    if (visual->getState() == skt::State::selection)
    {
      (visual->canCoincident()) ? coincidentAction->setEnabled(true) : coincidentAction->setDisabled(true);
      (visual->canHorizontal()) ? horizontalAction->setEnabled(true) : horizontalAction->setDisabled(true);
      (visual->canVertical()) ? verticalAction->setEnabled(true) : verticalAction->setDisabled(true);
      (visual->canTangent()) ? tangentAction->setEnabled(true) : tangentAction->setDisabled(true);
      (visual->canSymmetry()) ? symmetryAction->setEnabled(true) : symmetryAction->setDisabled(true);
      (visual->canParallel()) ? parallelAction->setEnabled(true) : parallelAction->setDisabled(true);
      (visual->canPerpendicular()) ? perpendicularAction->setEnabled(true) : perpendicularAction->setDisabled(true);
      (visual->canEqual()) ? equalAction->setEnabled(true) : equalAction->setDisabled(true);
      (visual->canEqualAngle()) ? equalAngleAction->setEnabled(true) : equalAngleAction->setDisabled(true);
      (visual->canMidPoint()) ? midPointAction->setEnabled(true) : midPointAction->setDisabled(true);
      (visual->canWhereDragged()) ? whereDraggedAction->setEnabled(true) : whereDraggedAction->setDisabled(true);
      (visual->canDistance()) ? distanceAction->setEnabled(true) : distanceAction->setDisabled(true);
      (visual->canDiameter()) ? diameterAction->setEnabled(true) : diameterAction->setDisabled(true);
      (visual->canAngle()) ? angleAction->setEnabled(true) : angleAction->setDisabled(true);
      
      commandCancelAction->setDisabled(true);
      (visual->canToggleConstruction()) ? toggleConstructionAction->setEnabled(true) : toggleConstructionAction->setDisabled(true);
      (visual->canRemove()) ? removeAction->setEnabled(true) : removeAction->setDisabled(true);
    }
    else
    {
      //we are running a command so disable things like constraints
      coincidentAction->setDisabled(true);
      horizontalAction->setDisabled(true);
      verticalAction->setDisabled(true);
      tangentAction->setDisabled(true);
      symmetryAction->setDisabled(true);
      parallelAction->setDisabled(true);
      perpendicularAction->setDisabled(true);
      equalAction->setDisabled(true);
      equalAngleAction->setDisabled(true);
      midPointAction->setDisabled(true);
      whereDraggedAction->setDisabled(true);
      distanceAction->setDisabled(true);
      diameterAction->setDisabled(true);
      angleAction->setDisabled(true);
      
      commandCancelAction->setEnabled(true);
      toggleConstructionAction->setEnabled(false);
      removeAction->setEnabled(false);
    }
  }
};

Sketch::Sketch(cmd::Sketch *cIn)
: Base("cmv::Sketch")
, stow(new Stow(cIn, this))
{
  connect(stow->radioGroup, SIGNAL(buttonToggled(int, bool)), this, SLOT(buttonToggled(int, bool)));
}

Sketch::~Sketch()
{
  stow->visual->showAll();
}

void Sketch::activate()
{
  node->sendBlocked(msg::buildHideThreeD(stow->feature->getId()));
  node->sendBlocked(msg::buildShowOverlay(stow->feature->getId()));
  stow->feature->draggerHide();
  int bid = stow->radioGroup->checkedId();
  if (bid < 0 || bid > 1) //initial state
  {
    stow->selection->setMask(skt::WorkPlaneOrigin | skt::WorkPlaneAxis | skt::Entity | skt::Constraint | skt::SelectionPlane);
    stow->sketchRadio->setChecked(true);
  }
  else
    buttonToggled(bid, true);
  node->sendBlocked(msg::Message(msg::Request | msg::Overlay | msg::Selection | msg::Freeze));
}

void Sketch::deactivate()
{
  stow->visual->cancel();
  stow->visual->clearSelection();
  stow->sketchSelectionStop();
  stow->selectionGo();
  stow->feature->draggerShow();
  
  node->sendBlocked(msg::Message(msg::Request | msg::Overlay | msg::Selection | msg::Thaw));
}

void Sketch::buttonToggled(int id, bool checked)
{
  if (id == 0)
  {
    if (checked)
    {
      stow->selectionStop();
      stow->sketchSelectionGo();
      stow->sketchGroup->setEnabled(true);
      node->sendBlocked(msg::buildStatusMessage("Select Command Or Entities"));
    }
    else
      stow->sketchGroup->setDisabled(true);
  }
  if (id == 1)
  {
    if (checked)
    {
      stow->sketchSelectionStop();
      stow->selectionGo();
      stow->csysGroup->setEnabled(true);
    }
    else
      stow->csysGroup->setDisabled(true);
  }
}

void Sketch::chainToggled(bool state)
{
  stow->visual->cancel();
  stow->unCheck(msg::Message());
  if (state)
    stow->visual->setChainOn();
  else
    stow->visual->setChainOff();
}

void Sketch::addPoint(bool state)
{
  if (state)
  {
    if (!stow->visual->isChainOn() && stow->visual->getState() != skt::State::selection)
      stow->visual->cancel();
    stow->visual->addPoint();
    stow->evaluateActiveControls();
  }
}

void Sketch::addLine(bool state)
{
  if (state)
  {
    if (!stow->visual->isChainOn() && stow->visual->getState() != skt::State::selection)
      stow->visual->cancel();
    stow->visual->addLine();
    stow->evaluateActiveControls();
  }
}

void Sketch::addArc(bool state)
{
  if (state)
  {
    if (!stow->visual->isChainOn() && stow->visual->getState() != skt::State::selection)
      stow->visual->cancel();
    stow->visual->addArc();
    stow->evaluateActiveControls();
  }
}

void Sketch::addCircle(bool state)
{
  if (state)
  {
    if (!stow->visual->isChainOn() && stow->visual->getState() != skt::State::selection)
      stow->visual->cancel();
    stow->visual->addCircle();
    stow->evaluateActiveControls();
  }
}

void Sketch::addBezier(bool state)
{
  if (state)
  {
    if (!stow->visual->isChainOn() && stow->visual->getState() != skt::State::selection)
      stow->visual->cancel();
    stow->visual->addCubicBezier();
    stow->evaluateActiveControls();
  }
}

void Sketch::remove()
{
  if (stow->parameter)
  {
    uint32_t h = stow->feature->getHPHandle(stow->parameter);
    if (h != 0)
      stow->feature->removeHPPair(h);
    stow->parameter = nullptr;
    delete stow->pEdit;
    stow->pEdit = nullptr;
  }
  
  if (stow->visual->getState() != skt::State::selection)
    stow->visual->cancel();
  stow->visual->remove();
  stow->feature->cleanHPPair();
}

void Sketch::cancel()
{
  stow->visual->cancel();
  stow->unCheck(msg::Message());
  stow->evaluateActiveControls();
}

void Sketch::toggleConstruction()
{
  stow->visual->toggleConstruction();
}

void Sketch::addCoincident()
{
  if (stow->visual->getState() != skt::State::selection)
    stow->visual->cancel();
  stow->visual->addCoincident();
  stow->evaluateActiveControls();
}

void Sketch::addHorizontal()
{
  if (stow->visual->getState() != skt::State::selection)
    stow->visual->cancel();
  stow->visual->addHorizontal();
}

void Sketch::addVertical()
{
  if (stow->visual->getState() != skt::State::selection)
    stow->visual->cancel();
  stow->visual->addVertical();
}

void Sketch::addTangent()
{
  if (stow->visual->getState() != skt::State::selection)
    stow->visual->cancel();
  stow->visual->addTangent();
}

void Sketch::addDistance()
{
  if (stow->visual->getState() != skt::State::selection)
    stow->visual->cancel();
  
  auto p = stow->visual->addDistance();
  if (p)
    stow->feature->addHPPair(p.get().first, p.get().second);
}

void Sketch::addDiameter()
{
  if (stow->visual->getState() != skt::State::selection)
    stow->visual->cancel();
  
  auto p = stow->visual->addDiameter();
  if (p)
    stow->feature->addHPPair(p.get().first, p.get().second);
}

void Sketch::addAngle()
{
  if (stow->visual->getState() != skt::State::selection)
    stow->visual->cancel();
  
  auto p = stow->visual->addAngle();
  if (p)
    stow->feature->addHPPair(p.get().first, p.get().second);
}

void Sketch::addSymmetric()
{
  if (stow->visual->getState() != skt::State::selection)
    stow->visual->cancel();
  stow->visual->addSymmetric();
}

void Sketch::addParallel()
{
  if (stow->visual->getState() != skt::State::selection)
    stow->visual->cancel();
  stow->visual->addParallel();
}

void Sketch::addPerpendicular()
{
  if (stow->visual->getState() != skt::State::selection)
    stow->visual->cancel();
  stow->visual->addPerpendicular();
}

void Sketch::addEqual()
{
  if (stow->visual->getState() != skt::State::selection)
    stow->visual->cancel();
  stow->visual->addEqual();
}

void Sketch::addEqualAngle()
{
  if (stow->visual->getState() != skt::State::selection)
    stow->visual->cancel();
  stow->visual->addEqualAngle();
}

void Sketch::addMidpoint()
{
  if (stow->visual->getState() != skt::State::selection)
    stow->visual->cancel();
  stow->visual->addMidpoint();
}

void Sketch::addWhereDragged()
{
  if (stow->visual->getState() != skt::State::selection)
    stow->visual->cancel();
  stow->visual->addWhereDragged();
}

void Sketch::prmValueChanged()
{
  assert(stow->pEdit);
  assert(stow->parameter);
  
  double pValue = stow->parameter->getDouble();
  stow->feature->getSolver()->updateConstraintValue(stow->feature->getHPHandle(stow->parameter), pValue);
  stow->feature->getSolver()->solve(stow->feature->getSolver()->getGroup(), true);
  stow->visual->update();
}

void Sketch::prmConstantChanged()
{
  prmValueChanged();
}

void Sketch::linkedCSysChanged()
{
  project->clearAllInputs(stow->feature->getId());
  uuid lId = stow->csysWidget->getCSysLinkId();
  if (!lId.is_nil())
    project->connect(lId, stow->feature->getId(), {ftr::InputType::linkCSys});
  stow->command->localUpdate();
}

void Sketch::viewEntitiesToggled(bool state)
{
  if (state)
    stow->visual->showEntity();
  else
    stow->visual->hideEntity();
}

void Sketch::viewConstraintsToggled(bool state)
{
  if (state)
    stow->visual->showConstraint();
  else
    stow->visual->hideConstraint();
}

void Sketch::viewWorkToggled(bool state)
{
  if (state)
    stow->visual->showPlane();
  else
    stow->visual->hidePlane();
}

void Sketch::selectEntitiesToggled(bool state)
{
  if (state)
    stow->selection->entitiesOn();
  else
    stow->selection->entitiesOff();
}

void Sketch::selectConstraintsToggled(bool state)
{
  if (state)
    stow->selection->constraintsOn();
  else
    stow->selection->constraintsOff();
}

void Sketch::selectWorkToggled(bool state)
{
  if (state)
  {
    stow->selection->workPlaneAxesOn();
    stow->selection->workPlaneOriginOn();
  }
  else
  {
    stow->selection->workPlaneAxesOff();
    stow->selection->workPlaneOriginOff();
  }
}
