/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2015  Thomas S. Anderson blobfish.at.gmx.com
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

#include <QLabel>
#include <QKeyEvent>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QTimer>
#include <QTextStream>
#include <QFileDialog>

#include <lccmanager.h>

#include "tools/idtools.h"
#include "tools/infotools.h"
#include "tools/tlsstring.h"
#include "expressions/exprmanager.h"
#include "expressions/exprvalue.h"
#include "application/appapplication.h"
#include "project/prjproject.h"
#include "project/prjmessage.h"
#include "application/appmainwindow.h"
#include "feature/ftrbase.h"
#include "parameter/prmvariant.h"
#include "parameter/prmparameter.h"
#include "message/msgnode.h"
#include "message/msgsift.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "dialogs/dlgexpressionedit.h"
#include "dialogs/dlgwidgetgeometry.h"
#include "dialogs/dlgparameter.h"

using namespace dlg;

//build the appropriate widget editor.
class WidgetFactoryVisitor : public boost::static_visitor<QWidget*>
{
public:
  WidgetFactoryVisitor() = delete;
  WidgetFactoryVisitor(Parameter *dialogIn) : dialog(dialogIn){assert(dialog);}
  QWidget* operator()(double) const {return buildDouble();}
  QWidget* operator()(int) const {return buildInt();}
  QWidget* operator()(bool) const {return buildBool();}
  QWidget* operator()(const std::string&) const {return buildString();}
  QWidget* operator()(const boost::filesystem::path&) const {return buildPath();}
  QWidget* operator()(const osg::Vec3d&) const {return buildVec3d();}
  QWidget* operator()(const osg::Quat&) const {return buildQuat();}
  QWidget* operator()(const osg::Matrixd&) const {return buildMatrix();}
  
private:
  Parameter *dialog;
  
  QWidget* buildDouble() const //!< might used for more than just doubles.
  {
    ExpressionEdit *editLine = new ExpressionEdit(dialog);
    if (dialog->parameter->isConstant())
      QTimer::singleShot(0, editLine->trafficLabel, SLOT(setTrafficGreenSlot()));
    else
      QTimer::singleShot(0, editLine->trafficLabel, SLOT(setLinkSlot()));
    
    //this might be needed for other types with some changes
    ExpressionEditFilter *filter = new ExpressionEditFilter(dialog);
    editLine->lineEdit->installEventFilter(filter);
    QObject::connect(filter, SIGNAL(requestLinkSignal(QString)), dialog, SLOT(requestLinkSlot(QString)));
    
    QObject::connect(editLine->lineEdit, SIGNAL(textEdited(QString)), dialog, SLOT(textEditedDoubleSlot(QString)));
    QObject::connect(editLine->lineEdit, SIGNAL(returnPressed()), dialog, SLOT(updateDoubleSlot()));
    QObject::connect(editLine->trafficLabel, SIGNAL(requestUnlinkSignal()), dialog, SLOT(requestUnlinkSlot()));
    
    return editLine;
  }
    
  QWidget* buildInt() const
  {
    QWidget *widgetOut = nullptr;
    int value = static_cast<int>(*(dialog->parameter));
    
    if (dialog->parameter->isEnumeration())
    {
      QComboBox *out = new QComboBox(dialog);
      for (const auto &s : dialog->parameter->getEnumeration())
        out->addItem(s);
      assert(value < out->count());
      out->setCurrentIndex(value);
      QObject::connect(out, SIGNAL(currentIndexChanged(int)), dialog, SLOT(intChangedSlot(int)));
      widgetOut = out;
    }
    else
    {
      QLineEdit* out = new QLineEdit(dialog);
      out->setText(QString::number(value));
      QObject::connect(out, SIGNAL(editingFinished()), dialog, SLOT(intChangedSlot()));
      widgetOut = out;
    }
    
    assert(widgetOut);
    return widgetOut;
  }
  
  QWidget* buildBool() const
  {
    QComboBox *out = new QComboBox(dialog);
    out->addItem(QObject::tr("True"), QVariant(true));
    out->addItem(QObject::tr("False"), QVariant(false));
    
    if (static_cast<bool>(*(dialog->parameter)))
      out->setCurrentIndex(0);
    else
      out->setCurrentIndex(1);
    
    QObject::connect(out, SIGNAL(currentIndexChanged(int)), dialog, SLOT(boolChangedSlot(int)));
    
    return out;
  }
  
