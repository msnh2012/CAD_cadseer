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

#include <boost/filesystem.hpp>
#include <boost/optional/optional.hpp>

#include <QSettings>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QHelpEvent>
#include <QComboBox>
#include <QPushButton>
#include <QFileDialog>

#include "application/appapplication.h"
#include "application/appmainwindow.h"
#include "project/prjproject.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "message/msgmessage.h"
#include "parameter/prmvariant.h"
#include "parameter/prmparameter.h"
#include "parameter/prmexpressionlink.h"
#include "expressions/exprmanager.h"
#include "expressions/exprstringtranslator.h"
#include "tools/idtools.h"
#include "commandview/cmvcsyswidget.h"
#include "commandview/cmvtrafficsignal.h"
#include "commandview/cmvparameterwidgets.h"

using boost::uuids::uuid;

using namespace cmv;

//this always assigns to parameter. Make a temp parameter if needed.
class ParameterVisitor : public boost::static_visitor<bool>
{
  prm::Parameter *parameter = nullptr;
  expr::Value ev;
public:
  ParameterVisitor(prm::Parameter *pIn, const expr::Value& evIn)
  : parameter(pIn)
  , ev(evIn)
  {}
  
  bool operator()(double) const
  {
    expr::DoubleVisitor edv;
    double newValue = boost::apply_visitor(edv, ev);
    if (!parameter->isValidValue(newValue))
      return false;
    parameter->setValue(newValue);
    return true;
  }
  
  bool operator()(int) const
  {
    expr::DoubleVisitor edv;
    int newValue = static_cast<int>(boost::apply_visitor(edv, ev));
    if (!parameter->isValidValue(newValue))
      return false;
    parameter->setValue(newValue);
    return true;
  }
  bool operator()(bool) const {assert(0); return false;} //currently unsupported formula type.
  bool operator()(const std::string&) const {assert(0); return false;} //currently unsupported formula type.
  bool operator()(const boost::filesystem::path&) const {assert(0); return false;} //currently unsupported formula type.
  bool operator()(const osg::Vec3d&) const
  {
    expr::VectorVisitor evv;
    osg::Vec3d newValue = boost::apply_visitor(evv, ev);
    parameter->setValue(newValue);
    return true;
  }
  bool operator()(const osg::Quat&) const
  {
    //parameters not supporting Quaternions at this time.
    return false;
    
//       expr::QuatVisitor eqv;
//       osg::Quat newValue = boost::apply_visitor(eqv, ev);
//       parameter->setValue(newValue);
//       return true;
  }
  bool operator()(const osg::Matrixd&) const
  {
    expr::MatrixVisitor emv;
    osg::Matrixd newValue = boost::apply_visitor(emv, ev);
    parameter->setValue(newValue);
    return true;
  }
};

struct cmv::ExpressionEdit::Stow
{
  ExpressionEdit *parentWidget;
  prm::Parameter *parameter;
  prm::Observer pObserver;
  QLineEdit *lineEdit;
  TrafficSignal *trafficSignal;
  
  Stow() = delete;
  Stow(ExpressionEdit *pwIn, prm::Parameter *pIn)
  : parentWidget(pwIn)
  , parameter(pIn)
  , pObserver(std::bind(&Stow::valueChanged, this), std::bind(&Stow::constantChanged, this))
  {
    buildGui();
    //no need to call valueChanged. Constant will do it all
    constantChanged();
    parameter->connect(pObserver);
    QObject::connect(lineEdit, &QLineEdit::textEdited, parentWidget, &ExpressionEdit::editingSlot);
    QObject::connect(lineEdit, &QLineEdit::editingFinished, parentWidget, &ExpressionEdit::finishedEditingSlot);
  }
  
  void valueChanged()
  {
    if (parameter->isConstant())
    {
      lineEdit->setText(static_cast<QString>(*parameter));
      lineEdit->selectAll();
    }
  }
  
