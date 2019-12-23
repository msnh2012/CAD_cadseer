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

#include <osgViewer/Viewer>

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
#include "dialogs/dlgcsyswidget.h"
#include "dialogs/dlgvizfilter.h"
#include "parameter/prmparameter.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrsketch.h"
#include "sketch/sktsolver.h"
#include "sketch/sktnodemasks.h"
#include "sketch/sktselection.h"
#include "sketch/sktvisual.h"
#include "dialogs/dlgsketch.h"

using boost::uuids::uuid;

namespace dlg
{
  struct Sketch::Stow
  {
    ftr::Sketch *feature = nullptr;
    std::shared_ptr<prm::Parameter> csys;
    osg::ref_ptr<skt::Selection> selection;
    CSysWidget *csysWidget = nullptr;
    QTabWidget *tabWidget = nullptr;
    ExpressionEdit *pEdit = nullptr;
    prm::Parameter *parameter = nullptr; //!< currently selected parameter or nullptr
    std::unique_ptr<msg::Node> node;
    std::unique_ptr<msg::Sift> sift;
    bool isAccepted = false;
    bool wasVisible3d = false;
    bool wasVisibleOverlay = false;
    
    Stow (ftr::Sketch *fIn)
    : feature(fIn)
    , csys(std::make_shared<prm::Parameter>(*feature->getParameter(prm::Names::CSys)))
    , selection(new skt::Selection(feature->getVisual()))
    {
      
    }
  };
}

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

Sketch::Sketch(ftr::Sketch *sIn, QWidget *parent):
QDialog(parent)
, stow(std::make_unique<Stow>(sIn))
{
  stow->node = std::make_unique<msg::Node>();
  stow->node->connect(msg::hub());
  stow->sift = std::make_unique<msg::Sift>();
  stow->sift->name = "dlg::Sketch";
  stow->node->setHandler(std::bind(&msg::Sift::receive, stow->sift.get(), std::placeholders::_1));
  
  stow->node->sendBlocked(msg::Message(msg::Request | msg::Overlay | msg::Selection | msg::Freeze));
  
  buildGui();
  initGui();
  
  WidgetGeometry *filter = new WidgetGeometry(this, "dlg::Sketch");
  this->installEventFilter(filter);
  
  setupDispatcher();
}

Sketch::~Sketch()
{
  stow->node->sendBlocked(msg::Message(msg::Request | msg::Overlay | msg::Selection | msg::Thaw));
}

void Sketch::reject()
{
  stow->isAccepted = false;
  finishDialog();
  QDialog::reject();
}

void Sketch::accept()
{
  stow->isAccepted = true;
  finishDialog();
  QDialog::accept();
  if (prf::manager().rootPtr->dragger().triggerUpdateOnFinish())
    app::instance()->queuedMessage(msg::Message(msg::Request | msg::Project | msg::Update));
}

void Sketch::sketchSelectionGo()
{
  osgViewer::View* view = app::instance()->getMainWindow()->getViewer()->getOsgViewer();
  assert(view);
  view->addEventHandler(stow->selection.get());
  stow->feature->getVisual()->setActiveSketch();
  stow->feature->getOverlaySwitch()->setNodeMask(stow->feature->getOverlaySwitch()->getNodeMask() | skt::ActiveSketch.to_ulong());
}

void Sketch::sketchSelectionStop()
{
  osgViewer::View* view = app::instance()->getMainWindow()->getViewer()->getOsgViewer();
  assert(view);
  view->removeEventHandler(stow->selection.get());
  stow->feature->getVisual()->clearActiveSketch();
  stow->feature->getOverlaySwitch()->setNodeMask(stow->feature->getOverlaySwitch()->getNodeMask() & (~skt::ActiveSketch).to_ulong());
}

void Sketch::selectionGo()
{
  stow->node->sendBlocked(msg::buildSelectionMask(slc::AllEnabled));
}

void Sketch::selectionStop()
{
  stow->node->sendBlocked(msg::buildSelectionMask(~slc::All));
}

void Sketch::draggerShow()
{
  stow->feature->draggerShow();
}

void Sketch::draggerHide()
{
  stow->feature->draggerHide();
}

void Sketch::setupDispatcher()
{
  stow->sift->insert
  (
    msg::Response | msg::Sketch | msg::Selection | msg::Add
    , std::bind(&Sketch::selectParameterDispatched, this, std::placeholders::_1)
  );
}

void Sketch::selectParameterDispatched(const msg::Message &mIn)
{
  boost::uuids::uuid pId = mIn.getSLC().shapeId;
  if (pId.is_nil() || (!stow->feature->hasParameter(pId)))
  {
    stow->pEdit->lineEdit->clear();
    stow->pEdit->setDisabled(true);
    stow->parameter = nullptr;
    return;
  }
  stow->parameter = stow->feature->getParameter(pId);
  activateWindow();
  stow->pEdit->setEnabled(true);
  if (stow->parameter->isConstant())
    setEditUnlinked();
  else
    setEditLinked();
}

void Sketch::initGui()
{
  stow->wasVisible3d = stow->feature->isVisible3D();
  stow->wasVisibleOverlay = stow->feature->isVisibleOverlay();
  stow->node->sendBlocked(msg::buildHideThreeD(stow->feature->getId()));
  stow->node->sendBlocked(msg::buildShowOverlay(stow->feature->getId()));
  stow->selection->setMask(skt::WorkPlaneOrigin | skt::WorkPlaneAxis | skt::Entity | skt::Constraint | skt::SelectionPlane);
  draggerHide();
  selectionStop();
}
    