  QWidget* buildString() const
  {
    QLineEdit* out = new QLineEdit(dialog);
    out->setText(QString::fromStdString(static_cast<std::string>(*(dialog->parameter))));
    
    //connect to dialog slots
    
    return out;
  }
  
  QWidget* buildPath() const
  {
    QWidget *ww = new QWidget(dialog); //widget wrapper.
    QHBoxLayout *hl = new QHBoxLayout();
    hl->setContentsMargins(0, 0, 0, 0);
    ww->setLayout(hl);
    
    QLineEdit *le = new QLineEdit(ww);
    le->setObjectName(QString("TheLineEdit")); //we use this to search in the slot.
    le->setText(QString::fromStdString(static_cast<boost::filesystem::path>(*(dialog->parameter)).string()));
    hl->addWidget(le);
    
    QPushButton *bb = new QPushButton(QObject::tr("Browse"), ww);
    hl->addWidget(bb);
    QObject::connect(bb, SIGNAL(clicked()), dialog, SLOT(browseForPathSlot()));
    
    return ww;
  }
  
  QWidget* buildVec3d() const
  {
    osg::Vec3d theVec = static_cast<osg::Vec3d>(*(dialog->parameter));
    
    QWidget *ww = new QWidget(dialog); //widget wrapper.
    QVBoxLayout *vl = new QVBoxLayout();
    vl->setContentsMargins(0, 0, 0, 0);
    ww->setLayout(vl);
    
    QHBoxLayout *xLayout = new QHBoxLayout();
    QLabel *xLabel = new QLabel(QObject::tr("X:"), ww);
    xLayout->addWidget(xLabel);
    QLineEdit *xLineEdit = new QLineEdit(ww);
    xLineEdit->setObjectName("XLineEdit");
    xLineEdit->setText(QString::number(theVec.x(), 'f', 12));
    xLayout->addWidget(xLineEdit);
    vl->addLayout(xLayout);
    QObject::connect(xLineEdit, SIGNAL(editingFinished()), dialog, SLOT(vectorChangedSlot()));
    
    QHBoxLayout *yLayout = new QHBoxLayout();
    QLabel *yLabel = new QLabel(QObject::tr("Y:"), ww);
    yLayout->addWidget(yLabel);
    QLineEdit *yLineEdit = new QLineEdit(ww);
    yLineEdit->setObjectName("YLineEdit");
    yLineEdit->setText(QString::number(theVec.y(), 'f', 12));
    yLayout->addWidget(yLineEdit);
    vl->addLayout(yLayout);
    QObject::connect(yLineEdit, SIGNAL(editingFinished()), dialog, SLOT(vectorChangedSlot()));
    
    QHBoxLayout *zLayout = new QHBoxLayout();
    QLabel *zLabel = new QLabel(QObject::tr("Z:"), ww);
    zLayout->addWidget(zLabel);
    QLineEdit *zLineEdit = new QLineEdit(ww);
    zLineEdit->setObjectName("ZLineEdit");
    zLineEdit->setText(QString::number(theVec.z(), 'f', 12));
    zLayout->addWidget(zLineEdit);
    vl->addLayout(zLayout);
    QObject::connect(zLineEdit, SIGNAL(editingFinished()), dialog, SLOT(vectorChangedSlot()));
    
    return ww;
    
    //maybe more elaborate control with vector selection?
  }
  
  QWidget* buildQuat() const
  {
    QLineEdit* out = new QLineEdit(dialog);
    
    QString buffer;
    QTextStream stream(&buffer);
    gu::osgQuatOut(stream, static_cast<osg::Quat>(*(dialog->parameter)));
    out->setText(buffer);
    
    //more elaborate control with quat selection?
    
    //connect to dialog slots
    
    return out;
  }
  
  QWidget* buildMatrix() const
  {
    QLineEdit* out = new QLineEdit(dialog);
    
    QString buffer;
    QTextStream stream(&buffer);
    gu::osgMatrixOut(stream, static_cast<osg::Matrixd>(*(dialog->parameter)));
    out->setText(buffer);
    
    //more elaborate control with matrix selection?
    
    //connect to dialog slots
    
    return out;
  }
};

//react to value has changed based upon type.
class ValueChangedVisitor : public boost::static_visitor<>
{
public:
  ValueChangedVisitor() = delete;
  ValueChangedVisitor(Parameter *dialogIn) : dialog(dialogIn){assert(dialog);}
  void operator()(double) const {dialog->valueHasChangedDouble();}
  void operator()(int) const {dialog->valueHasChangedInt();}
  void operator()(bool) const {dialog->valueHasChangedBool();}
  void operator()(const std::string&) const {}
  void operator()(const boost::filesystem::path&) const {dialog->valueHasChangedPath();}
  void operator()(const osg::Vec3d&) const {dialog->valueHasChangedVector();}
  void operator()(const osg::Quat&) const {}
  void operator()(const osg::Matrixd&) const {}
private:
  Parameter *dialog;
};

