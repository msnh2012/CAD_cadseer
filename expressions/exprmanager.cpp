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
#include <iostream>
#include <iomanip>
#include <sstream>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>

#include <QTextStream>

#include <lccmanager.h>

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "project/prjmessage.h"
#include "feature/ftrbase.h"
#include "parameter/prmparameter.h"
#include "parameter/prmvariant.h"
#include "message/msgnode.h"
#include "message/msgsift.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"

#include "tools/idtools.h"
#include "tools/tlsnameindexer.h"
#include "project/serial/generated/prjsrlprjsproject.h"
#include "expressions/exprmanager.h"

using namespace boost::uuids;

namespace
{
  /*! @brief Link between expression and parameters
   * 
   * boost multi_index link.
   */
  struct Link
  {
    Link(const uuid &pIdIn, int fIdIn): parameterId(pIdIn), expressionId(fIdIn){}
    uuid parameterId;
    int expressionId;
    
    //@{
    //! used as tags.
    struct ByParameterId{};
    struct ByExpressionId{};
    //@}
  };

  namespace BMI = boost::multi_index;
  using Links = boost::multi_index_container
  <
    Link,
    BMI::indexed_by
    <
      BMI::ordered_unique //parameter can only be linked to one formula
      <
        BMI::tag<Link::ByParameterId>,
        BMI::member<Link, uuid, &Link::parameterId>
      >,
      BMI::ordered_non_unique //expression can be linked to many parameters.
      <
        BMI::tag<Link::ByExpressionId>,
        BMI::member<Link, int, &Link::expressionId>
      >
    >
  >;
}

using namespace expr;

struct Manager::Stow
{
  lcc::Manager manager;
  Links links;
  msg::Node node;
  msg::Sift sift;
  
  //used to test compatibility between exrpressions and parameters.
  //will assign value if assign=true
  class ParameterValueVisitor : public boost::static_visitor<Amity>
  {
    prm::Parameter *parameter = nullptr;
    expr::Value ev;
    bool assign = false;
  public:
    ParameterValueVisitor(prm::Parameter *pIn, const expr::Value &evIn, bool assignIn)
    : parameter(pIn)
    , ev(evIn)
    , assign(assignIn)
    {}
    
    Amity operator()(double) const
    {
      expr::DoubleVisitor edv;
      double newValue = boost::apply_visitor(edv, ev);
      if (!parameter->isValidValue(newValue))
        return Amity::InvalidValue;
      if (assign)
      {
        if (parameter->setValue(newValue))
          return Amity::Mutate;
        else
          return Amity::Equal;
      }
      return Amity::Mutate;
    }
    
    Amity operator()(int) const
    {
      expr::DoubleVisitor edv;
      int newValue = static_cast<int>(boost::apply_visitor(edv, ev));
      if (!parameter->isValidValue(newValue))
        return Amity::InvalidValue;
      if (assign)
      {
        if (parameter->setValue(newValue))
          return Amity::Mutate;
        else
          return Amity::Equal;
      }
      return Amity::Mutate;
    }
    Amity operator()(bool) const
    {
      expr::DoubleVisitor edv;
      bool newValue = static_cast<bool>(boost::apply_visitor(edv, ev));
      if (!parameter->isValidValue(newValue))
        return Amity::InvalidValue;
      if (assign)
      {
        if (parameter->setValue(newValue))
          return Amity::Mutate;
        else
          return Amity::Equal;
      }
      return Amity::Mutate;
    }
    Amity operator()(const std::string&) const {assert(0); return Amity::Incompatible;} //currently unsupported formula type.
    Amity operator()(const boost::filesystem::path&) const {assert(0); return Amity::Incompatible;} //currently unsupported formula type.
    Amity operator()(const osg::Vec3d&) const
    {
      expr::VectorVisitor evv;
      osg::Vec3d newValue = boost::apply_visitor(evv, ev);
      if (assign)
      {
        if (parameter->setValue(newValue))
          return Amity::Mutate;
        else
          return Amity::Equal;
      }
      return Amity::Mutate;
    }
    Amity operator()(const osg::Quat&) const
    {
      expr::QuatVisitor eqv;
      osg::Quat newValue = boost::apply_visitor(eqv, ev);
      if (assign)
      {
        if (parameter->setValue(newValue))
          return Amity::Mutate;
        else
          return Amity::Equal;
      }
      return Amity::Mutate;
    }
    Amity operator()(const osg::Matrixd&) const
    {
      expr::MatrixVisitor emv;
      osg::Matrixd newValue = boost::apply_visitor(emv, ev);
      if (assign)
      {
        if (parameter->setValue(newValue))
          return Amity::Mutate;
        else
          return Amity::Equal;
      }
      return Amity::Mutate;
    }
    Amity operator()(const ftr::Picks&) const {assert(0); return Amity::Incompatible;}
  };
  
