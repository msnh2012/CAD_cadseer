/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2021 Thomas S. Anderson blobfish.at.gmx.com
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

#include <sstream>

#include <QSettings>
#include <QLineEdit>
#include <QHelpEvent>
#include <QHBoxLayout>
#include <QTimer>

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "message/msgmessage.h"
#include "parameter/prmparameter.h"
#include "expressions/exprmanager.h"
#include "commandview/cmvtrafficsignal.h"
#include "commandview/cmvexpressionedit.h"

using boost::uuids::uuid;

using namespace cmv;

void BaseEdit::buildGui()
{
  assert(lineEdit);
  assert(trafficSignal);
  
  setContentsMargins(0, 0, 0, 0);
  QHBoxLayout *layout = new QHBoxLayout();
  layout->setSpacing(0);
  layout->setContentsMargins(0, 0, 0, 0);
  setLayout(layout);
  
  lineEdit->setAcceptDrops(false); //the edit line accepts drops by default.
  lineEdit->setWhatsThis(tr("Value Or Name Of Linked Expression"));
  layout->addWidget(lineEdit);
  setFocusProxy(lineEdit);
  
  trafficSignal->setMinimumSize(1, 1);
  trafficSignal->setScaledContents(true);
  trafficSignal->setFixedSize(lineEdit->height(), lineEdit->height());
  layout->addWidget(trafficSignal);
}

void BaseEdit::goToolTip(const QString &tipIn)
{
  assert(lineEdit);
  lineEdit->setToolTip(tipIn);
  
  QPoint point(0.0, -1.5 * (lineEdit->frameGeometry().height()));
  QHelpEvent *toolTipEvent = new QHelpEvent(QEvent::ToolTip, point, lineEdit->mapToGlobal(point));
  qApp->postEvent(lineEdit, toolTipEvent);
}

struct ParameterEdit::Stow
{
  ParameterEdit *parent;
  prm::Parameter *parameter;
  prm::Observer pObserver;
  
  Stow(ParameterEdit *parentIn, prm::Parameter *parameterIn)
  : parent(parentIn)
  , parameter(parameterIn)
  , pObserver
  (
    std::bind(&Stow::valueChanged, this)
    , std::bind(&Stow::constantChanged, this)
    , std::bind(&Stow::activeChanged, this)
  )
  {
    parameter->connect(pObserver);
    QTimer::singleShot(0, [this](){constantChanged();});
    QTimer::singleShot(0, [this](){activeChanged();});
  }
  
  void valueChanged()
  {
    if (parameter->isConstant())
    {
      parent->lineEdit->setText(parameter->adaptToQString());
      parent->lineEdit->selectAll();
    }
  }
  
  void constantChanged()
  {
    if (parameter->isConstant())
    {
      parent->lineEdit->setReadOnly(false);
      parent->lineEdit->setText(parameter->adaptToQString());
      parent->lineEdit->selectAll();
    }
    else
    {
      parent->lineEdit->setReadOnly(true);
      parent->lineEdit->clearFocus();
      parent->lineEdit->deselect();

      expr::Manager &manager = app::instance()->getProject()->getManager();
      auto oLinked = manager.getLinked(parameter->getId());
      assert(oLinked);
      auto oName = manager.getExpressionName(*oLinked);
      assert(oName);
      parent->lineEdit->setText(QString::fromStdString(*oName));
    }
  }
  
  void activeChanged()
  {
    if (parameter->isActive())
    {
      parent->lineEdit->setEnabled(true);
    }
    else
    {
      parent->lineEdit->deselect();
      parent->lineEdit->setDisabled(true);
    }
  }
};

ParameterEdit::ParameterEdit(QWidget *parentIn, prm::Parameter *parameterIn)
: BaseEdit(parentIn)
, stow(std::make_unique<Stow>(this, parameterIn))
{
  assert(parameterIn); //Don't set me up.
  lineEdit = new QLineEdit(this);
  trafficSignal = new trf::Signal(this, stow->parameter);
  buildGui();
  connect(trafficSignal, &trf::Signal::prmValueChanged, this, &ParameterEdit::prmValueChanged);
  connect(trafficSignal, &trf::Signal::prmConstantChanged, this, &ParameterEdit::prmConstantChanged);
  connect(lineEdit, &QLineEdit::textEdited, this, &ParameterEdit::editing);
  connect(lineEdit, &QLineEdit::editingFinished, this, &ParameterEdit::finishedEditing);
}

