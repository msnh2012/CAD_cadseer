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
#include "expressions/exprmanager.h"
#include "commandview/cmvtrafficsignal.h"

namespace
{
  int getId(const QString &stringIn)
  {
    int idOut = -1;
    if (stringIn.startsWith("ExpressionId;"))
    {
      QStringList split = stringIn.split(";");
      if (split.size() == 2)
        idOut = split.at(1).toInt();
    }
    return idOut;
  }
  
  QPixmap buildPixmap(const QString &resourceNameIn)
  {
    //the qlabel will scale
    QPixmap temp = QPixmap(resourceNameIn).scaled(128, 128, Qt::KeepAspectRatio);
    QPixmap out(128, 128);
    QPainter painter(&out);
    painter.fillRect(out.rect(), qApp->palette().color(QPalette::Window));
    painter.drawPixmap(out.rect(), temp, temp.rect());
    painter.end();
    return out;
  }

  thread_local QPixmap trafficRed = buildPixmap(":/resources/images/trafficRed.svg");
  thread_local QPixmap trafficYellow = buildPixmap(":/resources/images/trafficYellow.svg");
  thread_local QPixmap trafficGreen = buildPixmap(":/resources/images/trafficGreen.svg");
  thread_local QPixmap trafficLink = buildPixmap(":resources/images/linkIcon.svg");
}

using namespace cmv::trf;

struct Signal::Stow
{
  Signal *parent;
  prm::Parameter *parameter;
  prm::Observer pObserver;
  QAction *unlinkAction = nullptr;
  
  Stow() = delete;
  Stow(Signal *parentIn, prm::Parameter *parameterIn)
  : parent(parentIn)
  , parameter(parameterIn)
  {
    assert(parent);
    
    if (parameter && parameter->isExpressionLinkable())
    {
      pObserver.constantHandler = std::bind(&Stow::constantChanged, this);
      pObserver.activeHandler = std::bind(&Stow::activeChanged, this);
      parameter->connect(pObserver);
      
      unlinkAction = new QAction(tr("unlink"), parent);
      QObject::connect(unlinkAction, &QAction::triggered, parent, &Signal::unlink);
      QTimer::singleShot(0, [this](){constantChanged();});
      QTimer::singleShot(0, [this](){activeChanged();});
    }
    else
      parent->setTrafficGreen();
  }
  
  void constantChanged()
  {
    assert(parameter);
    if (parameter->isConstant())
    {
      parent->setTrafficGreen(); //assumption
      parent->removeAction(unlinkAction);
    }
    else
    {
      parent->setTrafficLink();
      parent->addAction(unlinkAction);
    }
  }
  
  void activeChanged()
  {
    assert(parameter);
    if (parameter->isActive())
      parent->setEnabled(true);
    else
      parent->setDisabled(true);
  }
};

Signal::Signal(QWidget *parentIn, prm::Parameter *parameterIn)
: QLabel(parentIn)
, stow(std::make_unique<Stow>(this, parameterIn))
{
  assert(parentIn);
  setContentsMargins(0, 0, 0, 0);
  setToolTip(tr("Signal For Recognized Expression"));
  setWhatsThis(tr("Signal For Recognized Expression"));
  
  if (stow->parameter && stow->parameter->isExpressionLinkable())
  {
    setContextMenuPolicy(Qt::ActionsContextMenu);
    setAcceptDrops(true);
    setToolTip(tr("Drop Expression To Link. Right Click To Unlink"));
    setWhatsThis(tr("Drop Expression To Link. Right Click To Unlink"));
  }
}

Signal::~Signal() = default;

void Signal::setTrafficRed()
{
  setPixmap(trafficRed);
}

void Signal::setTrafficYellow()
{
  setPixmap(trafficYellow);
}

void Signal::setTrafficGreen()
{
  setPixmap(trafficGreen);
}

void Signal::setTrafficEmpty()
{
  setPixmap(QPixmap());
}

void Signal::setTrafficLink()
{
  assert(stow->parameter && stow->parameter->isExpressionLinkable());
  setPixmap(trafficLink);
}

void Signal::unlink()
{
  assert(stow->parameter && stow->parameter->isExpressionLinkable());
  app::instance()->getProject()->getManager().removeLink(stow->parameter->getId());
  app::instance()->messageSlot(msg::buildStatusMessage("Expression UnLinked"));
  prmConstantChanged();
  //unlinking doesn't change the value so we don't signal 'prmValueChanged'
}

void Signal::dragEnterEvent(QDragEnterEvent *event)
{
  if (event->mimeData()->hasText() && stow->parameter->isExpressionLinkable())
  {
    QString textIn = event->mimeData()->text();
    int id = getId(textIn);
    if (id >= 0 && app::instance()->getProject()->getManager().canLinkExpression(stow->parameter, id))
      event->acceptProposedAction();
  }
}

void Signal::dropEvent(QDropEvent *event)
{
  auto mo = [&](const std::string &s)
  {
    app::instance()->messageSlot(msg::buildStatusMessage(s, 2.0));
  };
  
  if (event->mimeData()->hasText() && stow->parameter->isExpressionLinkable())
  {
    QString textIn = event->mimeData()->text();
    int id = getId(textIn);
    
    event->acceptProposedAction();
    auto linkResults = app::instance()->getProject()->getManager().addLink(stow->parameter, id);
    switch (linkResults)
    {
      case expr::Amity::Incompatible:
        mo("Incompatible Types. Link Rejected");
        break;
      case expr::Amity::InvalidValue:
        mo("Invalid Value. Link Rejected");
        break;
      case expr::Amity::Equal:
        mo("Link Accepted");
        prmConstantChanged();
        break;
      case expr::Amity::Mutate:
        mo("Link Accepted");
        prmConstantChanged();
        prmValueChanged();
        break;
    }
  }
}