  void constantChanged()
  {
    if (parameter->isConstant())
    {
      lineEdit->setReadOnly(false);
      lineEdit->setText(static_cast<QString>(*parameter));
      lineEdit->selectAll();
    }
    else
    {
      lineEdit->setReadOnly(true);
      lineEdit->clearFocus();
      lineEdit->deselect();

      expr::Manager &manager = app::instance()->getProject()->getManager();
      assert(manager.hasParameterLink(parameter->getId()));
      lineEdit->setText(QString::fromStdString(manager.getFormulaName(manager.getFormulaLink(parameter->getId()))));
    }
  }
  
  void buildGui()
  {
    parentWidget->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout *layout = new QHBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    parentWidget->setLayout(layout);
    
    lineEdit = new QLineEdit(parentWidget);
    lineEdit->setAcceptDrops(false); //the edit line accepts drops by default.
    lineEdit->setWhatsThis(tr("Value Or Name Of Linked Expression"));
    layout->addWidget(lineEdit);
    parentWidget->setFocusProxy(lineEdit);
    
    trafficSignal = new TrafficSignal(parentWidget, parameter);
    trafficSignal->setMinimumSize(1, 1);
    trafficSignal->setScaledContents(true);
    trafficSignal->setFixedSize(lineEdit->height(), lineEdit->height());
    layout->addWidget(trafficSignal);
  }
};

ExpressionEdit::ExpressionEdit(QWidget *parent, prm::Parameter *pIn)
: QWidget(parent)
, stow(std::make_unique<Stow>(this, pIn))
{}

ExpressionEdit::~ExpressionEdit() = default;

void ExpressionEdit::goToolTipSlot(const QString &tipIn)
{
  stow->lineEdit->setToolTip(tipIn);
  
  QPoint point(0.0, -1.5 * (stow->lineEdit->frameGeometry().height()));
  QHelpEvent *toolTipEvent = new QHelpEvent(QEvent::ToolTip, point, stow->lineEdit->mapToGlobal(point));
  qApp->postEvent(stow->lineEdit, toolTipEvent);
}

void ExpressionEdit::editingSlot(const QString &textIn)
{
  assert(stow->parameter);
  stow->trafficSignal->setTrafficYellowSlot();
  qApp->processEvents(); //need this or we never see yellow signal.
  
  expr::Manager localManager;
  expr::StringTranslator translator(localManager);
  std::string formula("temp = ");
  formula += textIn.toStdString();
  if (translator.parseString(formula) == expr::StringTranslator::ParseSucceeded)
  {
    localManager.update();
    if (prm::canLinkExpression(localManager, stow->parameter, translator.getFormulaOutId()))
    {
      //going to make a temp parameter and assign the expression value.
      //this is so we can cast the parameter to a qstring.
      //random id just in case.
      prm::Parameter tp(*stow->parameter, gu::createRandomId());
      ParameterVisitor v(&tp, localManager.getFormulaValue(translator.getFormulaOutId()));
      if (boost::apply_visitor(v, stow->parameter->getStow().variant))
      {
        stow->trafficSignal->setTrafficGreenSlot();
        goToolTipSlot(static_cast<QString>(tp));
      }
      else
      {
        stow->trafficSignal->setTrafficRedSlot();
        goToolTipSlot(tr("Assignment failed"));
      }
    }
    else
    {
      stow->trafficSignal->setTrafficRedSlot();
      goToolTipSlot(tr("Type Mismatch"));
    }
  }
  else
  {
    stow->trafficSignal->setTrafficRedSlot();
    int position = translator.getFailedPosition() - 8; // 7 chars for 'temp = ' + 1
    goToolTipSlot(textIn.left(position) + "?");
  }
}

