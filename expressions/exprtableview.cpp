/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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

#include <iostream>
#include <assert.h>
#include <fstream>

#include <boost/filesystem.hpp>

#include <QLineEdit>
#include <QHeaderView>
#include <QAction>
#include <QContextMenuEvent>
#include <QMenu>
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QTimer>
#include <QClipboard>
#include <QShowEvent>
#include <QSettings>

#include "application/appapplication.h"
#include "application/appmessage.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "tools/idtools.h"
#include "dialogs/dlgexpressionedit.h"
#include "message/msgmessage.h"
#include "expressions/exprtableview.h"
#include "expressions/exprtablemodel.h"
#include "expressions/exprmanager.h"

namespace
{
  QAction* buildAction(QWidget *parent, std::function<void()> f, const QString &text, const QKeySequence &keys)
  {
    QAction *out = new QAction(text, parent);
    out->setShortcut(keys);
    out->setShortcutContext(Qt::WidgetShortcut);
    parent->addAction(out);
    QObject::connect(out, &QAction::triggered, f);
    return out;
  }
  
  QAction* buildAddScalarAction(QWidget *parent, std::function<void()> f)
  {
    return buildAction(parent, f, QObject::tr("Add Scalar Expression"), QKeySequence(Qt::Key_F9));
  }
  
  QAction* buildAddVectorAction(QWidget *parent, std::function<void()> f)
  {
    return buildAction(parent, f, QObject::tr("Add Vector Expression"), QKeySequence(Qt::Key_F10));
  }
  
  QAction* buildAddRotationAction(QWidget *parent, std::function<void()> f)
  {
    return buildAction(parent, f, QObject::tr("Add Rotation Expression"), QKeySequence(Qt::Key_F11));
  }
  
  QAction* buildAddCSysAction(QWidget *parent, std::function<void()> f)
  {
    return buildAction(parent, f, QObject::tr("Add CSys Expression"), QKeySequence(Qt::Key_F12));
  }
  
  QAction* buildRemoveExpressionAction(QWidget *parent, std::function<void()> f)
  {
    return buildAction(parent, f, QObject::tr("Remove Expression"), QKeySequence(Qt::CTRL + Qt::Key_D));
  }
  
  QAction* buildExportExpressionAction(QWidget *parent, std::function<void()> f)
  {
    return buildAction(parent, f, QObject::tr("Export Expression"), QKeySequence(Qt::CTRL + Qt::Key_E));
  }
  
  QAction* buildImportExpressionAction(QWidget *parent, std::function<void()> f)
  {
    return buildAction(parent, f, QObject::tr("Import Expression"), QKeySequence(Qt::CTRL + Qt::Key_I));
  }
  
  QAction* buildCopyExpressionValueAction(QWidget *parent, std::function<void()> f)
  {
    return buildAction(parent, f, QObject::tr("Copy Value"), QKeySequence(Qt::CTRL + Qt::Key_C));
  }
  
  void reportImportErrors(const std::vector<lcc::Result> &results)
  {
    bool shouldSend = false;
    std::ostringstream errorStream;
    errorStream << std::endl << "Import Errors and Warnings:" << std::endl;
    for (const auto &r : results)
    {
      if (!r.isAllGood())
      {
        shouldSend = true;
        errorStream << "Error: Expression '" << r.input << "'    Failed with: " << r.getError() << std::endl;
        continue;
      }
      if (!r.oldExpression.empty())
      {
        shouldSend = true;
        std::ostringstream warning;
        errorStream << "Warning: Expression '" << r.expressionName << "' Has Been Overwritten";
      }
    }
    if (shouldSend)
    {
      app::Message am;
      am.infoMessage = QString::fromStdString(errorStream.str());
      app::instance()->messageSlot(msg::Message(msg::Request | msg::Info | msg::Text, am));
    }
  }
}

using namespace expr;

TableViewBase::TableViewBase(QWidget* parentIn): QTableView(parentIn)
{
  this->verticalHeader()->setVisible(false);
  setSortingEnabled(true);
//   horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
  horizontalHeader()->setStretchLastSection(true);
  restoreSettings();
  connect(horizontalHeader(), &QHeaderView::sectionResized, this, &TableViewBase::columnResizedSlot);
  
  NameDelegate *nameDelegate = new NameDelegate(this);
  setItemDelegateForColumn(0, nameDelegate);
  ExpressionDelegate *expressionDelegate = new ExpressionDelegate(this);
  setItemDelegateForColumn(1, expressionDelegate);
}

