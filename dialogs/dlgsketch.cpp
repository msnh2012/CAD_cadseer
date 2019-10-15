/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2018  Thomas S. Anderson blobfish.at.gmx.com
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
#include <boost/optional/optional.hpp>

#include <QTabWidget>
#include <QToolBar>
#include <QDialogButtonBox>
#include <QBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QShowEvent>
#include <QHideEvent>
#include <QLineEdit>
#include <QTimer>

#include "application/appapplication.h"
#include "application/appmainwindow.h"
#include "project/prjproject.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "expressions/exprmanager.h"
#include "expressions/exprstringtranslator.h"
#include "expressions/exprvalue.h"
#include "message/msgnode.h"
#include "message/msgsift.h"
#include "selection/slcdefinitions.h"
#include "selection/slcmessage.h"
#include "viewer/vwrwidget.h"
#include "dialogs/dlgwidgetgeometry.h"
#include "dialogs/dlgenterfilter.h"
#include "dialogs/dlgexpressionedit.h"
#include "parameter/prmparameter.h"
#include "feature/ftrsketch.h"
#include "sketch/sktsolver.h"
#include "sketch/sktnodemasks.h"
#include "sketch/sktselection.h"
#include "sketch/sktvisual.h"
#include "dialogs/dlgsketch.h"

using namespace dlg;

EditPage::EditPage(QWidget *parent) : QWidget(parent) {}

EditPage::~EditPage() {}

void EditPage::showEvent(QShowEvent *e)
{
  QWidget::showEvent(e);
  shown();
}

void EditPage::hideEvent(QHideEvent *e)
{
  QWidget::hideEvent(e);
  hidden();
}

PositionPage::PositionPage(QWidget *parent) : QWidget(parent) {}

PositionPage::~PositionPage() {}

void PositionPage::showEvent(QShowEvent *e)
{
  QWidget::showEvent(e);
  shown();
}

void PositionPage::hideEvent(QHideEvent *e)
{
  QWidget::hideEvent(e);
  hidden();
}


Sketch::Sketch(ftr::Sketch *sIn, QWidget *parent):
QDialog(parent)
, sketch(sIn)
, selection(new skt::Selection(sketch->getVisual()))
{
  node = std::make_unique<msg::Node>();
  node->connect(msg::hub());
  sift = std::make_unique<msg::Sift>();
  sift->name = "dlg::Sketch";
  node->setHandler(std::bind(&msg::Sift::receive, sift.get(), std::placeholders::_1));
  
  node->sendBlocked(msg::Message(msg::Request | msg::Overlay | msg::Selection | msg::Freeze));
  
  buildGui();
  initGui();
  
  WidgetGeometry *filter = new WidgetGeometry(this, "dlg::Sketch");
  this->installEventFilter(filter);
  
  setupDispatcher();
}

Sketch::~Sketch()
{
  node->sendBlocked(msg::Message(msg::Request | msg::Overlay | msg::Selection | msg::Thaw));
}

void Sketch::reject()
{
  isAccepted = false;
  finishDialog();
  QDialog::reject();
}

void Sketch::accept()
{
  isAccepted = true;
  finishDialog();
  QDialog::accept();
  if (prf::manager().rootPtr->dragger().triggerUpdateOnFinish())
    app::instance()->queuedMessage(msg::Message(msg::Request | msg::Project | msg::Update));
}

void Sketch::sketchSelectionGo()
{
  osgViewer::View* view = app::instance()->getMainWindow()->getViewer()->getView(0);
  assert(view);
  view->addEventHandler(selection.get());
  sketch->getVisual()->setActiveSketch();
  sketch->getOverlaySwitch()->setNodeMask(sketch->getOverlaySwitch()->getNodeMask() | skt::ActiveSketch.to_ulong());
}