//react to constant has changed based upon type.
class ConstantChangedVisitor : public boost::static_visitor<>
{
public:
  ConstantChangedVisitor() = delete;
  ConstantChangedVisitor(Parameter *dialogIn) : dialog(dialogIn){assert(dialog);}
  void operator()(double) const {dialog->constantHasChangedDouble();}
  void operator()(int) const {dialog->constantHasChangedInt();}
  void operator()(bool) const {} //don't think bool should be anything other than constant?
  void operator()(const std::string&) const {}
  void operator()(const boost::filesystem::path&) const {}
  void operator()(const osg::Vec3d&) const {}
  void operator()(const osg::Quat&) const {}
  void operator()(const osg::Matrixd&) const {}
private:
  Parameter *dialog;
};

Parameter::Parameter(prm::Parameter *parameterIn, const boost::uuids::uuid &idIn):
  QDialog(app::instance()->getMainWindow()),
  parameter(parameterIn)
  , pObserver(new prm::Observer(std::bind(&Parameter::valueHasChanged, this), std::bind(&Parameter::constantHasChanged, this)))
{
  assert(parameter);
  
  feature = app::instance()->getProject()->findFeature(idIn);
  assert(feature);
  buildGui();
  constantHasChanged();
  
  parameter->connect(*pObserver);
  
  node = std::make_unique<msg::Node>();
  node->connect(msg::hub());
  sift = std::make_unique<msg::Sift>();
  sift->name = "dlg::Parameter";
  sift->insert
  (
    msg::Response | msg::Pre | msg::Remove | msg::Feature
    , std::bind(&Parameter::featureRemovedDispatched, this, std::placeholders::_1)
  );
  node->setHandler(std::bind(&msg::Sift::receive, sift.get(), std::placeholders::_1));
  
  WidgetGeometry *filter = new WidgetGeometry(this, "dlg::Parameter");
  this->installEventFilter(filter);
  
  this->setAttribute(Qt::WA_DeleteOnClose);
}

Parameter::~Parameter(){}

void Parameter::buildGui()
{
  QString title = feature->getName() + ": ";
  title += QString::fromStdString(parameter->getValueTypeString());
  this->setWindowTitle(title);
  
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  
  QHBoxLayout *editLayout = new QHBoxLayout();
  QLabel *nameLabel = new QLabel(parameter->getName(), this);
  editLayout->addWidget(nameLabel);
  
  WidgetFactoryVisitor fv(this);
  editWidget = boost::apply_visitor(fv, parameter->getStow().variant);
  editLayout->addWidget(editWidget);
  
  mainLayout->addLayout(editLayout);
}

void Parameter::requestLinkSlot(const QString &stringIn)
{
  //for now we only get here if we have a double. because connect inside double condition.
  
  int eId = stringIn.toInt();
  assert(eId >= 0);
  app::instance()->getProject()->expressionLink(parameter->getId(), eId);
  
  if (prf::manager().rootPtr->dragger().triggerUpdateOnFinish())
    node->send(msg::Mask(msg::Request | msg::Project | msg::Update));
  
  this->activateWindow();
}

void Parameter::requestUnlinkSlot()
{
  //for now we only get here if we have a double. because connect inside double condition.
  app::instance()->getProject()->expressionUnlink(parameter->getId());
  
  //unlinking itself shouldn't trigger an update because the parameter value is still the same.
}

void Parameter::valueHasChanged()
{
  ValueChangedVisitor v(this);
  boost::apply_visitor(v, parameter->getStow().variant);
}

void Parameter::valueHasChangedDouble()
{
  ExpressionEdit *eEdit = dynamic_cast<ExpressionEdit*>(editWidget);
  assert(eEdit);
  
  lastValue = static_cast<double>(*parameter);
  if (parameter->isConstant())
  {
    eEdit->lineEdit->setText(QString::fromStdString(tls::prettyDouble(lastValue)));
    eEdit->lineEdit->selectAll();
  }
  //if it is linked we shouldn't need to change.
}

