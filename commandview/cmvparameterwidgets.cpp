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
#include "commandview/cmvcsyswidget.h"
#include "commandview/cmvtrafficsignal.h"
#include "commandview/cmvexpressionedit.h"
#include "commandview/cmvparameterwidgets.h"

using boost::uuids::uuid;

using namespace cmv;

struct cmv::BoolCombo::Stow
{
  BoolCombo *parentWidget;
  prm::Parameter *parameter;
  prm::Observer pObserver;
  QComboBox *comboBox;
  cmv::trf::Signal *trafficSignal;
  
  Stow() = delete;
  Stow(BoolCombo *pwIn, prm::Parameter *pIn)
  : parentWidget(pwIn)
  , parameter(pIn)
  , pObserver
  (
    std::bind(&Stow::valueChanged, this)
    , std::bind(&Stow::constantChanged, this)
    , std::bind(&Stow::activeChanged, this)
  )
  {
    assert(parameter->getValueType() == typeid(bool));
    buildGui();
    //no need to call valueChanged. Constant will do it all
    constantChanged();
    activeChanged();
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
    
    trafficSignal = new cmv::trf::Signal(parentWidget, parameter);
    trafficSignal->setMinimumSize(1, 1);
    trafficSignal->setScaledContents(true);
    trafficSignal->setFixedSize(comboBox->height(), comboBox->height());
    layout->addWidget(trafficSignal);
  }
  void valueChanged()
  {
    bool value = parameter->getBool();
    if (value)
      comboBox->setCurrentIndex(0);
    else
      comboBox->setCurrentIndex(1);
  }
  
  void constantChanged()
  {
    if (parameter->isConstant())
      comboBox->setEnabled(true);
    else
      comboBox->setDisabled(true);
    valueChanged();
  }
  
  void activeChanged()
  {
    if (parameter->isActive())
      comboBox->setEnabled(true);
    else
      comboBox->setDisabled(true);
  }
};

BoolCombo::BoolCombo(QWidget *parent, prm::Parameter *pIn)
: ParameterBase(parent)
, stow(std::make_unique<Stow>(this, pIn))
{
  stow->comboBox->installEventFilter(this); //see ParameterBase source.
}

BoolCombo::~BoolCombo() = default;

void BoolCombo::currentChangedSlot(int index)
{
  bool newValue = (index == 0) ? true : false;
  assert(stow->parameter->isValidValue(newValue));
  prm::ObserverBlocker blocker(stow->pObserver);
  if (stow->parameter->setValue(newValue))
    prmValueChanged();
}

struct cmv::EnumCombo::Stow
{
  EnumCombo *parentWidget;
  prm::Parameter *parameter;
  prm::Observer pObserver;
  QComboBox *comboBox;
  cmv::trf::Signal *trafficSignal;
  
  Stow() = delete;
  Stow(EnumCombo *pwIn, prm::Parameter *pIn)
  : parentWidget(pwIn)
  , parameter(pIn)
  , pObserver
  (
    std::bind(&Stow::valueChanged, this)
    , std::bind(&Stow::constantChanged, this)
    , std::bind(&Stow::activeChanged, this)
  )
  {
    assert(parameter->getValueType() == typeid(int));
    assert(parameter->isEnumeration());
    buildGui();
    //no need to call valueChanged. Constant will do it all
    constantChanged();
    activeChanged();
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
    
    trafficSignal = new cmv::trf::Signal(parentWidget, parameter);
    trafficSignal->setMinimumSize(1, 1);
    trafficSignal->setScaledContents(true);
    trafficSignal->setFixedSize(comboBox->height(), comboBox->height());
    layout->addWidget(trafficSignal);
  }
  void valueChanged()
  {
    int value = parameter->getInt();
    assert(value >= 0 && value < comboBox->count());
    comboBox->setCurrentIndex(value);
  }
  
  void constantChanged()
  {
    if (parameter->isConstant())
      comboBox->setEnabled(true);
    else
      comboBox->setDisabled(true);
    valueChanged();
  }
  
  void activeChanged()
  {
    if (parameter->isActive())
      comboBox->setEnabled(true);
    else
      comboBox->setDisabled(true);
  }
};

EnumCombo::EnumCombo(QWidget *parent, prm::Parameter *pIn)
: ParameterBase(parent)
, stow(std::make_unique<Stow>(this, pIn))
{
  stow->comboBox->installEventFilter(this); //see ParameterBase source.
}