void Sketch::sketchSelectionStop()
{
  osgViewer::View* view = app::instance()->getMainWindow()->getViewer()->getView(0);
  assert(view);
  view->removeEventHandler(selection.get());
  sketch->getVisual()->clearActiveSketch();
  sketch->getOverlaySwitch()->setNodeMask(sketch->getOverlaySwitch()->getNodeMask() & (~skt::ActiveSketch).to_ulong());
}

void Sketch::selectionGo()
{
  node->sendBlocked(msg::buildSelectionMask(slc::AllEnabled));
}

void Sketch::selectionStop()
{
  node->sendBlocked(msg::buildSelectionMask(~slc::AllEnabled));
}

void Sketch::draggerShow()
{
  sketch->draggerShow();
}

void Sketch::draggerHide()
{
  sketch->draggerHide();
}

void Sketch::setupDispatcher()
{
  sift->insert
  (
    msg::Response | msg::Sketch | msg::Selection | msg::Add
    , std::bind(&Sketch::selectParameterDispatched, this, std::placeholders::_1)
  );
}

void Sketch::selectParameterDispatched(const msg::Message &mIn)
{
  boost::uuids::uuid pId = mIn.getSLC().shapeId;
  if (pId.is_nil() || (!sketch->hasParameter(pId)))
  {
    pEdit->lineEdit->clear();
    pEdit->setDisabled(true);
    parameter = nullptr;
    return;
  }
  parameter = sketch->getParameter(pId);
  activateWindow();
  pEdit->setEnabled(true);
  if (parameter->isConstant())
    setEditUnlinked();
  else
    setEditLinked();
}

void Sketch::initGui()
{
  wasVisible3d = sketch->isVisible3D();
  wasVisibleOverlay = sketch->isVisibleOverlay();
  node->sendBlocked(msg::buildHideThreeD(sketch->getId()));
  node->sendBlocked(msg::buildShowOverlay(sketch->getId()));
  selection->setMask(skt::WorkPlaneOrigin | skt::WorkPlaneAxis | skt::Entity | skt::Constraint | skt::SelectionPlane);
  draggerHide();
}
    
