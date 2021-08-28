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

#include <QSettings>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QHelpEvent>
#include <QComboBox>
#include <QPushButton>
#include <QFileDialog>
#include <QTableView>
#include <QAbstractTableModel>
#include <QHeaderView>
#include <QTimer>

#include "application/appapplication.h"
#include "application/appmainwindow.h"
#include "project/prjproject.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "message/msgmessage.h"
#include "parameter/prmvariant.h"
#include "parameter/prmparameter.h"
#include "expressions/exprmanager.h"
#include "tools/idtools.h"
#include "commandview/cmvtrafficsignal.h"
#include "commandview/cmvtablewidgets.h"

using boost::uuids::uuid;

//build the appropriate widget editor. Needs to be in sync with visitor in cmvtable.cpp.
namespace
{
  struct WidgetFactoryVisitor
  {
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
    QWidget* operator()(const ftr::Picks&) const {return new QWidget(parent);}
    //we have to convert picks to messages for pick widget, so we treat separately
    //and we should not call this visitor on picks.
    
  private:
    QWidget *parent;
    prm::Parameter* parameter;
    
    QWidget* buildDouble() const
    {
      return new cmv::tbl::ExpressionEdit(parent, parameter);
    }
      
    QWidget* buildInt() const
    {
      QWidget *widgetOut = nullptr;
      
      if (parameter->isEnumeration())
      {
        QComboBox *comboBox = new QComboBox(parent);
        for (const auto &s : parameter->getEnumeration())
          comboBox->addItem(s);
        comboBox->setCurrentIndex(parameter->getInt());
        return comboBox;
      }
      else
        widgetOut = new cmv::tbl::ExpressionEdit(parent, parameter);
      
      assert(widgetOut);
      return widgetOut;
    }
    
    QWidget* buildBool() const
    {
      QComboBox *comboBox = new QComboBox(parent);
      comboBox->addItem(QObject::tr("True"));
      comboBox->addItem(QObject::tr("False"));
      if (parameter->getBool())
        comboBox->setCurrentIndex(0);
      else
        comboBox->setCurrentIndex(1);
      return comboBox;
    }
    
    QWidget* buildString() const
    {
      return new QLineEdit(parameter->adaptToQString(), parent);
    }
    
    QWidget* buildPath() const
    {
      return new cmv::tbl::PathEdit(parent, parameter);
    }
    
    QWidget* buildVec3d() const
    {
      return new cmv::tbl::ExpressionEdit(parent, parameter);
    }
    
    QWidget* buildQuat() const
    {
      return new cmv::tbl::ExpressionEdit(parent, parameter);
    }
    
    QWidget* buildMatrix() const
    {
      return new cmv::tbl::ExpressionEdit(parent, parameter);
    }
  };
}

QWidget* cmv::tbl::buildWidget(QWidget *parent, prm::Parameter *pIn)
{
  WidgetFactoryVisitor v(parent, pIn);
  return std::visit(v, pIn->getStow().variant);
}

using namespace cmv::tbl;

Base::Base(QWidget *p) : QWidget(p){}

/* Hack for delegate: With my custom widget, editing via a delegate would not
 * finish when focus was lost. Apparently the delegate expects behavior from its'
 * editor widget, not its' children, that is not documented. The delegate assigns an event filter
 * on the created editor. This override forwards events from children to 'this',
 * the custom widget used as delegate editor, So the delegate can respond accordingly.
 * The appropriate children will need to have this installed as an event filter.
 * 
 * https://stackoverflow.com/questions/12145522/why-pressing-of-tab-key-emits-only-qeventshortcutoverride-event
 * https://forum.qt.io/topic/3778/item-delegate-editor-focus-problem/9
 */
bool Base::eventFilter(QObject *watched, QEvent *event)
{
  if(event->type() == QEvent::KeyPress)
  {
    QKeyEvent* keyEvent = static_cast<QKeyEvent *>(event);
    if(keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab)
    {
      QApplication::postEvent(this, new QKeyEvent(keyEvent->type(), keyEvent->key(), keyEvent->modifiers()));
      // Filter this event because the editor will be closed anyway
      return true;
    }
  }
  else if(event->type() == QEvent::FocusOut)
  {
    QFocusEvent* focusEvent = static_cast<QFocusEvent *>(event);
    QApplication::postEvent(this, new QFocusEvent(focusEvent->type(), focusEvent->reason()));

    // Don't filter because focus can be changed internally in editor
    return false;
  }

  return QWidget::eventFilter(watched, event);
}

struct ExpressionEdit::Stow
{
  ExpressionEdit *parent;
  prm::Parameter *parameter;
  QLineEdit *lineEdit = nullptr;
  trf::Signal *trafficSignal = nullptr;
  
  Stow(ExpressionEdit *parentIn, prm::Parameter *parameterIn)
  : parent(parentIn)
  , parameter(parameterIn)
  {
    buildGui();
  }
  
