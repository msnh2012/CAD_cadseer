/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2015  Thomas S. Anderson blobfish.at.gmx.com
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

#include <memory>
#include <limits>

#include <boost/signals2.hpp>
#include <boost/variant/get.hpp>

#include <QObject>

#include "tools/idtools.h"
#include "tools/infotools.h"
#include "project/serial/generated/prjsrlsptparameter.h"
#include "parameter/prmvariant.h"
#include "parameter/prmparameter.h"

using namespace prm;
using boost::filesystem::path;

Boundary::Boundary(double valueIn, Boundary::End endIn) :
  value(valueIn), end(endIn)
{}

Interval::Interval(const Boundary &lowerIn, const Boundary &upperIn) :
  lower(lowerIn), upper(upperIn)
{}

bool Interval::test(double testValue) const
{
  assert(lower.end != Boundary::End::None);
  assert(upper.end != Boundary::End::None);
  
  if (lower.end == Boundary::End::Open)
  {
    if (!(testValue > lower.value))
      return false;
  }
  else //lower.end == Boundary::End::Closed
  {
    if (!(testValue >= lower.value))
      return false;
  }
  
  if (upper.end == Boundary::End::Open)
  {
    if (!(testValue < upper.value))
      return false;
  }
  else //upper.end == Boundary::End::Closed
  {
    if (!(testValue <= upper.value))
      return false;
  }
  
  return true;
}

Constraint Constraint::buildAll()
{
  Boundary lower(std::numeric_limits<double>::lowest(), Boundary::End::Closed);
  Boundary upper(std::numeric_limits<double>::max(), Boundary::End::Closed);
  Interval interval(lower, upper);
  
  Constraint out;
  out.intervals.push_back(interval);
  return out;
}

Constraint Constraint::buildNonZeroPositive()
{
  Boundary lower(0.0, Boundary::End::Open);
  Boundary upper(std::numeric_limits<double>::max(), Boundary::End::Closed);
  Interval interval(lower, upper);
  
  Constraint out;
  out.intervals.push_back(interval);
  return out;
}

Constraint Constraint::buildZeroPositive()
{
  Boundary lower(0.0, Boundary::End::Closed);
  Boundary upper(std::numeric_limits<double>::max(), Boundary::End::Closed);
  Interval interval(lower, upper);
  
  Constraint out;
  out.intervals.push_back(interval);
  return out;
}

Constraint Constraint::buildNonZeroNegative()
{
  Boundary lower(std::numeric_limits<double>::lowest(), Boundary::End::Closed);
  Boundary upper(0.0, Boundary::End::Open);
  Interval interval(lower, upper);
  
  Constraint out;
  out.intervals.push_back(interval);
  return out;
}

Constraint Constraint::buildZeroNegative()
{
  Boundary lower(std::numeric_limits<double>::lowest(), Boundary::End::Closed);
  Boundary upper(0.0, Boundary::End::Closed);
  Interval interval(lower, upper);
  
  Constraint out;
  out.intervals.push_back(interval);
  return out;
}

Constraint Constraint::buildNonZero()
{
  Boundary lower1(std::numeric_limits<double>::lowest(), Boundary::End::Closed);
  Boundary upper1(0.0, Boundary::End::Open);
  Interval interval1(lower1, upper1);
  
  Boundary lower2(0.0, Boundary::End::Open);
  Boundary upper2(std::numeric_limits<double>::max(), Boundary::End::Closed);
  Interval interval2(lower2, upper2);
  
  Constraint out;
  out.intervals.push_back(interval1);
  out.intervals.push_back(interval2);
  return out;
}

Constraint Constraint::buildUnit()
{
  Boundary lower(0.0, Boundary::End::Closed);
  Boundary upper(1.0, Boundary::End::Closed);
  Interval interval(lower, upper);
  
  Constraint out;
  out.intervals.push_back(interval);
  return out;
}