void TableViewBase::columnResizedSlot(int, int, int)
{
  QSettings &settings = app::instance()->getUserSettings();
  settings.beginGroup("expr:widget");
  settings.setValue("header", horizontalHeader()->saveState());
  settings.endGroup();
}

void TableViewBase::showEvent(QShowEvent*)
{
  restoreSettings();
}

void TableViewBase::restoreSettings()
{
  QSettings &settings = app::instance()->getUserSettings();
  settings.beginGroup("expr:widget");
  horizontalHeader()->restoreState(settings.value("header").toByteArray());
  settings.endGroup();
}

TableViewAll::TableViewAll(QWidget* parentIn): TableViewBase(parentIn)
{
  buildActions();
}

void TableViewAll::addScalarSlot()
{
  TableModel *myModel = static_cast<TableModel *>(static_cast<AllProxyModel*>(this->model())->sourceModel());
  int rowCount = myModel->rowCount();
  myModel->addScalar();
  QModelIndex index = this->model()->index(rowCount, 0);
  
  this->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
  this->setCurrentIndex(index);
  QMetaObject::invokeMethod(this, "edit", Qt::QueuedConnection, Q_ARG(QModelIndex, index));
}

void TableViewAll::addVectorSlot()
{
  TableModel *myModel = static_cast<TableModel *>(static_cast<AllProxyModel*>(this->model())->sourceModel());
  int rowCount = myModel->rowCount();
  myModel->addVector();
  QModelIndex index = this->model()->index(rowCount, 0);
  
  this->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
  this->setCurrentIndex(index);
  QMetaObject::invokeMethod(this, "edit", Qt::QueuedConnection, Q_ARG(QModelIndex, index));
}

void TableViewAll::addRotationSlot()
{
  TableModel *myModel = static_cast<TableModel *>(static_cast<AllProxyModel*>(this->model())->sourceModel());
  int rowCount = myModel->rowCount();
  myModel->addRotation();
  QModelIndex index = this->model()->index(rowCount, 0);
  
  this->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
  this->setCurrentIndex(index);
  QMetaObject::invokeMethod(this, "edit", Qt::QueuedConnection, Q_ARG(QModelIndex, index));
}

void TableViewAll::addCSysSlot()
{
  TableModel *myModel = static_cast<TableModel *>(static_cast<AllProxyModel*>(this->model())->sourceModel());
  int rowCount = myModel->rowCount();
  myModel->addCSys();
  QModelIndex index = this->model()->index(rowCount, 0);
  
  this->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
  this->setCurrentIndex(index);
  QMetaObject::invokeMethod(this, "edit", Qt::QueuedConnection, Q_ARG(QModelIndex, index));
}

void TableViewAll::addToGroupSlot()
{
  QAction *action = dynamic_cast<QAction *>(QObject::sender());
  assert(action);
  
  QSortFilterProxyModel *pModel = dynamic_cast<QSortFilterProxyModel *>(this->model());
  assert(pModel);
  TableModel *myModel = dynamic_cast<TableModel *>(pModel->sourceModel());
  assert(myModel);
  
  QModelIndexList indexes = this->selectedIndexes();
  for (QModelIndexList::iterator it = indexes.begin(); it != indexes.end(); ++it)
  {
    QModelIndex sourceIndex = pModel->mapToSource(*it);
    myModel->addExpressionToGroup(sourceIndex, action->data().toInt());
  }
}

void TableViewAll::removeFormulaSlot()
{
  QSortFilterProxyModel *pModel = dynamic_cast<QSortFilterProxyModel *>(this->model());
  assert(pModel);
  TableModel *myModel = dynamic_cast<TableModel *>(pModel->sourceModel());
  assert(myModel);
  
  QModelIndexList indexes = this->selectedIndexes();
  QModelIndexList sourceIndexes;
  for (QModelIndexList::iterator it = indexes.begin(); it != indexes.end(); ++it)
    sourceIndexes.append(pModel->mapToSource(*it));
  myModel->removeExpression(sourceIndexes);
}

