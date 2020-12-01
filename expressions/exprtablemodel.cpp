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

#include <assert.h>
#include <fstream>
#include <iostream>
#include <sstream>

#include <boost/current_function.hpp>

#include <QCoreApplication>
#include <QtCore/QTextStream>
#include <QtCore/QStringList>
#include <QtCore/QMimeData>

#include <osg/Geometry> //yuck!

#include "tools/idtools.h"
#include "application/appapplication.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "message/msgsift.h"
#include "selection/slcmessage.h"
#include "feature/ftrbase.h"
#include "expressions/exprmanager.h"
#include "expressions/exprtablemodel.h"

using namespace expr;

TableModel::TableModel(Manager &eManagerIn, QObject* parent):
  QAbstractTableModel(parent), lastFailedMessage(""),
  eManager(eManagerIn)
{
}

TableModel::~TableModel(){}

int TableModel::columnCount(const QModelIndex&) const
{
  return 3;
}

int TableModel::rowCount(const QModelIndex&) const
{
  return static_cast<int>(eManager.getExpressionIds().size());
}

QVariant TableModel::data(const QModelIndex& index, int role) const
{
  auto eids = eManager.getExpressionIds();
  if ((role == Qt::DisplayRole) || (role == Qt::EditRole))
  {
    if (!index.isValid())
      return QVariant();
    assert(index.row() >= 0 && index.row() < static_cast<int>(eids.size()));
    if (index.column() == 0)
    {
      auto oName = eManager.getExpressionName(eids.at(index.row()));
      if (!oName)
        return QVariant();
      return QString::fromStdString(*oName);
    }
    if (index.column() == 1)
    {
      auto it = idToRhsMap.find(eids.at(index.row()));
      if (it == idToRhsMap.end())
      {
        std::string newRHS = eManager.buildRHSString(eids.at(index.row()));
        idToRhsMap.insert(std::make_pair(eids.at(index.row()), newRHS));
        return QString::fromStdString(newRHS);
      }
      return QString::fromStdString(it->second);
    }
    if (index.column() == 2)
    {
      std::ostringstream stream;
      stream << eManager.getExpressionValue(eids.at(index.row()));
      return QString::fromStdString(stream.str());
    }
  }
  if (role == Qt::UserRole)
    return eids.at(index.row());
  if (role == (Qt::UserRole + 1))
    return lastFailedMessage;
  
  return QVariant();
}

QVariant TableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
  {
    if (section == 0)
      return tr("Name");
    if (section == 1)
      return tr("Expression");
    if (section == 2)
      return tr("Value");
  }
  return QAbstractItemModel::headerData(section, orientation, role);
}

Qt::ItemFlags TableModel::flags(const QModelIndex& index) const
{
  static std::vector<Qt::ItemFlags> flagVector
  {
    Qt::ItemFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled),
    Qt::ItemFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled),
    Qt::ItemFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled)
  };
  
  if (!index.isValid())
    return Qt::NoItemFlags;
  return flagVector.at(index.column());
}

bool TableModel::setData(const QModelIndex& index, const QVariant& value, int)
{
  int fId = data(index, Qt::UserRole).toInt();
  assert(eManager.getExpressionName(fId)); //just a test for a valid id.
  if (index.column() == 0)
  {
    //rename formula.
    std::string oldName = *eManager.getExpressionName(fId);
    std::string newName = value.toString().toStdString();
    if (oldName == newName)
      return true;
    if (eManager.getExpressionId(newName))
    {
      lastFailedMessage = tr("Formula name already exists");
      return false;
    }
    //we need to make sure the new name will pass the parser.
    std::string testParse = newName + "=1";
    auto results = eManager.parseString(testParse);
    if (!results.isAllGood())
    {
      lastFailedMessage = tr("Invalid Name");
      return false;
    }
    eManager.removeExpression(results.expressionName);
    eManager.renameExpression(fId, newName);
    
    //add git message.
    std::ostringstream gitStream;
    gitStream << "Rename expression: " << oldName << ", to: " << newName;
    app::instance()->messageSlot(msg::buildGitMessage(gitStream.str()));
  }
  if (index.column() == 1)
  {
    //libcadcalc ensures that existing expressions are restored on failures. so we don't have to save and restore previous expression.
    std::string formulaName = *eManager.getExpressionName(fId);
    
    std::ostringstream stream;
    stream << formulaName << " = " << value.toString().toStdString();
    auto results = eManager.parseString(stream.str());
    if (!results.isAllGood())
    {
      lastFailedMessage = QString::fromStdString(results.getError());
      return false;
    }
    //add git message.
    std::ostringstream gitStream;
    gitStream << "Edit expression: " << stream.str();
    app::instance()->messageSlot(msg::buildGitMessage(gitStream.str()));
  }
  lastFailedMessage.clear();
  idToRhsMap.erase(fId);
  Q_EMIT dataChanged(index, index);
  
  app::instance()->queuedMessage(msg::Mask(msg::Request | msg::Project | msg::Update));
  
  return true;
}

