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

#include <cassert>

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QPainter>
#include <QAction>
#include <QMimeData>
#include <QTimer>

#include "tools/idtools.h"
#include "application/appapplication.h"
#include "project/prjproject.h"
#include "message/msgmessage.h"
#include "parameter/prmparameter.h"
#include "parameter/prmexpressionlink.h"
#include "commandview/cmvtrafficsignal.h"

using boost::uuids::uuid;

static uuid getId(const QString &stringIn)
{
  boost::uuids::uuid idOut = gu::createNilId();
  if (stringIn.startsWith("ExpressionId;"))
  {
    QStringList split = stringIn.split(";");
    if (split.size() == 2)
      idOut = gu::stringToId(split.at(1).toStdString());
  }
  return idOut;
}

struct cmv::TrafficSignal::Stow
{
  TrafficSignal *trafficSignal;
  prm::Parameter *parameter;
  prm::Observer pObserver;
  int iconSize = 128; //qlabel will scale.
  QPixmap trafficRed;
  QPixmap trafficYellow;
  QPixmap trafficGreen;
  QPixmap trafficLink;
  QAction *unlinkAction;
  
  Stow() = delete;
  Stow(TrafficSignal *tsIn, prm::Parameter *pIn)
  : trafficSignal(tsIn)
  , parameter(pIn)
  {
    assert(trafficSignal);
    assert(parameter);
    pObserver.constantHandler = std::bind(&Stow::constantChanged, this);
    parameter->connect(pObserver);
    
    trafficRed = buildPixmap(":/resources/images/trafficRed.svg");
    trafficYellow = buildPixmap(":/resources/images/trafficYellow.svg");
    trafficGreen = buildPixmap(":/resources/images/trafficGreen.svg");
    trafficLink = buildPixmap(":resources/images/linkIcon.svg");
    
    unlinkAction = new QAction(tr("unlink"), trafficSignal);
    QObject::connect(unlinkAction, &QAction::triggered, trafficSignal, &TrafficSignal::unlink);
    
    if (parameter->isConstant())
      QTimer::singleShot(0, trafficSignal, &TrafficSignal::setTrafficGreenSlot); //assumption
    else
      QTimer::singleShot(0, trafficSignal, &TrafficSignal::setTrafficLinkSlot);
  }
  
  void constantChanged()
  {
    if (parameter->isConstant())
      trafficSignal->setTrafficGreenSlot(); //assumption
    else
      trafficSignal->setTrafficLinkSlot();
  }
  
  QPixmap buildPixmap(const QString &resourceNameIn)
  {
    QPixmap temp = QPixmap(resourceNameIn).scaled(iconSize, iconSize, Qt::KeepAspectRatio);
    QPixmap out(iconSize, iconSize);
    QPainter painter(&out);
    painter.fillRect(out.rect(), qApp->palette().color(QPalette::Window));
    painter.drawPixmap(out.rect(), temp, temp.rect());
    painter.end();
    
    return out;
  }
};

using namespace cmv;

TrafficSignal::TrafficSignal(QWidget* parentIn, prm::Parameter *pIn)
: QLabel(parentIn)
, stow(std::make_unique<Stow>(this, pIn))
{
  assert(parentIn);
  setContentsMargins(0, 0, 0, 0);
  setContextMenuPolicy(Qt::ActionsContextMenu);
  setAcceptDrops(true);
  setToolTip(tr("Drop Expression To Link. Right Click To Unlink"));
  setWhatsThis(tr("Drop Expression To Link. Right Click To Unlink"));
}

TrafficSignal::~TrafficSignal() = default;

void TrafficSignal::setTrafficRedSlot()
{
  setPixmap(stow->trafficRed);
  this->removeAction(stow->unlinkAction);
}

void TrafficSignal::setTrafficYellowSlot()
{
  setPixmap(stow->trafficYellow);
  this->removeAction(stow->unlinkAction);
}

void TrafficSignal::setTrafficGreenSlot()
{
  setPixmap(stow->trafficGreen);
  this->removeAction(stow->unlinkAction);
}

void TrafficSignal::setTrafficLinkSlot()
{
  setPixmap(stow->trafficLink);
  this->addAction(stow->unlinkAction);
}

void TrafficSignal::unlink()
{
  prm::unLinkExpression(stow->parameter);
  app::instance()->messageSlot(msg::buildStatusMessage("Expression UnLinked"));
}

void TrafficSignal::dragEnterEvent(QDragEnterEvent *event)
{
  if (event->mimeData()->hasText())
  {
    QString textIn = event->mimeData()->text();
    uuid id = getId(textIn);
    if (!id.is_nil() && prm::canLinkExpression(stow->parameter, id))
      event->acceptProposedAction();
  }
}

void TrafficSignal::dropEvent(QDropEvent *event)
{
  if (event->mimeData()->hasText())
  {
    QString textIn = event->mimeData()->text();
    uuid id = getId(textIn);
    if (!id.is_nil() && prm::canLinkExpression(stow->parameter, id))
    {
      event->acceptProposedAction();
      if (prm::linkExpression(stow->parameter, id))
        app::instance()->messageSlot(msg::buildStatusMessage("Expression Link Accepted"));
      else
        app::instance()->messageSlot(msg::buildStatusMessage("Expression Link Rejected"));
    }
    else
      app::instance()->messageSlot(msg::buildStatusMessage("Expression Link Rejected"));
  }
}
