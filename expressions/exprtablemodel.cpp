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

#include <boost/current_function.hpp>

#include <QCoreApplication>
#include <QtCore/QTextStream>
#include <QtCore/QStringList>
#include <QtCore/QMimeData>

#include <osg/Geometry> //yuck!

#include <tools/idtools.h>
#include <application/application.h>
#include <project/project.h>
#include <message/node.h>
#include <message/sift.h>
#include <selection/message.h>
#include <feature/base.h>
#include <expressions/manager.h>
#include <expressions/stringtranslator.h>
#include <expressions/tablemodel.h>

using namespace expr;

TableModel::TableModel(Manager &eManagerIn, QObject* parent):
  QAbstractTableModel(parent), lastFailedPosition(-1), lastFailedText(""), lastFailedMessage(""),
  eManager(eManagerIn), sTranslator(new StringTranslator(eManager))
{
}

TableModel::~TableModel(){}

int TableModel::columnCount(const QModelIndex&) const
{
  return 3;
}

int TableModel::rowCount(const QModelIndex&) const
{
  return eManager.allGroup.formulaIds.size();
}

QVariant TableModel::data(const QModelIndex& index, int role) const
{
  if ((role == Qt::DisplayRole) | (role == Qt::EditRole))
  {
    assert(eManager.allGroup.formulaIds.size() > static_cast<std::size_t>(index.row()));
    boost::uuids::uuid currentId = eManager.allGroup.formulaIds.at(index.row());
    if (index.column() == 0)
      return QString::fromStdString(eManager.getFormulaName(currentId));
    if (index.column() == 1)
    {
      if (idToRhsMap.count(currentId) == 0)
        buildOrModifyMapEntry(currentId, sTranslator->buildStringRhs(currentId));
      return QString::fromStdString(getRhs(currentId));
    }
    if (index.column() == 2)
    {
      if (eManager.getFormulaValueType(currentId) == expr::ValueType::Scalar)
        return boost::get<double>(eManager.getFormulaValue(currentId));
      else if (eManager.getFormulaValueType(currentId) == expr::ValueType::Vector)
      {
        osg::Vec3d vector = boost::get<osg::Vec3d>(eManager.getFormulaValue(currentId));
        std::ostringstream stream;
        stream << "["
          << vector.x() << ", "
          << vector.y() << ", "
          << vector.z()
          << "]";
        return QString::fromStdString(stream.str());
      }
      else if (eManager.getFormulaValueType(currentId) == expr::ValueType::Quat)
      {
        osg::Quat quat = boost::get<osg::Quat>(eManager.getFormulaValue(currentId));
        osg::Vec3d quatVec;
        double angle;
        quat.getRotate(angle, quatVec);
        std::ostringstream stream;
        stream << "[["
        << quatVec.x() << ", "
        << quatVec.y() << ", "
        << quatVec.z()
        << "],"
        << angle
        << "]";
        return QString::fromStdString(stream.str());
      }
    }
  }
  
  if (role == Qt::UserRole)
  {
    std::stringstream stream;
    stream << gu::idToString(eManager.allGroup.formulaIds.at(index.row()));
    return QString::fromStdString(stream.str());
  }
  
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
  boost::uuids::uuid fId = gu::stringToId(this->data(index, Qt::UserRole).toString().toStdString());
  assert(eManager.hasFormula(fId));
  if (index.column() == 0)
  {
    //rename formula.
    std::string oldName = eManager.getFormulaName(fId);
    std::string newName = value.toString().toStdString();
    if (oldName == newName)
      return true;
    if (eManager.hasFormula(newName))
    {
      lastFailedMessage = tr("Formula name already exists");
      return false;
    }
    //we need to make sure the new name will pass the parser.
    std::string testParse = newName + "=1";
    if (sTranslator->parseString(testParse) != StringTranslator::ParseSucceeded)
    {
      lastFailedMessage = tr("Invalid characters in formula name");
      return false;
    }
    eManager.update(); //needs to updated before we can call remove.
    eManager.removeFormula(sTranslator->getFormulaOutId());
    eManager.setFormulaName(fId, newName);
    
    //add git message.
    std::ostringstream gitStream;
    gitStream << "Rename expression: " << oldName << ", to: " << newName;
    app::instance()->messageSlot(msg::buildGitMessage(gitStream.str()));
  }
  if (index.column() == 1)
  {
    std::string formulaName = eManager.getFormulaName(fId);
    //store current expression rhs to restore if needed.
    //not really liking the construction of the main LHS. Shouldn't need it.
    std::ostringstream oldStream;
    oldStream << formulaName << "=" << sTranslator->buildStringRhs(fId);
    
    //reparse expression.
    eManager.cleanFormula(fId);
    lastFailedText = value.toString();
    
    std::ostringstream stream;
    stream << formulaName << "=" << value.toString().toStdString();
    if (sTranslator->parseString(stream.str()) == StringTranslator::ParseFailed)
    {
      lastFailedMessage = tr("Parsing failed");
      //we are cacheing the failed position for now because the following call immediately over rights the value.
      lastFailedPosition = sTranslator->getFailedPosition() - formulaName.size() + 1;
      //when parsing fails, translator reverts any changes made during parse. So just add the original.
      if (sTranslator->parseString(oldStream.str()) == StringTranslator::ParseFailed)
      {
        std::cout << "couldn't restore formula in tableModel::setdata:    " << oldStream.str() << std::endl;
        assert(0);
      }
      eManager.update();
      return false;
    }
    std::string cycleName;
    if (eManager.hasCycle(fId, cycleName))
    {
      lastFailedMessage = tr("Cycle detected with ") + QString::fromStdString(cycleName);
      eManager.cleanFormula(fId);
      if (sTranslator->parseString(oldStream.str()) == StringTranslator::ParseFailed)
      {
        std::cout << "couldn't restore formula, from cycle, in tableModel::setdata" << std::endl;
        assert(0);
      }
      lastFailedPosition = value.toString().indexOf(QString::fromStdString(cycleName));
      eManager.update();
      return false;
    }
    eManager.setFormulaDependentsDirty(fId);
    eManager.update();
    
    //add git message.
    std::ostringstream gitStream;
    gitStream << "Edit expression: " << stream.str();
    app::instance()->messageSlot(msg::buildGitMessage(gitStream.str()));
    
  }
  lastFailedText.clear();
  lastFailedMessage.clear();
  lastFailedPosition = -1;
  removeRhs(fId);
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
  std::ostringstream stream;
  for (QModelIndexList::const_iterator it = indexes.constBegin(); it != indexes.constEnd(); ++it)
  {
    stream
      << "ExpressionId;"
      << this->data(this->index(it->row(), 0), Qt::UserRole).toString().toStdString()
      << std::endl;
  }
  
  mimeData->setText(QString::fromStdString(stream.str()));
  return mimeData;
}