EnumCombo::~EnumCombo() = default;

void EnumCombo::currentChangedSlot(int index)
{
  assert(stow->parameter->isValidValue(index));
  prm::ObserverBlocker blocker(stow->pObserver);
  if (stow->parameter->setValue(index))
    prmValueChanged();
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
  , pObserver
  (
    std::bind(&Stow::valueChanged, this)
    , std::bind(&Stow::constantChanged, this)
    , std::bind(&Stow::activeChanged, this)
  )
  {
    assert(parameter->getValueType() == typeid(std::string));
    buildGui();
    //no need to call valueChanged. Constant will do it all
    constantChanged();
    activeChanged();
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
    lineEdit->setText(parameter->adaptToQString());
  }
  
  void constantChanged() //not needed as strings can't be linked, but oh well.
  {
    if (parameter->isConstant())
      lineEdit->setEnabled(true);
    else
      lineEdit->setDisabled(true);
    valueChanged();
  }
  
  void activeChanged()
  {
    if (parameter->isActive())
      lineEdit->setEnabled(true);
    else
      lineEdit->setDisabled(true);
  }
};

StringEdit::StringEdit(QWidget *parent, prm::Parameter *pIn)
: ParameterBase(parent)
, stow(std::make_unique<Stow>(this, pIn))
{
  stow->lineEdit->installEventFilter(this); //see ParameterBase source.
}

StringEdit::~StringEdit() = default;

void StringEdit::finishedEditingSlot()
{
  QString value = stow->lineEdit->text();
  prm::ObserverBlocker blocker(stow->pObserver);
  if (stow->parameter->setValue(value.toStdString()))
    prmValueChanged();
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
  , pObserver
  (
    std::bind(&Stow::valueChanged, this)
    , std::bind(&Stow::constantChanged, this)
    , std::bind(&Stow::activeChanged, this)
  )
  {
    assert(parameter->getValueType() == typeid(boost::filesystem::path));
    buildGui();
    //no need to call valueChanged. Constant will do it all
    constantChanged();
    activeChanged();
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
    lineEdit->setText(parameter->adaptToQString());
  }
  
  void constantChanged() //not needed as paths can't be linked, but oh well.
  {
    if (parameter->isConstant())
      lineEdit->setEnabled(true);
    else
      lineEdit->setDisabled(true);
    valueChanged();
  }
  
  void activeChanged()
  {
    if (parameter->isActive())
    {
      lineEdit->setEnabled(true);
      button->setEnabled(true);
    }
    else
    {
      lineEdit->setDisabled(true);
      button->setDisabled(true);
    }
  }
};

PathEdit::PathEdit(QWidget *parent, prm::Parameter *pIn)
: ParameterBase(parent)
, stow(std::make_unique<Stow>(this, pIn))
{
  stow->lineEdit->installEventFilter(this); //see ParameterBase source.
}

PathEdit::~PathEdit() = default;

void PathEdit::finishedEditingSlot()
{
  QString value = stow->lineEdit->text();
  boost::filesystem::path p(value.toStdString());
  if (boost::filesystem::exists(p))
  {
    prm::ObserverBlocker blocker(stow->pObserver);
    if (stow->parameter->setValue(boost::filesystem::path(value.toStdString())))
      prmValueChanged();
  }
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
    stow->parameter->setValue(op);
}

//build the appropriate widget editor.
namespace cmv
{
  struct WidgetFactoryVisitor
  {
    WidgetFactoryVisitor() = delete;
    WidgetFactoryVisitor(QWidget *pIn, prm::Parameter* prmIn)
    : parent(pIn)
    , parameter(prmIn)
    {assert(parent); assert(parameter);}
    ParameterBase* operator()(double) const {return buildDouble();}
    ParameterBase* operator()(int) const {return buildInt();}
    ParameterBase* operator()(bool) const {return buildBool();}
    ParameterBase* operator()(const std::string&) const {return buildString();}
    ParameterBase* operator()(const boost::filesystem::path&) const {return buildPath();}
    ParameterBase* operator()(const osg::Vec3d&) const {return buildVec3d();}
    ParameterBase* operator()(const osg::Quat&) const {return buildQuat();}
    ParameterBase* operator()(const osg::Matrixd&) const {return buildMatrix();}
    ParameterBase* operator()(const ftr::Picks&) const {return new ParameterBase(parent);}
    //we have to convert picks to messages for pick widget, so we treat separately
    //and we should not call this visitor on picks.
    
