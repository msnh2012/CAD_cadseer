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

#include <QIcon>
#include <QHeaderView>
#include <QMouseEvent>
#include <QTimer>
#include <QStyledItemDelegate>
#include <QComboBox>

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "selection/slcmessage.h"
#include "message/msgmessage.h"
#include "message/msgnode.h"
#include "message/msgsift.h"
#include "tools/idtools.h"
#include "commandview/cmvselectioncue.h"
#include "commandview/cmvselection.h"

using boost::uuids::uuid;

namespace
{
  class TableDelegate : public QStyledItemDelegate
  {
    QStringList items =
    {
      QObject::tr("None")
      , QObject::tr("Tangent")
    };
    
    mutable QComboBox *jfc = nullptr;
  public:
    explicit TableDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
    {}
    
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex&) const override
    {
      jfc = new QComboBox(parent);
      jfc->addItems(items);
      return jfc;
    }
    
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex&) const override
    {
      editor->setGeometry(option.rect);
    }
    
    void accrueChanged(int)
    {
      commitData(jfc);
      closeEditor(jfc);
    }
    
    void setEditorData(QWidget*, const QModelIndex &index) const override
    {
      if (!index.isValid()) return;
      
      int entry = index.model()->data(index, Qt::EditRole).toInt();
      jfc->setCurrentIndex(entry);
      connect(jfc, qOverload<int>(&QComboBox::currentIndexChanged), this, &TableDelegate::accrueChanged);
    }
    
    void setModelData(QWidget*, QAbstractItemModel* model, const QModelIndex& index) const override
    {
      if (!index.isValid())
        return;
      model->setData(index, jfc->currentIndex());
      
      auto *view = dynamic_cast<cmv::edt::View*>(this->parent());
      assert(view);
      view->selectionModel()->clear();
    }
  };
}

using namespace cmv::edt;

struct Model::Stow
{
  Model& model;
  prm::Parameter *parameter;
  slc::Messages messages;
  tbl::SelectionCue cue;
  
  Stow(Model& mIn, prm::Parameter *prmIn, const tbl::SelectionCue &cueIn)
  : model(mIn)
  , parameter(prmIn)
  , cue(cueIn)
  {}
};

Model::Model(QObject *parent, prm::Parameter *prmIn, const tbl::SelectionCue &cueIn)
: QAbstractTableModel(parent)
, stow(std::make_unique<Stow>(*this, prmIn, cueIn))
{}

//can't send selection clear from destructor. Not called when editor closes as expected.
Model::~Model() = default;

int Model::rowCount(const QModelIndex&) const
{
  //passed index is always invalid for table model, so don't test
  return stow->messages.size();
}

int Model::columnCount(const QModelIndex&) const
{
  //passed index is always invalid for table model, so don't test
  if(stow->cue.accrueEnabled)
    return 2;
  return 1;
}

QVariant Model::data(const QModelIndex &indexIn, int role) const
{
  if (!indexIn.isValid())
    return QVariant();
  
  assert(indexIn.row() >= 0 && indexIn.row() < static_cast<int>(stow->messages.size()));
  const auto &msg = stow->messages.at(indexIn.row());
  
  switch (indexIn.column())
  {
    case 0:
    {
      switch (role)
      {
        case Qt::DisplayRole:
          return QString::fromStdString(slc::getNameOfType(msg.type));
        case Qt::DecorationRole:
          switch (msg.type)
          {
            case slc::Type::None:
              return QVariant();
            case slc::Type::Object:
              return QIcon(":/resources/images/selectObjects.svg");
            case slc::Type::Feature:
              return QIcon(":/resources/images/selectFeatures.svg");
            case slc::Type::Solid:
              return QIcon(":/resources/images/selectSolids.svg");
            case slc::Type::Shell:
              return QIcon(":/resources/images/selectShells.svg");
            case slc::Type::Face:
              return QIcon(":/resources/images/selectFaces.svg");
            case slc::Type::Wire:
              return QIcon(":/resources/images/selectWires.svg");
            case slc::Type::Edge:
              return QIcon(":/resources/images/selectEdges.svg");
            case slc::Type::StartPoint:
              return QIcon(":/resources/images/selectVertices.svg");
            case slc::Type::EndPoint:
              return QIcon(":/resources/images/selectPointsEnd.svg");
            case slc::Type::MidPoint:
              return QIcon(":/resources/images/selectPointsMid.svg");
            case slc::Type::CenterPoint:
              return QIcon(":/resources/images/selectPointsCenter.svg");
            case slc::Type::QuadrantPoint:
              return QIcon(":/resources/images/selectPointsQuandrant.svg");
            case slc::Type::NearestPoint:
              return QIcon(":/resources/images/selectPointsNearest.svg");
            case slc::Type::ScreenPoint:
              return QIcon(":/resources/images/selectPointsScreen.svg");
          }
          return QVariant();
        case Qt::ToolTipRole:
          std::ostringstream stream;
          stream << gu::idToShortString(msg.featureId) << ":" << gu::idToShortString(msg.shapeId);
          return QString::fromStdString(stream.str());
      }
      break;
    }
    case 1:
    {
      switch (role)
      {
        case Qt::DisplayRole:
          {return static_cast<QString>(msg.accrue);}
        case Qt::EditRole:
          {return static_cast<int>(msg.accrue);}
      }
    }
  }
  
  return QVariant();
}

