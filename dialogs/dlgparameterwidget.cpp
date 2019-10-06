/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2019  Thomas S. Anderson blobfish.at.gmx.com
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

#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTimer>

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "expressions/exprmanager.h"
#include "expressions/exprstringtranslator.h"
#include "expressions/exprvalue.h"
#include "message/msgmessage.h"
#include "parameter/prmparameter.h"
#include "tools/idtools.h"
#include "dialogs/dlgenterfilter.h"
#include "dialogs/dlgexpressionedit.h"
#include "dialogs/dlgparameterwidget.h"

using namespace dlg;

ParameterWidget::ParameterWidget(QWidget *parent, const std::vector<prm::Parameter*> &psIn)
: QWidget(parent)
, parameters(psIn)
{
  assert(!parameters.empty()); //don't pass me an empty vector of parameters.
  buildGui();
}

ParameterWidget::~ParameterWidget() = default;

void ParameterWidget::buildGui()
{
  this->setContentsMargins(0, 0, 0, 0);
  QGridLayout *layout = new QGridLayout();
  
  int row = 0;
  int column = 0;
  for (auto *p : parameters)
  {
    auto *c = new ParameterContainer(this, p);
    containers.push_back(c);
    layout->addWidget(c->getLabel(), row, column++, Qt::AlignVCenter | Qt::AlignRight);
    layout->addWidget(c->getEditor(), row++, column--, Qt::AlignVCenter | Qt::AlignLeft);
  }
  QTimer::singleShot(0, containers.front()->getEditor(), SLOT(setFocus()));
  this->setLayout(layout);
}

void ParameterWidget::syncLinks()
{
  for (auto *c : containers)
    c->syncLink();
}

struct ParameterContainer::Stow
{
  prm::Parameter *parameter = nullptr;
  QLabel *label = nullptr;
  ExpressionEdit *editor = nullptr;
  boost::uuids::uuid linkId = gu::createNilId();
};

ParameterContainer::ParameterContainer(QWidget *parent, prm::Parameter *parameterIn)
: QObject(parent)
, stow(std::make_unique<Stow>())
{
  stow->parameter = parameterIn;
  stow->label = new QLabel(stow->parameter->getName(), parent);
  stow->editor = new ExpressionEdit(parent);
  
  ExpressionEditFilter *filter = new ExpressionEditFilter(parent);
  stow->editor->lineEdit->installEventFilter(filter);
  EnterFilter *ef = new EnterFilter(this);
  ef->setShouldClearFocus(true);
  stow->editor->lineEdit->installEventFilter(ef);
  QObject::connect(filter, SIGNAL(requestLinkSignal(QString)), this, SLOT(requestParameterLinkSlot(QString)));
  QObject::connect(stow->editor->lineEdit, SIGNAL(textEdited(QString)), this, SLOT(textEditedParameterSlot(QString)));
  QObject::connect(stow->editor->lineEdit, SIGNAL(editingFinished()), this, SLOT(updateParameterSlot()));
  QObject::connect(stow->editor->trafficLabel, SIGNAL(requestUnlinkSignal()), this, SLOT(requestParameterUnlinkSlot()));
  
  if (stow->parameter->isConstant())
    setEditUnlinked();
  else
  {
    expr::Manager &m = app::instance()->getProject()->getManager();
    stow->linkId = m.getFormulaLink(stow->parameter->getId());
    setEditLinked();
  }
}

ParameterContainer::~ParameterContainer() = default;

QLabel* ParameterContainer::getLabel()
{
  return stow->label;
}

ExpressionEdit* ParameterContainer::getEditor()
{
  return stow->editor;
}

void ParameterContainer::syncLink()
{
  expr::Manager &m = app::instance()->getProject()->getManager();
  if (stow->linkId.is_nil())
  {
    if (m.hasParameterLink(stow->parameter->getId()))
      m.removeParameterLink(stow->parameter->getId());
  }
  else
  {
    if (m.hasParameterLink(stow->parameter->getId()))
    {
      if (m.getFormulaLink(stow->parameter->getId()) == stow->linkId)
        return;
      m.removeParameterLink(stow->parameter->getId());
    }
    m.addLink(stow->parameter->getId(), stow->linkId);
  }
}

