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

#ifndef EXPR_TABLEMODEL_H
#define EXPR_TABLEMODEL_H

#include <memory>
#include <vector>

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

#include "selection/slccontainer.h"

namespace lcc{struct Result;}
namespace msg{struct Message; struct Node; struct Sift;}

namespace expr{
  class Manager;

/*! @class TableModel @brief Main interface between Manager and Qt MVC
 * 
 */
class TableModel : public QAbstractTableModel
{
  Q_OBJECT
public:
    explicit TableModel(expr::Manager &eManagerIn, QObject* parent = 0);
    ~TableModel() override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QStringList mimeTypes() const override;
    QMimeData* mimeData (const QModelIndexList &) const override;
    //! Add expression to a group.
    void addExpressionToGroup(const QModelIndex &indexIn, int groupIdIn);
    //! Remove expressions
    void removeExpression(const QModelIndexList &indexesIn);
    //! Export expressions to a text file.
    void exportExpressions(QModelIndexList&, std::ostream&) const;
    //! Import expressions from a text file. Will over write existing expressions.
    std::vector<lcc::Result> importExpressions(std::istream &streamIn, int groupId = -1);
    
    //! Message indicating the cause of the failure.
    QString lastFailedMessage;
    
    //! @name Adding new expression by type.
    ///@{
    void addScalar();
    void addVector();
    void addRotation();
    void addCSys();
    ///@}
    
    const expr::Manager& getManager(){return eManager;}
    
  //@{
  //! items related to temp parsing in the editor delegate.
public Q_SLOTS:
  void parseStringSlot(const QString &textIn); //!< no lhs or equals.
Q_SIGNALS:
  void parseWorkingSignal();
  void parseSucceededSignal(const QString&); //!< string representation of value.
  void parseSucceededSignal();
  void parseFailedSignal(const QString&); //!< string showing failure position.
  void parseFailedSignal();
  void groupChangedSignal(int); //!< adding happens from 'all' table and we need to let the group model know.
private:
  const std::string testFormulaName = "f9d2e8f0_2354_40ef_8d3c_4b8ced3a2504"; //!< unique name for temp.
  //@}
    
private:
  //! Manager containing expressions and groups.
  expr::Manager &eManager;
  //! tableview calls into ::data every paint event. Way too many! cache rhs strings for speed.
  mutable std::map<int, std::string> idToRhsMap;
  
  std::string createUniqueName(const std::string&) const;
  void addCommon(const std::string&);
};

/*! @brief Proxy model for the grouping view.
 */
class GroupProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT
public:
  explicit GroupProxyModel(expr::Manager &eManagerIn, int groupIdIn, QObject *parent = 0);
  //! Remove the expressions from the group.
  void removeFromGroup(const QModelIndexList &indexesIn);
  //! Rename the user group this model is referencing.
  bool renameGroup(const std::string &newName);
  //! Get the name of the user group this model is referencing.
  std::string getGroupName() const;
  //! Remove the group from manager and signal tab widget remove view.
  void removeGroup();
  //! Import the expressions from the file and add to the user group.
  std::vector<lcc::Result> importExpressions(std::istream &streamIn);
  
  //! @name Adding new expression by type.
  ///@{
  QModelIndex addScalar();
  QModelIndex addVector();
  QModelIndex addRotation();
  QModelIndex addCSys();
  ///@}
  
protected:
  bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
  
public Q_SLOTS:
  void groupChangedSlot(int);
private:
  //! Reference to manager
  expr::Manager &eManager;
  //! Group id this model is referencing.
  int groupId;
};

/* this one is a little different in that we are NOT letting it get of sync.
 * I don't like this but I don't want to an observer to both model and view.
 */
//! @brief Proxy model for selection expressions view.
class SelectionProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT
public:
  explicit SelectionProxyModel(expr::Manager &, QObject *parent = 0);
  ~SelectionProxyModel() override;
  
protected:
  bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
  
private:
  //! Reference to manager
  expr::Manager &eManager;
  std::unique_ptr<msg::Node> node;
  std::unique_ptr<msg::Sift> sift;
  void setupDispatcher();
  slc::Containers containers;
  void selectionAdditionDispatched(const msg::Message&);
  void selectionSubtractionDispatched(const msg::Message&);
};

//! @brief Proxy model for all expressions view.
class AllProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT
public:
  explicit AllProxyModel(QObject *parent = 0);
};
}

#endif // EXPR_TABLEMODEL_H
