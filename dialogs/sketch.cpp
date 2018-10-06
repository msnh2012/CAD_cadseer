/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018  tanderson <email>
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
 */

#include <QTabWidget>
#include <QToolBar>
#include <QDialogButtonBox>
#include <QBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QShowEvent>
#include <QHideEvent>

#include "application/application.h"
#include "application/mainwindow.h"
#include "preferences/preferencesXML.h"
#include "preferences/manager.h"
#include "message/message.h"
#include "message/observer.h"
#include "selection/definitions.h"
#include "viewer/widget.h"
#include "dialogs/widgetgeometry.h"
#include "feature/sketch.h"
#include "sketch/nodemasks.h"
#include "sketch/selection.h"
#include "sketch/visual.h"
#include "dialogs/sketch.h"

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
, observer(new msg::Observer())
{
  buildGui();
  initGui();
  
  observer->name = "dlg::Sketch";
  
  WidgetGeometry *filter = new WidgetGeometry(this, "dlg::Sketch");
  this->installEventFilter(filter);
}

Sketch::~Sketch(){}

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
  observer->outBlocked(msg::buildSelectionMask(slc::AllEnabled));
}

void Sketch::selectionStop()
{
  observer->outBlocked(msg::buildSelectionMask(~slc::AllEnabled));
}

void Sketch::draggerShow()
{
  sketch->draggerShow();
}

void Sketch::draggerHide()
{
  sketch->draggerHide();
}


void Sketch::initGui()
{
  wasVisible3d = sketch->isVisible3D();
  wasVisibleOverlay = sketch->isVisibleOverlay();
  observer->outBlocked(msg::buildHideThreeD(sketch->getId()));
  observer->outBlocked(msg::buildShowOverlay(sketch->getId()));
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
  
  QVBoxLayout *vl = new QVBoxLayout();
  vl->addLayout(el);
  vl->addWidget(ctb);
  vl->addWidget(dtb);
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
    observer->outBlocked(msg::buildShowThreeD(sketch->getId()));
  else
    observer->outBlocked(msg::buildHideThreeD(sketch->getId()));
  if (wasVisibleOverlay)
    observer->outBlocked(msg::buildShowOverlay(sketch->getId()));
  else
    observer->outBlocked(msg::buildHideOverlay(sketch->getId()));
  
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
  sketch->getVisual()->addDistance();
}

void Sketch::addDiameter()
{
  if (sketch->getVisual()->getState() != skt::State::selection)
    sketch->getVisual()->cancel();
  sketch->getVisual()->addDiameter();
}

void Sketch::addAngle()
{
  if (sketch->getVisual()->getState() != skt::State::selection)
    sketch->getVisual()->cancel();
  sketch->getVisual()->addAngle();
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
  if (sketch->getVisual()->getState() != skt::State::selection)
    sketch->getVisual()->cancel();
  sketch->getVisual()->remove();
}

void Sketch::cancel()
{
  sketch->getVisual()->cancel();
}

void Sketch::toggleConstruction()
{
  sketch->getVisual()->toggleConstruction();
}