void TableViewAll::exportFormulaSlot()
{
  QSortFilterProxyModel *pModel = dynamic_cast<QSortFilterProxyModel *>(this->model());
  assert(pModel);
  TableModel *myModel = dynamic_cast<TableModel *>(pModel->sourceModel());
  assert(myModel);
  
  QString filePath = QFileDialog::getSaveFileName
  (
    this,
    tr("Save File Name"),
    QString::fromStdString(prf::manager().rootPtr->project().lastDirectory().get()),
    tr("Expressions (*.exp)")
  );
  if (filePath.isEmpty())
    return;
  if (!filePath.endsWith(".exp"))
    filePath += ".exp";
  
  boost::filesystem::path p = filePath.toStdString();
  prf::manager().rootPtr->project().lastDirectory() = p.remove_filename().string();
  prf::manager().saveConfig();
  
  QModelIndexList indexes = this->selectedIndexes();
  QModelIndexList sourceIndexes;
  for (QModelIndexList::iterator it = indexes.begin(); it != indexes.end(); ++it)
    sourceIndexes.append(pModel->mapToSource(*it));
  
  std::ofstream fileStream;
  fileStream.open(filePath.toStdString().c_str());
  if (!fileStream.is_open())
  {
    QMessageBox::critical(this, tr("Error:"), tr("Couldn't Open File"));
    return;
  }
  myModel->exportExpressions(sourceIndexes, fileStream);
  fileStream.close();
}

void TableViewAll::importFormulaSlot()
{
  QSortFilterProxyModel *pModel = dynamic_cast<QSortFilterProxyModel *>(this->model());
  assert(pModel);
  TableModel *myModel = dynamic_cast<TableModel *>(pModel->sourceModel());
  assert(myModel);
  
  QString filePath = QFileDialog::getOpenFileName
  (
    this,
    tr("Open File Name"),
    QString::fromStdString(prf::manager().rootPtr->project().lastDirectory().get()),
    tr("Expressions (*.exp)")
  );
  if (filePath.isEmpty())
    return;
  
  boost::filesystem::path p = filePath.toStdString();
  prf::manager().rootPtr->project().lastDirectory() = p.remove_filename().string();
  prf::manager().saveConfig();
  
  std::ifstream fileStream;
  fileStream.open(filePath.toStdString().c_str());
  if (!fileStream.is_open())
    return;
  
  auto results = myModel->importExpressions(fileStream);
  reportImportErrors(results);
}

void TableViewAll::copyFormulaValueSlot()
{
  QSortFilterProxyModel *pModel = dynamic_cast<QSortFilterProxyModel *>(this->model()); assert(pModel);
  TableModel *myModel = dynamic_cast<TableModel *>(pModel->sourceModel()); assert(myModel);
  
  QModelIndexList indexes = this->selectedIndexes();
  if (indexes.size() != 1)
    return;
  
  QModelIndex sourceIndex = myModel->index(pModel->mapToSource(indexes.front()).row(), 2);
  QApplication::clipboard()->setText(myModel->data(sourceIndex).toString());
}