  void buildGui()
  {
    lineEdit = new QLineEdit(parent);
    trafficSignal = new trf::Signal(parent, parameter);
    
    parent->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout *layout = new QHBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    parent->setLayout(layout);
    
    lineEdit->setAcceptDrops(false); //the edit line accepts drops by default.
    lineEdit->setWhatsThis(tr("Value Or Name Of Linked Expression"));
    layout->addWidget(lineEdit);
    
    trafficSignal->setMinimumSize(1, 1);
    trafficSignal->setScaledContents(true);
    trafficSignal->setFixedSize(lineEdit->height(), lineEdit->height());
    layout->addWidget(trafficSignal);
    
    lineEdit->setText(parameter->adaptToQString());
  }
  
  void goToolTip(const QString &tipIn)
  {
    assert(lineEdit);
    lineEdit->setToolTip(tipIn);
    
    QPoint point(0.0, -1.5 * (lineEdit->frameGeometry().height()));
    QHelpEvent *toolTipEvent = new QHelpEvent(QEvent::ToolTip, point, lineEdit->mapToGlobal(point));
    qApp->postEvent(lineEdit, toolTipEvent);
  }
};

ExpressionEdit::ExpressionEdit(QWidget *parentIn, prm::Parameter *parameterIn)
: Base(parentIn)
, stow(std::make_unique<Stow>(this, parameterIn))
{
  assert(parameterIn); //Don't set me up.
  
  setFocusProxy(stow->lineEdit);
  stow->lineEdit->installEventFilter(this);
  
  connect(stow->lineEdit, &QLineEdit::textEdited, this, &ExpressionEdit::editing);
  connect(stow->lineEdit, &QLineEdit::editingFinished, this, &ExpressionEdit::finishedEditing);
}

ExpressionEdit::~ExpressionEdit() = default;

QString ExpressionEdit::text() const
{
  return stow->lineEdit->text();
}

void ExpressionEdit::editing(const QString &textIn)
{
  assert(stow->parameter);
  stow->trafficSignal->setTrafficYellow();
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
      stow->trafficSignal->setTrafficGreen();
      std::ostringstream stream;
      stream << localManager.getExpressionValue(*oId);
      stow->goToolTip(QString::fromStdString(stream.str()));
    }
    else
    {
      stow->trafficSignal->setTrafficRed();
      stow->goToolTip(tr("Invalid Type Or Value"));
    }
  }
  else
  {
    stow->trafficSignal->setTrafficRed();
    int position = result.errorPosition - 8; // 7 chars for 'temp = ' + 1
    stow->goToolTip(textIn.left(position) + "?");
  }
}

void ExpressionEdit::finishedEditing()
{
  //not sure what this is going to be if it at all?
  
  /*
  assert(stow->parameter);
  if (!stow->parameter->isConstant())
    return;
  
  auto mo = [&](const std::string &s)
  {
    app::instance()->queuedMessage(msg::buildStatusMessage(s, 2.0));
  };

  expr::Manager localManager;
  std::string formula("temp = ");
  formula += stow->lineEdit->text().toStdString();
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
        stow->trafficSignal->setTrafficRed();
        break;
      case expr::Amity::InvalidValue:
        mo("Invalid Value");
        stow->trafficSignal->setTrafficRed();
        break;
      case expr::Amity::Equal:
        mo("Same Value");
        stow->trafficSignal->setTrafficGreen();
        break;
      case expr::Amity::Mutate:
        mo("Value Changed");
        stow->trafficSignal->setTrafficGreen();
        break;
    }
  }
  else
  {
    mo("Parsing failed");
    stow->trafficSignal->setTrafficRed();
  }
  
  */
}

struct PathEdit::Stow
{
  PathEdit *parentWidget;
  const prm::Parameter *parameter;
  QLineEdit *lineEdit;
  QPushButton *button;
  
  Stow() = delete;
  Stow(PathEdit *pwIn, const prm::Parameter *pIn)
  : parentWidget(pwIn)
  , parameter(pIn)
  {
    assert(parameter->getValueType() == typeid(boost::filesystem::path));
    buildGui();
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
    lineEdit->setText(parameter->adaptToQString());
  }
};

PathEdit::PathEdit(QWidget *parent, const prm::Parameter *pIn)
: Base(parent)
, stow(std::make_unique<Stow>(this, pIn))
{
  setFocusProxy(stow->lineEdit);
  stow->lineEdit->installEventFilter(this);
}

PathEdit::~PathEdit() = default;

QString PathEdit::text() const
{
  return stow->lineEdit->text();
}

void PathEdit::finishedEditingSlot()
{
  //not sure if I want this slot or what it should look like
  /*
  QString value = stow->lineEdit->text();
  boost::filesystem::path p(value.toStdString());
  if (boost::filesystem::exists(p))
  {
    prm::ObserverBlocker blocker(stow->pObserver);
    if (stow->parameter->setValue(boost::filesystem::path(value.toStdString())))
      prmValueChanged();
  }
  */
}

void PathEdit::browseSlot()
{
  boost::filesystem::path cp = stow->parameter->getPath();
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
    stow->lineEdit->setText(QString::fromStdString(op.string()));
}