void Parameter::valueHasChangedInt()
{
  int value = static_cast<int>(*parameter);
  if (parameter->isEnumeration())
  {
    QComboBox *cb = dynamic_cast<QComboBox*>(editWidget);
    assert(cb);
    
    assert(value < cb->count());
    cb->setCurrentIndex(value);
  }
  else
  {
    QLineEdit *lineEdit = dynamic_cast<QLineEdit*>(editWidget);
    assert(lineEdit);
    
    if (parameter->isConstant())
    {
      lineEdit->setText(QString::number(value));
      lineEdit->selectAll();
    }
  }
}

void Parameter::valueHasChangedBool()
{
  QComboBox *cBox = dynamic_cast<QComboBox*>(editWidget);
  assert(cBox);
  
  bool cwv = cBox->currentData().toBool(); //current widget value
  bool cpv = static_cast<bool>(*parameter); //current parameter value
  if (cwv != cpv)
  {
    if (cpv)
      cBox->setCurrentIndex(0);
    else
      cBox->setCurrentIndex(1);
    //setting this will trigger this' changed slot, but at that
    //point combo box value and parameter value will be equal,
    //so, no recurse.
  }
}

void Parameter::valueHasChangedPath()
{
  //find the line edit child widget and get text.
  QLineEdit *lineEdit = editWidget->findChild<QLineEdit*>(QString("TheLineEdit"));
  assert(lineEdit);
  if (!lineEdit)
    return;
  
  lineEdit->setText(QString::fromStdString(static_cast<boost::filesystem::path>(*parameter).string()));
}

void Parameter::valueHasChangedVector()
{
  QLineEdit *xLineEdit = editWidget->findChild<QLineEdit*>(QString("XLineEdit"));
  assert(xLineEdit); if (!xLineEdit) return;
  
  QLineEdit *yLineEdit = editWidget->findChild<QLineEdit*>(QString("YLineEdit"));
  assert(yLineEdit); if (!yLineEdit) return;
  
  QLineEdit *zLineEdit = editWidget->findChild<QLineEdit*>(QString("ZLineEdit"));
  assert(zLineEdit); if (!zLineEdit) return;
  
  osg::Vec3d theVec = static_cast<osg::Vec3d>(*parameter);
  
  xLineEdit->setText(QString::number(theVec.x(), 'f', 12));
  yLineEdit->setText(QString::number(theVec.y(), 'f', 12));
  zLineEdit->setText(QString::number(theVec.z(), 'f', 12));
}

void Parameter::constantHasChanged()
{
  ConstantChangedVisitor v(this);
  boost::apply_visitor(v, parameter->getStow().variant);
}

void Parameter::constantHasChangedDouble()
{
  ExpressionEdit *eEdit = dynamic_cast<ExpressionEdit*>(editWidget);
  assert(eEdit);
  
  if (parameter->isConstant())
  {
    eEdit->trafficLabel->setTrafficGreenSlot();
    eEdit->lineEdit->setReadOnly(false);
    eEdit->setFocus();
  }
  else
  {
    const expr::Manager &eManager = static_cast<app::Application *>(qApp)->getProject()->getManager();
    auto linked = eManager.getLinked(parameter->getId());
    assert(linked);
    auto formulaName = eManager.getExpressionName(*linked);
    assert(formulaName);
    
    eEdit->trafficLabel->setLinkSlot();
    eEdit->lineEdit->setText(QString::fromStdString(*formulaName));
    eEdit->clearFocus();
    eEdit->lineEdit->deselect();
    eEdit->lineEdit->setReadOnly(true);
  }
  valueHasChangedDouble();
}

void Parameter::constantHasChangedInt()
{
  valueHasChangedInt();
}

void Parameter::featureRemovedDispatched(const msg::Message &messageIn)
{
  prj::Message pMessage = messageIn.getPRJ();
  if(pMessage.feature->getId() == feature->getId())
    this->reject();
}