void ExpressionEdit::finishedEditingSlot()
{
  assert(stow->parameter);
  if (!stow->parameter->isConstant())
    return;

  expr::Manager localManager;
  expr::StringTranslator translator(localManager);
  std::string formula("temp = ");
  formula += stow->lineEdit->text().toStdString();
  if (translator.parseString(formula) == expr::StringTranslator::ParseSucceeded)
  {
    localManager.update();
    if (prm::canLinkExpression(localManager, stow->parameter, translator.getFormulaOutId()))
    {
      ParameterVisitor v(stow->parameter, localManager.getFormulaValue(translator.getFormulaOutId()));
      if (!boost::apply_visitor(v, stow->parameter->getStow().variant))
      {
        app::instance()->queuedMessage(msg::buildStatusMessage(QObject::tr("Value out of range").toStdString(), 2.0));
        stow->trafficSignal->setTrafficRedSlot();
      }
    }
    else
    {
      app::instance()->queuedMessage(msg::buildStatusMessage(QObject::tr("Type Mismatch").toStdString(), 2.0));
      stow->trafficSignal->setTrafficRedSlot();
    }
  }
  else
  {
    app::instance()->queuedMessage(msg::buildStatusMessage(QObject::tr("Parsing failed").toStdString(), 2.0));
    stow->trafficSignal->setTrafficRedSlot();
  }
//   stow->editor->lineEdit->setText(static_cast<QString>(*stow->parameter));
  stow->trafficSignal->setTrafficGreenSlot();
}

struct cmv::BoolCombo::Stow
{
  BoolCombo *parentWidget;
  prm::Parameter *parameter;
  prm::Observer pObserver;
  QComboBox *comboBox;
  TrafficSignal *trafficSignal;
  
  Stow() = delete;
  Stow(BoolCombo *pwIn, prm::Parameter *pIn)
  : parentWidget(pwIn)
  , parameter(pIn)
  , pObserver(std::bind(&Stow::valueChanged, this), std::bind(&Stow::constantChanged, this))
  {
    assert(parameter->getValueType() == typeid(bool));
    buildGui();
    //no need to call valueChanged. Constant will do it all
    constantChanged();
    parameter->connect(pObserver);
    QObject::connect(comboBox, SIGNAL(currentIndexChanged(int)), parentWidget, SLOT(currentChangedSlot(int)));
  }
  
  void buildGui()
  {
    parentWidget->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout *layout = new QHBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    parentWidget->setLayout(layout);
    
    comboBox = new QComboBox(parentWidget);
    comboBox->setWhatsThis(tr("Value Of Parameter"));
    comboBox->addItem(tr("True"));
    comboBox->addItem(tr("False"));
    layout->addWidget(comboBox);
    
    trafficSignal = new TrafficSignal(parentWidget, parameter);
    trafficSignal->setMinimumSize(1, 1);
    trafficSignal->setScaledContents(true);
    trafficSignal->setFixedSize(comboBox->height(), comboBox->height());
    layout->addWidget(trafficSignal);
  }
  void valueChanged()
  {
    bool value = static_cast<bool>(*parameter);
    if (value)
      comboBox->setCurrentIndex(0);
    else
      comboBox->setCurrentIndex(1);
  }
  
  void constantChanged()
  {
    if (parameter->isConstant())
    {
      comboBox->setEnabled(true);
    }
    else
    {
      comboBox->setDisabled(true);
    }
    valueChanged();
  }
};

BoolCombo::BoolCombo(QWidget *parent, prm::Parameter *pIn)
: QWidget(parent)
, stow(std::make_unique<Stow>(this, pIn))
{}

BoolCombo::~BoolCombo() = default;

void BoolCombo::currentChangedSlot(int index)
{
  bool newValue = (index == 0) ? true : false;
  assert(stow->parameter->isValidValue(newValue));
  prm::ObserverBlocker blocker(stow->pObserver);
  stow->parameter->setValue(newValue);
}

struct cmv::EnumCombo::Stow
{
  EnumCombo *parentWidget;
  prm::Parameter *parameter;
  prm::Observer pObserver;
  QComboBox *comboBox;
  TrafficSignal *trafficSignal;
  
  Stow() = delete;
  Stow(EnumCombo *pwIn, prm::Parameter *pIn)
  : parentWidget(pwIn)
  , parameter(pIn)
  , pObserver(std::bind(&Stow::valueChanged, this), std::bind(&Stow::constantChanged, this))
  {
    assert(parameter->getValueType() == typeid(int));
    assert(parameter->isEnumeration());
    buildGui();
    //no need to call valueChanged. Constant will do it all
    constantChanged();
    parameter->connect(pObserver);
    QObject::connect(comboBox, SIGNAL(currentIndexChanged(int)), parentWidget, SLOT(currentChangedSlot(int)));
  }
  