void ParameterContainer::requestParameterLinkSlot(const QString &stringIn)
{
  assert(stow->parameter);
  assert(stow->editor);
  
  boost::uuids::uuid eId = gu::stringToId(stringIn.toStdString());
  assert(!eId.is_nil()); //project asserts on presence of expression eId.
  prj::Project *project = app::instance()->getProject();
  double eValue = boost::get<double>(project->getManager().getFormulaValue(eId));
  
  if (stow->parameter->isValidValue(eValue))
  {
//     project->expressionLink(stow->parameter->getId(), eId);
    stow->linkId = eId;
    stow->parameter->setConstant(false);
    setEditLinked();
  }
  else
  {
    app::instance()->queuedMessage(msg::buildStatusMessage(QObject::tr("Value out of range").toStdString(), 2.0));
  }
  
  stow->label->activateWindow();
}

void ParameterContainer::requestParameterUnlinkSlot()
{
  assert(stow->parameter);
  assert(stow->editor);
//   app::instance()->getProject()->expressionUnlink(stow->parameter->getId());
  stow->linkId = gu::createNilId();
  stow->parameter->setConstant(true);
  setEditUnlinked();
}

void ParameterContainer::setEditLinked()
{
  assert(stow->parameter);
  assert(stow->editor);
  
  stow->editor->trafficLabel->setLinkSlot();
  stow->editor->clearFocus();
  stow->editor->lineEdit->deselect();
  stow->editor->lineEdit->setReadOnly(true);
  
  expr::Manager &manager = app::instance()->getProject()->getManager();
  stow->editor->lineEdit->setText(QString::fromStdString(manager.getFormulaName(stow->linkId)));
}

void ParameterContainer::setEditUnlinked()
{
  assert(stow->parameter);
  assert(stow->editor);
  
  stow->editor->trafficLabel->setTrafficGreenSlot();
  stow->editor->lineEdit->setReadOnly(false);
  stow->editor->lineEdit->setText(QString::number(static_cast<double>(*stow->parameter), 'f', 12));
  stow->editor->lineEdit->selectAll();
  stow->editor->setFocus();
}

void ParameterContainer::updateParameterSlot()
{
  assert(stow->parameter);
  assert(stow->editor);
  if (!stow->parameter->isConstant())
    return;

  expr::Manager localManager;
  expr::StringTranslator translator(localManager);
  std::string formula("temp = ");
  formula += stow->editor->lineEdit->text().toStdString();
  if (translator.parseString(formula) == expr::StringTranslator::ParseSucceeded)
  {
    localManager.update();
    assert(localManager.getFormulaValueType(translator.getFormulaOutId()) == expr::ValueType::Scalar);
    double value = boost::get<double>(localManager.getFormulaValue(translator.getFormulaOutId()));
    if (stow->parameter->isValidValue(value))
    {
      stow->parameter->setValue(value);
    }
    else
    {
      app::instance()->queuedMessage(msg::buildStatusMessage(QObject::tr("Value out of range").toStdString(), 2.0));
    }
  }
  else
  {
    app::instance()->queuedMessage(msg::buildStatusMessage(QObject::tr("Parsing failed").toStdString(), 2.0));
  }
  stow->editor->lineEdit->setText(QString::number(static_cast<double>(*stow->parameter), 'f', 12));
  stow->editor->trafficLabel->setTrafficGreenSlot();
}

void ParameterContainer::textEditedParameterSlot(const QString &textIn)
{
  assert(stow->editor);
  
  stow->editor->trafficLabel->setTrafficYellowSlot();
  qApp->processEvents(); //need this or we never see yellow signal.
  
  expr::Manager localManager;
  expr::StringTranslator translator(localManager);
  std::string formula("temp = ");
  formula += textIn.toStdString();
  if (translator.parseString(formula) == expr::StringTranslator::ParseSucceeded)
  {
    localManager.update();
    stow->editor->trafficLabel->setTrafficGreenSlot();
    assert(localManager.getFormulaValueType(translator.getFormulaOutId()) == expr::ValueType::Scalar);
    double value = boost::get<double>(localManager.getFormulaValue(translator.getFormulaOutId()));
    stow->editor->goToolTipSlot(QString::number(value));
  }
  else
  {
    stow->editor->trafficLabel->setTrafficRedSlot();
    int position = translator.getFailedPosition() - 8; // 7 chars for 'temp = ' + 1
    stow->editor->goToolTipSlot(textIn.left(position) + "?");
  }
}