void Parameter::updateDoubleSlot()
{
  //for now we only get here if we have a double. because connect inside double condition.
  
  //if we are linked, we shouldn't need to do anything.
  if (!parameter->isConstant())
    return;
  
  ExpressionEdit *eEdit = dynamic_cast<ExpressionEdit*>(editWidget);
  assert(eEdit);

  auto fail = [&]()
  {
    eEdit->lineEdit->setText(QString::fromStdString(tls::prettyDouble(lastValue)));
    eEdit->lineEdit->selectAll();
    eEdit->trafficLabel->setTrafficGreenSlot();
  };

  std::ostringstream gitStream;
  gitStream
    << QObject::tr("Feature: ").toStdString() << feature->getName().toStdString()
    << QObject::tr("    Parameter ").toStdString() << parameter->getName().toStdString();

  lcc::Manager eman;
  std::string formula("temp = ");
  formula += eEdit->lineEdit->text().toStdString();
  auto results = eman.parseString(formula);
  if (results.isAllGood())
  {
    if (results.value.size() == 1)
    {
      double value = results.value.at(0);
      if (parameter->isValidValue(value))
      {
        parameter->setValue(value);
        if (prf::manager().rootPtr->dragger().triggerUpdateOnFinish())
        {
          gitStream  << QObject::tr("    changed to: ").toStdString() << static_cast<double>(*parameter);
          node->send(msg::buildGitMessage(gitStream.str()));
          node->send(msg::Mask(msg::Request | msg::Project | msg::Update));
        }
      }
      else
      {
        node->send(msg::buildStatusMessage(QObject::tr("Value out of range").toStdString()));
        fail();
      }
    }
    else
    {
      node->send(msg::buildStatusMessage("Wrong Type"));
      fail();
    }
  }
  else
  {
    node->send(msg::buildStatusMessage(results.getError()));
    fail();
  }
}

void Parameter::textEditedDoubleSlot(const QString &textIn)
{
  ExpressionEdit *eEdit = dynamic_cast<ExpressionEdit*>(editWidget);
  assert(eEdit);
  
  eEdit->trafficLabel->setTrafficYellowSlot();
  qApp->processEvents(); //need this or we never see yellow signal.
  
  lcc::Manager eman;
  std::string formula("temp = ");
  formula += textIn.toStdString();
  auto results = eman.parseString(formula);
  if (results.isAllGood())
  {
    if (results.value.size() == 1)
    {
      eEdit->trafficLabel->setTrafficGreenSlot();
      double value = results.value.at(0);
      eEdit->goToolTipSlot(QString::fromStdString(tls::prettyDouble(value)));
    }
    else
    {
      eEdit->trafficLabel->setTrafficRedSlot();
      eEdit->goToolTipSlot("Wrong Type");
    }
  }
  else
  {
    eEdit->trafficLabel->setTrafficRedSlot();
    int position = results.errorPosition - 8; // 7 chars for 'temp = ' + 1
    eEdit->goToolTipSlot(textIn.left(position) + "?");
  }
}

void Parameter::boolChangedSlot(int i)
{
  //we don't need to respond to this value change, so block.
  prm::ObserverBlocker block(*pObserver);
  
  bool cwv = static_cast<QComboBox*>(QObject::sender())->itemData(i).toBool(); //current widget value
  if(parameter->setValue(cwv))
  {
    std::ostringstream gitStream;
    gitStream
    << QObject::tr("Feature: ").toStdString() << feature->getName().toStdString()
    << QObject::tr("    Parameter ").toStdString() << parameter->getName().toStdString();
    gitStream  << QObject::tr("    changed to: ").toStdString() << static_cast<bool>(*parameter);
    node->send(msg::buildGitMessage(gitStream.str()));
    if (prf::manager().rootPtr->dragger().triggerUpdateOnFinish())
      node->send(msg::Mask(msg::Request | msg::Project | msg::Update));
  }
}

void Parameter::browseForPathSlot()
{
  //we are manually setting the lineEdit so we can block value signal.
  prm::ObserverBlocker block(*pObserver);
  
  //find the line edit child widget and get text.
  QLineEdit *lineEdit = editWidget->findChild<QLineEdit*>(QString("TheLineEdit"));
  assert(lineEdit);
  if (!lineEdit)
    return;
  
  namespace bfs = boost::filesystem;
  bfs::path t = lineEdit->text().toStdString();
  std::string extension = "(*";
  if (t.has_extension())
    extension += t.extension().string();
  else
    extension += ".*";
  extension += ")";
  
  if (parameter->getPathType() == prm::PathType::Read)
  {
    //read expects the file to exist. start browse from project directory if doesn't exist.
    if (!bfs::exists(t))
      t = prf::manager().rootPtr->project().lastDirectory().get();
    
    QString fileName = QFileDialog::getOpenFileName
    (
      this,
      tr("Browse For Read"),
      QString::fromStdString(t.string()),
      QString::fromStdString(extension)
    );
    
    if (fileName.isEmpty())
      return;
    
    bfs::path p = fileName.toStdString();
    prf::manager().rootPtr->project().lastDirectory() = p.remove_filename().string();
    prf::manager().saveConfig();
    
    lineEdit->setText(fileName);
  }
  else if (parameter->getPathType() == prm::PathType::Write)
  {
    if (!bfs::exists(t.parent_path()))
    {
      t = prf::manager().rootPtr->project().lastDirectory().get();
      t /= "file" + extension;
    }
    
    QString fileName = QFileDialog::getSaveFileName
    (
      this,
      tr("Browse For Write"),
      QString::fromStdString(t.string()),
      QString::fromStdString(extension)
    );
    
    if (fileName.isEmpty())
      return;
    
    bfs::path p = fileName.toStdString();
    prf::manager().rootPtr->project().lastDirectory() = p.remove_filename().string();
    prf::manager().saveConfig();
    
    lineEdit->setText(fileName);
  }
}