std::vector<Group> TableModel::getGroups()
{
  return eManager.userDefinedGroups;
}

void TableModel::addFormulaToGroup(const QModelIndex& indexIn, const QString& groupIdIn)
{
  boost::uuids::uuid groupId = gu::stringToId(groupIdIn.toStdString());
  boost::uuids::uuid formulaId = gu::stringToId(this->data(indexIn, Qt::UserRole).toString().toStdString());
  assert(eManager.hasUserGroup(groupId));
  assert(eManager.hasFormula(formulaId));
  eManager.addFormulaToUserGroup(groupId, formulaId);
}

void TableModel::addDefaultRow()
{
  std::string name;
  std::string expression;
  
  for (int nameIndex = 0; nameIndex < 100000; ++nameIndex) //more than 100000? something is wrong!
  {
    std::ostringstream stream;
    stream << "Default_" << nameIndex;
    name = stream.str();
    if (!eManager.hasFormula(stream.str()))
    {
      stream << "=1.0";
      expression = stream.str();
      break;
    }
  }
  assert(!expression.empty());
  
  this->beginInsertRows(QModelIndex(), eManager.allGroup.formulaIds.size(), eManager.allGroup.formulaIds.size());
  sTranslator->parseString(expression);
  eManager.update();
  this->endInsertRows();
  
  //add git message.
  std::ostringstream gitStream;
  gitStream << "Adding expression: " << name;
  app::instance()->messageSlot(msg::buildGitMessage(gitStream.str()));
}

