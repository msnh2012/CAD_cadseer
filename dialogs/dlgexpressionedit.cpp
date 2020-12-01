/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#include <boost/uuid/uuid.hpp>

#include <QCoreApplication>
#include <QResizeEvent>
#include <QPainter>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QAction>
#include <QMimeData>
#include <QTimer>

#include <lccmanager.h>

#include "tools/idtools.h"
#include "tools/tlsstring.h"
#include "dialogs/dlgexpressionedit.h"

using namespace dlg;

TrafficLabel::TrafficLabel(QWidget* parentIn) : QLabel(parentIn)
{
  setContentsMargins(0, 0, 0, 0);
  
  setContextMenuPolicy(Qt::ActionsContextMenu);
  unlinkAction = new QAction(tr("unlink"), this);
  connect(unlinkAction, SIGNAL(triggered()), this, SIGNAL(requestUnlinkSignal()));
  
  updatePixmaps();
}

void TrafficLabel::updatePixmaps()
{
  assert(iconSize > 0);
  trafficRed = buildPixmap(":/resources/images/trafficRed.svg");
  trafficYellow = buildPixmap(":/resources/images/trafficYellow.svg");
  trafficGreen = buildPixmap(":/resources/images/trafficGreen.svg");
  link = buildPixmap(":resources/images/linkIcon.svg");
}

QPixmap TrafficLabel::buildPixmap(const QString &resourceNameIn)
{
  QPixmap temp = QPixmap(resourceNameIn).scaled(iconSize, iconSize, Qt::KeepAspectRatio);
  QPixmap out(iconSize, iconSize);
  QPainter painter(&out);
  painter.fillRect(out.rect(), this->palette().color(QPalette::Window));
  painter.drawPixmap(out.rect(), temp, temp.rect());
  painter.end();
  
  return out;
}

void TrafficLabel::setTrafficRedSlot()
{
  setPixmap(trafficRed);
  this->removeAction(unlinkAction);
  current = 1;
}

void TrafficLabel::setTrafficYellowSlot()
{
  setPixmap(trafficYellow);
  this->removeAction(unlinkAction);
  current = 2;
}

void TrafficLabel::setTrafficGreenSlot()
{
  setPixmap(trafficGreen);
  this->removeAction(unlinkAction);
  current = 3;
}

void TrafficLabel::setLinkSlot()
{
  setPixmap(link);
  this->addAction(unlinkAction);
  current = 4;
}

ExpressionEdit::ExpressionEdit(QWidget* parent, Qt::WindowFlags f) :
  QWidget(parent, f)
{
  setupGui();
}

void ExpressionEdit::setupGui()
{
  this->setContentsMargins(0, 0, 0, 0);
  QHBoxLayout *layout = new QHBoxLayout();
  layout->setSpacing(0);
  layout->setContentsMargins(0, 0, 0, 0);
  
  lineEdit = new QLineEdit(this);
  layout->addWidget(lineEdit);
  
  trafficLabel = new TrafficLabel(this);
  trafficLabel->setMinimumSize(1, 1);
  trafficLabel->setScaledContents(true);
  trafficLabel->setFixedSize(lineEdit->height(), lineEdit->height());
  layout->addWidget(trafficLabel);
  
  this->setLayout(layout);
  this->setFocusProxy(lineEdit);
  
  setAcceptDrops(false); //the edit line accepts drops by default.
  lineEdit->setWhatsThis(tr("Constant value for editing or name of linked expression"));
  trafficLabel->setWhatsThis(tr("Status of parse or linked icon. When linked, right click to unlink"));
}

void ExpressionEdit::goToolTipSlot(const QString &tipIn)
{
  lineEdit->setToolTip(tipIn);
  
  QPoint point(0.0, -1.5 * (lineEdit->frameGeometry().height()));
  QHelpEvent *toolTipEvent = new QHelpEvent(QEvent::ToolTip, point, lineEdit->mapToGlobal(point));
  qApp->postEvent(lineEdit, toolTipEvent);
}