void TableViewAll::contextMenuEvent(QContextMenuEvent* event)
{
  QMenu menu;
  menu.addAction(addScalarAction);
  menu.addAction(addVectorAction);
  menu.addAction(addRotationAction);
  menu.addAction(addCSysAction);
  menu.addAction(removeFormulaAction);

  //add to group sub menu.
  QSortFilterProxyModel *pModel = dynamic_cast<QSortFilterProxyModel *>(this->model());
  assert(pModel);
  TableModel *tModel = dynamic_cast<TableModel *>(pModel->sourceModel());
  assert(tModel);
  QMenu *groupMenu = menu.addMenu(tr("Add To Group"));
  groupMenu->setDisabled(true); //default to off. turn on below.
  const expr::Manager& man = tModel->getManager();
  for (auto id : man.getGroupIds())
  {
    auto oName = man.getGroupName(id);
    assert(oName);
    QAction *currentAction = groupMenu->addAction(QString::fromStdString(*oName));
    currentAction->setData(QVariant(id));
    connect(currentAction, SIGNAL(triggered()), this, SLOT(addToGroupSlot()));
  }
    
  menu.addSeparator();
  
  menu.addAction(exportFormulaAction);
  menu.addAction(importFormulaAction);
  
  menu.addSeparator();
  
  //add Group.
  menu.addAction(addGroupAction);
  
  menu.addSeparator();
  
  //copy value to clipboard
  menu.addAction(copyFormulaValueAction);
  
  if(!this->selectionModel()->hasSelection())
  {
    removeFormulaAction->setDisabled(true);
    exportFormulaAction->setDisabled(true);
  }
  else
  {
    removeFormulaAction->setEnabled(true);
    exportFormulaAction->setEnabled(true);
    if (!man.getGroupIds().empty())
      groupMenu->setEnabled(true);
  }
  
  if
  (
    (this->selectionModel()->selectedIndexes().size() == 1)
    && (this->selectionModel()->selectedIndexes().front().column() == 2)
  )
    copyFormulaValueAction->setEnabled(true);
  else
    copyFormulaValueAction->setDisabled(true);
  
  menu.exec(event->globalPos());
}

void TableViewAll::buildActions()
{
  addScalarAction = buildAddScalarAction(this, std::bind(&TableViewAll::addScalarSlot, this));
  addVectorAction = buildAddVectorAction(this, std::bind(&TableViewAll::addVectorSlot, this));
  addRotationAction = buildAddRotationAction(this, std::bind(&TableViewAll::addRotationSlot, this));
  addCSysAction = buildAddCSysAction(this, std::bind(&TableViewAll::addCSysSlot, this));
  removeFormulaAction = buildRemoveExpressionAction(this, std::bind(&TableViewAll::removeFormulaSlot, this));
  copyFormulaValueAction = buildCopyExpressionValueAction(this, std::bind(&TableViewAll::copyFormulaValueSlot, this));
  
  exportFormulaAction = buildExportExpressionAction(this, std::bind(&TableViewAll::exportFormulaSlot, this));
  importFormulaAction = buildImportExpressionAction(this, std::bind(&TableViewAll::importFormulaSlot, this));

  addGroupAction = buildAction(this, std::bind(&TableViewAll::addGroupSignal, this), tr("Add Group"), QKeySequence(Qt::CTRL + Qt::Key_G));
}

TableViewGroup::TableViewGroup(QWidget* parentIn): TableViewBase(parentIn)
{
  buildActions();
}

void TableViewGroup::addScalarSlot()
{
  QModelIndex proxyIndex = static_cast<GroupProxyModel*>(this->model())->addScalar();
  
  this->selectionModel()->select(proxyIndex, QItemSelectionModel::ClearAndSelect);
  this->setCurrentIndex(proxyIndex);
  QMetaObject::invokeMethod(this, "edit", Qt::QueuedConnection, Q_ARG(QModelIndex, proxyIndex));
}

void TableViewGroup::addVectorSlot()
{
  QModelIndex proxyIndex = static_cast<GroupProxyModel*>(this->model())->addVector();
  
  this->selectionModel()->select(proxyIndex, QItemSelectionModel::ClearAndSelect);
  this->setCurrentIndex(proxyIndex);
  QMetaObject::invokeMethod(this, "edit", Qt::QueuedConnection, Q_ARG(QModelIndex, proxyIndex));
}

void TableViewGroup::addRotationSlot()
{
  QModelIndex proxyIndex = static_cast<GroupProxyModel*>(this->model())->addRotation();
  
  this->selectionModel()->select(proxyIndex, QItemSelectionModel::ClearAndSelect);
  this->setCurrentIndex(proxyIndex);
  QMetaObject::invokeMethod(this, "edit", Qt::QueuedConnection, Q_ARG(QModelIndex, proxyIndex));
}

void TableViewGroup::addCSysSlot()
{
  QModelIndex proxyIndex = static_cast<GroupProxyModel*>(this->model())->addCSys();
  
  this->selectionModel()->select(proxyIndex, QItemSelectionModel::ClearAndSelect);
  this->setCurrentIndex(proxyIndex);
  QMetaObject::invokeMethod(this, "edit", Qt::QueuedConnection, Q_ARG(QModelIndex, proxyIndex));
}

