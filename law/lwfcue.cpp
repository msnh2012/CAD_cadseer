/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2019 Thomas S. Anderson blobfish.at.gmx.com
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

#include <boost/optional.hpp>

#include <osg/Vec3d>

#include <Law_Constant.hxx>
#include <Law_Linear.hxx>
#include <Law_Interpol.hxx>
#include <Law_Composite.hxx>

#include "project/serial/xsdcxxoutput/lawfunction.h"
#include "law/lwfcue.h"

using namespace lwf;

using prm::Parameter;
using prm::Names::Position;

prj::srl::lwf::Data Data::serialOut() const
{
  prj::srl::lwf::Parameters psOut;
  for (const auto &ip : internalParameters)
    psOut.array().push_back(ip.serialOut());
  return prj::srl::lwf::Data(static_cast<int>(subType), psOut);
}

void Data::serialIn(const prj::srl::lwf::Data &dataIn)
{
  subType = static_cast<lwf::Type>(dataIn.subType());
  for (const auto &ipIn : dataIn.internalParameters().array())
    internalParameters.emplace_back(ipIn);
}

Cue::Cue()
{
  //default constructed cue creates a constant law [0, 1] with value of 1.0
  setConstant(1.0);
}

Cue::Cue(const prj::srl::lwf::Cue &cIn)
{
  serialIn(cIn);
}

void Cue::setConstant(double vIn)
{
  Parameter p0(Position, osg::Vec3d(0.0, vIn, 0.0));
  Parameter p1(Position, osg::Vec3d(1.0, vIn, 0.0));
  setConstant(p0, p1);
}

void Cue::setConstant(const prm::Parameter &begin, const prm::Parameter &end)
{
  osg::Vec3d p0 = begin.operator osg::Vec3d();
  osg::Vec3d p1 = end.operator osg::Vec3d();
  assert(p0.x() < p1.x());
  assert(p0.y() == p1.y());
  type = Type::constant;
  boundaries.clear();
  datas.clear();
  boundaries.push_back(begin);
  boundaries.push_back(end);
}

void Cue::setLinear(double firstValue, double lastValue)
{
  Parameter p0(Position, osg::Vec3d(0.0, firstValue, 0.0));
  Parameter p1(Position, osg::Vec3d(1.0, lastValue, 0.0));
  setLinear(p0, p1);
}

void Cue::setLinear(const prm::Parameter &begin, const prm::Parameter &end)
{
  osg::Vec3d p0 = begin.operator osg::Vec3d();
  osg::Vec3d p1 = end.operator osg::Vec3d();
  assert(p0.x() < p1.x());
  type = Type::linear;
  boundaries.clear();
  datas.clear();
  boundaries.push_back(begin);
  boundaries.push_back(end);
}

void Cue::setInterpolate
(
  double firstValue
  , double lastValue
  , double firstDerivative
  , double lastDerivative
)
{
  Parameter p0(Position, osg::Vec3d(0.0, firstValue, firstDerivative));
  Parameter p1(Position, osg::Vec3d(1.0, lastValue, lastDerivative));
  setInterpolate(p0, p1);
}

void Cue::setInterpolate(const prm::Parameter &begin, const prm::Parameter &end)
{
  osg::Vec3d p0 = begin.operator osg::Vec3d();
  osg::Vec3d p1 = end.operator osg::Vec3d();
  assert(p0.x() < p1.x());
  type = Type::interpolate;
  boundaries.clear();
  datas.clear();
  boundaries.push_back(prm::Parameter(begin));
  boundaries.push_back(prm::Parameter(end));
}

void Cue::setInterpolateParameters(const Parameters &psIn)
{
  //note that type in Data is left at the default 'none'
  //would be 'interpolate' in a composite scenario
  assert(type == Type::interpolate);
  datas.clear();
  datas.push_back(Data(psIn));
}

static std::vector<osg::Vec3d> disperse(osg::Vec3d start, osg::Vec3d finish, int count)
{
  start.z() = 0.0;
  finish.z() = 0.0;
  
  osg::Vec3d direction = finish - start;
  double distance = direction.length();
  direction.normalize();
  double increment = distance / static_cast<double>(count + 1);
  
  std::vector<osg::Vec3d> out;
  for (int index = 0; index < count; ++index)
    out.push_back(osg::Vec3d(start + (direction * ((index + 1) * increment))));
  return out;
}