void TableModel::removeFormula(const QModelIndexList &indexesIn)
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
  
  eManager.update(); //ensure graph is up to date before calling remove formulas.
  //instead of trying to track down any dependent formulas we will just invalidate the model
  beginResetModel();
  for (std::vector<int>::const_iterator rowIt = mappedRows.begin(); rowIt != mappedRows.end(); ++rowIt)
  {
    boost::uuids::uuid currentId = gu::stringToId(this->data(indexMap.value(*rowIt), Qt::UserRole).toString().toStdString());
    
    //add git message. Doing all this in loop because formatting is specific and it's handled in gitManager.
    std::ostringstream gitStream;
    gitStream << "Remove expression: " << eManager.getFormulaName(currentId);
    app::instance()->messageSlot(msg::buildGitMessage(gitStream.str()));
    
    eManager.removeFormula(currentId);
  }
  idToRhsMap.clear();
  endResetModel();
  
  app::instance()->messageSlot(msg::Mask(msg::Request | msg::Project | msg::Update));
}

void TableModel::exportExpressions(QModelIndexList& indexesIn, std::ostream &streamIn) const
{
  std::set<boost::uuids::uuid> selectedIds;
  QModelIndexList::const_iterator it;
  for (it = indexesIn.constBegin(); it != indexesIn.constEnd(); ++it)
    selectedIds.insert(gu::stringToId(this->data(*it, Qt::UserRole).toString().toStdString()));
  
  //loop through all selected ids and get their dependents.
  std::vector<boost::uuids::uuid> dependentIds;
  for (it = indexesIn.constBegin(); it != indexesIn.constEnd(); ++it)
  {
    boost::uuids::uuid currentId = gu::stringToId(this->data(*it, Qt::UserRole).toString().toStdString());
    std::vector<boost::uuids::uuid> tempIds = eManager.getDependentFormulaIds(currentId);
    std::copy(tempIds.begin(), tempIds.end(), std::back_inserter(dependentIds));
  }
  
  //add the dependentIds to the selected set.
  std::copy(dependentIds.begin(), dependentIds.end(), std::inserter(selectedIds, selectedIds.begin()));
  
  //going to loop through all formula ids and see if they are present in the selected ids.
  //this allows us to write the formulas out in a dependent order, so the importing will build the formula
  //in the correct order.
  std::vector<boost::uuids::uuid> allFormulaIds = eManager.getAllFormulaIdsSorted();
  std::vector<boost::uuids::uuid>::const_iterator fIt;
  for (fIt = allFormulaIds.begin(); fIt != allFormulaIds.end(); ++fIt)
  {
    if(selectedIds.find(*fIt) == selectedIds.end())
      continue;
    streamIn << sTranslator->buildStringAll(*fIt) << std::endl;
  }
}

void TableModel::importExpressions(std::istream &streamIn, boost::uuids::uuid groupId)
{
  //preprocess the file.
  std::vector<std::string> filteredExpressions;
  
  std::string lineBuffer;
  while(std::getline(streamIn, lineBuffer))
  {
    if(lineBuffer.empty())
      continue;
    std::size_t position = lineBuffer.find('=');
    if (position == std::string::npos)
      continue;
    std::string name = lineBuffer.substr(0, position);
    name.erase(0, name.find_first_not_of(' '));
    name.erase(name.find_last_not_of(' ') + 1);
    if (name.empty())
      continue;
    //don't import over an expression.
    if (eManager.hasFormula(name))
      continue;
    filteredExpressions.push_back(lineBuffer);
  }
  if (filteredExpressions.empty())
    return;
  
  if (!groupId.is_nil())
    assert(eManager.hasUserGroup(groupId));
  
  int currentSize = eManager.allGroup.formulaIds.size();
  int countAdded = 0;
  std::vector<std::string>::const_iterator it;
  for (it = filteredExpressions.begin(); it != filteredExpressions.end(); ++it)
  {
    if (sTranslator->parseString(*it) != StringTranslator::ParseSucceeded)
      continue;
    if (!groupId.is_nil())
    {
      boost::uuids::uuid fId = sTranslator->getFormulaOutId();
      eManager.addFormulaToUserGroup(groupId, fId);
    }
    eManager.update(); //see project::open on why this has to be done in the loop.
    countAdded++;
  }
  
  /* ideally I would do the 'beginInsertRows' call before actually modifing the underlying
   * model data. The problem is, I don't know how many rows are going to be added until I
   * get through the parsing. If parsing fails on a particular expression it gets skipped.
   */
  this->beginInsertRows(QModelIndex(), currentSize, currentSize + countAdded - 1);
  this->endInsertRows();
}