Constraint Constraint::buildNonZeroAngle()
{
  Constraint out;
  
  {
    Boundary lower(-360.0, Boundary::End::Closed);
    Boundary upper(0.0, Boundary::End::Open);
    Interval interval(lower, upper);
    out.intervals.push_back(interval);
  }
  {
    Boundary lower(0.0, Boundary::End::Open);
    Boundary upper(360.0, Boundary::End::Closed);
    Interval interval(lower, upper);
    out.intervals.push_back(interval);
  }
  
  return out;
}

Constraint Constraint::buildNonZeroPositiveAngle()
{
  Constraint out;
  Boundary lower(0.0, Boundary::End::Open);
  Boundary upper(360.0, Boundary::End::Closed);
  Interval interval(lower, upper);
  out.intervals.push_back(interval);
  
  return out;
}

void Constraint::unitTest()
{
  Constraint all = Constraint::buildAll();
  assert(all.test(-1200.345) == true);
  assert(all.test(-234) == true);
  assert(all.test(13456.987) == true);
  assert(all.test(593) == true);
  assert(all.test(0) == true);
  
  Constraint nonZeroPositive = Constraint::buildNonZeroPositive();
  assert(nonZeroPositive.test(-231.432) == false);
  assert(nonZeroPositive.test(-562) == false);
  assert(nonZeroPositive.test(0) == false);
  assert(nonZeroPositive.test(74321) == true);
  assert(nonZeroPositive.test(984.3587) == true);
  
  Constraint zeroPositive = Constraint::buildZeroPositive();
  assert(zeroPositive.test(-231.432) == false);
  assert(zeroPositive.test(-562) == false);
  assert(zeroPositive.test(0) == true);
  assert(zeroPositive.test(74321) == true);
  assert(zeroPositive.test(984.3587) == true);
  
  Constraint nonZeroNegative = Constraint::buildNonZeroNegative();
  assert(nonZeroNegative.test(-231.432) == true);
  assert(nonZeroNegative.test(-562) == true);
  assert(nonZeroNegative.test(0) == false);
  assert(nonZeroNegative.test(74321) == false);
  assert(nonZeroNegative.test(984.3587) == false);
  
  Constraint zeroNegative = Constraint::buildZeroNegative();
  assert(zeroNegative.test(-231.432) == true);
  assert(zeroNegative.test(-562) == true);
  assert(zeroNegative.test(0) == true);
  assert(zeroNegative.test(74321) == false);
  assert(zeroNegative.test(984.3587) == false);
  
  Constraint nonZero = Constraint::buildNonZero();
  assert(nonZero.test(-231.432) == true);
  assert(nonZero.test(-562) == true);
  assert(nonZero.test(0) == false);
  assert(nonZero.test(74321) == true);
  assert(nonZero.test(984.3587) == true);
  
  Constraint unit = Constraint::buildUnit();
  assert(unit.test(-100) == false);
  assert(unit.test(-0.125) == false);
  assert(unit.test(0) == true);
  assert(unit.test(0.5) == true);
  assert(unit.test(1) == true);
  assert(unit.test(1.125) == false);
  assert(unit.test(100) == false);
}

bool Constraint::test(double testValue) const
{
  if(intervals.empty())
    return true;
  
  //value needs to be in only one interval.
  bool out = false;
  for (auto it = intervals.begin(); it != intervals.end(); ++it)
    out |= it->test(testValue);
  
  return out;
}

namespace prm
{
  struct Observer::Stow
  {
    std::vector<boost::signals2::scoped_connection> connections;
    std::vector<boost::signals2::shared_connection_block> blockers;
  };
  struct Subject::Stow
  {
    boost::signals2::signal<void ()> valueSignal;
    boost::signals2::signal<void ()> constantSignal;
    boost::signals2::signal<void ()> activeSignal;
  };
}

Observer::Observer()
: stow(std::make_unique<Stow>())
{}

Observer::Observer(Handler vhIn)
: stow(std::make_unique<Stow>())
, valueHandler(vhIn)
{}

Observer::Observer(Handler vhIn, Handler chIn)
: stow(std::make_unique<Stow>())
, valueHandler(vhIn)
, constantHandler(chIn)
{}