void Cue::addInternalParameter(double prm)
{
  if (type == Type::interpolate)
  {
    int count = 0;
    if (!datas.empty())
      count = datas.front().internalParameters.size();
    count++;
    datas.clear();
    
    osg::Vec3d p0 = boundaries.front().operator osg::Vec3d();
    osg::Vec3d p1 = boundaries.back().operator osg::Vec3d();
    std::vector<osg::Vec3d> internalPoints = disperse(p0, p1, count);
    Parameters prms;
    for (const auto &p : internalPoints)
      prms.push_back(prm::Parameter(Position, p));
    datas.push_back(Data(prms));
  }
  if (type == Type::composite)
  {
    int di = compositeIndex(prm);
    if (di == -1)
      return;
    int count = datas.at(di).internalParameters.size();
    count++;
    datas.at(di).internalParameters.clear();
    
    osg::Vec3d p0 = boundaries.at(di).operator osg::Vec3d();
    osg::Vec3d p1 = boundaries.at(di + 1).operator osg::Vec3d();
    std::vector<osg::Vec3d> internalPoints = disperse(p0, p1, count);
    Parameters prms;
    for (const auto &p : internalPoints)
      prms.push_back(prm::Parameter(Position, p));
    datas.at(di).internalParameters = prms;
  }
}

void Cue::removeInternalParameter(double prm)
{
  if (type == Type::interpolate)
  {
    int count = 0;
    if (!datas.empty())
      count = datas.front().internalParameters.size();
    count--;
    datas.clear();
    
    if (count <= 0)
      return;
    
    osg::Vec3d p0 = boundaries.front().operator osg::Vec3d();
    osg::Vec3d p1 = boundaries.back().operator osg::Vec3d();
    std::vector<osg::Vec3d> internalPoints = disperse(p0, p1, count);
    Parameters prms;
    for (const auto &p : internalPoints)
      prms.push_back(prm::Parameter(Position, p));
    datas.push_back(Data(prms));
  }
  if (type == Type::composite)
  {
    int di = compositeIndex(prm);
    if (di == -1)
      return;
    int count = datas.at(di).internalParameters.size();
    count--;
    datas.at(di).internalParameters.clear();
    
    if (count <= 0)
      return;
    
    osg::Vec3d p0 = boundaries.at(di).operator osg::Vec3d();
    osg::Vec3d p1 = boundaries.at(di + 1).operator osg::Vec3d();
    std::vector<osg::Vec3d> internalPoints = disperse(p0, p1, count);
    Parameters prms;
    for (const auto &p : internalPoints)
      prms.push_back(prm::Parameter(Position, p));
    datas.at(di).internalParameters = prms;
  }
}

void Cue::setComposite(double firstValue)
{
  Parameter p0(Position, osg::Vec3d(0.0, firstValue, 0.0));
  setComposite(p0);
}

void Cue::setComposite(const prm::Parameter &pIn)
{
  type = Type::composite;
  boundaries.clear();
  datas.clear();
  boundaries.push_back(pIn);
}

void Cue::appendConstant(double endParameter)
{
  //constant law has same ending value as beginning value.
  osg::Vec3d p0 = boundaries.back().operator osg::Vec3d();
  Parameter p1(Position, osg::Vec3d(endParameter, p0.y(), 0.0));
  
  assert(endParameter > p0.x());
  datas.push_back(Data(Type::constant));
  boundaries.push_back(p1);
}

void Cue::appendLinear(double endParameter, double endValue)
{
  Parameter p1(Position, osg::Vec3d(endParameter, endValue, 0.0));
  appendLinear(p1);
}

void Cue::appendLinear(const prm::Parameter &endParameter)
{
  osg::Vec3d p0 = boundaries.back().operator osg::Vec3d();
  assert(endParameter.operator osg::Vec3d().x() > p0.x());
  datas.push_back(Data(Type::linear));
  boundaries.push_back(endParameter);
}

void Cue::appendInterpolate(double endParameter, double endValue)
{
  osg::Vec3d np(endParameter, endValue, 0.0);
  prm::Parameter prm(Position, np);
  appendInterpolate(prm);
}