void TableViewGroup::contextMenuEvent(QContextMenuEvent* event)
{
  QMenu menu;
  
  menu.addAction(addScalarAction);
  menu.addAction(addVectorAction);
  menu.addAction(addRotationAction);
  menu.addAction(addCSysAction);
  menu.addAction(removeFormulaAction);
  menu.addAction(removeFromGroupAction);
  
  menu.addSeparator();
  
  menu.addAction(exportFormulaAction);
  menu.addAction(importFormulaAction);
  
  menu.addSeparator();
  
  menu.addAction(renameGroupAction);
  menu.addAction(removeGroupAction);
  
  menu.addSeparator();
  
  //copy value to clipboard
  menu.addAction(copyFormulaValueAction);
  
  if(!this->selectionModel()->hasSelection())
  {
    removeFormulaAction->setDisabled(true);
    removeFromGroupAction->setDisabled(true);
    exportFormulaAction->setDisabled(true);
  }
  else
  {
    removeFormulaAction->setEnabled(true);
    removeFromGroupAction->setEnabled(true);
    exportFormulaAction->setEnabled(true);
  }
  
  if
  (
    (this->selectionModel()->selectedIndexes().size() == 1)
    && (this->selectionModel()->selectedIndexes().front().column() == 2)
  )
    copyFormulaValueAction->setEnabled(true);
  else
    copyFormulaValueAction->setDisabled(true);
  
  menu.exec(event->globalPos());
}

void TableViewGroup::removeFormulaSlot()
{
  QSortFilterProxyModel *pModel = dynamic_cast<QSortFilterProxyModel *>(this->model());
  assert(pModel);
  TableModel *myModel = dynamic_cast<TableModel *>(pModel->sourceModel());
  assert(myModel);
  
  QModelIndexList indexes = this->selectedIndexes();
  QModelIndexList sourceIndexes;
  for (QModelIndexList::iterator it = indexes.begin(); it != indexes.end(); ++it)
    sourceIndexes.append(pModel->mapToSource(*it));
  myModel->removeExpression(sourceIndexes);
}

void TableViewGroup::removeFromGroupSlot()
{
  GroupProxyModel *pModel = dynamic_cast<GroupProxyModel *>(this->model());
  assert(pModel);
  pModel->removeFromGroup(this->selectedIndexes());
}

void TableViewGroup::renameGroupSlot()
{
  GroupProxyModel *pModel = dynamic_cast<GroupProxyModel *>(this->model());
  assert(pModel);
  
  QString oldName = QString::fromStdString(pModel->getGroupName());
  
  bool ok;
  QString newName = QInputDialog::getText(this, tr("Enter New Name"), tr("Name:"), QLineEdit::Normal, oldName, &ok);
  if (!ok || newName.isEmpty() || (newName == oldName))
    return;
  
  if (pModel->renameGroup(newName.toStdString()))
    Q_EMIT groupRenamedSignal(this, newName);
  else
    QMessageBox::critical(this, tr("Error:"), tr("Name Already Exists"));
}

void TableViewGroup::removeGroupSlot()
{
  GroupProxyModel *pModel = dynamic_cast<GroupProxyModel *>(this->model());
  assert(pModel);
  
  pModel->removeGroup();
  Q_EMIT(groupRemovedSignal(this));
}

void TableViewGroup::exportFormulaSlot()
{
  QSortFilterProxyModel *pModel = dynamic_cast<QSortFilterProxyModel *>(this->model());
  assert(pModel);
  TableModel *myModel = dynamic_cast<TableModel *>(pModel->sourceModel());
  assert(myModel);
  
  QString filePath = QFileDialog::getSaveFileName
  (
    this,
    tr("Save File Name"),
    QString::fromStdString(prf::manager().rootPtr->project().lastDirectory().get()),
    tr("Expressions (*.exp)")
  );
  if (filePath.isEmpty())
    return;
  if (!filePath.endsWith(".exp"))
    filePath += ".exp";
  
  boost::filesystem::path p = filePath.toStdString();
  prf::manager().rootPtr->project().lastDirectory() = p.remove_filename().string();
  prf::manager().saveConfig();
  
  QModelIndexList indexes = this->selectedIndexes();
  QModelIndexList sourceIndexes;
  for (QModelIndexList::iterator it = indexes.begin(); it != indexes.end(); ++it)
    sourceIndexes.append(pModel->mapToSource(*it));
  
  std::ofstream fileStream;
  fileStream.open(filePath.toStdString().c_str());
  if (!fileStream.is_open())
  {
    QMessageBox::critical(this, tr("Error:"), tr("Couldn't Open File"));
    return;
  }
  myModel->exportExpressions(sourceIndexes, fileStream);
  fileStream.close();
}