void Sketch::buildGui()
{
  tabWidget = new QTabWidget(this);
  tabWidget->addTab(buildEditPage(), tr("Edit"));
  tabWidget->addTab(buildPositionPage(), tr("Position"));
  
  QDialogButtonBox *buttons = new QDialogButtonBox
    (QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
  QHBoxLayout *buttonLayout = new QHBoxLayout();
  buttonLayout->addStretch();
  buttonLayout->addWidget(buttons);
  connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
  
  QVBoxLayout *vl = new QVBoxLayout();
  vl->addWidget(tabWidget);
  vl->addLayout(buttonLayout);
  this->setLayout(vl);
}

QWidget* Sketch::buildPositionPage()
{
  //nothing yet.
  PositionPage *out = new PositionPage(tabWidget);
  
  connect(out, &PositionPage::shown, this, &Sketch::selectionGo);
  connect(out, &PositionPage::shown, this, &Sketch::draggerShow);
  connect(out, &PositionPage::hidden, this, &Sketch::selectionStop);
  connect(out, &PositionPage::hidden, this, &Sketch::draggerHide);
  
  return out;
}

QWidget* Sketch::buildEditPage()
{
  EditPage *out = new EditPage(tabWidget);
  
  QToolBar *etb = new QToolBar(out); //entity tool bar
  QAction *spa = new QAction(QIcon(":resources/images/sketchPoint.svg"), tr("Point"), out);
  connect(spa, &QAction::triggered, this, &Sketch::addPoint);
  etb->addAction(spa);
  QAction *sla = new QAction(QIcon(":resources/images/sketchLine.svg"), tr("Line"), out);
  connect(sla, &QAction::triggered, this, &Sketch::addLine);
  etb->addAction(sla);
  QAction *saa = new QAction(QIcon(":resources/images/sketchArc.svg"), tr("Arc"), out);
  connect(saa, &QAction::triggered, this, &Sketch::addArc);
  etb->addAction(saa);
  QAction *sca = new QAction(QIcon(":resources/images/sketchCircle.svg"), tr("Circle"), out);
  connect(sca, &QAction::triggered, this, &Sketch::addCircle);
  etb->addAction(sca);
  QAction *sba = new QAction(QIcon(":resources/images/sketchBezeir.svg"), tr("Bezeir"), out);
  etb->addAction(sba);
  
  QToolBar *ctrlb = new QToolBar(out); //control tool bar
  QAction *cca = new QAction(QIcon(":resources/images/editCommandCancel.svg"), tr("Cancel Command"), out);
  connect(cca, &QAction::triggered, this, &Sketch::cancel);
  ctrlb->addAction(cca);
  QAction *tca = new QAction(QIcon(":resources/images/constructionBase.svg"), tr("Toggle Construction"), out);
  connect(tca, &QAction::triggered, this, &Sketch::toggleConstruction);
  ctrlb->addAction(tca);
  QAction *ra = new QAction(QIcon(":resources/images/editRemove.svg"), tr("Remove"), out);
  connect(ra, &QAction::triggered, this, &Sketch::remove);
  ctrlb->addAction(ra);
  
  QHBoxLayout *el = new QHBoxLayout();
  el->addWidget(etb);
  el->addStretch();
  el->addWidget(ctrlb);
  
  QToolBar *ctb = new QToolBar(out); //constraint tool bar
  QAction *coincidentAction = new QAction(QIcon(":resources/images/sketchCoincident.svg"), tr("Coincident"), out);
  connect(coincidentAction, &QAction::triggered, this, &Sketch::addCoincident);
  ctb->addAction(coincidentAction);
  QAction *horizontalAction = new QAction(QIcon(":resources/images/sketchHorizontal.svg"), tr("Horizontal"), out);
  connect(horizontalAction, &QAction::triggered, this, &Sketch::addHorizontal);
  ctb->addAction(horizontalAction);
  QAction *verticalAction = new QAction(QIcon(":resources/images/sketchVertical.svg"), tr("Vertical"), out);
  connect(verticalAction, &QAction::triggered, this, &Sketch::addVertical);
  ctb->addAction(verticalAction);
  QAction *tangentAction = new QAction(QIcon(":resources/images/sketchTangent.svg"), tr("Tangent"), out);
  connect(tangentAction, &QAction::triggered, this, &Sketch::addTangent);
  ctb->addAction(tangentAction);
  QAction *symmetryAction = new QAction(QIcon(":resources/images/sketchSymmetry.svg"), tr("Symmetry"), out);
  connect(symmetryAction, &QAction::triggered, this, &Sketch::addSymmetric);
  ctb->addAction(symmetryAction);
  QAction *parallelAction = new QAction(QIcon(":resources/images/sketchParallel.svg"), tr("Parallel"), out);
  connect(parallelAction, &QAction::triggered, this, &Sketch::addParallel);
  ctb->addAction(parallelAction);
  QAction *perpendicularAction = new QAction(QIcon(":resources/images/sketchPerpendicular.svg"), tr("Perpendicular"), out);
  connect(perpendicularAction, &QAction::triggered, this, &Sketch::addPerpendicular);
  ctb->addAction(perpendicularAction);
  QAction *equalAction = new QAction(QIcon(":resources/images/sketchEqual.svg"), tr("Equal"), out);
  connect(equalAction, &QAction::triggered, this, &Sketch::addEqual);
  ctb->addAction(equalAction);
  QAction *equalAngleAction = new QAction(QIcon(":resources/images/sketchEqualAngle.svg"), tr("Equal Angle"), out);
  connect(equalAngleAction, &QAction::triggered, this, &Sketch::addEqualAngle);
  ctb->addAction(equalAngleAction);
  QAction *midPointAction = new QAction(QIcon(":resources/images/sketchMidPoint.svg"), tr("Mid Point"), out);
  connect(midPointAction, &QAction::triggered, this, &Sketch::addMidpoint);
  ctb->addAction(midPointAction);
  QAction *whereDraggedAction = new QAction(QIcon(":resources/images/sketchDragged.svg"), tr("Dragged"), out);
  connect(whereDraggedAction, &QAction::triggered, this, &Sketch::addWhereDragged);
  ctb->addAction(whereDraggedAction);
  
  QToolBar *dtb = new QToolBar(out); //dimension tool bar
  QAction *distanceAction = new QAction(QIcon(":resources/images/sketchDistance.svg"), tr("Distance"), out);
  connect(distanceAction, &QAction::triggered, this, &Sketch::addDistance);
  dtb->addAction(distanceAction);
  QAction *diameterAction = new QAction(QIcon(":resources/images/sketchDiameter.svg"), tr("Diameter"), out);
  connect(diameterAction, &QAction::triggered, this, &Sketch::addDiameter);
  dtb->addAction(diameterAction);
  QAction *angleAction = new QAction(QIcon(":resources/images/sketchAngle.svg"), tr("Angle"), out);
  connect(angleAction, &QAction::triggered, this, &Sketch::addAngle);
  dtb->addAction(angleAction);
  
  pEdit = new ExpressionEdit(this);
  pEdit->lineEdit->clear();
  pEdit->setDisabled(true);
  ExpressionEditFilter *filter = new ExpressionEditFilter(this);
  pEdit->lineEdit->installEventFilter(filter);
  EnterFilter *ef = new EnterFilter(this);
  pEdit->lineEdit->installEventFilter(ef);
  QObject::connect(filter, SIGNAL(requestLinkSignal(QString)), this, SLOT(requestParameterLinkSlot(QString)));
  QObject::connect(pEdit->lineEdit, SIGNAL(textEdited(QString)), this, SLOT(textEditedParameterSlot(QString)));
  QObject::connect(ef, SIGNAL(enterPressed()), this, SLOT(updateParameterSlot()));
  QObject::connect(pEdit->trafficLabel, SIGNAL(requestUnlinkSignal()), this, SLOT(requestParameterUnlinkSlot()));
  
  QHBoxLayout *dtbLayout = new QHBoxLayout();
  dtbLayout->addWidget(dtb);
  dtbLayout->addStretch();
  dtbLayout->addWidget(pEdit);
  
  QVBoxLayout *vl = new QVBoxLayout();
  vl->addLayout(el);
  vl->addWidget(ctb);
  vl->addLayout(dtbLayout);
  vl->addStretch();
  out->setLayout(vl);
  
  connect(out, &EditPage::shown, this, &Sketch::sketchSelectionGo);
  connect(out, &EditPage::hidden, this, &Sketch::sketchSelectionStop);
  
  return out;
}

void Sketch::finishDialog()
{
  sketch->getVisual()->clearSelection();
  sketchSelectionStop();
  selectionGo();
  sketch->draggerShow();
  
  if (wasVisible3d)
    node->sendBlocked(msg::buildShowThreeD(sketch->getId()));
  else
    node->sendBlocked(msg::buildHideThreeD(sketch->getId()));
  if (wasVisibleOverlay)
    node->sendBlocked(msg::buildShowOverlay(sketch->getId()));
  else
    node->sendBlocked(msg::buildHideOverlay(sketch->getId()));
  
  msg::Message mOut(msg::Mask(msg::Request | msg::Command | msg::Done));
  app::instance()->queuedMessage(mOut);
}

void Sketch::addPoint()
{
  if (sketch->getVisual()->getState() != skt::State::selection)
    sketch->getVisual()->cancel();
  sketch->getVisual()->addPoint();
}

void Sketch::addLine()
{
  if (sketch->getVisual()->getState() != skt::State::selection)
    sketch->getVisual()->cancel();
  sketch->getVisual()->addLine();
}

void Sketch::addArc()
{
  if (sketch->getVisual()->getState() != skt::State::selection)
    sketch->getVisual()->cancel();
  sketch->getVisual()->addArc();
}

void Sketch::addCircle()
{
  if (sketch->getVisual()->getState() != skt::State::selection)
    sketch->getVisual()->cancel();
  sketch->getVisual()->addCircle();
}

void Sketch::addCoincident()
{
  if (sketch->getVisual()->getState() != skt::State::selection)
    sketch->getVisual()->cancel();
  sketch->getVisual()->addCoincident();
}

void Sketch::addHorizontal()
{
  if (sketch->getVisual()->getState() != skt::State::selection)
    sketch->getVisual()->cancel();
  sketch->getVisual()->addHorizontal();
}

void Sketch::addVertical()
{
  if (sketch->getVisual()->getState() != skt::State::selection)
    sketch->getVisual()->cancel();
  sketch->getVisual()->addVertical();
}

void Sketch::addTangent()
{
  if (sketch->getVisual()->getState() != skt::State::selection)
    sketch->getVisual()->cancel();
  sketch->getVisual()->addTangent();
}

void Sketch::addDistance()
{
  if (sketch->getVisual()->getState() != skt::State::selection)
    sketch->getVisual()->cancel();
  
  auto p = sketch->getVisual()->addDistance();
  if (p)
    sketch->addHPPair(p.get().first, p.get().second);
}

void Sketch::addDiameter()
{
  if (sketch->getVisual()->getState() != skt::State::selection)
    sketch->getVisual()->cancel();
  
  auto p = sketch->getVisual()->addDiameter();
  if (p)
    sketch->addHPPair(p.get().first, p.get().second);
}

void Sketch::addAngle()
{
  if (sketch->getVisual()->getState() != skt::State::selection)
    sketch->getVisual()->cancel();
  
  auto p = sketch->getVisual()->addAngle();
  if (p)
    sketch->addHPPair(p.get().first, p.get().second);
}

void Sketch::addSymmetric()
{
  if (sketch->getVisual()->getState() != skt::State::selection)
    sketch->getVisual()->cancel();
  sketch->getVisual()->addSymmetric();
}

void Sketch::addParallel()
{
  if (sketch->getVisual()->getState() != skt::State::selection)
    sketch->getVisual()->cancel();
  sketch->getVisual()->addParallel();
}

void Sketch::addPerpendicular()
{
  if (sketch->getVisual()->getState() != skt::State::selection)
    sketch->getVisual()->cancel();
  sketch->getVisual()->addPerpendicular();
}

void Sketch::addEqual()
{
  if (sketch->getVisual()->getState() != skt::State::selection)
    sketch->getVisual()->cancel();
  sketch->getVisual()->addEqual();
}

void Sketch::addEqualAngle()
{
  if (sketch->getVisual()->getState() != skt::State::selection)
    sketch->getVisual()->cancel();
  sketch->getVisual()->addEqualAngle();
}

void Sketch::addMidpoint()
{
  if (sketch->getVisual()->getState() != skt::State::selection)
    sketch->getVisual()->cancel();
  sketch->getVisual()->addMidpoint();
}

void Sketch::addWhereDragged()
{
  if (sketch->getVisual()->getState() != skt::State::selection)
    sketch->getVisual()->cancel();
  sketch->getVisual()->addWhereDragged();
}

void Sketch::remove()
{
  if (parameter)
  {
    uint32_t h = sketch->getHPHandle(parameter);
    if (h != 0)
      sketch->removeHPPair(h);
    parameter = nullptr;
    pEdit->lineEdit->clear();
    pEdit->setDisabled(true);
  }
  
  if (sketch->getVisual()->getState() != skt::State::selection)
    sketch->getVisual()->cancel();
  sketch->getVisual()->remove();
  sketch->cleanHPPair();
}

void Sketch::cancel()
{
  sketch->getVisual()->cancel();
}

void Sketch::toggleConstruction()
{
  sketch->getVisual()->toggleConstruction();
}

void Sketch::requestParameterLinkSlot(const QString &stringIn)
{
  assert(parameter);
  assert(pEdit);
  
  boost::uuids::uuid eId = gu::stringToId(stringIn.toStdString());
  assert(!eId.is_nil()); //project asserts on presence of expression eId.
  prj::Project *project = app::instance()->getProject();
  double eValue = boost::get<double>(project->getManager().getFormulaValue(eId));
  
  if (parameter->isValidValue(eValue))
  {
    project->expressionLink(parameter->getId(), eId);
    setEditLinked();
    sketch->getVisual()->reHighlight();
    
    sketch->getSolver()->updateConstraintValue(sketch->getHPHandle(parameter), eValue);
    sketch->getSolver()->solve(sketch->getSolver()->getGroup(), true);
    sketch->getVisual()->update();
  }
  else
  {
    node->send(msg::buildStatusMessage(QObject::tr("Value out of range").toStdString(), 2.0));
  }
  
  this->activateWindow();
}

void Sketch::requestParameterUnlinkSlot()
{
  assert(parameter);
  assert(pEdit);
  
  app::instance()->getProject()->expressionUnlink(parameter->getId());
  
  setEditUnlinked();
  sketch->getVisual()->reHighlight();
  //don't need to update, because unlinking doesn't change parameter value.
}

void Sketch::setEditLinked()
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

void Sketch::setEditUnlinked()
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

void Sketch::updateParameterSlot()
{
  assert(pEdit);
  assert(parameter);
  
  if (!parameter->isConstant())
    return;

  expr::Manager localManager;
  expr::StringTranslator translator(localManager);
  std::string formula("temp = ");
  formula += pEdit->lineEdit->text().toStdString();
  if (translator.parseString(formula) == expr::StringTranslator::ParseSucceeded)
  {
    localManager.update();
    assert(localManager.getFormulaValueType(translator.getFormulaOutId()) == expr::ValueType::Scalar);
    double value = boost::get<double>(localManager.getFormulaValue(translator.getFormulaOutId()));
    if (parameter->isValidValue(value))
    {
      parameter->setValue(value);
      sketch->getSolver()->updateConstraintValue(sketch->getHPHandle(parameter), value);
      sketch->getSolver()->solve(sketch->getSolver()->getGroup(), true);
      sketch->getVisual()->update();
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
  pEdit->lineEdit->setText(QString::number(static_cast<double>(*parameter), 'f', 12));
  pEdit->lineEdit->selectAll();
  pEdit->trafficLabel->setTrafficGreenSlot();
}

void Sketch::textEditedParameterSlot(const QString &textIn)
{
  assert(pEdit);
  
  pEdit->trafficLabel->setTrafficYellowSlot();
  qApp->processEvents(); //need this or we never see yellow signal.
  
  expr::Manager localManager;
  expr::StringTranslator translator(localManager);
  std::string formula("temp = ");
  formula += textIn.toStdString();
  if (translator.parseString(formula) == expr::StringTranslator::ParseSucceeded)
  {
    localManager.update();
    pEdit->trafficLabel->setTrafficGreenSlot();
    assert(localManager.getFormulaValueType(translator.getFormulaOutId()) == expr::ValueType::Scalar);
    double value = boost::get<double>(localManager.getFormulaValue(translator.getFormulaOutId()));
    pEdit->goToolTipSlot(QString::number(value));
  }
  else
  {
    pEdit->trafficLabel->setTrafficRedSlot();
    int position = translator.getFailedPosition() - 8; // 7 chars for 'temp = ' + 1
    pEdit->goToolTipSlot(textIn.left(position) + "?");
  }
}