void Cue::appendInterpolate(const prm::Parameter &endParameter)
{
  assert(endParameter.operator osg::Vec3d().x() > boundaries.back().operator osg::Vec3d().x());
  datas.push_back(Data(Type::interpolate));
  boundaries.push_back(endParameter);
}

void Cue::appendInterpolate(const prm::Parameter &endParameter, const Parameters &internalParametersIn)
{
  appendInterpolate(endParameter);
  datas.back().internalParameters = internalParametersIn;
}

void Cue::remove(int index)
{
  assert(type == Type::composite);
  assert(index >= 0);
  assert(index < static_cast<int>(datas.size()));
  assert(boundaries.size() > 2); //don't try to remove the last law
  
  //we want to keep the overall parameter range the same.
  //so make the new first parameter equal to the old first parameter, if we remove the first law
  //and the new last parameter equal to the old last parameter, if we remove the last law
  boost::optional<double> firstParameter, lastParameter;
  if (index == 0)
    firstParameter = static_cast<osg::Vec3d>(boundaries.front()).x();
  if (index == static_cast<int>(datas.size()) - 1)
    lastParameter = static_cast<osg::Vec3d>(boundaries.back()).x();
  
  datas.erase(datas.begin() + index);
  boundaries.erase(boundaries.begin() + index + 1);
  
  if (firstParameter)
  {
    osg::Vec3d temp = static_cast<osg::Vec3d>(boundaries.front());
    temp.x() = firstParameter.get();
    boundaries.front().setValueQuiet(temp);
  }
  if (lastParameter)
  {
    osg::Vec3d temp = static_cast<osg::Vec3d>(boundaries.back());
    temp.x() = lastParameter.get();
    boundaries.back().setValueQuiet(temp);
  }
}

void Cue::smooth()
{
  if (type != Type::composite)
    return;
  
  auto getSlope = [](double x1, double y1, double x2, double y2)
  {
    return (y2 - y1) / (x2 - x1);
  };
  
  if (periodic) //smooth any interpolation between front and back.
  {
    osg::Vec3d bf = boundaries.front().operator osg::Vec3d();
    osg::Vec3d bb = boundaries.back().operator osg::Vec3d();
    bb.y() = bf.y();
    
    Type t0 = datas.front().subType;
    Type t1 = datas.back().subType;
    std::size_t bs = boundaries.size();
    
    if (t0 == Type::linear && t1 == Type::interpolate)
    {
      bb.z() = getSlope
      (
        bf.x()
        , bf.y()
        , boundaries.at(1).operator osg::Vec3d().x()
        , boundaries.at(1).operator osg::Vec3d().y()
      );
    }
    else if (t0 == Type::interpolate && t1 == Type::linear)
    {
      bf.z() = getSlope
      (
        boundaries.at(bs - 2).operator osg::Vec3d().x()
        , boundaries.at(bs - 2).operator osg::Vec3d().y()
        , boundaries.at(bs - 1).operator osg::Vec3d().x()
        , boundaries.at(bs - 1).operator osg::Vec3d().y()
      );
    }
    else if (t0 == Type::constant || t1 == Type::constant)
    {
      bf.z() = 0.0;
      bb.z() = 0.0;
    }
    else if (bf.z() != bb.z())
    {
      bf.z() = 0.0;
      bb.z() = 0.0;
    }
    
    boundaries.front().setValueQuiet(bf);
    boundaries.back().setValueQuiet(bb);
  }
  
  for (std::size_t index = 1; index < (boundaries.size() - 1); ++index)
  {
    Type t0 = datas.at(index - 1).subType;
    Type t1 = datas.at(index).subType;
    osg::Vec3d point = boundaries.at(index).operator osg::Vec3d();
    
    if
    (
      (t0 == Type::constant && t1 == Type::interpolate)
      || (t0 == Type::interpolate && t1 == Type::constant)
    )
      point.z() = 0.0;
    
    if (t0 == Type::linear && t1 == Type::interpolate)
    {
      point.z() = getSlope
      (
        boundaries.at(index - 1).operator osg::Vec3d().x()
        , boundaries.at(index - 1).operator osg::Vec3d().y()
        , boundaries.at(index).operator osg::Vec3d().x()
        , boundaries.at(index).operator osg::Vec3d().y()
      );
    }
    
    if (t0 == Type::interpolate && t1 == Type::linear)
    {
      point.z() = getSlope
      (
        boundaries.at(index).operator osg::Vec3d().x()
        , boundaries.at(index).operator osg::Vec3d().y()
        , boundaries.at(index + 1).operator osg::Vec3d().x()
        , boundaries.at(index + 1).operator osg::Vec3d().y()
      );
    }
    /* we ignore c0 conditions with adjacent constants and linears.
     * we also ignore adjacent interpolates. left to user to assign appropriate slope.
     */
    boundaries.at(index).setValueQuiet(point);
  }
}