  //can't make parameter const. We can assign or test.
  Amity common(prm::Parameter *parameter, const expr::Value &evIn, bool assign)
  {
    //we only visit if the types are acceptable otherwise a crash.
    ParameterValueVisitor v(parameter, evIn, assign);
    auto goVisit = [&]() -> Amity {return boost::apply_visitor(v, parameter->getStow().variant);};
    
    if (parameter->getValueType() == evIn.type())
      return goVisit();
    //here we allow conversions from an expression double to an int or boolean parameter.
    if (evIn.type() == typeid(double) &&  (parameter->getValueType() == typeid(int) || parameter->getValueType() == typeid(bool)))
      return goVisit();
    
    return Amity::Incompatible;
  }
  
  Value getExpressionValue(int idIn) const
  {
    Value out;
    auto lv = manager.getExpressionValue(idIn);
    if (lv.size() == 1)
      out = lv.at(0);
    else if (lv.size() == 3)
      out = osg::Vec3d(lv.at(0), lv.at(1), lv.at(2));
    else if (lv.size() == 4)
      out = osg::Quat(lv.at(0), lv.at(1), lv.at(2), lv.at(3));
    else if (lv.size() == 7)
    {
      osg::Matrixd temp;
      temp.setRotate(osg::Quat(lv.at(0), lv.at(1), lv.at(2), lv.at(3)));
      temp.setTrans(lv.at(4), lv.at(5), lv.at(6));
      out = temp;
    }
    else
      assert(0); //unrecognized value from libcadcalc
    return out;
  }
  
  bool canLinkExpression(prm::Parameter *p, int eId)
  {
    auto ev = getExpressionValue(eId);
    if (common(p, ev, false) >= Amity::Equal)
      return true;
    return false;
  }

  bool canLinkExpression(const uuid &pId, int eId)
  {
    prm::Parameter *p = app::instance()->getProject()->findParameter(pId);
    assert(p);
    if (!p)
      return false;
    return canLinkExpression(p, eId);
  }

  Amity addLink(const uuid &pId, int eId)
  {
    prm::Parameter *p = app::instance()->getProject()->findParameter(pId);
    assert(p);
    if (!p)
      return Amity::Incompatible;
    return addLink(p, eId);
  }
  
  Amity addLink(prm::Parameter *p, int eId)
  {
    auto ev = getExpressionValue(eId);
    auto result = common(p, ev, true);
    if (result >= Amity::Equal)
    {
      removeLink(p->getId()); //it is ok if it doesn't exist.
      links.insert(Link(p->getId(), eId));
      p->setConstant(false);
      
      std::ostringstream gm;
      gm << "Linking parameter: " << p->getName().toStdString() << " to formula: " << *manager.getExpressionName(eId);
      app::instance()->messageSlot(msg::buildGitMessage(gm.str()));
    }
    
    return result;
  }

  void removeLink(const uuid &pId)
  {
    //a parameter can only have one link. so no equal range for this.
    auto it = links.get<Link::ByParameterId>().find(pId);
    if (it != links.get<Link::ByParameterId>().end())
    {
      prm::Parameter *p = app::instance()->getProject()->findParameter(pId);
      assert(p); if (!p) return;
      
      p->setConstant(true);
      std::ostringstream gm;
      gm << "Unlinking parameter: " << p->getName().toStdString();
      app::instance()->messageSlot(msg::buildGitMessage(gm.str()));
      
      links.get<Link::ByParameterId>().erase(it);
    }
  }

