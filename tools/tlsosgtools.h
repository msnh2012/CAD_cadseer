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

#ifndef TLS_OSGTOOLS_H
#define TLS_OSGTOOLS_H

#include <boost/optional/optional.hpp>

#include <osg/Vec3d>
#include <osg/Matrixd>

namespace tls
{
  boost::optional<osg::Matrixd> matrixFrom3Points(const osg::Vec3d&, const osg::Vec3d&, const osg::Vec3d&);
  boost::optional<osg::Matrixd> matrixFrom3Points(const std::vector<osg::Vec3d>&);
}

#endif //TLS_OSGTOOLS_H