void TableViewGroup::importFormulaSlot()
{
  GroupProxyModel *pModel = dynamic_cast<GroupProxyModel *>(this->model());
  assert(pModel);
  
  QString filePath = QFileDialog::getOpenFileName
  (
    this,
    tr("Open File Name"),
    QString::fromStdString(prf::manager().rootPtr->project().lastDirectory().get()),
    tr("Expressions (*.exp)")
  );
  if (filePath.isEmpty())
    return;
  
  boost::filesystem::path p = filePath.toStdString();
  prf::manager().rootPtr->project().lastDirectory() = p.remove_filename().string();
  prf::manager().saveConfig();
  
  std::ifstream fileStream;
  fileStream.open(filePath.toStdString().c_str());
  if (!fileStream.is_open())
    return;
  
  auto results = pModel->importExpressions(fileStream);
  reportImportErrors(results);
}

void TableViewGroup::copyFormulaValueSlot()
{
  GroupProxyModel *pModel = dynamic_cast<GroupProxyModel *>(this->model()); assert(pModel);
  TableModel *myModel = dynamic_cast<TableModel *>(pModel->sourceModel()); assert(myModel);
  
  QModelIndexList indexes = this->selectedIndexes();
  if (indexes.size() != 1)
    return;
  
  QModelIndex sourceIndex = myModel->index(pModel->mapToSource(indexes.front()).row(), 2);
  QApplication::clipboard()->setText(myModel->data(sourceIndex).toString());
}

void TableViewGroup::buildActions()
{
  addScalarAction = buildAddScalarAction(this, std::bind(&TableViewGroup::addScalarSlot, this));
  addVectorAction = buildAddVectorAction(this, std::bind(&TableViewGroup::addVectorSlot, this));
  addRotationAction = buildAddRotationAction(this, std::bind(&TableViewGroup::addRotationSlot, this));
  addCSysAction = buildAddCSysAction(this, std::bind(&TableViewGroup::addCSysSlot, this));
  removeFormulaAction = buildRemoveExpressionAction(this, std::bind(&TableViewGroup::removeFormulaSlot, this));
  copyFormulaValueAction = buildCopyExpressionValueAction(this, std::bind(&TableViewGroup::copyFormulaValueSlot, this));
  
  exportFormulaAction = buildExportExpressionAction(this, std::bind(&TableViewGroup::exportFormulaSlot, this));
  importFormulaAction = buildImportExpressionAction(this, std::bind(&TableViewGroup::importFormulaSlot, this));
  
  removeFromGroupAction = buildAction(this, std::bind(&TableViewGroup::removeFromGroupSlot, this), tr("Remove From Group"), QKeySequence(Qt::CTRL + Qt::Key_R));
  renameGroupAction = buildAction(this, std::bind(&TableViewGroup::renameGroupSlot, this), tr("Rename Group"), QKeySequence(Qt::CTRL + Qt::Key_M));
  removeGroupAction = buildAction(this, std::bind(&TableViewGroup::removeGroupSlot, this), tr("Remove Group"), QKeySequence(Qt::CTRL + Qt::Key_K));
}

TableViewSelection::TableViewSelection(QWidget* parent): TableViewBase(parent)
{
  copyFormulaValueAction = buildCopyExpressionValueAction(this, std::bind(&TableViewSelection::copyFormulaValueSlot, this));
}