  void buildGui()
  {
    parentWidget->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout *layout = new QHBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    parentWidget->setLayout(layout);
    
    comboBox = new QComboBox(parentWidget);
    comboBox->setWhatsThis(tr("Value Of Parameter"));
    comboBox->addItems(parameter->getEnumeration());
    layout->addWidget(comboBox);
    
    trafficSignal = new TrafficSignal(parentWidget, parameter);
    trafficSignal->setMinimumSize(1, 1);
    trafficSignal->setScaledContents(true);
    trafficSignal->setFixedSize(comboBox->height(), comboBox->height());
    layout->addWidget(trafficSignal);
  }
  void valueChanged()
  {
    int value = static_cast<int>(*parameter);
    assert(value >= 0 && value < comboBox->count());
    comboBox->setCurrentIndex(value);
  }
  
  void constantChanged()
  {
    if (parameter->isConstant())
    {
      comboBox->setEnabled(true);
    }
    else
    {
      comboBox->setDisabled(true);
    }
    valueChanged();
  }
};

EnumCombo::EnumCombo(QWidget *parent, prm::Parameter *pIn)
: QWidget(parent)
, stow(std::make_unique<Stow>(this, pIn))
{}

EnumCombo::~EnumCombo() = default;

void EnumCombo::currentChangedSlot(int index)
{
  assert(stow->parameter->isValidValue(index));
  prm::ObserverBlocker blocker(stow->pObserver);
  stow->parameter->setValue(index);
}

struct cmv::StringEdit::Stow
{
  StringEdit *parentWidget;
  prm::Parameter *parameter;
  prm::Observer pObserver;
  QLineEdit *lineEdit;
  
  Stow() = delete;
  Stow(StringEdit *pwIn, prm::Parameter *pIn)
  : parentWidget(pwIn)
  , parameter(pIn)
  , pObserver(std::bind(&Stow::valueChanged, this), std::bind(&Stow::constantChanged, this))
  {
    assert(parameter->getValueType() == typeid(std::string));
    buildGui();
    //no need to call valueChanged. Constant will do it all
    constantChanged();
    parameter->connect(pObserver);
    QObject::connect(lineEdit, SIGNAL(editingFinished()), parentWidget, SLOT(finishedEditingSlot()));
  }
  
  void buildGui()
  {
    parentWidget->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout *layout = new QHBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    parentWidget->setLayout(layout);
    
    lineEdit = new QLineEdit(parentWidget);
    lineEdit->setWhatsThis(tr("Value Of Parameter"));
    layout->addWidget(lineEdit);
  }
  
  void valueChanged()
  {
    auto value = static_cast<QString>(*parameter);
    lineEdit->setText(value);
  }
  
  void constantChanged() //not needed as strings can't be linked, but oh well.
  {
    if (parameter->isConstant())
    {
      lineEdit->setEnabled(true);
    }
    else
    {
      lineEdit->setDisabled(true);
    }
    valueChanged();
  }
};

StringEdit::StringEdit(QWidget *parent, prm::Parameter *pIn)
: QWidget(parent)
, stow(std::make_unique<Stow>(this, pIn))
{}

StringEdit::~StringEdit() = default;

void StringEdit::finishedEditingSlot()
{
  QString value = stow->lineEdit->text();
  prm::ObserverBlocker blocker(stow->pObserver);
  stow->parameter->setValue(value.toStdString());
}

struct cmv::PathEdit::Stow
{
  PathEdit *parentWidget;
  prm::Parameter *parameter;
  prm::Observer pObserver;
  QLineEdit *lineEdit;
  QPushButton *button;
  
