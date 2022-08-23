/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2022 Thomas S. Anderson blobfish.at.gmx.com
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

#include <QListView>
#include <QAbstractListModel>
#include <QHBoxLayout>
#include <QMimeData>

#include "dialogs/dlgsplitterdecorated.h"
#include "commandview/cmvdraglist.h"

/* I have no idea how to get qt's model view to remove the list item when dragging
 * from the ListModel to the ProtoModel. Gave up trying to get the magic combination
 * of settings and overloaded functions between the model and view. I just added
 * a function object to remove the item from the ListModel in the ProtoModel.
 */

namespace
{
  class ProtoModel : public QAbstractListModel
  {
  public:
    QStringList protoList;
    QString uniqueName;
    std::function<void(int)> removeFromList;
    
    ProtoModel(QObject *pIn, const QStringList &lIn, const QString &uniqueNameIn)
    : QAbstractListModel(pIn)
    , protoList(lIn)
    , uniqueName(uniqueNameIn){}
    
    int rowCount(const QModelIndex& = QModelIndex()) const override
    {
      return protoList.size();
    }
    
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
      if (!index.isValid()) return QVariant();
      if (role == Qt::DisplayRole) return protoList.at(index.row());
      if (role == Qt::EditRole) return index.row();
      return QVariant();
    }
    
    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
      //works for both because we disable dropping on the master view
      if (!index.isValid())
        return Qt::ItemIsDropEnabled; //this allows dropping on view!
      return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    }
    
    QStringList mimeTypes() const override
    {
      QStringList types;
      types << "text/plain";
      return types;
    }
    
    QMimeData* mimeData(const QModelIndexList &indexes) const override
    {
      QMimeData *mimeData = new QMimeData();
      QString out;
      if (!indexes.isEmpty())
      {
        out += uniqueName + "_proto_" + ";";
        out += data(indexes.front(), Qt::EditRole).toString();
      }
      mimeData->setText(out);
      return mimeData;
    }
    
    Qt::DropActions supportedDropActions() const override
    {
      return Qt::CopyAction;
    }
    
    int decode(const QMimeData *data) const
    {
      if (!data || !data->hasText()) return -1;
      if (!data->text().startsWith(uniqueName + "_list_" + ";")) return -1;
      auto split = data->text().split(";");
      if (split.size() != 2) return -1;
      int index = split.at(1).toInt();
      if (index < 0 || index >= protoList.size()) return -1;
      return index;
    }
    
    bool canDropMimeData(const QMimeData *data, Qt::DropAction, int, int, const QModelIndex&) const override
    {
      int decoded = decode(data);
      if (decoded != -1)
        return true;
      return false;
    }
    
    bool dropMimeData(const QMimeData * data, Qt::DropAction, int, int, const QModelIndex&) override
    {
      int decoded = decode(data);
      if (decoded == -1)
        return false;
      assert(removeFromList);
      removeFromList(decoded);
      return true;
    }
  };
  
  class ListModel : public QAbstractListModel
  {
  public:
    QStringList protoList;
    QString uniqueName;
    std::vector<int> refList;
    
    ListModel(QObject *pIn, const QStringList &listIn, const QString &uniqueNameIn)
    : QAbstractListModel(pIn)
    , protoList(listIn)
    , uniqueName(uniqueNameIn){}
    
    void setList(const QStringList &slIn)
    {
      beginResetModel();
      refList.clear();
      for (const auto &s : slIn)
      {
        int index = protoList.indexOf(s);
        if (index != -1)
          refList.push_back(index);
      }
      endResetModel();
    }
    
    int rowCount(const QModelIndex& = QModelIndex()) const override
    {
      return refList.size();
    }
    
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
      if (!index.isValid())
        return QVariant();
      if (role == Qt::DisplayRole)
        return protoList.at(refList.at(index.row()));
      if (role == Qt::EditRole)
        return refList.at(index.row());
      return QVariant();
    }
    
    bool insertRows(int row, int count, const QModelIndex&) override
    {
      beginInsertRows(QModelIndex(), row, row + count - 1);
      refList.insert(refList.begin() + row, count, 0);
      endInsertRows();
      return true;
    }
    
    bool removeRows(int row, int count, const QModelIndex&) override
    {
      beginRemoveRows(QModelIndex(), row, row + count - 1);
      refList.erase(refList.begin() + row, refList.begin() + row + count);
      endRemoveRows();
      return true;
    }
    
    bool setData(const QModelIndex& index, const QVariant &value, int role) override
    {
      if (!index.isValid() || role != Qt::EditRole)
        return false;
      refList[index.row()] = value.toInt();
      emit dataChanged(index, index, {role});
      return true;
    }
    
    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
      if (index.isValid())
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
      return Qt::ItemIsDropEnabled;
    }
    
    QStringList mimeTypes() const override
    {
      QStringList types;
      types << "text/plain";
      return types;
    }
    
    QMimeData* mimeData(const QModelIndexList &indexes) const override
    {
      QMimeData *mimeData = new QMimeData();
      QString out;
      if (!indexes.isEmpty())
      {
        out += uniqueName + "_list_" + ";";
        out += QString::number(indexes.front().row());
      }
      mimeData->setText(out);
      return mimeData;
    }
    
    Qt::DropActions supportedDropActions() const override
    {
      return Qt::CopyAction;
    }
    
    int decode(const QMimeData *data) const
    {
      if (!data || !data->hasText()) return -1;
      if (!data->text().startsWith(uniqueName + "_proto_" + ";")) return -1;
      auto split = data->text().split(";");
      if (split.size() != 2) return -1;
      int index = split.at(1).toInt();
      if (index < 0 || index >= protoList.size()) return -1;
      
      return index;
    }
    
    bool canDropMimeData(const QMimeData *data, Qt::DropAction, int, int, const QModelIndex&) const override
    {
      if (decode(data) != -1)
        return true;
      return false;
    }
    
    bool dropMimeData(const QMimeData *data, Qt::DropAction, int row, int, const QModelIndex&) override
    {
      int decoded = decode(data);
      if (decoded == -1) return false;
      if (row < 0 || row > rowCount()) row = rowCount();
      if (insertRow(row))
        return setData(index(row, 0), decoded, Qt::EditRole);
      return false;
    }
    
    QStringList getStrings()
    {
      QStringList out;
      for (auto index : refList)
        out.push_back(protoList.at(index));
      return out;
    }
    
    void externalRemove(int row)
    {
      if (row < 0 || row >= static_cast<int>(refList.size())) return;
      removeRow(row);
    }
  };
}