void TableViewSelection::contextMenuEvent(QContextMenuEvent* event)
{
  if
  (
    (this->selectionModel()->selectedIndexes().size() == 1)
    && (this->selectionModel()->selectedIndexes().front().column() == 2)
  )
  {
    QMenu menu;
    menu.addAction(copyFormulaValueAction);
    menu.exec(event->globalPos());
  }
}

void TableViewSelection::copyFormulaValueSlot()
{
  SelectionProxyModel *pModel = dynamic_cast<SelectionProxyModel *>(this->model()); assert(pModel);
  TableModel *myModel = dynamic_cast<TableModel *>(pModel->sourceModel()); assert(myModel);
  
  QModelIndexList indexes = this->selectedIndexes();
  if (indexes.size() != 1)
    return;
  
  QModelIndex sourceIndex = myModel->index(pModel->mapToSource(indexes.front()).row(), 2);
  QApplication::clipboard()->setText(myModel->data(sourceIndex).toString());
}

NameDelegate::NameDelegate(QObject* parent): QStyledItemDelegate(parent){}

void NameDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
  QLineEdit *lineEdit = static_cast<QLineEdit *>(editor);
  if (!model->setData(index, lineEdit->text(), Qt::EditRole))
  {
    //view must be used as parent when constructing the delegate.
    QAbstractItemView *view = dynamic_cast<QAbstractItemView *>(this->parent());
    assert(view);
    QMessageBox::critical(view, tr("Error:"), model->data(index, (Qt::UserRole+1)).toString());
    QMetaObject::invokeMethod(view, "edit", Qt::QueuedConnection, Q_ARG(QModelIndex, index));
  }
}

ExpressionDelegate::ExpressionDelegate(QObject *parent): QStyledItemDelegate(parent){}

QWidget* ExpressionDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex&) const
{
  dlg::ExpressionEdit *editor = new dlg::ExpressionEdit(parent);
  return editor;
}

void ExpressionDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
  dlg::ExpressionEdit *eEditor = dynamic_cast<dlg::ExpressionEdit *>(editor); assert(eEditor);
  eEditor->lineEdit->setText(index.model()->data(index, Qt::EditRole).toString());
  QTimer::singleShot(0, eEditor->trafficLabel, SLOT(setTrafficGreenSlot()));
  eEditor->trafficLabel->setTrafficGreenSlot(); //expect current string is valid.
  
  const QSortFilterProxyModel *proxyModel = dynamic_cast<const QSortFilterProxyModel *>(index.model());
  assert(proxyModel);
  //I have to be able to use the model when temp parsing!
  TableModel *tableModel = const_cast<TableModel*>(dynamic_cast<const TableModel *>(proxyModel->sourceModel()));
  assert(tableModel);
  
  connect (eEditor->lineEdit, SIGNAL(textEdited(QString)), tableModel, SLOT(parseStringSlot(QString)));
  connect (tableModel, SIGNAL(parseWorkingSignal()), eEditor->trafficLabel, SLOT(setTrafficYellowSlot()));
  connect (tableModel, SIGNAL(parseSucceededSignal(QString)), editor, SLOT(goToolTipSlot(QString)));
  connect (tableModel, SIGNAL(parseSucceededSignal()), eEditor->trafficLabel, SLOT(setTrafficGreenSlot()));
  connect (tableModel, SIGNAL(parseFailedSignal(QString)), editor, SLOT(goToolTipSlot(QString)));
  connect (tableModel, SIGNAL(parseFailedSignal()), eEditor->trafficLabel, SLOT(setTrafficRedSlot()));
}

void ExpressionDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex&) const
{
  //this is called before setEditorData.
  editor->setGeometry(option.rect);
}

void ExpressionDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
  dlg::ExpressionEdit *eEditor = dynamic_cast<dlg::ExpressionEdit *>(editor);
  assert(eEditor);
  if (!model->setData(index, eEditor->lineEdit->text(), Qt::EditRole))
  {
    //view must be used as parent when constructing the delegate.
    QAbstractItemView *view = dynamic_cast<QAbstractItemView *>(this->parent());
    assert(view);
    QMessageBox::critical(view, tr("Error:"), model->data(index, (Qt::UserRole+1)).toString());
    QMetaObject::invokeMethod(view, "edit", Qt::QueuedConnection, Q_ARG(QModelIndex, index));
  }
}