  private:
    QWidget *parent;
    prm::Parameter* parameter;
    
    ParameterBase* buildDouble() const
    {
      return new ParameterEdit(parent, parameter);
    }
      
    ParameterBase* buildInt() const
    {
      ParameterBase *widgetOut = nullptr;
      
      if (parameter->isEnumeration())
        widgetOut = new EnumCombo(parent, parameter);
      else
        widgetOut = new ParameterEdit(parent, parameter);
      
      assert(widgetOut);
      return widgetOut;
    }
    
    ParameterBase* buildBool() const
    {
      return new BoolCombo(parent, parameter);
    }
    
    ParameterBase* buildString() const
    {
      return new StringEdit(parent, parameter);
    }
    
    ParameterBase* buildPath() const
    {
      return new PathEdit(parent, parameter);
    }
    
    ParameterBase* buildVec3d() const
    {
      ParameterEdit *ee = new ParameterEdit(parent, parameter);
      return ee;
    }
    
    ParameterBase* buildQuat() const
    {
      ParameterEdit *ee = new ParameterEdit(parent, parameter);
      return ee;
    }
    
    ParameterBase* buildMatrix() const
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
      ParameterBase *pWidget = std::visit(v, p->getStow().variant);
      layout->addWidget(pWidget, row++, column--); //don't use alignment or won't stretch to fill area.
      QObject::connect(pWidget, &ParameterBase::prmValueChanged, parentWidget, &ParameterWidget::prmValueChanged);
      QObject::connect(pWidget, &ParameterBase::prmConstantChanged, parentWidget, &ParameterWidget::prmConstantChanged);
    }
  }
  
  boost::optional<ParameterBase*> findWidget(prm::Parameter *pIn)
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
    auto out = dynamic_cast<ParameterBase*>(widget);
    assert(out);
    if (!out)
      return boost::none;
    return out;
  }
};

ParameterWidget::ParameterWidget(QWidget *parent, const std::vector<prm::Parameter*> &psIn)
: ParameterBase(parent)
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

ParameterBase* ParameterWidget::getWidget(prm::Parameter *pIn)
{
  auto widget = stow->findWidget(pIn);
  assert(widget);
  if (!widget)
    return nullptr;
  return widget.get();
}

ParameterBase* ParameterWidget::buildWidget(QWidget *parent, prm::Parameter *pIn)
{
  cmv::WidgetFactoryVisitor v(parent, pIn);
  ParameterBase *pWidget = std::visit(v, pIn->getStow().variant);
  return pWidget;
}