Qt::ItemFlags Model::flags(const QModelIndex &indexIn) const
{
  switch (indexIn.column())
  {
    case 0:
      return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    case 1:
      return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
  }
  
  return Qt::NoItemFlags;
}

bool Model::setData(const QModelIndex &indexIn, const QVariant &variant, int role)
{
  if (!indexIn.isValid())
    return false;
  
  if (role == Qt::EditRole && indexIn.column() == 1)
  {
    int row = indexIn.row();
    assert (row >= 0 && row < static_cast<int>(stow->messages.size()));
    stow->messages.at(row).accrue = static_cast<slc::Accrue>(variant.toInt());
    dataChanged(indexIn, indexIn, {Qt::EditRole});
    return true;
  }
  return false;
}

void Model::setMessages(const slc::Messages &msIn)
{
  beginResetModel();
  stow->messages = msIn;
  endResetModel();
}

const slc::Messages& Model::getMessages() const
{
  return stow->messages;
}

const cmv::tbl::SelectionCue& Model::getCue() const
{
  return stow->cue;
}

struct View::Stow
{
  View *view;
  Model *model;
  
  msg::Node node;
  msg::Sift sift;
  
  Stow() = delete;
  Stow(const Stow&) = delete;
  Stow(const Stow&&) = delete;
  Stow& operator=(const Stow&) = delete;
  Stow& operator=(const Stow&&) = delete;
  Stow(View *vIn, Model *mIn) : view(vIn), model(mIn)
  {
    node.connect(msg::hub());
    sift.name = "cmv::edt::view";
    node.setHandler(std::bind(&msg::Sift::receive, &sift, std::placeholders::_1));
    setupDispatcher();
  }
  
  void slcAdded(const msg::Message &mIn)
  {
    if (view->isHidden())
      return;
    if (slc::has(model->stow->messages, mIn.getSLC()))
      return;
    
    auto go = [&]()
    {
      model->beginInsertRows(QModelIndex(), model->stow->messages.size(), model->stow->messages.size());
      model->stow->messages.push_back(mIn.getSLC());
      if (model->getCue().forceTangentAccrue)
      {
        model->stow->messages.back().accrue = slc::Accrue::Tangent;
        syncAll();
      }
      model->endInsertRows();
    };
    
    if (!model->stow->messages.empty() && model->stow->cue.singleSelection)
    {
      assert(model->stow->messages.size() == 1); //how can we have more than 1 in a single selection.
      model->beginResetModel();
      model->stow->messages.clear();
      model->stow->messages.push_back(mIn.getSLC());
      if (model->getCue().forceTangentAccrue)
        model->stow->messages.back().accrue = slc::Accrue::Tangent;
      model->endResetModel();
      syncAll();
      return;
    }
    
    if (view->selectionModel()->hasSelection())
    {
      QSignalBlocker block(view->selectionModel());
      view->selectionModel()->clear();
      go();
      syncAll();
    }
    else
      go();
    
    //empty selections has column widths foobar
    view->resizeRowsToContents();
    view->resizeColumnsToContents();
  }
  
