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
#include "dialogs/dlgexpressionedit.h"
#include "dialogs/dlgenterfilter.h"
#include "commandview/cmvcsyswidget.h"
#include "parameter/prmparameter.h"
#include "expressions/exprmanager.h"
#include "expressions/exprstringtranslator.h"
#include "expressions/exprvalue.h"
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
  prm::Parameter *parameter = nullptr; //!< currently selected parameter or nullptr
  dlg::ExpressionEdit *pEdit = nullptr;
  osg::ref_ptr<skt::Selection> selection;
  CSysWidget *csysWidget = nullptr;
  
  QRadioButton *sketchRadio = nullptr;
  QRadioButton *csysRadio = nullptr;
  QButtonGroup *radioGroup = nullptr;
  QGroupBox *sketchGroup = nullptr;
  QGroupBox *csysGroup = nullptr;
  
  Stow(cmd::Sketch *cIn, cmv::Sketch *vIn)
  : view(vIn)
  , command(cIn)
  , feature(command->feature)
  , selection(new skt::Selection(feature->getVisual()))
  {
    buildGui();
    
    QSettings &settings = app::instance()->getUserSettings();
    settings.beginGroup("cmv::Sketch");
    //load settings
    settings.endGroup();
    
    view->sift->insert
    (
      msg::Response | msg::Sketch | msg::Selection | msg::Add
      , std::bind(&Stow::selectParameterDispatched, this, std::placeholders::_1)
    );
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
    QAction *spa = new QAction(QIcon(":resources/images/sketchPoint.svg"), tr("Point"), view);
    connect(spa, &QAction::triggered, view, &Sketch::addPoint);
    etb->addAction(spa);
    QAction *sla = new QAction(QIcon(":resources/images/sketchLine.svg"), tr("Line"), view);
    connect(sla, &QAction::triggered, view, &Sketch::addLine);
    etb->addAction(sla);
    QAction *saa = new QAction(QIcon(":resources/images/sketchArc.svg"), tr("Arc"), view);
    connect(saa, &QAction::triggered, view, &Sketch::addArc);
    etb->addAction(saa);
    QAction *sca = new QAction(QIcon(":resources/images/sketchCircle.svg"), tr("Circle"), view);
    connect(sca, &QAction::triggered, view, &Sketch::addCircle);
    etb->addAction(sca);
    QAction *sba = new QAction(QIcon(":resources/images/sketchBezeir.svg"), tr("Bezeir"), view);
    etb->addAction(sba);
    //TODO add connection for bezeir
    etb->addSeparator();
    
    QAction *cca = new QAction(QIcon(":resources/images/editCommandCancel.svg"), tr("Cancel Command"), view);
    connect(cca, &QAction::triggered, view, &Sketch::cancel);
    etb->addAction(cca);
    QAction *tca = new QAction(QIcon(":resources/images/constructionBase.svg"), tr("Toggle Construction"), view);
    connect(tca, &QAction::triggered, view, &Sketch::toggleConstruction);
    etb->addAction(tca);
    QAction *ra = new QAction(QIcon(":resources/images/editRemove.svg"), tr("Remove"), view);
    connect(ra, &QAction::triggered, view, &Sketch::remove);
    etb->addAction(ra);
    etb->addSeparator();

    QAction *distanceAction = new QAction(QIcon(":resources/images/sketchDistance.svg"), tr("Distance"), view);
    connect(distanceAction, &QAction::triggered, view, &Sketch::addDistance);
    etb->addAction(distanceAction);
    QAction *diameterAction = new QAction(QIcon(":resources/images/sketchDiameter.svg"), tr("Diameter"), view);
    connect(diameterAction, &QAction::triggered, view, &Sketch::addDiameter);
    etb->addAction(diameterAction);
    QAction *angleAction = new QAction(QIcon(":resources/images/sketchAngle.svg"), tr("Angle"), view);
    connect(angleAction, &QAction::triggered, view, &Sketch::addAngle);
    etb->addAction(angleAction);
    
    QToolBar *ctb = new QToolBar(view); //constraint tool bar
    ctb->setOrientation(Qt::Vertical);
    QAction *coincidentAction = new QAction(QIcon(":resources/images/sketchCoincident.svg"), tr("Coincident"), view);
    connect(coincidentAction, &QAction::triggered, view, &Sketch::addCoincident);
    ctb->addAction(coincidentAction);
    QAction *horizontalAction = new QAction(QIcon(":resources/images/sketchHorizontal.svg"), tr("Horizontal"), view);
    connect(horizontalAction, &QAction::triggered, view, &Sketch::addHorizontal);
    ctb->addAction(horizontalAction);
    QAction *verticalAction = new QAction(QIcon(":resources/images/sketchVertical.svg"), tr("Vertical"), view);
    connect(verticalAction, &QAction::triggered, view, &Sketch::addVertical);
    ctb->addAction(verticalAction);
    QAction *tangentAction = new QAction(QIcon(":resources/images/sketchTangent.svg"), tr("Tangent"), view);
    connect(tangentAction, &QAction::triggered, view, &Sketch::addTangent);
    ctb->addAction(tangentAction);
    QAction *symmetryAction = new QAction(QIcon(":resources/images/sketchSymmetry.svg"), tr("Symmetry"), view);
    connect(symmetryAction, &QAction::triggered, view, &Sketch::addSymmetric);
    ctb->addAction(symmetryAction);
    QAction *parallelAction = new QAction(QIcon(":resources/images/sketchParallel.svg"), tr("Parallel"), view);
    connect(parallelAction, &QAction::triggered, view, &Sketch::addParallel);
    ctb->addAction(parallelAction);
    QAction *perpendicularAction = new QAction(QIcon(":resources/images/sketchPerpendicular.svg"), tr("Perpendicular"), view);
    connect(perpendicularAction, &QAction::triggered, view, &Sketch::addPerpendicular);
    ctb->addAction(perpendicularAction);
    QAction *equalAction = new QAction(QIcon(":resources/images/sketchEqual.svg"), tr("Equal"), view);
    connect(equalAction, &QAction::triggered, view, &Sketch::addEqual);
    ctb->addAction(equalAction);
    QAction *equalAngleAction = new QAction(QIcon(":resources/images/sketchEqualAngle.svg"), tr("Equal Angle"), view);
    connect(equalAngleAction, &QAction::triggered, view, &Sketch::addEqualAngle);
    ctb->addAction(equalAngleAction);
    QAction *midPointAction = new QAction(QIcon(":resources/images/sketchMidPoint.svg"), tr("Mid Point"), view);
    connect(midPointAction, &QAction::triggered, view, &Sketch::addMidpoint);
    ctb->addAction(midPointAction);
    QAction *whereDraggedAction = new QAction(QIcon(":resources/images/sketchDragged.svg"), tr("Dragged"), view);
    connect(whereDraggedAction, &QAction::triggered, view, &Sketch::addWhereDragged);
    ctb->addAction(whereDraggedAction);

    pEdit = new dlg::ExpressionEdit(view);
    pEdit->lineEdit->clear();
    pEdit->setDisabled(true);
    dlg::ExpressionEditFilter *filter = new dlg::ExpressionEditFilter(view);
    pEdit->lineEdit->installEventFilter(filter);
    dlg::EnterFilter *ef = new dlg::EnterFilter(view);
    pEdit->lineEdit->installEventFilter(ef);
    QObject::connect(filter, &dlg::ExpressionEditFilter::requestLinkSignal, view, &Sketch::requestParameterLinkSlot);
    QObject::connect(pEdit->lineEdit, &QLineEdit::textEdited, view, &Sketch::textEditedParameterSlot);
    QObject::connect(ef, &dlg::EnterFilter::enterPressed, view, &Sketch::updateParameterSlot);
    QObject::connect(pEdit->trafficLabel, &dlg::TrafficLabel::requestUnlinkSignal, view, &Sketch::requestParameterUnlinkSlot);
    
    auto *tbLayout = new QHBoxLayout();
    tbLayout->addWidget(ctb);
    tbLayout->addWidget(etb);
    tbLayout->addStretch();
    
    auto *sgLayout = new QVBoxLayout();
    sgLayout->addLayout(tbLayout);
    sgLayout->addWidget(pEdit);
    
    sketchGroup = new QGroupBox(view);
    sketchGroup->setLayout(sgLayout);
    auto *siLayout = new QHBoxLayout(); //sketch indent layout.
    siLayout->addSpacing(20);
    siLayout->addWidget(sketchGroup);
    
    csysGroup = new QGroupBox(view);
    csysGroup->setDisabled(true);
    auto *p = feature->getParameter(prm::Names::CSys);
    if (p)
    {
      csysWidget = new CSysWidget(view, p);
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
    feature->getVisual()->setActiveSketch();
    feature->getOverlaySwitch()->setNodeMask(feature->getOverlaySwitch()->getNodeMask() | skt::ActiveSketch.to_ulong());
  }

  void sketchSelectionStop()
  {
    osgViewer::View* view = app::instance()->getMainWindow()->getViewer()->getOsgViewer();
    assert(view);
    view->removeEventHandler(selection.get());
    feature->getVisual()->clearActiveSketch();
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
    boost::uuids::uuid pId = mIn.getSLC().shapeId;
    if (pId.is_nil() || (!feature->hasParameter(pId)))
    {
      pEdit->lineEdit->clear();
      pEdit->setDisabled(true);
      parameter = nullptr;
      return;
    }
    parameter = feature->getParameter(pId);
    view->activateWindow();
    pEdit->setEnabled(true);
    if (parameter->isConstant())
      setEditUnlinked();
    else
      setEditLinked();
  }
  
  void setEditLinked()
  {
    assert(parameter);
    assert(pEdit);
    assert(!parameter->isConstant());
    
    pEdit->trafficLabel->setLinkSlot();
    pEdit->clearFocus();
    pEdit->lineEdit->deselect();
    pEdit->lineEdit->setReadOnly(true);
    
    expr::Manager &manager = app::instance()->getProject()->getManager();
    pEdit->lineEdit->setText(QString::fromStdString(manager.getFormulaName(manager.getFormulaLink(parameter->getId()))));
  }

  void setEditUnlinked()
  {
    assert(parameter);
    assert(pEdit);
    assert(parameter->isConstant());
    
    pEdit->trafficLabel->setTrafficGreenSlot();
    pEdit->lineEdit->setReadOnly(false);
    pEdit->lineEdit->setText(QString::number(static_cast<double>(*parameter), 'f', 12));
    pEdit->lineEdit->selectAll();
    pEdit->setFocus();
  }
};

