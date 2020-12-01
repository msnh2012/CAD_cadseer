/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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

#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QFontMetrics>
#include <QTimer>
#include <QKeyEvent>

#include <lccmanager.h>

#include "application/appapplication.h"
#include "message/msgmessage.h"
#include "tools/tlsstring.h"
#include "dialogs/dlgexpressionedit.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "application/appincrementwidget.h"

using namespace app;

struct IncrementWidget::Stow
{
  dlg::ExpressionEdit *lineEdit = nullptr;
  QString title;
  double &prefRef;
  lcc::Manager eman;
  
  Stow(const QString &titleIn, double &prefRefIn)
  : title(titleIn)
  , prefRef(prefRefIn)
  {}
};

IncrementWidget::IncrementWidget(QWidget *parent, const QString &titleIn, double &prefRefIn)
: QWidget(parent)
, stow(std::make_unique<Stow>(titleIn, prefRefIn))
{
  buildGui();
}

IncrementWidget::~IncrementWidget() = default;

void IncrementWidget::buildGui()
{
  auto *layout = new QHBoxLayout();
  layout->setSpacing(0);
  QLabel *label = new QLabel(stow->title, this);
  layout->addWidget(label);
  stow->lineEdit = new dlg::ExpressionEdit(this);
  layout->addWidget(stow->lineEdit);
  layout->setContentsMargins(0, 0, 0, 0);
  this->setLayout(layout);
  update();
  connect(stow->lineEdit->lineEdit, SIGNAL(textEdited(QString)), this, SLOT(textEditedSlot(QString)));
  connect(stow->lineEdit->lineEdit, SIGNAL(editingFinished()), this, SLOT(editingFinishedSlot()));
  connect(stow->lineEdit->lineEdit, SIGNAL(returnPressed()), this, SLOT(returnPressedSlot()));
  HighlightOnFocusFilter *filter = new HighlightOnFocusFilter(stow->lineEdit->lineEdit);
  stow->lineEdit->lineEdit->installEventFilter(filter);
}

void IncrementWidget::update()
{
  stow->lineEdit->lineEdit->setText(QString::fromStdString(tls::prettyDouble(stow->prefRef)));
  stow->lineEdit->lineEdit->setCursorPosition(0);
}

void IncrementWidget::textEditedSlot(const QString &textIn)
{
  assert(stow->lineEdit);
  assert(stow->lineEdit->trafficLabel);
  stow->lineEdit->trafficLabel->setTrafficYellowSlot();
  qApp->processEvents(); //need this or we never see yellow signal.
  
  std::string formula("temp = ");
  formula += textIn.toStdString();
  stow->eman.reset();
  auto result = stow->eman.parseString(formula);
  if (result.isAllGood())
  {
    if (result.value.size() == 1)
    {
      stow->lineEdit->trafficLabel->setTrafficGreenSlot();
      stow->lineEdit->goToolTipSlot(QString::fromStdString(tls::prettyDouble(result.value.at(0))));
    }
    else
    {
      stow->lineEdit->trafficLabel->setTrafficRedSlot();
      stow->lineEdit->goToolTipSlot("Error: Wrong Type");
    }
  }
  else
  {
    stow->lineEdit->trafficLabel->setTrafficRedSlot();
    int position = result.errorPosition - 8; // 7 chars for 'temp = ' + 1
    stow->lineEdit->goToolTipSlot(textIn.left(position) + "?");
  }
}

void IncrementWidget::returnPressedSlot()
{
  stow->lineEdit->lineEdit->setCursorPosition(0);
  QTimer::singleShot(0, stow->lineEdit->lineEdit, SLOT(selectAll()));
}

void IncrementWidget::editingFinishedSlot()
{
  stow->lineEdit->trafficLabel->setPixmap(QPixmap());
  stow->lineEdit->lineEdit->setToolTip(this->toolTip());
  
  auto error = [&](const std::string &e)
  {
    app::instance()->messageSlot(msg::buildStatusMessage(e, 2.0));
  };
  
  std::string formula("temp = ");
  formula += stow->lineEdit->lineEdit->text().toStdString();
  stow->eman.reset();
  auto result = stow->eman.parseString(formula);
  if (result.isAllGood())
  {
    if (result.value.size() == 1)
    {
      stow->prefRef = result.value.at(0);
      prf::manager().saveConfig();
    }
    else
      error("Error: Wrong Type");
  }
  else
    error(result.parseErrorMessage);
  
  update();
}

bool HighlightOnFocusFilter::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::FocusIn)
  {
    QLineEdit *lineEdit = qobject_cast<QLineEdit*>(obj);
    assert(lineEdit);
    QTimer::singleShot(0, lineEdit, SLOT(selectAll()));
  }
  else if (event->type() == QEvent::KeyPress)
  {
    QKeyEvent *kEvent = dynamic_cast<QKeyEvent*>(event);
    assert(kEvent);
    if (kEvent->key() == Qt::Key_Escape)
    {
      QLineEdit *lineEdit = qobject_cast<QLineEdit*>(obj);
      assert(lineEdit);
      //here we will just set the text to invalid and
      //send a return event. this should restore the value
      //to what it was prior to editing.
      lineEdit->setText("?");
      QKeyEvent *freshEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
      qApp->postEvent(lineEdit, freshEvent);
    }
  }
  
  return QObject::eventFilter(obj, event);
}