QStringList TableModel::mimeTypes() const
{
  QStringList types;
  types << "text/plain";
  return types;
}

QMimeData* TableModel::mimeData(const QModelIndexList& indexes) const
{
  QMimeData *mimeData = new QMimeData();
  QString out = "ExpressionId";
  for (QModelIndexList::const_iterator it = indexes.constBegin(); it != indexes.constEnd(); ++it)
  {
    out += ";";
    out += this->data(this->index(it->row(), 0), Qt::UserRole).toString();
  }
  mimeData->setText(out);
  return mimeData;
}

void TableModel::addExpressionToGroup(const QModelIndex& indexIn, int groupIdIn)
{
  int formulaId = this->data(indexIn, Qt::UserRole).toInt();
  assert(eManager.hasGroup(groupIdIn));
  assert(eManager.hasExpression(formulaId));
  eManager.addExpressionToGroup(groupIdIn, formulaId);
  groupChangedSignal(groupIdIn);
}

void TableModel::addScalar()
{
  std::string expression = createUniqueName("aScalar") + " = 0.0";
  addCommon(expression);
}

void TableModel::addVector()
{
  std::string expression = createUniqueName("aVector") + " = VZERO";
  addCommon(expression);
}

void TableModel::addRotation()
{
  std::string expression = createUniqueName("aRotation") + " = RZERO";
  addCommon(expression);
}

void TableModel::addCSys()
{
  std::string expression = createUniqueName("aSystem") + " = CZERO";
  addCommon(expression);
}

std::string TableModel::createUniqueName(const std::string &base) const
{
  std::string out;
  for (int nameIndex = 0; nameIndex < 100000; ++nameIndex) //more than 100000? something is wrong!
  {
    std::ostringstream stream;
    stream << base << "_" << nameIndex;
    out = stream.str();
    if (eManager.getExpressionId(out))
      continue;
    break;
  }
  assert(!eManager.getExpressionId(out));
  return out;
}

void TableModel::addCommon(const std::string &expression)
{
  int size = static_cast<int>(eManager.getExpressionIds().size());
  this->beginInsertRows(QModelIndex(), size, size);
  auto result = eManager.parseString(expression);
  this->endInsertRows();
  
  //add git message.
  std::ostringstream gitStream;
  gitStream << "Adding expression: " << result.expressionName;
  app::instance()->messageSlot(msg::buildGitMessage(gitStream.str()));
}

void TableModel::removeExpression(const QModelIndexList &indexesIn)
{
  std::vector<int> mappedRows;
  QMap<int, QPersistentModelIndex> indexMap;
  QModelIndexList::const_iterator it = indexesIn.constBegin();
  for (; it != indexesIn.constEnd(); ++it)
  {
    int currentRow = (*it).row();
    if (std::find(mappedRows.begin(), mappedRows.end(), currentRow) == mappedRows.end())
      mappedRows.push_back(currentRow);
    indexMap.insert(currentRow, QPersistentModelIndex(this->index((*it).row(), 0)));
  }
  std::sort(mappedRows.begin(), mappedRows.end());
  std::reverse(mappedRows.begin(), mappedRows.end());
  
  //instead of trying to track down any dependent formulas we will just invalidate the model
  beginResetModel();
  for (std::vector<int>::const_iterator rowIt = mappedRows.begin(); rowIt != mappedRows.end(); ++rowIt)
  {
    auto currentId = this->data(indexMap.value(*rowIt), Qt::UserRole).toInt();
    
    //add git message. Doing all this in loop because formatting is specific and it's handled in gitManager.
    std::ostringstream gitStream;
    auto oName = eManager.getExpressionName(currentId);
    assert(oName);
    gitStream << "Remove expression: " << *oName;
    app::instance()->messageSlot(msg::buildGitMessage(gitStream.str()));
    
    eManager.removeExpression(currentId);
  }
  idToRhsMap.clear();
  endResetModel();
  
  app::instance()->messageSlot(msg::Mask(msg::Request | msg::Project | msg::Update));
}

void TableModel::exportExpressions(QModelIndexList& indexesIn, std::ostream &os) const
{
  std::vector<int> selectedIds;
  QModelIndexList::const_iterator it;
  for (it = indexesIn.constBegin(); it != indexesIn.constEnd(); ++it)
    selectedIds.emplace_back(this->data(*it, Qt::UserRole).toInt());
  
  if (selectedIds.empty())
    eManager.exportExpressions(os);
  else
    eManager.exportExpressions(os, selectedIds);
}