namespace
{
  class TableModel : public QAbstractTableModel
  {
  public:
    TableModel(QObject *parent)
    : QAbstractTableModel(parent)
    {}
    int addParameter(prm::Parameter *pIn)
    {
      int index = static_cast<int>(parameters.size());
      beginInsertRows(QModelIndex(), index, index);
      parameters.push_back(pIn);
      endInsertRows();
      return static_cast<int>(parameters.size()) - 1;
    }
    void removeParameter(int index)
    {
      beginRemoveRows(QModelIndex(), index, index);
      auto it = parameters.begin() + index;
      parameters.erase(it);
      endRemoveRows();
    }
    prm::Parameter* getParameter(int index) const
    {
      assert(index >= 0 && index < static_cast<int>(parameters.size()));
      return parameters.at(index);
    }
    int rowCount(const QModelIndex&) const override
    {
      return static_cast<int>(parameters.size());
    }
    int columnCount(const QModelIndex&) const override
    {
      return 2;
    }
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
      if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
      {
        if (section == 0)
          return tr("Name");
        if (section == 1)
          return tr("Value");
      }
      return QAbstractTableModel::headerData(section, orientation, role);
    }
    QVariant data(const QModelIndex &index, int role) const override
    {
      if (!index.isValid())
        return QVariant();
      if (role == Qt::DisplayRole)
      {
        assert(index.row() >= 0 && index.row() < static_cast<int>(parameters.size()));
        const auto *p = parameters.at(index.row());
        if (index.column() == 0)
          return p->getName();
        if (index.column() == 1)
          return p->adaptToQString();
      }
      return QVariant();
    }
    bool setData(const QModelIndex &index, const QVariant&, int) override
    {
      dataChanged(index, index);
      return true;
    }
    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
      Qt::ItemFlags out = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
      if (index.column() == 1)
        out |= Qt::ItemIsEditable;
      return out;
    }
  private:
    std::vector<prm::Parameter*> parameters;
  };
  
  
  
  
    /*! @brief Delegate expression editor
   * 
   * Again!
   */
  class ExpressionDelegate : public QStyledItemDelegate
  {
  public:
    explicit ExpressionDelegate(ParameterTable *parent)
    : QStyledItemDelegate(parent)
    , table(parent){}
    
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex &index) const override
    {
      prm::Parameter *p = static_cast<const TableModel*>(index.model())->getParameter(index.row());
      cmv::WidgetFactoryVisitor v(parent, p);
      return std::visit(v, p->getStow().variant);
    }

    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex&) const override
    {
      editor->setGeometry(option.rect);
    }
    
    void setEditorData(QWidget*, const QModelIndex&) const override
    {
      //shouldn't need to do anything here as we initialized everything when we created
    }

    void setModelData(QWidget*, QAbstractItemModel* model, const QModelIndex& index) const override
    {
      if (!index.isValid())
        return;
      
      //right now we are only using doubles/expression edits in parameter table.
      //the editor itself should set the value of the parameter. so here
      //all we need to do is trick the model into thinking the value has changed.
      TableModel *tm = dynamic_cast<TableModel*>(model);
      assert(tm);
      QVariant dummy;
      tm->setData(index, dummy, Qt::EditRole);
      table->prmValueChanged();
    }
  private:
    ParameterTable *table = nullptr;
  };
}

struct cmv::ParameterTable::Stow
{
  ParameterTable *parentWidget;
  TableModel *model;
  QTableView *view;
  
  Stow() = delete;
  Stow(ParameterTable *parentWidgetIn)
  : parentWidget(parentWidgetIn)
  {
    buildGui();
  }
  
  void buildGui()
  {
    QVBoxLayout *layout = new QVBoxLayout();
    parentWidget->setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    parentWidget->setContentsMargins(0, 0, 0, 0);
    parentWidget->setSizePolicy(parentWidget->sizePolicy().horizontalPolicy(), QSizePolicy::Maximum);
    
    model = new TableModel(parentWidget);
    view = new QTableView(parentWidget);
    view->setModel(model);
    view->setSizePolicy(view->sizePolicy().horizontalPolicy(), QSizePolicy::Maximum);
    view->horizontalHeader()->setStretchLastSection(true);
    view->verticalHeader()->setVisible(false);
    view->setSelectionBehavior(QAbstractItemView::SelectItems);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    connect (view->selectionModel(), &QItemSelectionModel::selectionChanged, parentWidget, &ParameterTable::selectionHasChanged);
    layout->addWidget(view);
    
    ExpressionDelegate *ed = new ExpressionDelegate(parentWidget);
    view->setItemDelegateForColumn(1, ed);
  }
};

ParameterTable::ParameterTable(QWidget *parentIn)
: ParameterBase(parentIn)
, stow(std::make_unique<Stow>(this))
{}

ParameterTable::~ParameterTable() = default;

void ParameterTable::addParameter(prm::Parameter *p)
{
  int row = stow->model->addParameter(p);
  auto *sm = stow->view->selectionModel();
  assert(sm);
  QModelIndex mi = stow->model->index(row, 1);
  assert(mi.isValid());
  auto flags = QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent;
  sm->select(mi, flags);
}

void ParameterTable::removeParameter(int index)
{
  stow->model->removeParameter(index);
}

int ParameterTable::getCurrentSelectedIndex()
{
  if (!stow->view->selectionModel()->hasSelection())
    return -1;
  auto indexList = stow->view->selectionModel()->selectedIndexes();
  if (indexList.isEmpty())
    return -1;
  return indexList.front().row();
}

void ParameterTable::setCurrentSelectedIndex(int index)
{
  if (index >= stow->model->rowCount(QModelIndex()) || index < 0)
    return;
  stow->view->selectionModel()->select(stow->model->index(index, 1), QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent);
}

void ParameterTable::selectionChanged(const QItemSelection&, const QItemSelection&)
{
  emit selectionHasChanged();
}