  Stow() = delete;
  Stow(PathEdit *pwIn, prm::Parameter *pIn)
  : parentWidget(pwIn)
  , parameter(pIn)
  , pObserver(std::bind(&Stow::valueChanged, this), std::bind(&Stow::constantChanged, this))
  {
    assert(parameter->getValueType() == typeid(boost::filesystem::path));
    buildGui();
    //no need to call valueChanged. Constant will do it all
    constantChanged();
    parameter->connect(pObserver);
    QObject::connect(lineEdit, &QLineEdit::editingFinished, parentWidget, &PathEdit::finishedEditingSlot);
    QObject::connect(button, &QPushButton::clicked, parentWidget, &PathEdit::browseSlot);
  }
  
  void buildGui()
  {
    parentWidget->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout *layout = new QHBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    parentWidget->setLayout(layout);
    
    lineEdit = new QLineEdit(parentWidget);
    lineEdit->setWhatsThis(tr("Value Of Parameter"));
    layout->addWidget(lineEdit);
    
    button = new QPushButton(tr("Browse"), parentWidget); //TODO add icon
    layout->addWidget(button);
  }
  
  void valueChanged()
  {
    auto value = static_cast<QString>(*parameter);
    lineEdit->setText(value);
  }
  
  void constantChanged() //not needed as paths can't be linked, but oh well.
  {
    if (parameter->isConstant())
    {
      lineEdit->setEnabled(true);
    }
    else
    {
      lineEdit->setDisabled(true);
    }
    valueChanged();
  }
};

PathEdit::PathEdit(QWidget *parent, prm::Parameter *pIn)
: QWidget(parent)
, stow(std::make_unique<Stow>(this, pIn))
{}

PathEdit::~PathEdit() = default;

void PathEdit::finishedEditingSlot()
{
  QString value = stow->lineEdit->text();
  boost::filesystem::path p(value.toStdString());
  if (boost::filesystem::exists(p))
  {
    prm::ObserverBlocker blocker(stow->pObserver);
    stow->parameter->setValue(boost::filesystem::path(value.toStdString()));
  }
}