  void removeLink(int eId)
  {
    auto its = links.get<Link::ByExpressionId>().equal_range(eId);
    
    for (auto copy = its; copy.first != copy.second; ++copy.first)
    {
      prm::Parameter *p = app::instance()->getProject()->findParameter(copy.first->parameterId);
      assert(p); if (!p) continue;
      p->setConstant(true);
      std::ostringstream gm;
      gm << "Unlinking parameter: " << p->getName().toStdString();
      app::instance()->messageSlot(msg::buildGitMessage(gm.str()));
    }
    
    links.get<Link::ByExpressionId>().erase(its.first, its.second);
  }

  bool isLinked(const uuid &pId) const
  {
    return links.get<Link::ByParameterId>().find(pId) != links.get<Link::ByParameterId>().end();
  }

  bool isLinked(int eId) const
  {
    //expressions can be linked to multiple parameters but we don't care, just look for first one here.
    return links.get<Link::ByExpressionId>().find(eId) != links.get<Link::ByExpressionId>().end();
  }

  std::optional<int> getLinked(const uuid &pId) const
  {
    auto it = links.get<Link::ByParameterId>().find(pId);
    if (it == links.get<Link::ByParameterId>().end())
      return std::nullopt;
    return it->expressionId;
  }

  std::vector<uuid> getLinked(int eId) const
  {
    std::vector<uuid> out;
    auto its = links.get<Link::ByExpressionId>().equal_range(eId);
    for (; its.first != its.second; ++its.first)
      out.emplace_back(its.first->parameterId);
    return out;
  }

  void dispatchLinks(const std::vector<int> &eIds)
  {
    for (auto eId : eIds)
    {
      auto its = links.get<Link::ByExpressionId>().equal_range(eId);
      for (; its.first != its.second; ++its.first)
      {
        prm::Parameter *p = app::instance()->getProject()->findParameter(its.first->parameterId);
        assert(p); if (!p) continue;
        auto ev = getExpressionValue(its.first->expressionId);
        if (common(p, ev, true) <= Amity::InvalidValue)
          app::instance()->messageSlot(msg::buildStatusMessage("Failed to update parameter: " + p->getName().toStdString(), 2.0));
      }
    }
  }
  
  Amity assignParameter(prm::Parameter *p, int eId)
  {
    auto ev = getExpressionValue(eId);
    return common(p, ev, true);
  }
  
  //builds a new default expression.
  int make(const std::string &base, const std::string &rhs)
  {
    tls::NameIndexer ni(100);
    std::string current = base + ni.buildSuffix();
    while (manager.hasExpression(current))
      current = base + "_" + ni.buildSuffix();
    current += rhs;
    auto result = manager.parseString(current);
    assert(result.isAllGood());
    auto oId = manager.getExpressionId(result.expressionName);
    assert(oId);
    return *oId;
  }
};

Manager::Manager() : stow(std::make_unique<Stow>())
{
  stow->node.connect(msg::hub());
  stow->sift.name = "expr::Manager";
  stow->node.setHandler(std::bind(&msg::Sift::receive, &stow->sift, std::placeholders::_1));
  setupDispatcher();
}

Manager::~Manager() = default;

void Manager::writeOutGraph(const std::string& pathName)
{
  stow->manager.writeOutGraph(pathName);
}

void Manager::dumpLinks(std::ostream& stream) const
{
  for (const auto &c : stow->links.get<Link::ByExpressionId>())
  {
    prm::Parameter *p = app::instance()->getProject()->findParameter(c.parameterId);
    assert(p);
    
    stream << "parameter id: " << gu::idToString(p->getId())
      << "    parameter name: " << std::left << std::setw(20) << p->getName().toStdString()
      << "    expression id: " << c.expressionId << std::endl;
  }
}

void Manager::setupDispatcher()
{
  stow->sift.insert
  (
    msg::Response | msg::Pre | msg::Remove | msg::Feature
    , std::bind(&Manager::featureRemovedDispatched, this, std::placeholders::_1)
  );
}

void Manager::featureRemovedDispatched(const msg::Message &messageIn)
{
  prj::Message message = messageIn.getPRJ();
  uuid featureId = message.feature->getId();
  
  auto *f = app::instance()->getProject()->findFeature(featureId);
  assert(f); if (!f) return;
  
  for (const auto &p : f->getParameters())
    removeLink(p->getId());
}

