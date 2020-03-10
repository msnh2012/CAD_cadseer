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

#ifndef DLG_SELECTIONLIST_H
#define DLG_SELECTIONLIST_H

#include <QTableView>
#include <QAbstractTableModel>
#include <QStyledItemDelegate>

// class QLayout;

namespace dlg
{
  class SelectionButton;
  
  class SelectionModel : public QAbstractTableModel
  {
    Q_OBJECT
  public:
    SelectionModel(QObject*, SelectionButton*);
    int rowCount(const QModelIndex&) const override;
    int columnCount(const QModelIndex&) const override;
    QVariant data(const QModelIndex&, int) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    
    const SelectionButton* getButton(){return button;}
    bool accrueEnabled = true;
    
  Q_SIGNALS:
    void accrueChanged();
  public Q_SLOTS:
    void populateList();
  private:
    SelectionButton *button;
  };
  
  class SelectionView : public QTableView
  {
    Q_OBJECT
  public:
    SelectionView(QWidget *);
    SelectionView(QWidget*, QLayout*);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
  public Q_SLOTS:
    void clickedSlot(const QModelIndex&);
    void reset() override;
  private:
    QLayout *layout = nullptr;
  };
  
//! @brief Delegate for editing accrue types.
class AccrueDelegate : public QStyledItemDelegate
{
  Q_OBJECT
public:
  //! Parent must be the table view.
  explicit AccrueDelegate(QObject *parent);
  //! Creates the editor.
  virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const;
  //! Fill editor with text to edit.
  virtual void setEditorData(QWidget* editor, const QModelIndex& index) const;
  //! Match editor to cell size.
  virtual void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const;
  //! Set the model data or re-edit upon failure.
  virtual void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const;
};
}

#endif //DLG_SELECTIONLIST_H