std::vector<lcc::Result> TableModel::importExpressions(std::istream &streamIn, int groupId)
{
  beginResetModel();
  //any existing expressions will be overwrote and added to group
  auto results = eManager.importExpressions(streamIn);
  if (groupId != -1)
  {
    assert(eManager.hasGroup(groupId));
    for (const auto &r : results)
    {
      if (r.isAllGood())
      {
        auto oId = eManager.getExpressionId(r.expressionName);
        assert(oId);
        eManager.addExpressionToGroup(groupId, *oId);
      }
    }
  }
  endResetModel();
  return results;
}

void TableModel::parseStringSlot(const QString &textIn)
{
  Q_EMIT parseWorkingSignal();
  qApp->processEvents(); //need this or we never see yellow signal.
  
  assert(!eManager.hasExpression(testFormulaName));
  
  std::ostringstream parseStream;
  parseStream << testFormulaName << "=" << textIn.toStdString();
  auto result = eManager.parseString(parseStream.str());
  if (result.isAllGood())
  {
    std::ostringstream valueStream;
    valueStream << eManager.getExpressionValue(testFormulaName);
    QString string;
    Q_EMIT parseSucceededSignal(QString::fromStdString(valueStream.str()));
    Q_EMIT parseSucceededSignal();
  }
  else
  {
    int position = result.errorPosition - testFormulaName.size() - 1;
    QString string = textIn.left(position) + "?";
    Q_EMIT parseFailedSignal(string);
    Q_EMIT parseFailedSignal();
  }
  
  if (eManager.hasExpression(testFormulaName))
    eManager.removeExpression(testFormulaName);
}

GroupProxyModel::GroupProxyModel(Manager &eManagerIn, int groupIdIn, QObject* parent)
  : QSortFilterProxyModel(parent), eManager(eManagerIn), groupId(groupIdIn)
{
  setDynamicSortFilter(false);
}

bool GroupProxyModel::filterAcceptsRow(int source_row, const QModelIndex&) const
{
  int formulaId = sourceModel()->data(sourceModel()->index(source_row, 0), Qt::UserRole).toInt();
  return eManager.groupHasExpression(groupId, formulaId);
}

QModelIndex GroupProxyModel::addScalar()
{
  TableModel *myModel = dynamic_cast<TableModel *>(this->sourceModel());
  assert(myModel);
  int rowCount = myModel->rowCount();
  myModel->addScalar();
  QModelIndex sourceIndex = myModel->index(rowCount, 0);
  int formulaId = myModel->data(sourceIndex, Qt::UserRole).toInt();
  eManager.addExpressionToGroup(groupId, formulaId);
  this->invalidateFilter();
  QModelIndex out = this->mapFromSource(sourceIndex);
  assert(out.isValid());
  return out;
}

QModelIndex GroupProxyModel::addVector()
{
  TableModel *myModel = dynamic_cast<TableModel *>(this->sourceModel());
  assert(myModel);
  int rowCount = myModel->rowCount();
  myModel->addVector();
  QModelIndex sourceIndex = myModel->index(rowCount, 0);
  int formulaId = myModel->data(sourceIndex, Qt::UserRole).toInt();
  eManager.addExpressionToGroup(groupId, formulaId);
  this->invalidateFilter();
  QModelIndex out = this->mapFromSource(sourceIndex);
  assert(out.isValid());
  return out;
}

QModelIndex GroupProxyModel::addRotation()
{
  TableModel *myModel = dynamic_cast<TableModel *>(this->sourceModel());
  assert(myModel);
  int rowCount = myModel->rowCount();
  myModel->addRotation();
  QModelIndex sourceIndex = myModel->index(rowCount, 0);
  int formulaId = myModel->data(sourceIndex, Qt::UserRole).toInt();
  eManager.addExpressionToGroup(groupId, formulaId);
  this->invalidateFilter();
  QModelIndex out = this->mapFromSource(sourceIndex);
  assert(out.isValid());
  return out;
}

QModelIndex GroupProxyModel::addCSys()
{
    TableModel *myModel = dynamic_cast<TableModel *>(this->sourceModel());
  assert(myModel);
  int rowCount = myModel->rowCount();
  myModel->addCSys();
  QModelIndex sourceIndex = myModel->index(rowCount, 0);
  int formulaId = myModel->data(sourceIndex, Qt::UserRole).toInt();
  eManager.addExpressionToGroup(groupId, formulaId);
  this->invalidateFilter();
  QModelIndex out = this->mapFromSource(sourceIndex);
  assert(out.isValid());
  return out;
}

