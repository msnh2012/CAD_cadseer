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
#include "tools/tlsstring.h"
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

Observer::~Observer() noexcept = default;
Observer::Observer(Observer &&) noexcept = default;
Observer& Observer::operator=(Observer &&) noexcept = default;

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

struct StringVisitor
{
  int precision = -1;
  StringVisitor(int precisionIn = -1): precision(precisionIn){}
  std::string operator()(double dIn) const {return tls::prettyDouble(dIn, precision);}
  std::string operator()(int iIn) const{return std::to_string(iIn);}
  std::string operator()(bool b) const {return (b) ? "True" : "False";}
  std::string operator()(const std::string &sIn) const {return sIn;}
  std::string operator()(const boost::filesystem::path &pIn) const {return pIn.string();}
  std::string operator()(const osg::Vec3d &vIn) const {return tls::valueString(vIn, precision);}
  std::string operator()(const osg::Quat &qIn) const {return tls::valueString(qIn, precision);}
  std::string operator()(const osg::Matrixd &mIn) const {return tls::valueString(mIn, precision);}
  std::string operator()(const ftr::Picks &picks) const
  {
    std::ostringstream out;
    for (const auto &p : picks)
      out << gu::idToShortString(p.shapeHistory.getRootId()) << std::endl;
    std::string outstr = out.str();
    if (!outstr.empty())
      outstr.pop_back();
    return outstr;
  }
};

struct TypeVisitor
{
  const std::type_info& operator()(double v){return typeid(v);}
  const std::type_info& operator()(int v){return typeid(v);}
  const std::type_info& operator()(bool v){return typeid(v);}
  const std::type_info& operator()(const std::string &v){return typeid(v);}
  const std::type_info& operator()(const boost::filesystem::path &v){return typeid(v);}
  const std::type_info& operator()(const osg::Vec3d &v){return typeid(v);}
  const std::type_info& operator()(const osg::Quat &v){return typeid(v);}
  const std::type_info& operator()(const osg::Matrixd &v){return typeid(v);}
  const std::type_info& operator()(const ftr::Picks &v){return typeid(v);}
};