ParameterEdit::~ParameterEdit() = default;

void ParameterEdit::editing(const QString &textIn)
{
  assert(stow->parameter);
  trafficSignal->setTrafficYellow();
  qApp->processEvents(); //need this or we never see yellow signal.
  
  expr::Manager localManager;
  std::string formula("temp = ");
  formula += textIn.toStdString();
  auto result = localManager.parseString(formula);
  if (result.isAllGood())
  {
    auto oId = localManager.getExpressionId(result.expressionName);
    assert(oId);
    if (localManager.canLinkExpression(stow->parameter, *oId))
    {
      trafficSignal->setTrafficGreen();
      std::ostringstream stream;
      stream << localManager.getExpressionValue(*oId);
      goToolTip(QString::fromStdString(stream.str()));
    }
    else
    {
      trafficSignal->setTrafficRed();
      goToolTip(tr("Invalid Type Or Value"));
    }
  }
  else
  {
    trafficSignal->setTrafficRed();
    int position = result.errorPosition - 8; // 7 chars for 'temp = ' + 1
    goToolTip(textIn.left(position) + "?");
  }
}

void ParameterEdit::finishedEditing()
{
  //We are leaving failed text in edit box. Is that what the user wants?
  assert(stow->parameter);
  if (!stow->parameter->isConstant())
    return;
  
  auto mo = [&](const std::string &s)
  {
    app::instance()->queuedMessage(msg::buildStatusMessage(s, 2.0));
  };

  expr::Manager localManager;
  std::string formula("temp = ");
  formula += lineEdit->text().toStdString();
  auto result = localManager.parseString(formula);
  if (result.isAllGood())
  {
    auto oId = localManager.getExpressionId(result.expressionName);
    assert(oId);
    auto result = localManager.assignParameter(stow->parameter, *oId);
    switch (result)
    {
      case expr::Amity::Incompatible:
        mo("Incompatible Types");
        trafficSignal->setTrafficRed();
        break;
      case expr::Amity::InvalidValue:
        mo("Invalid Value");
        trafficSignal->setTrafficRed();
        break;
      case expr::Amity::Equal:
        mo("Same Value");
        trafficSignal->setTrafficGreen();
        break;
      case expr::Amity::Mutate:
        mo("Value Changed");
        prmValueChanged();
        trafficSignal->setTrafficGreen();
        break;
    }
  }
  else
  {
    mo("Parsing failed");
    trafficSignal->setTrafficRed();
  }
}

ExpressionEdit::ExpressionEdit(QWidget *parentIn)
: BaseEdit(parentIn)
{
  lineEdit = new QLineEdit(this);
  trafficSignal = new trf::Signal(this);
  buildGui();
  connect(lineEdit, &QLineEdit::textEdited, this, &ExpressionEdit::editing);
}

ExpressionEdit::~ExpressionEdit() = default;

void ExpressionEdit::parseSucceeded(const QString &tIn)
{
  goToolTip(tIn);
  trafficSignal->setTrafficGreen();
}

void ExpressionEdit::parseFailed(const QString &tIn)
{
  goToolTip(tIn);
  trafficSignal->setTrafficRed();
}

void ExpressionEdit::setText(const QString &tIn)
{
  lineEdit->setText(tIn);
}

QString ExpressionEdit::getText()
{
  return lineEdit->text();
}

void ExpressionEdit::setTrafficGreen()
{
  trafficSignal->setTrafficGreen();
}

void ExpressionEdit::setTrafficYellow()
{
  trafficSignal->setTrafficYellow();
}

void ExpressionEdit::setTrafficRed()
{
  trafficSignal->setTrafficRed();
}
