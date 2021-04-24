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

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTimer>
#include <QKeyEvent>

#include "application/appapplication.h"
#include "tools/tlsstring.h"
#include "commandview/cmvexpressionedit.h"
#include "parameter/prmparameter.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "application/appincrementwidget.h"

using namespace app;

struct IncrementWidget::Stow
{
  cmv::ParameterEdit *lineEdit = nullptr;
  QString title;
  double &prefRef;
  prm::Parameter parameter;
  prm::Observer observer;
  
  void valueChanged()
  {
    prefRef = parameter.getDouble();
    prf::manager().saveConfig();
  }
  
  Stow(const QString &titleIn, double &prefRefIn)
  : title(titleIn)
  , prefRef(prefRefIn)
  , parameter(QObject::tr("dummy"), prefRefIn)
  , observer(std::bind(&Stow::valueChanged, this))
  {
    parameter.setExpressionLinkable(false);
    parameter.connect(observer);
  }
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
  stow->lineEdit = new cmv::ParameterEdit(this, &stow->parameter);
  layout->addWidget(stow->lineEdit);
  layout->setContentsMargins(0, 0, 0, 0);
  this->setLayout(layout);
//   HighlightOnFocusFilter *filter = new HighlightOnFocusFilter(stow->lineEdit->lineEdit);
//   stow->lineEdit->lineEdit->installEventFilter(filter);
}

void IncrementWidget::externalUpdate()
{
  stow->parameter.setValue(stow->prefRef);
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
