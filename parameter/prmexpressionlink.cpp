/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2020 Thomas S. Anderson blobfish.at.gmx.com
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

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "message/msgmessage.h"
#include "expressions/exprmanager.h"
#include "expressions/exprvalue.h"
#include "parameter/prmvariant.h"
#include "parameter/prmparameter.h"
#include "parameter/prmexpressionlink.h"

using boost::uuids::uuid;

namespace prm
{
  class ParameterValueVisitor : public boost::static_visitor<bool>
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
    
    bool operator()(double) const
    {
      expr::DoubleVisitor edv;
      double newValue = boost::apply_visitor(edv, ev);
      if (!parameter->isValidValue(newValue))
        return false;
      if (assign)
        parameter->setValue(newValue);
      return true;
    }
    
    bool operator()(int) const
    {
      expr::DoubleVisitor edv;
      int newValue = static_cast<int>(boost::apply_visitor(edv, ev));
      if (!parameter->isValidValue(newValue))
        return false;
      if (assign)
        parameter->setValue(newValue);
      return true;
    }
    bool operator()(bool) const
    {
      expr::DoubleVisitor edv;
      bool newValue = static_cast<bool>(boost::apply_visitor(edv, ev));
      if (!parameter->isValidValue(newValue))
        return false;
      if (assign)
        parameter->setValue(newValue);
      return true;
    }
    bool operator()(const std::string&) const {assert(0); return false;} //currently unsupported formula type.
    bool operator()(const boost::filesystem::path&) const {assert(0); return false;} //currently unsupported formula type.
    bool operator()(const osg::Vec3d&) const
    {
      expr::VectorVisitor evv;
      osg::Vec3d newValue = boost::apply_visitor(evv, ev);
      if (assign)
        parameter->setValue(newValue);
      return true;
    }
    bool operator()(const osg::Quat&) const
    {
      //parameters not supporting Quaternions at this time.
      return false;
      
//       expr::QuatVisitor eqv;
//       osg::Quat newValue = boost::apply_visitor(eqv, ev);
//       parameter->setValue(newValue);
//       return true;
    }
    bool operator()(const osg::Matrixd&) const
    {
      expr::MatrixVisitor emv;
      osg::Matrixd newValue = boost::apply_visitor(emv, ev);
      if (assign)
        parameter->setValue(newValue);
      return true;
    }
  };
}

using namespace prm;

//can't make parameter const. shared code that needs non-const
static bool common(expr::Manager& em, Parameter *parameter, const uuid &fidIn, bool assign)
{
  if (!em.hasFormula(fidIn))
    return false;
  
  //we only visit if the types are acceptable otherwise crash.
  ParameterValueVisitor v(parameter, em.getFormulaValue(fidIn), assign);
  auto goVisit = [&]() -> bool {return boost::apply_visitor(v, parameter->getStow().variant);};
  
  const auto &pvtIn = parameter->getValueType();
  auto evtIn = em.getFormulaValueType(fidIn);
  if (pvtIn == typeid(double) && evtIn == expr::ValueType::Scalar && goVisit())
    return true;
  if (pvtIn == typeid(int) && evtIn == expr::ValueType::Scalar && goVisit())
    return true;
  if (pvtIn == typeid(bool) && evtIn == expr::ValueType::Scalar && goVisit())
    return true;
  if (pvtIn == typeid(osg::Vec3d) && evtIn == expr::ValueType::Vector && goVisit())
    return true;
  if (pvtIn == typeid(osg::Quat) && evtIn == expr::ValueType::Quat && goVisit())
    return true;
  if (pvtIn == typeid(osg::Matrixd) && evtIn == expr::ValueType::CSys && goVisit())
    return true;
  
  return false;
}

bool prm::canLinkExpression(expr::Manager &em, Parameter *parameter, const uuid &fidIn)
{
  return common(em, parameter, fidIn, false);
}

bool prm::canLinkExpression(Parameter *parameter, const uuid &fidIn)
{
  auto &em = app::instance()->getProject()->getManager();
  return common(em, parameter, fidIn, false);
}

bool prm::linkExpression(Parameter *parameter, const uuid &fidIn)
{
  auto &em = app::instance()->getProject()->getManager();
  if (common(em, parameter, fidIn, true))
  {
    if (em.hasParameterLink(parameter->getId()))
      em.removeParameterLink(parameter->getId());
    em.addLink(parameter->getId(), fidIn);
    parameter->setConstant(false);
    
    std::ostringstream gm;
    gm << "Linking parameter: " << parameter->getName().toStdString() << " to formula: " << em.getFormulaName(fidIn);
    app::instance()->messageSlot(msg::buildGitMessage(gm.str()));
    
    return true;
  }
  return false;
}

void prm::unLinkExpression(Parameter *parameter)
{
  auto &em = app::instance()->getProject()->getManager();
  if (em.hasParameterLink(parameter->getId()))
  {
    em.removeParameterLink(parameter->getId());
    parameter->setConstant(true);
    
    std::ostringstream gm;
    gm << "Unlinking parameter: " << parameter->getName().toStdString();
    app::instance()->messageSlot(msg::buildGitMessage(gm.str()));
  }
}