Observer::Observer(Handler vhIn, Handler chIn, Handler ahIn)
: stow(std::make_unique<Stow>())
, valueHandler(vhIn)
, constantHandler(chIn)
, activeHandler(ahIn)
{}

Observer::~Observer() = default;
Observer::Observer(Observer &&) = default;
Observer& Observer::operator=(Observer &&) = default;

void Observer::block()
{
  for (auto &c : stow->connections)
    stow->blockers.push_back(boost::signals2::shared_connection_block(c));
}

void Observer::unblock()
{
  stow->blockers.clear();
}


Subject::Subject()
: stow(std::make_unique<Stow>())
{}

Subject::~Subject() = default;
Subject::Subject(Subject &&) noexcept = default;
Subject& Subject::operator=(Subject &&) noexcept = default;

void Subject::addValueHandler(Handler vhIn)
{
  stow->valueSignal.connect(vhIn);
}

void Subject::addConstantHandler(Handler chIn)
{
  stow->constantSignal.connect(chIn);
}

void Subject::addActiveHandler(Handler ahIn)
{
  stow->activeSignal.connect(ahIn);
}

void Subject::connect(Observer &oIn)
{
  if (oIn.valueHandler)
    oIn.stow->connections.push_back(stow->valueSignal.connect(oIn.valueHandler));
  if (oIn.constantHandler)
    oIn.stow->connections.push_back(stow->constantSignal.connect(oIn.constantHandler));
  if (oIn.activeHandler)
    oIn.stow->connections.push_back(stow->activeSignal.connect(oIn.activeHandler));
}

void Subject::sendValueChanged() const
{
  stow->valueSignal();
}

void Subject::sendConstantChanged() const
{
  stow->constantSignal();
}

void Subject::sendActiveChanged() const
{
  stow->activeSignal();
}

class DoubleVisitor : public boost::static_visitor<double>
{
public:
  double operator()(double d) const {return d;}
  double operator()(int) const {assert(0); return 0.0;}
  double operator()(bool) const {assert(0); return 0.0;}
  double operator()(const std::string&) const {assert(0); return 0.0;}
  double operator()(const boost::filesystem::path&) const {assert(0); return 0.0;}
  double operator()(const osg::Vec3d&) const {assert(0); return 0.0;}
  double operator()(const osg::Quat&) const {assert(0); return 0.0;}
  double operator()(const osg::Matrixd&) const {assert(0); return 0.0;}
  double operator()(const ftr::Picks&) const {assert(0); return 0.0;}
};

class IntVisitor : public boost::static_visitor<int>
{
public:
  int operator()(double) const {assert(0); return 0;}
  int operator()(int i) const {return i;}
  int operator()(bool) const {assert(0); return 0;}
  int operator()(const std::string&) const {assert(0); return 0;}
  int operator()(const boost::filesystem::path&) const {assert(0); return 0;}
  int operator()(const osg::Vec3d&) const {assert(0); return 0;}
  int operator()(const osg::Quat&) const {assert(0); return 0;}
  int operator()(const osg::Matrixd&) const {assert(0); return 0;}
  int operator()(const ftr::Picks&) const {assert(0); return 0;}
};

class BoolVisitor : public boost::static_visitor<bool>
{
public:
  bool operator()(double) const {assert(0); return false;}
  bool operator()(int) const {assert(0); return false;}
  bool operator()(bool b) const {return b;}
  bool operator()(const std::string&) const {assert(0); return false;}
  bool operator()(const boost::filesystem::path&) const {assert(0); return false;}
  bool operator()(const osg::Vec3d&) const {assert(0); return false;}
  bool operator()(const osg::Quat&) const {assert(0); return false;}
  bool operator()(const osg::Matrixd&) const {assert(0); return false;}
  bool operator()(const ftr::Picks&) const {assert(0); return false;}
};