void TableModel::buildOrModifyMapEntry(const boost::uuids::uuid &idIn, const std::string &rhsIn) const
{
  IdToRhsMap::iterator it;
  bool dummy;
  std::tie(it, dummy) = idToRhsMap.insert(std::make_pair(idIn, rhsIn));
  it->second = rhsIn;
}

std::string TableModel::getRhs(const boost::uuids::uuid &idIn) const
{
  IdToRhsMap::iterator it = idToRhsMap.find(idIn);
  assert(it != idToRhsMap.end());
  return it->second;
}

void TableModel::removeRhs(const boost::uuids::uuid &idIn) const
{
  IdToRhsMap::iterator it = idToRhsMap.find(idIn);
  assert(it != idToRhsMap.end());
  idToRhsMap.erase(it);
}

void TableModel::parseStringSlot(const QString &textIn)
{
  Q_EMIT parseWorkingSignal();
  qApp->processEvents(); //need this or we never see yellow signal.
  
  assert(!eManager.hasFormula(testFormulaName));
  
  std::ostringstream parseStream;
  parseStream << testFormulaName << "=" << textIn.toStdString();
  if (sTranslator->parseString(parseStream.str()) != StringTranslator::ParseSucceeded)
  {
    int position = sTranslator->getFailedPosition() - testFormulaName.size() - 1;
    QString string = textIn.left(position) + "?";
    Q_EMIT parseFailedSignal(string);
    Q_EMIT parseFailedSignal();
  }
  else
  {
    sTranslator->eManager.update();
    QString string;
    if(sTranslator->eManager.getFormulaValueType(sTranslator->getFormulaOutId()) == expr::ValueType::Scalar)
    {
      string = QString::number(boost::get<double>(sTranslator->eManager.getFormulaValue(
        sTranslator->getFormulaOutId())));
    }
    else if (sTranslator->eManager.getFormulaValueType(sTranslator->getFormulaOutId()) == expr::ValueType::Vector)
    {
      osg::Vec3d vector = boost::get<osg::Vec3d>(sTranslator->eManager.getFormulaValue(
        sTranslator->getFormulaOutId()));
      std::ostringstream stream;
      stream << "["
        << vector.x() << ", "
        << vector.y() << ", "
        << vector.z()
        << "]";
      string = QString::fromStdString(stream.str());
    }
    else if (sTranslator->eManager.getFormulaValueType(sTranslator->getFormulaOutId()) == expr::ValueType::Quat)
    {
      osg::Quat quat = boost::get<osg::Quat>(sTranslator->eManager.getFormulaValue(
        sTranslator->getFormulaOutId()));
      osg::Vec3d quatVec;
      double angle;
      quat.getRotate(angle, quatVec);
      std::ostringstream stream;
      stream << "[["
        << quatVec.x() << ", "
        << quatVec.y() << ", "
        << quatVec.z()
        << "],"
        << angle
        << "]";
      string = QString::fromStdString(stream.str());
    }
    Q_EMIT parseSucceededSignal(string);
    Q_EMIT parseSucceededSignal();
  }
  
  if (eManager.hasFormula(testFormulaName))
    eManager.removeFormula(testFormulaName);
}

BaseProxyModel::BaseProxyModel(QObject* parent): QSortFilterProxyModel(parent)
{

}

