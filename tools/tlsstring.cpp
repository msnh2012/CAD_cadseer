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

#include <sstream>
#include <iomanip>
#include <limits>

#include <osg/Vec3d>
#include <osg/Quat>
#include <osg/Matrixd>

#include "tools/tlsstring.h"

std::string tls::prettyDouble(const double &dIn, int precision)
{
  //std::numeric_limits<double>::digits10: gives '25.300000000000001' for '25.3'
  //std::numeric_limits<double>::digits10 - 1: gives '25.3' for '25.3'
  if (precision < 0)
    precision = std::numeric_limits<double>::digits10 - 1;
  std::ostringstream stream;
  stream << std::setprecision(precision) << std::fixed << dIn;
  std::string out = stream.str();
  out.erase(out.find_last_not_of('0') + 1, std::string::npos);
  if (out.back() == '.')
    out += '0';
  return out;
}
  
std::string tls::valueString(const osg::Vec3d &v, int p)
{
  std::stringstream out;
  out << "[" << prettyDouble(v.x(), p) << ", " << prettyDouble(v.y(), p) << ", " << prettyDouble(v.z(), p) << "]";
  return out.str();
}

std::string tls::valueString(const osg::Quat &q, int p)
{
  std::stringstream out;
  out << "[" << prettyDouble(q.x(), p) << ", " << prettyDouble(q.y(), p) << ", " << prettyDouble(q.z(), p) << ", " << prettyDouble(q.w(), p) << "]";
  return out.str();
}

std::string tls::valueString(const osg::Matrixd &m, int p)
{
  osg::Quat q = m.getRotate();
  osg::Vec3d t = m.getTrans();
  std::stringstream out;
  out << "[" << prettyDouble(q.x(), p) << ", " << prettyDouble(q.y(), p) << ", " << prettyDouble(q.z(), p) << ", " << prettyDouble(q.w(), p)
  << ", " << prettyDouble(t.x(), p) << ", " << prettyDouble(t.y(), p) << ", " << prettyDouble(t.z(), p) << "]";
  return out.str();
}