void PathEdit::browseSlot()
{
  boost::filesystem::path cp = static_cast<boost::filesystem::path>(*stow->parameter);
  if (!boost::filesystem::exists(cp))
    cp = prf::manager().rootPtr->project().lastDirectory().get();
  boost::filesystem::path op;
  prm::PathType pt = stow->parameter->getPathType();
  if (pt == prm::PathType::Directory)
  {
    QString file = QFileDialog::getExistingDirectory
    (
      app::instance()->getMainWindow()
      , stow->parameter->getName()
      , QString::fromStdString(cp.string())
      , QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    if (file.isEmpty())
      return;
    op = file.toStdString();
  }
  else if (pt == prm::PathType::Read)
  {
    QString file = QFileDialog::getOpenFileName
    (
      app::instance()->getMainWindow()
      , stow->parameter->getName()
      , QString::fromStdString(cp.string())
      , QString()
      , nullptr
      , QFileDialog::DontResolveSymlinks
    );
    if (file.isEmpty())
      return;
    op = file.toStdString();
  }
  else if (pt == prm::PathType::Write)
  {
    QString file = QFileDialog::getSaveFileName
    (
      app::instance()->getMainWindow()
      , stow->parameter->getName()
      , QString::fromStdString(cp.string())
      , QString()
      , nullptr
      , QFileDialog::DontResolveSymlinks
    );
    if (file.isEmpty())
      return;
    op = file.toStdString();
  }
  
  if (!op.empty())
    stow->parameter->setValue(op);
}

//build the appropriate widget editor.
namespace cmv
{
  class WidgetFactoryVisitor : public boost::static_visitor<QWidget*>
  {
  public:
    WidgetFactoryVisitor() = delete;
    WidgetFactoryVisitor(QWidget *pIn, prm::Parameter* prmIn)
    : parent(pIn)
    , parameter(prmIn)
    {assert(parent); assert(parameter);}
    QWidget* operator()(double) const {return buildDouble();}
    QWidget* operator()(int) const {return buildInt();}
    QWidget* operator()(bool) const {return buildBool();}
    QWidget* operator()(const std::string&) const {return buildString();}
    QWidget* operator()(const boost::filesystem::path&) const {return buildPath();}
    QWidget* operator()(const osg::Vec3d&) const {return buildVec3d();}
    QWidget* operator()(const osg::Quat&) const {return buildQuat();}
    QWidget* operator()(const osg::Matrixd&) const {return buildMatrix();}
    
  private:
    QWidget *parent;
    prm::Parameter* parameter;
    
    QWidget* buildDouble() const
    {
      return new ExpressionEdit(parent, parameter);
    }
      
    QWidget* buildInt() const
    {
      QWidget *widgetOut = nullptr;
      
      if (parameter->isEnumeration())
        widgetOut = new EnumCombo(parent, parameter);
      else
        widgetOut = new ExpressionEdit(parent, parameter);
      
      assert(widgetOut);
      return widgetOut;
    }
    
    QWidget* buildBool() const
    {
      return new BoolCombo(parent, parameter);
    }
    
    QWidget* buildString() const
    {
      return new StringEdit(parent, parameter);
    }
    
    QWidget* buildPath() const
    {
      return new PathEdit(parent, parameter);
    }
    
    QWidget* buildVec3d() const
    {
      ExpressionEdit *ee = new ExpressionEdit(parent, parameter);
      ee->setDisabled(true); //can't parse yet
      return ee;
    }
    
    QWidget* buildQuat() const
    {
      ExpressionEdit *ee = new ExpressionEdit(parent, parameter);
      ee->setDisabled(true); //can't parse yet
      return ee;
    }
    
    QWidget* buildMatrix() const
    {
      cmv::CSysWidget *cw = new cmv::CSysWidget(parent, parameter);
      return cw;
    }
  };
}

struct cmv::ParameterWidget::Stow
{
  ParameterWidget *parentWidget;
  std::vector<prm::Parameter*> parameters;
  QGridLayout *layout;
  
  Stow() = delete;
  Stow(ParameterWidget *pw, const std::vector<prm::Parameter*> &psIn)
  : parentWidget(pw)
  , parameters(psIn)
  {
    assert(!parameters.empty()); //don't pass me an empty vector of parameters.
    buildGui();
  }
  
  void buildGui()
  {
    parentWidget->setContentsMargins(0, 0, 0, 0);
    layout = new QGridLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    parentWidget->setLayout(layout);
    
    int row = 0;
    int column = 0;
    for (auto *p : parameters)
    {
      QLabel *label = new QLabel(p->getName(), parentWidget);
      layout->addWidget(label, row, column++, Qt::AlignVCenter | Qt::AlignRight);
      
      cmv::WidgetFactoryVisitor v(parentWidget, p);
      QWidget *pWidget = boost::apply_visitor(v, p->getStow().variant);
      layout->addWidget(pWidget, row++, column--); //don't use alignment or won't stretch to fill area.
    }
  }
  
  boost::optional<QWidget*> findWidget(prm::Parameter *pIn)
  {
    auto it = std::find(parameters.begin(), parameters.end(), pIn);
    if (it == parameters.end())
      return boost::none;
    int row = std::distance(parameters.begin(), it);
    auto *layoutItem = layout->itemAtPosition(row, 1);
    if (!layoutItem)
      return boost::none;
    auto *widget = layout->itemAtPosition(row, 1)->widget();
    if (!widget)
      return boost::none;
    return widget;
  }
};

ParameterWidget::ParameterWidget(QWidget *parent, const std::vector<prm::Parameter*> &psIn)
: QWidget(parent)
, stow(std::make_unique<Stow>(this, psIn))
{}

ParameterWidget::~ParameterWidget() = default;

void ParameterWidget::enableWidget(prm::Parameter *pIn)
{
  auto widget = stow->findWidget(pIn);
  assert(widget);
  if (!widget)
    return;
  widget.get()->setEnabled(true);
}

void ParameterWidget::disableWidget(prm::Parameter *pIn)
{
  auto widget = stow->findWidget(pIn);
  assert(widget);
  if (!widget)
    return;
  widget.get()->setDisabled(true);
}

QWidget* ParameterWidget::getWidget(prm::Parameter *pIn)
{
  auto widget = stow->findWidget(pIn);
  assert(widget);
  if (!widget)
    return nullptr;
  return widget.get();
}