//this allows us to add a row at the bottom of the table. then edit the name, then tab and edit formula and then sort.
bool BaseProxyModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  bool out = QSortFilterProxyModel::setData(index, value, role);
  if (index.column() == 1 && out)
    QMetaObject::invokeMethod(this, "delayedSortSlot", Qt::QueuedConnection);
  return out;
}

void BaseProxyModel::delayedSortSlot()
{
  this->sort(0);
}

GroupProxyModel::GroupProxyModel(Manager &eManagerIn, boost::uuids::uuid groupIdIn, QObject* parent)
  : BaseProxyModel(parent), eManager(eManagerIn), groupId(groupIdIn)
{

}

bool GroupProxyModel::filterAcceptsRow(int source_row, const QModelIndex&) const
{
  boost::uuids::uuid formulaId =
    gu::stringToId(sourceModel()->data(sourceModel()->index(source_row, 0), Qt::UserRole).toString().toStdString());
  
  return eManager.doesUserGroupContainFormula(groupId, formulaId);
}

QModelIndex GroupProxyModel::addDefaultRow()
{
  TableModel *myModel = dynamic_cast<TableModel *>(this->sourceModel());
  assert(myModel);
  int rowCount = myModel->rowCount();
  myModel->addDefaultRow();
  QModelIndex sourceIndex = myModel->index(rowCount, 0);
  boost::uuids::uuid formulaId = gu::stringToId(myModel->data(sourceIndex, Qt::UserRole).toString().toStdString());
  eManager.addFormulaToUserGroup(groupId, formulaId);
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
    boost::uuids::uuid id = gu::stringToId
      (this->sourceModel()->data(currentSource, Qt::UserRole).toString().toStdString());
    assert(eManager.doesUserGroupContainFormula(groupId, id));
    eManager.removeFormulaFromUserGroup(groupId, id);
  }
  QMetaObject::invokeMethod(this, "refreshSlot", Qt::QueuedConnection);
}

std::string GroupProxyModel::getGroupName() const
{
  assert(eManager.hasUserGroup(groupId));
  return eManager.getUserGroupName(groupId);
}

bool GroupProxyModel::renameGroup(const std::string& newName)
{
  assert(eManager.hasUserGroup(groupId));
  if (eManager.hasUserGroup(newName))
    return false;
  eManager.renameUserGroup(groupId, newName);
  return true;
}

void GroupProxyModel::refreshSlot()
{
  this->invalidateFilter();
  this->sort(0);
}

void GroupProxyModel::removeGroup()
{
  assert(eManager.hasUserGroup(groupId));
  eManager.removeUserGroup(groupId);
}

void GroupProxyModel::importExpressions(std::istream &streamIn)
{
  TableModel *tableModel = dynamic_cast<TableModel *>(this->sourceModel());
  assert(tableModel);
  tableModel->importExpressions(streamIn, groupId);
}

SelectionProxyModel::SelectionProxyModel(expr::Manager &eManagerIn, QObject* parent):
  BaseProxyModel(parent), eManager(eManagerIn)
{
  node = std::make_unique<msg::Node>();
  node->connect(msg::hub());
  sift = std::make_unique<msg::Sift>();
  sift->name = "expr::SelectionProxyModel";
  node->setHandler(std::bind(&msg::Sift::receive, sift.get(), std::placeholders::_1));
  setupDispatcher();
}

SelectionProxyModel::~SelectionProxyModel(){}

bool SelectionProxyModel::filterAcceptsRow(int source_row, const QModelIndex&) const
{
  using boost::uuids::uuid;
  
  boost::uuids::uuid formulaId = gu::stringToId(sourceModel()->data(sourceModel()->index(source_row, 0), Qt::UserRole).toString().toStdString());
  std::vector<uuid> linkedParameterIds = eManager.getParametersLinked(formulaId);
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
  this->sort(0);
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
  this->sort(0);
}

AllProxyModel::AllProxyModel(QObject* parent): BaseProxyModel(parent)
{
}

void AllProxyModel::refresh()
{
  this->sort(0);
}