class QStringVisitor : public boost::static_visitor<QString>
{
public:
  QString operator()(double dIn) const {return QString::number(dIn, 'f', 12);}
  QString operator()(int iIn) const {return QString::number(iIn);}
  QString operator()(bool b) const {return (b) ? QObject::tr("True") : QObject::tr("False");}
  QString operator()(const std::string &sIn) const {return QString::fromStdString(sIn);}
  QString operator()(const boost::filesystem::path &pIn) const {return QString::fromStdString(pIn.string());}
  QString operator()(const osg::Vec3d &vIn) const {return gu::osgVectorOut(vIn);}
  QString operator()(const osg::Quat &qIn) const {return gu::osgQuatOut(qIn);}
  QString operator()(const osg::Matrixd &mIn) const {return gu::osgMatrixOut(mIn);}
  QString operator()(const ftr::Picks &psIn) const {return QObject::tr("Picks");} //TODO
};

class PathVisitor : public boost::static_visitor<path>
{
public:
  path operator()(double) const {assert(0); return path();}
  path operator()(int) const {assert(0); return path();}
  path operator()(bool) const {assert(0); return path();}
  path operator()(const std::string&) const {assert(0); return path();}
  path operator()(const boost::filesystem::path &p) const {return p;}
  path operator()(const osg::Vec3d&) const {assert(0); return path();}
  path operator()(const osg::Quat&) const {assert(0); return path();}
  path operator()(const osg::Matrixd&) const {assert(0); return path();}
  path operator()(const ftr::Picks&) const {assert(0); return path();}
};

class Vec3dVisitor : public boost::static_visitor<osg::Vec3d>
{
public:
  osg::Vec3d operator()(double) const {assert(0); return osg::Vec3d();}
  osg::Vec3d operator()(int) const {assert(0); return osg::Vec3d();}
  osg::Vec3d operator()(bool) const {assert(0); return osg::Vec3d();}
  osg::Vec3d operator()(const std::string&) const {assert(0); return osg::Vec3d();}
  osg::Vec3d operator()(const boost::filesystem::path&) const {assert(0); return osg::Vec3d();}
  osg::Vec3d operator()(const osg::Vec3d &v) const {return v;}
  osg::Vec3d operator()(const osg::Quat&) const {assert(0); return osg::Vec3d();}
  osg::Vec3d operator()(const osg::Matrixd&) const {assert(0); return osg::Vec3d();}
  osg::Vec3d operator()(const ftr::Picks&) const {assert(0); return osg::Vec3d();}
};

class QuatVisitor : public boost::static_visitor<osg::Quat>
{
public:
  osg::Quat operator()(double) const {assert(0); return osg::Quat();}
  osg::Quat operator()(int) const {assert(0); return osg::Quat();}
  osg::Quat operator()(bool) const {assert(0); return osg::Quat();}
  osg::Quat operator()(const std::string&) const {assert(0); return osg::Quat();}
  osg::Quat operator()(const boost::filesystem::path&) const {assert(0); return osg::Quat();}
  osg::Quat operator()(const osg::Vec3d&) const {return osg::Quat();}
  osg::Quat operator()(const osg::Quat &q) const {return q;}
  osg::Quat operator()(const osg::Matrixd&) const {assert(0); return osg::Quat();}
  osg::Quat operator()(const ftr::Picks&) const {assert(0); return osg::Quat();}
};

class MatrixdVisitor : public boost::static_visitor<osg::Matrixd>
{
public:
  osg::Matrixd operator()(double) const {assert(0); return osg::Matrixd::identity();}
  osg::Matrixd operator()(int) const {assert(0); return osg::Matrixd::identity();}
  osg::Matrixd operator()(bool) const {assert(0); return osg::Matrixd::identity();}
  osg::Matrixd operator()(const std::string&) const {assert(0); return osg::Matrixd::identity();}
  osg::Matrixd operator()(const boost::filesystem::path&) const {assert(0); return osg::Matrixd::identity();}
  osg::Matrixd operator()(const osg::Vec3d&) const {assert(0); return osg::Matrixd::identity();}
  osg::Matrixd operator()(const osg::Quat&) const {assert(0); return osg::Matrixd::identity();}
  osg::Matrixd operator()(const osg::Matrixd &mIn) const {return mIn;}
  osg::Matrixd operator()(const ftr::Picks&) const {assert(0); return osg::Matrixd::identity();}
};