struct TypeStringVisitor
{
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

struct SrlVisitor
{
  using SPV = prj::srl::spt::ParameterValue;
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

Parameter::Parameter(const QString &nameIn, const std::string &valueIn, std::string_view tagIn) :
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
  return std::visit(TypeVisitor(), stow->variant);
}

std::string Parameter::getValueTypeString() const
{
  return std::visit(TypeStringVisitor(), stow->variant);
}

bool Parameter::isEnumeration() const
{
  return (!enumeration.isEmpty());
}

void Parameter::setEnumeration(const QStringList &lIn)
{
  assert(getValueType() == typeid(int));
  enumeration = lIn;
  
  Boundary lower(0.0, Boundary::End::Closed);
  Boundary upper(enumeration.size() - 1, Boundary::End::Closed);
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
  int value = getInt();
  assert(value < enumeration.size());
  return enumeration.at(value);
}

const Stow& Parameter::getStow() const
{
  return *stow;
}

bool Parameter::setValue(double valueIn)
{
  if (getDouble() == valueIn)
    return false;
  
  if (!isValidValue(valueIn))
    return false;
  
  stow->variant = valueIn;
  subject.sendValueChanged();
  return true;
}

double Parameter::getDouble() const
{
  assert(getValueType() == typeid(double));
  return std::get<double>(stow->variant);
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
  if (getInt() == valueIn)
    return false;
  
  if (!isValidValue(valueIn))
    return false;
  
  stow->variant = valueIn;
  subject.sendValueChanged();
  return true;
}

bool Parameter::isValidValue(const int &valueIn) const
{
  return constraint.test(static_cast<double>(valueIn));
}

int Parameter::getInt() const
{
  assert(getValueType() == typeid(int));
  return std::get<int>(stow->variant);
}

bool Parameter::setValue(bool valueIn)
{
  if (getBool() == valueIn)
    return false;
  
  stow->variant = valueIn;
  subject.sendValueChanged();
  return true;
}

bool Parameter::getBool() const
{
  assert(getValueType() == typeid(bool));
  return std::get<bool>(stow->variant);
}

bool Parameter::setValue(const std::string &valueIn)
{
  if (getString() == valueIn)
    return false;
  
  stow->variant = valueIn;
  subject.sendValueChanged();
  return true;
}

const std::string& Parameter::getString() const
{
  assert(getValueType() == typeid(std::string));
  return std::get<std::string>(stow->variant);
}

bool Parameter::setValue(const path &valueIn)
{
  if (getPath() == valueIn)
    return false;
  
  stow->variant = valueIn;
  subject.sendValueChanged();
  return true;
}

const boost::filesystem::path& Parameter::getPath() const
{
  assert(getValueType() == typeid(boost::filesystem::path));
  return std::get<boost::filesystem::path>(stow->variant);
}

bool Parameter::setValue(const osg::Vec3d &vIn)
{
  if (getVector() == vIn)
    return false;
  
  stow->variant = vIn;
  subject.sendValueChanged();
  return true;
}

const osg::Vec3d& Parameter::getVector() const
{
  assert(getValueType() == typeid(osg::Vec3d));
  return std::get<osg::Vec3d>(stow->variant);
}

bool Parameter::setValue(const osg::Quat &qIn)
{
  if (getQuat() == qIn)
    return false;
  
  stow->variant = qIn;
  subject.sendValueChanged();
  return true;
}

const osg::Quat& Parameter::getQuat() const
{
  assert(getValueType() == typeid(osg::Quat));
  return std::get<osg::Quat>(stow->variant);
}

bool Parameter::setValue(const osg::Matrixd &mIn)
{
  if (getMatrix() == mIn)
    return false;
  
  stow->variant = mIn;
  subject.sendValueChanged();
  return true;
}

const osg::Matrixd& Parameter::getMatrix() const
{
  assert(getValueType() == typeid(osg::Matrixd));
  return std::get<osg::Matrixd>(stow->variant);
}

bool Parameter::setValue(const ftr::Pick &pIn)
{
  ftr::Picks np(1, pIn);
  if (getPicks() == np)
    return false;
  
  stow->variant = np;
  subject.sendValueChanged();
  return true;
}

bool Parameter::setValue(const ftr::Picks &psIn)
{
  if (getPicks() == psIn)
    return false;
  
  stow->variant = psIn;
  subject.sendValueChanged();
  return true;
}

const ftr::Picks& Parameter::getPicks() const
{
  assert(getValueType() == typeid(ftr::Picks));
  return std::get<ftr::Picks>(stow->variant);
}

std::string Parameter::adaptToString(int precision) const
{
  //enum hack on int.
  if (isEnumeration())
    return getEnumerationString().toStdString(); 
  return std::visit(StringVisitor(precision), stow->variant);
}

QString Parameter::adaptToQString(int precision) const
{
  return QString::fromStdString(adaptToString(precision));
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

 //serial rename
prj::srl::spt::Parameter Parameter::serialOut() const
{
  return prj::srl::spt::Parameter
  (
    gu::idToString(id)
    , name.toStdString()
    , tag
    , constant
    , active
    , std::visit(SrlVisitor(), stow->variant)
  );
}

 //serial rename
void Parameter::serialIn(const prj::srl::spt::Parameter &spIn)
{
  id = gu::stringToId(spIn.id());
  name = QString::fromStdString(spIn.name());
  tag = spIn.tag();
  constant = spIn.constant();
  active = spIn.active();
  
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
    auto &vr = std::get<ftr::Picks>(stow->variant);
    for (const auto &pIn : vIn.aPicks())
      vr.emplace_back(pIn);
  }
}