QTextStream& Manager::getInfo(QTextStream &stream) const
{
  stream << Qt::endl << QObject::tr("Expressions:") << Qt::endl;
  for (auto id : getExpressionIds())
  {
    std::ostringstream temp; temp << getExpressionValue(id);
    stream << QString::fromStdString(*getExpressionName(id))
    << "    " << QString::fromStdString(temp.str()) << Qt::endl;
  }
  
  return stream;
}

lcc::Result Manager::parseString(const std::string &sIn)
{
  auto result = stow->manager.parseString(sIn);
  dispatchLinks(result.updatedIds);
  return result;
}

int Manager::makeScalar(const std::string &base)
{
  return stow->make(base, " = 0.0");
}

int Manager::makeVector(const std::string &base)
{
  return stow->make(base, " = VZERO");
}

int Manager::makeRotation(const std::string &base)
{
  return stow->make(base, " = RZERO");
}

int Manager::makeCSys(const std::string &base)
{
  return stow->make(base, " = CZERO");
}

std::string Manager::buildExpressionString(const std::string &sIn) const
{
  return stow->manager.buildExpressionString(sIn);
}
std::string Manager::buildExpressionString(int idIn) const
{
  return stow->manager.buildExpressionString(idIn);
}

std::string Manager::buildRHSString(const std::string &sIn) const
{
  auto oid = stow->manager.getExpressionId(sIn);
  assert(oid);
  return buildRHSString(*oid);
}

std::string Manager::buildRHSString(int idIn) const
{
  assert(stow->manager.getExpressionName(idIn));
  std::string fullExpression = stow->manager.buildExpressionString(idIn);
  assert(!fullExpression.empty());
  auto pos = fullExpression.find('=', 0);
  assert(pos != std::string::npos);
  auto start = fullExpression.find_first_not_of(' ', pos + 1);
  assert(start != std::string::npos);
  return fullExpression.substr(start);
}

bool Manager::hasExpression(const std::string &nameIn)
{
  return stow->manager.hasExpression(nameIn);
}

bool Manager::hasExpression(int idIn)
{
  return stow->manager.hasExpression(idIn);
}

std::optional<int> Manager::getExpressionId(const std::string &sIn) const
{
  return stow->manager.getExpressionId(sIn);
}

std::optional<std::string> Manager::getExpressionName(int idIn) const
{
  return stow->manager.getExpressionName(idIn);
}

void Manager::removeExpression(const std::string &sIn)
{
  auto oeid = stow->manager.getExpressionId(sIn);
  if (oeid)
  {
    stow->removeLink(*oeid);
    stow->manager.removeExpression(*oeid);
  }
  stow->manager.removeExpression(sIn);
}

void Manager::removeExpression(int idIn)
{
  stow->removeLink(idIn);
  stow->manager.removeExpression(idIn);
}

void Manager::renameExpression(const std::string &oldName, const std::string &newName)
{
  stow->manager.renameExpression(oldName, newName);
}

void Manager::renameExpression(int idIn, const std::string &newName)
{
  stow->manager.renameExpression(idIn, newName);
}

std::vector<std::string> Manager::getExpressionNames() const
{
  return stow->manager.getExpressionNames();
}

std::vector<int> Manager::getExpressionIds() const
{
  return stow->manager.getExpressionIds();
}

Value Manager::getExpressionValue(const std::string &sIn) const
{
  auto oId = stow->manager.getExpressionId(sIn);
  assert(oId);
  return getExpressionValue(*oId);
}

Value Manager::getExpressionValue(int idIn) const
{
  return stow->getExpressionValue(idIn);
}

bool Manager::hasGroup(const std::string &sIn) const
{
  return stow->manager.hasGroup(sIn);
}

bool Manager::hasGroup(int idIn) const
{
  return stow->manager.hasGroup(idIn);
}

int Manager::addGroup(const std::string &newGroupName)
{
  return stow->manager.addGroup(newGroupName);
}

void Manager::removeGroup(int groupId)
{
  stow->manager.removeGroup(groupId);
}

void Manager::removeGroup(const std::string &groupName)
{
  stow->manager.removeGroup(groupName);
}

std::vector<int> Manager::getGroupIds() const
{
  return stow->manager.getGroupIds();
}