void Parameter::vectorChangedSlot()
{
  //block value signal.
  prm::ObserverBlocker block(*pObserver);
  
  QLineEdit *xLineEdit = editWidget->findChild<QLineEdit*>(QString("XLineEdit"));
  assert(xLineEdit); if (!xLineEdit) return;
  
  QLineEdit *yLineEdit = editWidget->findChild<QLineEdit*>(QString("YLineEdit"));
  assert(yLineEdit); if (!yLineEdit) return;
  
  QLineEdit *zLineEdit = editWidget->findChild<QLineEdit*>(QString("ZLineEdit"));
  assert(zLineEdit); if (!zLineEdit) return;
  
  osg::Vec3d out
  (
    xLineEdit->text().toDouble(),
    yLineEdit->text().toDouble(),
    zLineEdit->text().toDouble()
  );
  
  if(parameter->setValue(out))
  {
    QString buffer;
    QTextStream vStream(&buffer);
    gu::osgVectorOut(vStream, static_cast<osg::Vec3d>(*parameter));
    
    std::ostringstream gitStream;
    gitStream
    << QObject::tr("Feature: ").toStdString() << feature->getName().toStdString()
    << QObject::tr("    Parameter ").toStdString() << parameter->getName().toStdString();
    gitStream  << QObject::tr("    changed to: ").toStdString() << buffer.toStdString();
    node->send(msg::buildGitMessage(gitStream.str()));
    if (prf::manager().rootPtr->dragger().triggerUpdateOnFinish())
      node->send(msg::Mask(msg::Request | msg::Project | msg::Update));
  }
}

void Parameter::intChangedSlot()
{
  //block value signal.
  prm::ObserverBlocker block(*pObserver);
  
  QLineEdit *lineEdit = dynamic_cast<QLineEdit*>(editWidget);
  assert(lineEdit);
  if (!lineEdit)
    return;
  
  int tv = lineEdit->text().toInt(); //temp value;
  if (parameter->isValidValue(tv))
  {
    if (parameter->setValue(tv))
    {
      std::ostringstream gitStream;
      gitStream
      << QObject::tr("Feature: ").toStdString() << feature->getName().toStdString()
      << QObject::tr("    Parameter ").toStdString() << parameter->getName().toStdString();
      gitStream  << QObject::tr("    changed to: ").toStdString() << tv;
      node->send(msg::buildGitMessage(gitStream.str()));
      if (prf::manager().rootPtr->dragger().triggerUpdateOnFinish())
        node->send(msg::Mask(msg::Request | msg::Project | msg::Update));
    }
  }
  
  //if we fail to set the value, we need to reset the editline.
  //connection is blocked, so in any case we need to update the edit line.
  lineEdit->setText(QString::number(static_cast<int>(*parameter)));
  lineEdit->selectAll();
}

void Parameter::intChangedSlot(int currentIndex)
{
  if (currentIndex == static_cast<int>(*parameter))
    return;
  
  //block value signal.
  prm::ObserverBlocker block(*pObserver);
  
  QComboBox *cb = dynamic_cast<QComboBox*>(editWidget);
  assert(cb);
  if (!cb)
    return;
  
  assert(parameter->isValidValue(currentIndex));
  if (parameter->setValue(currentIndex))
  {
    std::ostringstream gitStream;
    gitStream
    << QObject::tr("Feature: ").toStdString() << feature->getName().toStdString()
    << QObject::tr("    Parameter ").toStdString() << parameter->getName().toStdString()
    << QObject::tr("    changed to: ").toStdString() << parameter->getEnumerationString().toStdString();
    node->send(msg::buildGitMessage(gitStream.str()));
    if (prf::manager().rootPtr->dragger().triggerUpdateOnFinish())
      node->send(msg::Mask(msg::Request | msg::Project | msg::Update));
  }
}
