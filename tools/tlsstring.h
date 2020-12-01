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

#ifndef TLS_STRING_H
#define TLS_STRING_H

#include <string>

namespace osg{class Vec3d; class Quat; class Matrixd;}

namespace tls
{
  //! convert double to a formatted string. precision less than zero uses default value. see source
  std::string prettyDouble(const double &dIn, int precision = -1);
  //! convert an osg vector value to an expression conforming string.
  std::string valueString(const osg::Vec3d&, int = -1);
  //! convert an osg quaternion value to an expression conforming string.
  std::string valueString(const osg::Quat&, int = -1);
  //! convert an osg matrix value to an expression conforming string.
  std::string valueString(const osg::Matrixd&, int = -1);
}

#endif
