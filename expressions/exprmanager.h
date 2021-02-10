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

#ifndef EXPR_MANAGER_H
#define EXPR_MANAGER_H

#include <memory>

#include <boost/uuid/uuid.hpp>

#include <lccresult.h>
#include <lccserial.h>

#include "expressions/exprvalue.h"

class QTextStream;

namespace prm{class Parameter;}
namespace msg{struct Message;}
namespace prj{namespace srl{namespace prjs{class Expression;}}}

namespace expr{

/*! @class Manager @brief interface to store and manage expressions.
 * 
 * Expression and groups are stored in libcadcalc. Because
 * naming can be volatile, groups and expressions have an id so they can
 * be referenced in a static way. Use ids over names whenever possible.
 * Groups only contain ids to the expressions they hold.
 */
class Manager
{
public:
  Manager();
  ~Manager();
  
  //! write a graphviz file for the graph.
  void writeOutGraph(const std::string &pathName);
  //! Write a list of links to stream.
  void dumpLinks(std::ostream &stream) const;
  //! Write expression information to Qt stream.
  QTextStream& getInfo(QTextStream&) const;
  
public:
  lcc::Result parseString(const std::string&);
  
  /*! @name Make. Create a new typed expression with a unique name
   * based from passed in string and an index. returns id of new expression
   */
  ///@{
  int makeScalar(const std::string&);
  int makeVector(const std::string&);
  int makeRotation(const std::string&);
  int makeCSys(const std::string&);
  ///@}
  
  std::string buildExpressionString(const std::string&) const;
  std::string buildExpressionString(int) const;
  std::string buildRHSString(const std::string&) const;
  std::string buildRHSString(int) const;
  bool hasExpression(const std::string&);
  bool hasExpression(int);
  std::optional<int> getExpressionId(const std::string&) const;
  std::optional<std::string> getExpressionName(int) const;
  void removeExpression(const std::string&);
  void removeExpression(int);
  void renameExpression(const std::string&, const std::string&);
  void renameExpression(int, const std::string&);
  std::vector<std::string> getExpressionNames() const;
  std::vector<int> getExpressionIds() const;
  
  Value getExpressionValue(const std::string&) const;
  Value getExpressionValue(int) const;
  
  bool hasGroup(const std::string&) const;
  bool hasGroup(int) const;
  int addGroup(const std::string&);
  void removeGroup(int);
  void removeGroup(const std::string&);
  std::vector<int> getGroupIds() const;
  std::vector<std::string> getGroupNames() const;
  std::optional<std::string> getGroupName(int) const;
  std::optional<int> getGroupId(const std::string&) const;
  void renameGroup(int, const std::string&);
  
  void addExpressionToGroup(int group, int expression);
  void removeExpressionFromGroup(int group, int expression);
  bool groupHasExpression(int group, int expression) const;
  const std::vector<int>& getExpressionsOfGroup(int group) const;
  
  void exportExpressions(std::ostream&) const;
  void exportExpressions(std::ostream&, const std::vector<int>&) const;
  std::vector<lcc::Result> importExpressions(std::istream&);
  
  /*! @name Expression links. integers are expression ids. uuids are parameters.
   * A Parameter can be linked to only 1 expression, however an expression
   * can be linked to many parameters.
   */
  ///@{
  bool canLinkExpression(prm::Parameter*, int); //check compatible types.
  bool canLinkExpression(const boost::uuids::uuid&, int); //check compatible types.
  bool addLink(const boost::uuids::uuid&, int); //!< only if types allow and a valid parameter value.
  bool addLink(prm::Parameter*, int); //!< only if types allow and a valid parameter value.
  void removeLink(const boost::uuids::uuid&); //!< remove the link by parameter id. 0 or 1.
  void removeLink(int); //!< remove all links to expression. 0 or many.
  bool isLinked(const boost::uuids::uuid&) const;
  bool isLinked(int) const;
  std::optional<int> getLinked(const boost::uuids::uuid&) const; //!< get optional expression id linked to parameter id passed in. 0 or 1
  std::vector<boost::uuids::uuid> getLinked(int) const; //!< get all parameter ids linked to passed in expression id. 0 or many.
  void dispatchLinks(const std::vector<int>&); //!< assign values from expressions to parameters.
  bool assignParameter(prm::Parameter*, int); //!< assign expression value to parameter without creating a link.
  ///@}
  
  prj::srl::prjs::Expression serialOut() const;
  void serialIn(const prj::srl::prjs::Expression&);

private:
  struct Stow;
  std::unique_ptr<Stow> stow;
  void setupDispatcher();
  void featureRemovedDispatched(const msg::Message &);
};

}

#endif // EXPR_MANAGER_H
