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

#include <QHeaderView>
#include <QComboBox>
#include <QLayout>

#include "tools/idtools.h"
#include "selection/slcaccrue.h"
#include "dialogs/dlgselectionbutton.h"
#include "dialogs/dlgselectionlist.h"

using namespace dlg;

static QString makeShortString(const slc::Message &m)
{
  QString out = QString::fromStdString(gu::idToShortString(m.featureId));
  if (!m.shapeId.is_nil())
    out += ":" + QString::fromStdString(gu::idToShortString(m.shapeId));
  return out;
}

static QString makeFullString(const slc::Message &m)
{
  QString out = QString::fromStdString(gu::idToString(m.featureId));
  if (!m.shapeId.is_nil())
    out += ":" + QString::fromStdString(gu::idToString(m.shapeId));
  return out;
}

SelectionModel::SelectionModel(QObject *parent, SelectionButton *buttonIn)
: QAbstractTableModel(parent)
, button(buttonIn)
{
  connect(button, &SelectionButton::dirty, this, &SelectionModel::populateList);
}

int SelectionModel::rowCount(const QModelIndex&) const
{
  return static_cast<int>(button->getMessages().size());
}

int SelectionModel::columnCount(const QModelIndex&) const
{
  return 2;
}

QVariant SelectionModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid())
    return QVariant();
  if (index.column() == 0)
  {
    if (role == Qt::DisplayRole)
      return makeShortString(button->getMessages().at(index.row()));
    if (role == Qt::ToolTipRole)
      return makeFullString(button->getMessages().at(index.row()));
  }
  else //column == 1
  {
    slc::Accrue accrue(button->getMessages().at(index.row()).accrue);
    if (role == Qt::DisplayRole)
      return static_cast<QString>(accrue);
    if (role == Qt::EditRole)
      return static_cast<int>(accrue);
  }
  return QVariant();
}

Qt::ItemFlags SelectionModel::flags(const QModelIndex &index) const
{
  if (!index.isValid())
    return Qt::ItemFlags();
  if (index.column() == 0)
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  //column == 1
  if (accrueEnabled)
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
  return Qt::ItemFlags();
}

bool SelectionModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  if (!index.isValid())
    return false;
  
  auto goChanged = [&]()
  {
    if (button->isChecked())
      button->syncToSelection();
    dataChanged(index, index);
  };
  
  if (role == Qt::EditRole)
  {
    button->setAccrue(index.row(), slc::Accrue(value.toInt()));
    accrueChanged();
    goChanged();
    return true;
  }
  else if (role == Qt::UserRole)
  {
    button->setAngle(index.row(), value.toDouble());
    goChanged();
    return true;
  }
  
  return QAbstractTableModel::setData(index, value, role);
}

void SelectionModel::populateList()
{
  beginResetModel();
  endResetModel();
}

SelectionView::SelectionView(QWidget *parent)
: QTableView(parent)
{
  this->horizontalHeader()->hide();
  this->verticalHeader()->hide();
  
  QSizePolicy adjust(sizePolicy());
  adjust.setVerticalPolicy(QSizePolicy::Expanding);
  setSizePolicy(adjust);
  
  setSelectionMode(QAbstractItemView::SingleSelection);
  connect(this, &QTableView::clicked, this, &SelectionView::clickedSlot);
}

SelectionView::SelectionView(QWidget *parent, QLayout *lIn)
: SelectionView(parent)
{
  layout = lIn;
}

void SelectionView::clickedSlot(const QModelIndex &index)
{
  if (!index.isValid())
    return;
  auto *m = dynamic_cast<SelectionModel*>(model());
  assert(m);
  if (m->flags(index) & Qt::ItemIsSelectable)
    m->getButton()->highlightIndex(index.row());
}

void SelectionView::reset()
{
  updateGeometry();
  QTableView::reset();
}

QSize SelectionView::sizeHint() const
{
  //ref
  //https://forum.qt.io/topic/40717/set-size-of-the-qlistview-to-fit-to-it-s-content/6
  
  int heightOut = (layout) ? layout->sizeHint().height() : 32;
  if (model())
  {
    // Determine the vertical space allocated beyond the viewport
    const int extraHeight = height() - viewport()->height() + 2; //extra 2 to ensure no scroll

    // Find the bounding rect of the last list item
    const QModelIndex index = model()->index(model()->rowCount() - 1, 0);
    const QRect r = visualRect(index);

    // Size the widget to the height of the bottom of the last item
    // plus the extra determined earlier
    int temp = r.y() + r.height() + extraHeight;
    heightOut = std::max(temp, heightOut);
  }
  return QSize(0, heightOut);
}

QSize SelectionView::minimumSizeHint() const
{
  QSize out = QTableView::minimumSizeHint();
  if (layout)
    out.setHeight(layout->sizeHint().height());
  
  return out;
}

AccrueDelegate::AccrueDelegate(QObject *parent): QStyledItemDelegate(parent)
{}

QWidget* AccrueDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex&) const
{
  QComboBox *editor = new QComboBox(parent);
  return editor;
}

void AccrueDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
  if (!index.isValid())
    return;
  QComboBox *e = dynamic_cast<QComboBox *>(editor); assert(e);
  
  for (const auto &a : slc::Accrue::accrues)
    e->addItem(static_cast<QString>(a));
  
  const SelectionModel *sm = dynamic_cast<const SelectionModel*>(index.model()); assert(sm);
  e->setCurrentIndex(sm->data(index, Qt::EditRole).toInt());
}

void AccrueDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex&) const
{
  //this is called before setEditorData.
  editor->setGeometry(option.rect);
}

void AccrueDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
  QComboBox *e = dynamic_cast<QComboBox *>(editor); assert(e);
  model->setData(index, e->currentIndex(), Qt::EditRole);
}