class PicksVisitor : public boost::static_visitor<ftr::Picks>
{
public:
  ftr::Picks operator()(double) const {assert(0); return ftr::Picks();}
  ftr::Picks operator()(int) const {assert(0); return ftr::Picks();}
  ftr::Picks operator()(bool) const {assert(0); return ftr::Picks();}
  ftr::Picks operator()(const std::string&) const {assert(0); return ftr::Picks();}
  ftr::Picks operator()(const boost::filesystem::path&) const {assert(0); return ftr::Picks();}
  ftr::Picks operator()(const osg::Vec3d&) const {assert(0); return ftr::Picks();}
  ftr::Picks operator()(const osg::Quat&) const {assert(0); return ftr::Picks();}
  ftr::Picks operator()(const osg::Matrixd&) const {assert(0); return ftr::Picks();}
  ftr::Picks operator()(const ftr::Picks &ps) const {return ps;}
};

class TypeStringVisitor : public boost::static_visitor<std::string>
{
public:
  std::string operator()(double) const {return "Double";}
  std::string operator()(int) const {return "Int";}
  std::string operator()(bool) const {return "Bool";}
  std::string operator()(const std::string&) const {return "String";}
  std::string operator()(const boost::filesystem::path&) const {return "Path";}
  std::string operator()(const osg::Vec3d&) const {return "Vec3d";}
  std::string operator()(const osg::Quat&) const {return "Quat";}
  std::string operator()(const osg::Matrixd&) const {return "Matrixd";}
  std::string operator()(const ftr::Picks&) const {return "Picks";}
};

class SrlVisitor : public boost::static_visitor<prj::srl::spt::ParameterValue>
{
  using SPV = prj::srl::spt::ParameterValue;
public:
  SPV operator()(double d) const
  {
    SPV out;
    out.aDouble() = d;
    return out;
  }
  SPV operator()(int i) const
  {
    SPV out;
    out.anInteger() = i;
    return out;
  }
  SPV operator()(bool b) const
  {
    SPV out;
    out.aBool() = b;
    return out;
  }
  SPV operator()(const std::string &s) const
  {
    SPV out;
    out.aString() = s;
    return out;
  }
  SPV operator()(const boost::filesystem::path &p) const
  {
    SPV out;
    out.aPath() = p.string();
    return out;
  }
  SPV operator()(const osg::Vec3d &v) const
  {
    SPV out;
    out.aVec3d() = prj::srl::spt::Vec3d(v.x(), v.y(), v.z());
    return out;
  }
  SPV operator()(const osg::Quat &q) const
  {
    SPV out;
    out.aQuat() = prj::srl::spt::Quat(q.x(), q.y(), q.z(), q.z());
    return out;
  }
  SPV operator()(const osg::Matrixd &m) const
  {
    SPV out;
    out.aMatrixd() = prj::srl::spt::Matrixd
    (
      m(0,0), m(0,1), m(0,2), m(0,3),
      m(1,0), m(1,1), m(1,2), m(1,3),
      m(2,0), m(2,1), m(2,2), m(2,3),
      m(3,0), m(3,1), m(3,2), m(3,3)
    );
    return out;
  }
  SPV operator()(const ftr::Picks &ps) const
  {
    SPV out;
    for (const auto &p : ps)
      out.aPicks().push_back(p.serialOut());
    return out;
  }
};

/*! @brief construct parameter from serial object
 * 
 * @warning constraints are not serialized.
 */
Parameter::Parameter(const prj::srl::spt::Parameter &pIn)
: name(QString())
, tag("")
, stow(new Stow(1.0))
, id(gu::createNilId())
, constraint()
{
  serialIn(pIn);
}