bool ExpressionEditFilter::eventFilter(QObject *obj, QEvent *event)
{
  auto getId = [](const QString &stringIn)
  {
    int idOut = -1;
    if (stringIn.startsWith("ExpressionId;"))
    {
      QStringList split = stringIn.split(";");
      if (split.size() == 2)
        idOut = split.at(1).toInt();
    }
    return idOut;
  };
  
  if (event->type() == QEvent::DragEnter)
  {
    QDragEnterEvent *dEvent = dynamic_cast<QDragEnterEvent*>(event);
    assert(dEvent);
    
    if (dEvent->mimeData()->hasText())
    {
      QString textIn = dEvent->mimeData()->text();
      int id = getId(textIn);
      if (id >= 0)
        dEvent->acceptProposedAction();
    }
    return true;
  }
  else if (event->type() == QEvent::Drop)
  {
    QDropEvent *dEvent = dynamic_cast<QDropEvent*>(event);
    assert(dEvent);
    
    if (dEvent->mimeData()->hasText())
    {
      QString textIn = dEvent->mimeData()->text();
      int id = getId(textIn);
      if (id >= 0)
      {
        dEvent->acceptProposedAction();
        Q_EMIT requestLinkSignal(QString::number(id));
      }
    }
    
    return true;
  }
  else
    return QObject::eventFilter(obj, event);
}

ExpressionDelegate::ExpressionDelegate(QObject *parent): QStyledItemDelegate(parent){}

QWidget* ExpressionDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex&) const
{
  eEditor = new dlg::ExpressionEdit(parent);
  return eEditor;
}

void ExpressionDelegate::initEditor() const
{
  if (isExpressionLinked)
  {
    QTimer::singleShot(0, eEditor->trafficLabel, SLOT(setLinkSlot()));
    eEditor->lineEdit->setReadOnly(true);
  }
  else
  {
    QTimer::singleShot(0, eEditor->trafficLabel, SLOT(setTrafficGreenSlot()));
    eEditor->lineEdit->setReadOnly(false);
  }
  
  connect (eEditor->lineEdit, SIGNAL(textEdited(QString)), this, SLOT(textEditedSlot(QString)));
  connect (eEditor->trafficLabel, SIGNAL(requestUnlinkSignal()), this, SLOT(requestUnlinkSlot()));
}

void ExpressionDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex&) const
{
  //this is called before setEditorData.
  editor->setGeometry(option.rect);
}

void ExpressionDelegate::textEditedSlot(const QString &textIn)
{
  assert(eEditor);
  eEditor->trafficLabel->setTrafficYellowSlot();
  qApp->processEvents(); //need this or we never see yellow signal.
  
  lcc::Manager eman;
  std::string formula("temp = ");
  formula += textIn.toStdString();
  auto result = eman.parseString(formula);
  if (result.isAllGood())
  {
    if (result.value.size() == 1)
    {
      eEditor->trafficLabel->setTrafficGreenSlot();
      double value = result.value.at(1);
      eEditor->goToolTipSlot(QString::number(value));
    }
    else
    {
      eEditor->trafficLabel->setTrafficRedSlot();
      eEditor->goToolTipSlot("Wrong Type");
    }
  }
  else
  {
    eEditor->trafficLabel->setTrafficRedSlot();
    int position = result.errorPosition - 8; // 7 chars for 'temp = ' + 1
    eEditor->goToolTipSlot(textIn.left(position) + "?");
  }
}

void ExpressionDelegate::requestUnlinkSlot()
{
  assert(isExpressionLinked); //shouldn't be able to get here if expression is not linked.
  
  QKeyEvent *event = new QKeyEvent (QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
  qApp->postEvent (eEditor, event);
  QTimer::singleShot(0, this, &ExpressionDelegate::requestUnlinkSignal);
}