void Sketch::buildGui()
{
  stow->tabWidget = new QTabWidget(this);
  stow->tabWidget->addTab(buildEditPage(), tr("Edit"));
  VizFilter *vf = new VizFilter(stow->tabWidget->widget(0));
  stow->tabWidget->widget(0)->installEventFilter(vf);
  connect(vf, &VizFilter::shown, this, &Sketch::selectionStop);
  
  stow->csysWidget = new CSysWidget(this, stow->csys.get());
  ftr::UpdatePayload payload = app::instance()->getProject()->getPayload(stow->feature->getId());
  std::vector<const ftr::Base*> inputs = payload.getFeatures(ftr::InputType::linkCSys);
  if (inputs.empty())
    stow->csysWidget->setCSysLinkId(gu::createNilId());
  else
    stow->csysWidget->setCSysLinkId(inputs.front()->getId());
  stow->tabWidget->addTab(stow->csysWidget, tr("CSys"));
  
  QDialogButtonBox *buttons = new QDialogButtonBox
    (QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
  QHBoxLayout *buttonLayout = new QHBoxLayout();
  buttonLayout->addStretch();
  buttonLayout->addWidget(buttons);
  connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
  
  QVBoxLayout *vl = new QVBoxLayout();
  vl->addWidget(stow->tabWidget);
  vl->addLayout(buttonLayout);
  this->setLayout(vl);
}

QWidget* Sketch::buildEditPage()
{
  EditPage *out = new EditPage(stow->tabWidget);
  
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
  
  stow->pEdit = new ExpressionEdit(this);
  stow->pEdit->lineEdit->clear();
  stow->pEdit->setDisabled(true);
  ExpressionEditFilter *filter = new ExpressionEditFilter(this);
  stow->pEdit->lineEdit->installEventFilter(filter);
  EnterFilter *ef = new EnterFilter(this);
  stow->pEdit->lineEdit->installEventFilter(ef);
  QObject::connect(filter, SIGNAL(requestLinkSignal(QString)), this, SLOT(requestParameterLinkSlot(QString)));
  QObject::connect(stow->pEdit->lineEdit, SIGNAL(textEdited(QString)), this, SLOT(textEditedParameterSlot(QString)));
  QObject::connect(ef, SIGNAL(enterPressed()), this, SLOT(updateParameterSlot()));
  QObject::connect(stow->pEdit->trafficLabel, SIGNAL(requestUnlinkSignal()), this, SLOT(requestParameterUnlinkSlot()));
  
  QHBoxLayout *dtbLayout = new QHBoxLayout();
  dtbLayout->addWidget(dtb);
  dtbLayout->addStretch();
  dtbLayout->addWidget(stow->pEdit);
  
  QVBoxLayout *vl = new QVBoxLayout();
  vl->addLayout(el);
  vl->addWidget(ctb);
  vl->addLayout(dtbLayout);
  vl->addStretch();
  out->setLayout(vl);
  
  connect(out, &EditPage::shown, this, &Sketch::sketchSelectionGo);
  connect(out, &EditPage::shown, [this](){stow->node->sendBlocked(msg::buildStatusMessage(QObject::tr("Edit sketch feature").toStdString()));});
  connect(out, &EditPage::hidden, this, &Sketch::sketchSelectionStop);
  
  return out;
}

void Sketch::finishDialog()
{
  stow->feature->getVisual()->clearSelection();
  sketchSelectionStop();
  selectionGo();
  stow->feature->draggerShow();
  
  if (stow->isAccepted)
  {
    //this might be inconsistent, because we have no transaction for sketch.
    stow->csysWidget->syncLinks();
    stow->feature->setCSys(static_cast<osg::Matrixd>(*stow->csys));
    
    prj::Project *project = app::instance()->getProject();
    std::vector<const ftr::Base*> inputs = project->getPayload(stow->feature->getId()).getFeatures(ftr::InputType::linkCSys);
    uuid oldLink = gu::createNilId();
    if (!inputs.empty())
      oldLink = inputs.front()->getId();
    
    uuid newLinked = stow->csysWidget->getCSysLinkId();
    if (newLinked != oldLink)
    {
      stow->feature->setModelDirty();
      project->clearAllInputs(stow->feature->getId());
      if (!newLinked.is_nil())
        project->connectInsert(newLinked, stow->feature->getId(), ftr::InputType{ftr::InputType::linkCSys});
    }
  }
  
  if (stow->wasVisible3d)
    stow->node->sendBlocked(msg::buildShowThreeD(stow->feature->getId()));
  else
    stow->node->sendBlocked(msg::buildHideThreeD(stow->feature->getId()));
  if (stow->wasVisibleOverlay)
    stow->node->sendBlocked(msg::buildShowOverlay(stow->feature->getId()));
  else
    stow->node->sendBlocked(msg::buildHideOverlay(stow->feature->getId()));
  
  msg::Message mOut(msg::Mask(msg::Request | msg::Command | msg::Done));
  app::instance()->queuedMessage(mOut);
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
    stow->node->send(msg::buildStatusMessage(QObject::tr("Value out of range").toStdString(), 2.0));
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
      stow->node->send(msg::buildStatusMessage(QObject::tr("Value out of range").toStdString(), 2.0));
    }
  }
  else
  {
    stow->node->send(msg::buildStatusMessage(QObject::tr("Parsing failed").toStdString(), 2.0));
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