  void slcRemoved(const msg::Message &mIn)
  {
    if (view->isHidden())
      return;
    const auto &slcMsg = mIn.getSLC();
    auto it = std::find(model->stow->messages.begin(), model->stow->messages.end(), slcMsg);
    if (it == model->stow->messages.end())
      return;
    
    auto go = [&]()
    {
      int index = std::distance(model->stow->messages.begin(), it);
      model->beginRemoveRows(QModelIndex(), index, index);
      model->stow->messages.erase(it);
      model->endRemoveRows();
    };
    
    if (view->selectionModel()->hasSelection())
    {
      //only erasing one.
      QSignalBlocker block(view->selectionModel());
      view->selectionModel()->clear();
      go();
      //We crash without the delay as syncAll causes a
      //recurse back to here before all handling of this
      //message has been done.
      QTimer::singleShot(0, [=](){this->syncAll();});
    }
    else
      go(); //don't have to worry about sync when removing all.
  }
  
  void slcMasked(const msg::Message &mIn)
  {
    if (view->isHidden())
      return;
    model->stow->cue.mask = mIn.getSLC().selectionMask;
  }
  
  void setupDispatcher()
  {
    sift.insert
    (
      {
        std::make_pair
        (
          msg::Response | msg::Post | msg::Selection | msg::Add
          , std::bind(&Stow::slcAdded, this, std::placeholders::_1)
        )
        , std::make_pair
        (
          msg::Response | msg::Post | msg::Selection | msg::Remove
          , std::bind(&Stow::slcRemoved, this, std::placeholders::_1)
        )
        , std::make_pair
        (
          msg::Response | msg::Selection | msg::SetMask
          , std::bind(&Stow::slcMasked, this, std::placeholders::_1)
        )
      }
    );
  }
  
  void syncAll() const
  {
    if (model->stow->cue.mask != slc::None)
      node.sendBlocked(msg::buildSelectionMask(model->stow->cue.mask));
    if (!model->stow->cue.statusPrompt.isEmpty())
      node.sendBlocked(msg::buildStatusMessage(model->stow->cue.statusPrompt.toStdString()));
    
    node.sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    for (const auto &m : model->stow->messages)
      node.sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Add, m));
  }
  
  void syncOne(int index) const
  {
    assert (index >= 0 && index < static_cast<int>(model->stow->messages.size()));
    node.sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    node.sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Add, model->stow->messages.at(index)));
  }
};

View::View(QWidget *parent, Model *model)
: QTableView(parent)
, stow(std::make_unique<Stow>(this, model))
{
  setSizePolicy(sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);
  horizontalHeader()->setStretchLastSection(true); //?
  verticalHeader()->setVisible(false);
  horizontalHeader()->setVisible(false);
  setSelectionBehavior(QAbstractItemView::SelectItems);
  setSelectionMode(QAbstractItemView::SingleSelection);
  setModel(model);
  connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &View::selectionHasChanged);
  setItemDelegateForColumn(1, new TableDelegate(this));
  QTimer::singleShot(0, this, &View::resizeRowsToContents);
  QTimer::singleShot(0, this, &View::resizeColumnsToContents);
  QTimer::singleShot(0, [&](){stow->syncAll();});
}

View::~View() = default;

void View::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::MiddleButton)
    return;
    
  QTableView::mousePressEvent(event);
}

void View::mouseReleaseEvent(QMouseEvent *event)
{
  if (event->button() == Qt::MiddleButton)
  {
    //following should cause a syncAll through selectionHasChanged.
    selectionModel()->clear();
    //middle pick seems to be changing selection like a left pick.
    //we don't want that so return
    return;
  }
    
  QTableView::mouseReleaseEvent(event);
}

void View::selectionHasChanged(const QItemSelection &fresh, const QItemSelection&)
{
  //Doc says, we won't get here when model is reset.
  if (fresh.empty())
    stow->syncAll();
  else
    stow->syncOne(fresh.indexes().front().row());
}

bool FocusFilter::eventFilter(QObject *watched, QEvent *event)
{
  //installed on selection persistent editor to prevent triggering 'dataChanged'
  //on losing focus.
  if(event->type() == QEvent::FocusOut)
    return true;

  return QObject::eventFilter(watched, event);
}