void GroupProxyModel::removeFromGroup(const QModelIndexList &indexesIn)
{
  std::vector <int> processedRows;
  QModelIndexList::const_iterator it;
  for (it = indexesIn.constBegin(); it != indexesIn.constEnd(); ++it)
  {
    QModelIndex currentSource = this->mapToSource(*it);
    assert(currentSource.isValid());
    if (std::find(processedRows.begin(), processedRows.end(),currentSource.row()) != processedRows.end())
      continue;
    processedRows.push_back(currentSource.row());
    int id = sourceModel()->data(currentSource, Qt::UserRole).toInt();
    assert(eManager.groupHasExpression(groupId, id));
    eManager.removeExpressionFromGroup(groupId, id);
  }
  this->invalidateFilter();
}

std::string GroupProxyModel::getGroupName() const
{
  assert(eManager.hasGroup(groupId));
  return *eManager.getGroupName(groupId);
}

bool GroupProxyModel::renameGroup(const std::string& newName)
{
  assert(eManager.hasGroup(groupId));
  if (eManager.hasGroup(newName))
    return false;
  eManager.renameGroup(groupId, newName);
  return true;
}

void GroupProxyModel::groupChangedSlot(int gId)
{
  if (gId != groupId)
    return;
  invalidateFilter();
}

void GroupProxyModel::removeGroup()
{
  assert(eManager.hasGroup(groupId));
  eManager.removeGroup(groupId);
}

std::vector<lcc::Result> GroupProxyModel::importExpressions(std::istream &streamIn)
{
  TableModel *tableModel = dynamic_cast<TableModel *>(this->sourceModel());
  assert(tableModel);
  return tableModel->importExpressions(streamIn, groupId);
}

SelectionProxyModel::SelectionProxyModel(expr::Manager &eManagerIn, QObject* parent):
  QSortFilterProxyModel(parent), eManager(eManagerIn)
{
  node = std::make_unique<msg::Node>();
  node->connect(msg::hub());
  sift = std::make_unique<msg::Sift>();
  sift->name = "expr::SelectionProxyModel";
  node->setHandler(std::bind(&msg::Sift::receive, sift.get(), std::placeholders::_1));
  setupDispatcher();
}

SelectionProxyModel::~SelectionProxyModel()
{
  setDynamicSortFilter(false);
}

bool SelectionProxyModel::filterAcceptsRow(int source_row, const QModelIndex&) const
{
  using boost::uuids::uuid;
  
  int formulaId = sourceModel()->data(sourceModel()->index(source_row, 0), Qt::UserRole).toInt();
  std::vector<uuid> linkedParameterIds = eManager.getLinked(formulaId);
  if (linkedParameterIds.empty())
    return false;
  
  for (const auto &container : containers)
  {
    if
    (
      (container.selectionType != slc::Type::Object) &&
      (container.selectionType != slc::Type::Feature)
    )
      continue;
  
    ftr::Base *feature = app::instance()->getProject()->findFeature(container.featureId);
    for (const auto &linkedParameterId : linkedParameterIds)
    {
      if (feature->hasParameter(linkedParameterId))
        return true;
    }
  }
  
  return false;
}

void SelectionProxyModel::setupDispatcher()
{
  sift->insert
  (
    {
      std::make_pair
      (
        msg::Response | msg::Post | msg::Selection | msg::Add
        , std::bind(&SelectionProxyModel::selectionAdditionDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Pre | msg::Selection | msg::Remove
        , std::bind(&SelectionProxyModel::selectionSubtractionDispatched, this, std::placeholders::_1)
      )
    }
  );
}

void SelectionProxyModel::selectionAdditionDispatched(const msg::Message &messageIn)
{
  slc::Message sMessage = messageIn.getSLC();
  slc::Container aContainer;
  aContainer.selectionType = sMessage.type;
  aContainer.featureId = sMessage.featureId;
  aContainer.featureType = sMessage.featureType;
  aContainer.shapeId = sMessage.shapeId;
  aContainer.pointLocation = sMessage.pointLocation;
  containers.push_back(aContainer);
  
  this->invalidateFilter();
}

void SelectionProxyModel::selectionSubtractionDispatched(const msg::Message &messageIn)
{
  slc::Message sMessage = messageIn.getSLC();
  slc::Container aContainer;
  aContainer.selectionType = sMessage.type;
  aContainer.featureId = sMessage.featureId;
  aContainer.featureType = sMessage.featureType;
  aContainer.shapeId = sMessage.shapeId;
  aContainer.pointLocation = sMessage.pointLocation;
  
  slc::Containers::iterator it = std::find(containers.begin(), containers.end(), aContainer);
  assert(it != containers.end());
  containers.erase(it);
  
  this->invalidateFilter();
}

AllProxyModel::AllProxyModel(QObject* parent): QSortFilterProxyModel(parent)
{
  setDynamicSortFilter(false);
}