std::vector<std::string> Manager::getGroupNames() const
{
  return stow->manager.getGroupNames();
}

std::optional<std::string> Manager::getGroupName(int idIn) const
{
  return stow->manager.getGroupName(idIn);
}

std::optional<int> Manager::getGroupId(const std::string &groupName) const
{
  return stow->manager.getGroupId(groupName);
}

void Manager::renameGroup(int idIn, const std::string &newName)
{
  stow->manager.renameGroup(idIn, newName);
}

void Manager::addExpressionToGroup(int group, int expression)
{
  stow->manager.addExpressionToGroup(group, expression);
}

void Manager::removeExpressionFromGroup(int group, int expression)
{
  stow->manager.removeExpressionFromGroup(group, expression);
  //update links!
}

bool Manager::groupHasExpression(int group, int expression) const
{
  return stow->manager.groupHasExpression(group, expression);
}

const std::vector<int>& Manager::getExpressionsOfGroup(int group) const
{
  return stow->manager.getExpressionsOfGroup(group);
}

void Manager::exportExpressions(std::ostream &os) const
{
  stow->manager.exportExpressions(os);
}

void Manager::exportExpressions(std::ostream &os, const std::vector<int> &eIdsIn) const
{
  stow->manager.exportExpressions(os, eIdsIn);
}

std::vector<lcc::Result> Manager::importExpressions(std::istream &is)
{
  return stow->manager.importExpressions(is);
}

//can't make parameter const as this function uses supporting functions common with assigning.
bool Manager::canLinkExpression(prm::Parameter *p, int eId)
{
  return stow->canLinkExpression(p, eId);
}

bool Manager::canLinkExpression(const boost::uuids::uuid& pId, int eId)
{
  return stow->canLinkExpression(pId, eId);
}

Amity Manager::addLink(const uuid &pId, int eId)
{
  return stow->addLink(pId, eId);
}

Amity Manager::addLink(prm::Parameter *p, int eId)
{
  return stow->addLink(p, eId);
}

void Manager::removeLink(const uuid &pId)
{
  stow->removeLink(pId);
}

void Manager::removeLink(int eId)
{
  stow->removeLink(eId);
}

bool Manager::isLinked(const uuid &pId) const
{
  return stow->isLinked(pId);
}

bool Manager::isLinked(int eId) const
{
  return stow->isLinked(eId);
}

std::optional<int> Manager::getLinked(const uuid &pId) const
{
  return stow->getLinked(pId);
}

std::vector<uuid> Manager::getLinked(int eId) const
{
  return stow->getLinked(eId);
}

void Manager::dispatchLinks(const std::vector<int> &eIds)
{
  stow->dispatchLinks(eIds);
}

Amity Manager::assignParameter(prm::Parameter *p, int eId)
{
  return stow->assignParameter(p, eId);
}

prj::srl::prjs::Expression Manager::serialOut() const
{
  prj::srl::prjs::Expression out;
  auto serialOut = stow->manager.serialOut();
  for (const auto &e : serialOut.expressions)
    out.expressions().push_back(prj::srl::prjs::Expressions(e.id, e.expression));
  for (const auto &g : serialOut.groups)
  {
    prj::srl::prjs::Groups go(g.id, g.name);
    for (const auto &eid : g.expressionIds)
      go.entries().push_back(eid);
    out.groups().push_back(go);
  }
  for (const auto &l : stow->links.get<Link::ByParameterId>())
    out.links().push_back(prj::srl::prjs::Links(gu::idToString(l.parameterId), l.expressionId));
  return out;
}

void Manager::serialIn(const prj::srl::prjs::Expression &sIn)
{
  lcc::Serial lccSerialIn;
  for (const auto &e : sIn.expressions())
    lccSerialIn.expressions.push_back({e.id(), e.stringForm()});
  for (const auto &g : sIn.groups())
    lccSerialIn.groups.push_back({g.id(), g.name(), g.entries()});
  auto results = stow->manager.serialIn(lccSerialIn);
  
  for (const auto &l : sIn.links())
    stow->links.insert(Link(gu::stringToId(l.parameterId()), l.expressionId()));
  
  assert(results.empty());
  if (!results.empty())
  {
    //what to do here? should I clean links.
    std::cout << "Error: problem loading expressions" << std::endl;
  }
}