using namespace cmv;

struct DragList::Stow
{
  DragList *parent = nullptr;
  ProtoModel *protoModel = nullptr;
  QListView *protoView = nullptr;
  ListModel *listModel = nullptr;
  QListView *listView = nullptr;
  dlg::SplitterDecorated *splitter = nullptr;
  
  Stow(DragList *parentIn, const QStringList &protoIn, const QString &uniqueNameIn)
  : parent(parentIn)
  , protoModel(new ProtoModel(parent, protoIn, uniqueNameIn))
  , listModel(new ListModel(parent, protoIn, uniqueNameIn))
  {
    /* had to trigger rowsInserted on dataChanged so the value would be set
     * before launching any updates */
    buildGui();
    protoModel->removeFromList = std::bind(&ListModel::externalRemove, listModel, std::placeholders::_1);
    QObject::connect(listModel, &ListModel::dataChanged, parent, &DragList::rowsInserted);
    QObject::connect(listModel, &ListModel::rowsRemoved, parent, &DragList::rowsRemoved);
    
    splitter->restoreSettings(uniqueNameIn + "::splitter");
  }
  
  void buildGui()
  {
    QHBoxLayout *mainLayout = new QHBoxLayout();
    parent->setLayout(mainLayout);
    parent->setContentsMargins(0, 0, 0, 0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    splitter = new dlg::SplitterDecorated();
    splitter->setOrientation(Qt::Horizontal);
    splitter->setChildrenCollapsible(false);
    mainLayout->addWidget(splitter);
    
    protoView = new QListView(parent);
    protoView->setModel(protoModel);
    protoView->setDragEnabled(true);
    protoView->setAcceptDrops(true);
    protoView->setDropIndicatorShown(true);
    protoView->setDefaultDropAction(Qt::CopyAction);
    protoView->setToolTip(QObject::tr("Prototype List"));
    protoView->setWhatsThis(QObject::tr("List Of All Possible Types"));
    
    listView = new QListView(parent);
    listView->setModel(listModel);
    listView->setDragEnabled(true);
    listView->setAcceptDrops(true);
    listView->setDropIndicatorShown(true);
    listView->setDefaultDropAction(Qt::MoveAction);
    listView->setToolTip(QObject::tr("Designation List"));
    listView->setWhatsThis(QObject::tr("List Of Designated"));
    
    splitter->addWidget(listView);
    splitter->addWidget(protoView);
  }
};

DragList::DragList(QWidget *parent, const QStringList &pIn, const QString &uniqueNameIn)
: QWidget(parent)
, stow(std::make_unique<Stow>(this, pIn, uniqueNameIn)){}

DragList::~DragList() = default;

void DragList::setList(const QStringList &lIn)
{
  //making sure passed in strings are present in proto list.
  QStringList filtered;
  for (const auto &sIn : lIn)
  {
    if (!stow->protoModel->protoList.contains(sIn))
      continue;
    filtered.append(sIn);
  }
  stow->listModel->setList(filtered);
}

void DragList::setList(const std::vector<int> &iIn)
{
  const auto &pl = stow->protoModel->protoList;
  QStringList filtered;
  for (auto i : iIn)
  {
    if (i < 0 || i >= pl.size())
      continue;
    filtered.append(pl.at(i));
  }
  stow->listModel->setList(filtered);
}

QStringList DragList::getStringList()
{
  return stow->listModel->getStrings();
}

std::vector<int> DragList::getIndexList()
{
  return stow->listModel->refList;
}

void DragList::rowsInserted(const QModelIndex &first, const QModelIndex&, const QVector<int>&)
{
  added(first.row());
}

void DragList::rowsRemoved(const QModelIndex&, int first, int)
{
  removed(first);
}

QListView* DragList::getListView()
{
  return stow->listView;
}

QListView* DragList::getProtoView()
{
  return stow->protoView;
}