void Cue::alignConstant()
{
  if (type == Type::constant)
  {
    assert(boundaries.size() == 2);
    osg::Vec3d b0 = static_cast<osg::Vec3d>(boundaries.front());
    osg::Vec3d b1 = static_cast<osg::Vec3d>(boundaries.back());
    b1.y() = b0.y();
    boundaries.back().setValueQuiet(b1);
  }
  else if (type == Type::composite)
  {
    for (std::size_t bi = 0; bi < boundaries.size() - 1; ++bi)
    {
      if (datas.at(bi).subType != Type::constant)
        continue;
      osg::Vec3d b0 = static_cast<osg::Vec3d>(boundaries.at(bi));
      osg::Vec3d b1 = static_cast<osg::Vec3d>(boundaries.at(bi + 1));
      b1.y() = b0.y();
      boundaries.at(bi + 1).setValueQuiet(b1);
    }
  }
}

int Cue::compositeIndex(double prm)
{
  assert(type == Type::composite);
  for (std::size_t bi = 0; bi < boundaries.size() - 1; ++bi)
  {
    osg::Vec3d b0 = static_cast<osg::Vec3d>(boundaries.at(bi));
    osg::Vec3d b1 = static_cast<osg::Vec3d>(boundaries.at(bi + 1));
    if (b0.x() < prm && b1.x() > prm)
      return bi;
  }
  return -1;
}

double Cue::squeezeBack()
{
  assert(type == Type::composite);
  osg::Vec3d last = static_cast<osg::Vec3d>(boundaries.back());
  osg::Vec3d nextLast = static_cast<osg::Vec3d>(*(boundaries.end() - 2));
  double freshParameter = nextLast.x() + ((last.x() - nextLast.x()) / 2.0);
  osg::Vec3d freshLast = last;
  freshLast.x() = freshParameter;
  boundaries.back().setValue(freshLast);
  
  return last.x();
}

std::vector<const prm::Parameter*> Cue::getParameters() const
{
  std::vector<const prm::Parameter*> out;
  
  for (auto &bp : boundaries)
    out.push_back(&bp);
  for (auto &d : datas)
  {
    for (auto &ip : d.internalParameters)
      out.push_back(&ip);
  }
  
  return out;
}

std::vector<prm::Parameter*> Cue::getParameters()
{
  std::vector<prm::Parameter*> out;
  
  for (auto &bp : boundaries)
    out.push_back(&bp);
  for (auto &d : datas)
  {
    for (auto &ip : d.internalParameters)
      out.push_back(&ip);
  }
  
  return out;
}