Parameter::Parameter(const QString& nameIn, double valueIn, std::string_view tagIn) :
  name(nameIn),
  tag(tagIn),
  stow(new Stow(valueIn)),
  id(gu::createRandomId()),
  constraint(Constraint::buildAll())
{
}

Parameter::Parameter(const QString& nameIn, int valueIn, std::string_view tagIn) :
  name(nameIn),
  tag(tagIn),
  stow(new Stow(valueIn)),
  id(gu::createRandomId()),
  constraint(Constraint::buildAll())
{
}

Parameter::Parameter(const QString& nameIn, bool valueIn, std::string_view tagIn) :
  name(nameIn),
  tag(tagIn),
  stow(new Stow(valueIn)),
  id(gu::createRandomId()),
  constraint()
{
}

Parameter::Parameter(const QString &nameIn, const boost::filesystem::path &valueIn, PathType ptIn, std::string_view tagIn) :
  name(nameIn),
  tag(tagIn),
  stow(new Stow(valueIn)),
  id(gu::createRandomId()),
  constraint(),
  pathType(ptIn)
{
}

Parameter::Parameter(const QString &nameIn, const osg::Vec3d &valueIn, std::string_view tagIn) :
  name(nameIn),
  tag(tagIn),
  stow(new Stow(valueIn)),
  id(gu::createRandomId()),
  constraint()
{
}

Parameter::Parameter(const QString &nameIn, const osg::Quat &valueIn, std::string_view tagIn) :
  name(nameIn),
  tag(tagIn),
  stow(new Stow(valueIn)),
  id(gu::createRandomId()),
  constraint()
{
}

Parameter::Parameter(const QString &nameIn, const osg::Matrixd &valueIn, std::string_view tagIn) :
  name(nameIn),
  tag(tagIn),
  stow(new Stow(valueIn)),
  id(gu::createRandomId()),
  constraint()
{
}

Parameter::Parameter(const QString &nameIn, const ftr::Picks &valueIn, std::string_view tagIn) :
  name(nameIn),
  tag(tagIn),
  stow(new Stow(valueIn)),
  id(gu::createRandomId()),
  constraint()
{
}

Parameter::Parameter(const Parameter &other, const boost::uuids::uuid &idIn) :
  Parameter(other)
{
  id = idIn;
}

Parameter::Parameter(const Parameter &other) :
  constant(other.constant),
  name(other.name),
  tag(other.tag),
  stow(new Stow(other.stow->variant)),
  id(other.id),
  constraint(other.constraint),
  pathType(other.pathType),
  enumeration(other.enumeration)
{
}

Parameter::~Parameter() = default;

Parameter& Parameter::operator=(const Parameter &other)
{
  constant = other.constant;
  name = other.name;
  tag = other.tag;
  stow = std::make_unique<Stow>(other.stow->variant);
  id = other.id;
  constraint = other.constraint;
  pathType = other.pathType;
  
  return *this;
}

void Parameter::setConstant(bool constantIn)
{
  if (constantIn == constant)
    return;
  constant = constantIn;
  subject.sendConstantChanged();
}

void Parameter::setActive(bool stateIn)
{
  if (stateIn == active)
    return;
  active = stateIn;
  subject.sendActiveChanged();
}

const std::type_info& Parameter::getValueType() const
{
  return stow->variant.type();
}

std::string Parameter::getValueTypeString() const
{
  return boost::apply_visitor(TypeStringVisitor(), stow->variant);
}

bool Parameter::isEnumeration() const
{
  assert(getValueType() == typeid(int));
  return (!enumeration.isEmpty());
}

void Parameter::setEnumeration(const QStringList &lIn)
{
  assert(getValueType() == typeid(int));
  enumeration = lIn;
  
  Boundary lower(0.0, Boundary::End::Closed);
  Boundary upper(enumeration.size(), Boundary::End::Closed);
  Interval interval(lower, upper);
  Constraint out;
  out.intervals.push_back(interval);
  setConstraint(out);
}

const QStringList& Parameter::getEnumeration() const
{
  assert(getValueType() == typeid(int));
  return enumeration;
}