Sketch::Sketch(cmd::Sketch *cIn)
: Base("cmv::Sketch")
, stow(new Stow(cIn, this))
{
  connect(stow->radioGroup, SIGNAL(buttonToggled(int, bool)), this, SLOT(buttonToggled(int, bool)));
}

Sketch::~Sketch() = default;

void Sketch::activate()
{
  int bid = stow->radioGroup->checkedId();
  if (bid < 0 || bid > 1) //initial state
  {
    node->sendBlocked(msg::buildHideThreeD(stow->feature->getId()));
    node->sendBlocked(msg::buildShowOverlay(stow->feature->getId()));
    stow->selection->setMask(skt::WorkPlaneOrigin | skt::WorkPlaneAxis | skt::Entity | skt::Constraint | skt::SelectionPlane);
    stow->feature->draggerHide();
    stow->sketchRadio->setChecked(true);
  }
  else
    buttonToggled(bid, true);
  node->sendBlocked(msg::Message(msg::Request | msg::Overlay | msg::Selection | msg::Freeze));
}

void Sketch::deactivate()
{
  
  stow->feature->getVisual()->clearSelection();
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

void Sketch::addPoint()
{
  if (stow->feature->getVisual()->getState() != skt::State::selection)
    stow->feature->getVisual()->cancel();
  stow->feature->getVisual()->addPoint();
}

void Sketch::addLine()
{
  if (stow->feature->getVisual()->getState() != skt::State::selection)
    stow->feature->getVisual()->cancel();
  stow->feature->getVisual()->addLine();
}

void Sketch::addArc()
{
  if (stow->feature->getVisual()->getState() != skt::State::selection)
    stow->feature->getVisual()->cancel();
  stow->feature->getVisual()->addArc();
}

void Sketch::addCircle()
{
  if (stow->feature->getVisual()->getState() != skt::State::selection)
    stow->feature->getVisual()->cancel();
  stow->feature->getVisual()->addCircle();
}

void Sketch::remove()
{
  if (stow->parameter)
  {
    uint32_t h = stow->feature->getHPHandle(stow->parameter);
    if (h != 0)
      stow->feature->removeHPPair(h);
    stow->parameter = nullptr;
    stow->pEdit->lineEdit->clear();
    stow->pEdit->setDisabled(true);
  }
  
  if (stow->feature->getVisual()->getState() != skt::State::selection)
    stow->feature->getVisual()->cancel();
  stow->feature->getVisual()->remove();
  stow->feature->cleanHPPair();
}

void Sketch::cancel()
{
  stow->feature->getVisual()->cancel();
}

void Sketch::toggleConstruction()
{
  stow->feature->getVisual()->toggleConstruction();
}

void Sketch::addCoincident()
{
  if (stow->feature->getVisual()->getState() != skt::State::selection)
    stow->feature->getVisual()->cancel();
  stow->feature->getVisual()->addCoincident();
}

void Sketch::addHorizontal()
{
  if (stow->feature->getVisual()->getState() != skt::State::selection)
    stow->feature->getVisual()->cancel();
  stow->feature->getVisual()->addHorizontal();
}

void Sketch::addVertical()
{
  if (stow->feature->getVisual()->getState() != skt::State::selection)
    stow->feature->getVisual()->cancel();
  stow->feature->getVisual()->addVertical();
}

void Sketch::addTangent()
{
  if (stow->feature->getVisual()->getState() != skt::State::selection)
    stow->feature->getVisual()->cancel();
  stow->feature->getVisual()->addTangent();
}

void Sketch::addDistance()
{
  if (stow->feature->getVisual()->getState() != skt::State::selection)
    stow->feature->getVisual()->cancel();
  
  auto p = stow->feature->getVisual()->addDistance();
  if (p)
    stow->feature->addHPPair(p.get().first, p.get().second);
}

void Sketch::addDiameter()
{
  if (stow->feature->getVisual()->getState() != skt::State::selection)
    stow->feature->getVisual()->cancel();
  
  auto p = stow->feature->getVisual()->addDiameter();
  if (p)
    stow->feature->addHPPair(p.get().first, p.get().second);
}

void Sketch::addAngle()
{
  if (stow->feature->getVisual()->getState() != skt::State::selection)
    stow->feature->getVisual()->cancel();
  
  auto p = stow->feature->getVisual()->addAngle();
  if (p)
    stow->feature->addHPPair(p.get().first, p.get().second);
}

void Sketch::addSymmetric()
{
  if (stow->feature->getVisual()->getState() != skt::State::selection)
    stow->feature->getVisual()->cancel();
  stow->feature->getVisual()->addSymmetric();
}

void Sketch::addParallel()
{
  if (stow->feature->getVisual()->getState() != skt::State::selection)
    stow->feature->getVisual()->cancel();
  stow->feature->getVisual()->addParallel();
}

void Sketch::addPerpendicular()
{
  if (stow->feature->getVisual()->getState() != skt::State::selection)
    stow->feature->getVisual()->cancel();
  stow->feature->getVisual()->addPerpendicular();
}

void Sketch::addEqual()
{
  if (stow->feature->getVisual()->getState() != skt::State::selection)
    stow->feature->getVisual()->cancel();
  stow->feature->getVisual()->addEqual();
}

void Sketch::addEqualAngle()
{
  if (stow->feature->getVisual()->getState() != skt::State::selection)
    stow->feature->getVisual()->cancel();
  stow->feature->getVisual()->addEqualAngle();
}

void Sketch::addMidpoint()
{
  if (stow->feature->getVisual()->getState() != skt::State::selection)
    stow->feature->getVisual()->cancel();
  stow->feature->getVisual()->addMidpoint();
}

void Sketch::addWhereDragged()
{
  if (stow->feature->getVisual()->getState() != skt::State::selection)
    stow->feature->getVisual()->cancel();
  stow->feature->getVisual()->addWhereDragged();
}

void Sketch::requestParameterLinkSlot(const QString &stringIn)
{
  assert(stow->parameter);
  assert(stow->pEdit);
  
  boost::uuids::uuid eId = gu::stringToId(stringIn.toStdString());
  assert(!eId.is_nil()); //project asserts on presence of expression eId.
  prj::Project *project = app::instance()->getProject();
  double eValue = boost::get<double>(project->getManager().getFormulaValue(eId));
  
  if (stow->parameter->isValidValue(eValue))
  {
    project->expressionLink(stow->parameter->getId(), eId);
    setEditLinked();
    stow->feature->getVisual()->reHighlight();
    
    stow->feature->getSolver()->updateConstraintValue(stow->feature->getHPHandle(stow->parameter), eValue);
    stow->feature->getSolver()->solve(stow->feature->getSolver()->getGroup(), true);
    stow->feature->getVisual()->update();
  }
  else
  {
    node->send(msg::buildStatusMessage(QObject::tr("Value out of range").toStdString(), 2.0));
  }
  
  this->activateWindow();
}

void Sketch::requestParameterUnlinkSlot()
{
  assert(stow->parameter);
  assert(stow->pEdit);
  
  app::instance()->getProject()->expressionUnlink(stow->parameter->getId());
  
  setEditUnlinked();
  stow->feature->getVisual()->reHighlight();
  //don't need to update, because unlinking doesn't change parameter value.
}

void Sketch::setEditLinked()
{
  assert(stow->parameter);
  assert(stow->pEdit);
  assert(!stow->parameter->isConstant());
  
  stow->pEdit->trafficLabel->setLinkSlot();
  stow->pEdit->clearFocus();
  stow->pEdit->lineEdit->deselect();
  stow->pEdit->lineEdit->setReadOnly(true);
  
  expr::Manager &manager = app::instance()->getProject()->getManager();
  stow->pEdit->lineEdit->setText(QString::fromStdString(manager.getFormulaName(manager.getFormulaLink(stow->parameter->getId()))));
}

void Sketch::setEditUnlinked()
{
  assert(stow->parameter);
  assert(stow->pEdit);
  assert(stow->parameter->isConstant());
  
  stow->pEdit->trafficLabel->setTrafficGreenSlot();
  stow->pEdit->lineEdit->setReadOnly(false);
  stow->pEdit->lineEdit->setText(QString::number(static_cast<double>(*stow->parameter), 'f', 12));
  stow->pEdit->lineEdit->selectAll();
  stow->pEdit->setFocus();
}

void Sketch::updateParameterSlot()
{
  assert(stow->pEdit);
  assert(stow->parameter);
  
  if (!stow->parameter->isConstant())
    return;

  expr::Manager localManager;
  expr::StringTranslator translator(localManager);
  std::string formula("temp = ");
  formula += stow->pEdit->lineEdit->text().toStdString();
  if (translator.parseString(formula) == expr::StringTranslator::ParseSucceeded)
  {
    localManager.update();
    assert(localManager.getFormulaValueType(translator.getFormulaOutId()) == expr::ValueType::Scalar);
    double value = boost::get<double>(localManager.getFormulaValue(translator.getFormulaOutId()));
    if (stow->parameter->isValidValue(value))
    {
      stow->parameter->setValue(value);
      stow->feature->getSolver()->updateConstraintValue(stow->feature->getHPHandle(stow->parameter), value);
      stow->feature->getSolver()->solve(stow->feature->getSolver()->getGroup(), true);
      stow->feature->getVisual()->update();
    }
    else
    {
      node->send(msg::buildStatusMessage(QObject::tr("Value out of range").toStdString(), 2.0));
    }
  }
  else
  {
    node->send(msg::buildStatusMessage(QObject::tr("Parsing failed").toStdString(), 2.0));
  }
  stow->pEdit->lineEdit->setText(QString::number(static_cast<double>(*stow->parameter), 'f', 12));
  stow->pEdit->lineEdit->selectAll();
  stow->pEdit->trafficLabel->setTrafficGreenSlot();
}

void Sketch::textEditedParameterSlot(const QString &textIn)
{
  assert(stow->pEdit);
  
  stow->pEdit->trafficLabel->setTrafficYellowSlot();
  qApp->processEvents(); //need this or we never see yellow signal.
  
  expr::Manager localManager;
  expr::StringTranslator translator(localManager);
  std::string formula("temp = ");
  formula += textIn.toStdString();
  if (translator.parseString(formula) == expr::StringTranslator::ParseSucceeded)
  {
    localManager.update();
    stow->pEdit->trafficLabel->setTrafficGreenSlot();
    assert(localManager.getFormulaValueType(translator.getFormulaOutId()) == expr::ValueType::Scalar);
    double value = boost::get<double>(localManager.getFormulaValue(translator.getFormulaOutId()));
    stow->pEdit->goToolTipSlot(QString::number(value));
  }
  else
  {
    stow->pEdit->trafficLabel->setTrafficRedSlot();
    int position = translator.getFailedPosition() - 8; // 7 chars for 'temp = ' + 1
    stow->pEdit->goToolTipSlot(textIn.left(position) + "?");
  }
}

void Sketch::linkedCSysChanged()
{
  project->clearAllInputs(stow->feature->getId());
  uuid lId = stow->csysWidget->getCSysLinkId();
  if (!lId.is_nil())
    project->connect(lId, stow->feature->getId(), {ftr::InputType::linkCSys});
  stow->command->localUpdate();
}