opencascade::handle<Law_Function> Cue::buildLawFunction() const
{
  auto buildConstant = [](const prm::Parameter &prm0, const prm::Parameter &prm1)
  -> opencascade::handle<Law_Constant>
  {
    opencascade::handle<Law_Constant> t = new Law_Constant();
    osg::Vec3d p0 = prm0.operator osg::Vec3d();
    osg::Vec3d p1 = prm1.operator osg::Vec3d();
    t->Set(p0.y(), p0.x(), p1.x());
    return t;
  };
  
  auto buildLinear = [](const prm::Parameter &prm0, const prm::Parameter &prm1)
  -> opencascade::handle<Law_Linear>
  {
    opencascade::handle<Law_Linear> t = new Law_Linear();
    osg::Vec3d p0 = prm0.operator osg::Vec3d();
    osg::Vec3d p1 = prm1.operator osg::Vec3d();
    t->Set(p0.x(), p0.y(), p1.x(), p1.y());
    return t;
  };
  
  auto buildInterpolate = []
  (
    const prm::Parameter &prm0
    , const prm::Parameter &prm1
    , const lwf::Parameters &ips
    , bool periodic
  )
  -> opencascade::handle<Law_Interpol>
  {
    osg::Vec3d p0 = prm0.operator osg::Vec3d();
    osg::Vec3d p1 = prm1.operator osg::Vec3d();
    int sizeOfPoints = 2;
    sizeOfPoints += ips.size();
    TColgp_Array1OfPnt2d points(1, sizeOfPoints);
    points.ChangeValue(1) = gp_Pnt2d(p0.x(), p0.y());
    int index = 2;
    for (const auto &ip : ips)
    {
      osg::Vec3d tp = ip.operator osg::Vec3d();
      points.ChangeValue(index) = gp_Pnt2d(tp.x(), tp.y());
      ++index;
    }
    points.ChangeValue(sizeOfPoints) = gp_Pnt2d(p1.x(), p1.y());
    
    opencascade::handle<Law_Interpol> t = new Law_Interpol();
    t->Set(points, p0.z(), p1.z(), periodic);
    
    return t;
  };
  
  opencascade::handle<Law_Function> out;
  
  if (type == Type::constant)
  {
    assert(boundaries.size() == 2);
    out = buildConstant(boundaries.front(), boundaries.back());
  }
  else if (type == Type::linear)
  {
    assert(boundaries.size() == 2);
    out = buildLinear(boundaries.front(), boundaries.back());
  }
  else if (type == Type::interpolate)
  {
    opencascade::handle<Law_Interpol> t;
    if (datas.empty())
    {
      lwf::Parameters junk;
      t = buildInterpolate(boundaries.front(), boundaries.back(), junk, periodic);
    }
    else
    {
      assert(datas.size() == 1); //why would there be more than one.
      t = buildInterpolate(boundaries.front(), boundaries.back(), datas.front().internalParameters, periodic);
    }
    out = t;
  }
  else if (type == Type::composite)
  {
    opencascade::handle<Law_Composite> t = new Law_Composite();
    int index = -1;
    for (const auto &data : datas)
    {
      ++index;
      if (data.subType == Type::constant)
        t->ChangeLaws().Append(buildConstant(boundaries.at(index), boundaries.at(index + 1)));
      else if (data.subType == Type::linear)
        t->ChangeLaws().Append(buildLinear(boundaries.at(index), boundaries.at(index + 1)));
      else if (data.subType == Type::interpolate)
      {
        t->ChangeLaws().Append
        (
          buildInterpolate
          (
            boundaries.at(index)
            , boundaries.at(index + 1)
            , data.internalParameters
            , false
          )
        );
      }
    }
    
    out = t;
  }
  
  assert(!out.IsNull());
  return out;
}

prj::srl::lwf::Cue Cue::serialOut() const
{
  prj::srl::lwf::Parameters bo;
  for (const auto &b : boundaries)
    bo.array().push_back(b.serialOut());
  
  prj::srl::lwf::Datas datasOut;
  for (const auto &d : datas)
    datasOut.array().push_back(d.serialOut());
  
  return prj::srl::lwf::Cue
  (
    static_cast<int>(type)
    , bo
    , datasOut
    , periodic
  );
}

void Cue::serialIn(const prj::srl::lwf::Cue &cueIn)
{
  for (const auto &bsIn : cueIn.boundaries().array())
    boundaries.emplace_back(bsIn);
  
  for (const auto &dsIn : cueIn.datas().array())
  {
    datas.emplace_back();
    datas.back().serialIn(dsIn);
  }
  
  type = static_cast<Type>(cueIn.type());
  periodic = cueIn.periodic();
}

std::vector<osg::Vec3d> lwf::discreteLaw(opencascade::handle<Law_Function> &law, int steps)
{
  std::vector<osg::Vec3d> out;
  
  double pf = 0.0;
  double pl = 1.0;
  law->Bounds(pf, pl);
  double stepDistance = (pl - pf) / steps;
  //number of points = steps + 1. Notice '<='
  for (int index = 0; index <= steps; ++index)
  {
    double cx = pf + stepDistance * static_cast<double>(index);
    double cy = law->Value(cx);
    out.push_back(osg::Vec3d(cx, cy, 0.0));
  }
  
  return out;
}