const QString& Parameter::getEnumerationString() const
{
  assert(getValueType() == typeid(int));
  int value = boost::apply_visitor(IntVisitor(), stow->variant);
  assert(value < enumeration.size());
  return enumeration.at(value);
}

const Stow& Parameter::getStow() const
{
  return *stow;
}

bool Parameter::setValue(double valueIn)
{
  if (setValueQuiet(valueIn))
  {
    subject.sendValueChanged();
    return true;
  }
  
  return false;
}

bool Parameter::setValueQuiet(double valueIn)
{
  if (boost::apply_visitor(DoubleVisitor(), stow->variant) == valueIn)
    return false;
  
  if (!isValidValue(valueIn))
    return false;
  
  stow->variant = valueIn;
  return true;
}

Parameter::operator double() const
{
  return boost::apply_visitor(DoubleVisitor(), stow->variant);
}

bool Parameter::isValidValue(const double &valueIn) const
{
  return constraint.test(valueIn);
}

void Parameter::setConstraint(const Constraint &cIn)
{
  /* not really worried about the current value conflicting
   * with new constraint. Constraints should be set at construction
   * and not swapped in and out during run
   */
  constraint = cIn;
}

bool Parameter::setValue(int valueIn)
{
  if (setValueQuiet(valueIn))
  {
    subject.sendValueChanged();
    return true;
  }
  
  return false;
}

bool Parameter::setValueQuiet(int valueIn)
{
  if (boost::apply_visitor(IntVisitor(), stow->variant) == valueIn)
    return false;
  
  if (!isValidValue(valueIn))
    return false;
  
  stow->variant = valueIn;
  return true;
}

bool Parameter::isValidValue(const int &valueIn) const
{
  return constraint.test(static_cast<double>(valueIn));
}

Parameter::operator int() const
{
  return boost::apply_visitor(IntVisitor(), stow->variant);
}

bool Parameter::setValue(bool valueIn)
{
  if (setValueQuiet(valueIn))
  {
    subject.sendValueChanged();
    return true;
  }
  
  return false;
}

bool Parameter::setValueQuiet(bool valueIn)
{
  if (boost::apply_visitor(BoolVisitor(), stow->variant) == valueIn)
    return false;
  
  stow->variant = valueIn;
  return true;
}

Parameter::operator bool() const
{
  return boost::apply_visitor(BoolVisitor(), stow->variant);
}

bool Parameter::setValue(const path &valueIn)
{
  if (setValueQuiet(valueIn))
  {
    subject.sendValueChanged();
    return true;
  }
  
  return false;
}

bool Parameter::setValueQuiet(const path &valueIn)
{
  if (boost::apply_visitor(PathVisitor(), stow->variant) == valueIn)
    return false;
  
  stow->variant = valueIn;
  return true;
}

Parameter::operator boost::filesystem::path() const
{
  return boost::apply_visitor(PathVisitor(), stow->variant);
}

bool Parameter::setValue(const osg::Vec3d &vIn)
{
  if (setValueQuiet(vIn))
  {
    subject.sendValueChanged();
    return true;
  }
  
  return false;
}

bool Parameter::setValueQuiet(const osg::Vec3d &vIn)
{
  if (boost::apply_visitor(Vec3dVisitor(), stow->variant) == vIn)
    return false;
  
  stow->variant = vIn;
  return true;
}

Parameter::operator osg::Vec3d() const
{
  return boost::apply_visitor(Vec3dVisitor(), stow->variant);
}

bool Parameter::setValue(const osg::Quat &qIn)
{
  if (setValueQuiet(qIn))
  {
    subject.sendValueChanged();
    return true;
  }
  
  return false;
}

bool Parameter::setValueQuiet(const osg::Quat &qIn)
{
  if (boost::apply_visitor(QuatVisitor(), stow->variant) == qIn)
    return false;
  
  stow->variant = qIn;
  return true;
}

Parameter::operator osg::Quat() const
{
  return boost::apply_visitor(QuatVisitor(), stow->variant);
}

bool Parameter::setValue(const osg::Matrixd &mIn)
{
  if (setValueQuiet(mIn))
  {
    subject.sendValueChanged();
    return true;
  }
  
  return false;
}

bool Parameter::setValueQuiet(const osg::Matrixd &mIn)
{
  if (boost::apply_visitor(MatrixdVisitor(), stow->variant) == mIn)
    return false;
  
  stow->variant = mIn;
  return true;
}

bool Parameter::setValue(const ftr::Pick &pIn)
{
  auto cp = boost::apply_visitor(PicksVisitor(), stow->variant);
  ftr::Picks np(1, pIn);
  if (cp == np)
    return false;
  
  stow->variant = np;
  subject.sendValueChanged();
  return true;
}

bool Parameter::setValue(const ftr::Picks &psIn)
{
  auto cp = boost::apply_visitor(PicksVisitor(), stow->variant);
  if (cp == psIn)
    return false;
  
  stow->variant = psIn;
  subject.sendValueChanged();
  return true;
}

void Parameter::connect(Observer &o)
{
  subject.connect(o);
}
void Parameter::connectValue(Handler h)
{
  subject.addValueHandler(h);
}

void Parameter::connectConstant(Handler h)
{
  subject.addConstantHandler(h);
}

void Parameter::connectActive(Handler h)
{
  subject.addActiveHandler(h);
}

Parameter::operator osg::Matrixd() const
{
  return boost::apply_visitor(MatrixdVisitor(), stow->variant);
}

//todo
Parameter::operator std::string() const{return std::string();}

Parameter::operator QString() const
{
  return boost::apply_visitor(QStringVisitor(), stow->variant);
}

 //serial rename
prj::srl::spt::Parameter Parameter::serialOut() const
{
  return prj::srl::spt::Parameter
  (
    name.toStdString()
    , tag
    , constant
    , gu::idToString(id)
    , boost::apply_visitor(SrlVisitor(), stow->variant)
  );
}

 //serial rename
void Parameter::serialIn(const prj::srl::spt::Parameter &spIn)
{
  name = QString::fromStdString(spIn.name());
  tag = spIn.tag();
  constant = spIn.constant();
  id = gu::stringToId(spIn.id());
  
  const auto &vIn = spIn.pValue(); //variant in
  if (vIn.aDouble().present())
      stow->variant = vIn.aDouble().get();
  else if (vIn.anInteger().present())
      stow->variant = vIn.anInteger().get();
  else if (vIn.aBool().present())
    stow->variant = vIn.aBool().get();
  else if (vIn.aString().present())
    stow->variant = vIn.aString().get();
  else if (vIn.aPath().present())
    stow->variant = boost::filesystem::path(vIn.aPath().get());
  else if (vIn.aVec3d().present())
    stow->variant = osg::Vec3d(vIn.aVec3d().get().x(), vIn.aVec3d().get().y(), vIn.aVec3d().get().z());
  else if (vIn.aQuat().present())
    stow->variant = osg::Quat(vIn.aQuat().get().x(), vIn.aQuat().get().y(), vIn.aQuat().get().z(), vIn.aQuat().get().w());
  else if (vIn.aMatrixd().present())
  {
    const auto &mIn = vIn.aMatrixd().get();
    stow->variant = osg::Matrixd
    (
      mIn.i0j0(), mIn.i0j1(), mIn.i0j2(), mIn.i0j3(),
      mIn.i1j0(), mIn.i1j1(), mIn.i1j2(), mIn.i1j3(),
      mIn.i2j0(), mIn.i2j1(), mIn.i2j2(), mIn.i2j3(),
      mIn.i3j0(), mIn.i3j1(), mIn.i3j2(), mIn.i3j3()
    );
  }
  else if (!vIn.aPicks().empty())
  {
    stow->variant = ftr::Picks();
    auto &vr = boost::get<ftr::Picks>(stow->variant);
    for (const auto &pIn : vIn.aPicks())
      vr.emplace_back(pIn);
  }
}
