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

#include "tools/tlsosgtools.h"

using osg::Vec3d;
using osg::Matrixd;
using boost::optional;

/*! @brief Create a matrix based upon 3 non collinear points.
* 
* @param orignIn point used for the matrix translation.
* @param xIn point on xAxis.
* @param yIn point on Y+ plane.
* @return An optional osg::Matrixd.
*/
optional<Matrixd> tls::matrixFrom3Points(const Vec3d &originIn, const Vec3d &xIn, const Vec3d &yIn)
{
  Vec3d xVector = xIn - originIn;
  Vec3d yVector = yIn - originIn;
  if (xVector.isNaN() || yVector.isNaN())
    return boost::none;
  xVector.normalize();
  yVector.normalize();
  if ((1 - std::fabs(xVector * yVector)) < std::numeric_limits<float>::epsilon())
    return boost::none;
  Vec3d zVector = xVector ^ yVector;
  if (zVector.isNaN())
    return boost::none;
  zVector.normalize();
  yVector = zVector ^ xVector;
  yVector.normalize();

  Matrixd fm;
  fm(0,0) = xVector.x(); fm(0,1) = xVector.y(); fm(0,2) = xVector.z();
  fm(1,0) = yVector.x(); fm(1,1) = yVector.y(); fm(1,2) = yVector.z();
  fm(2,0) = zVector.x(); fm(2,1) = zVector.y(); fm(2,2) = zVector.z();
  fm.setTrans(originIn);
  
  return fm;
}

/*! @brief Create a matrix based upon vector points.
* 
* @param pointsIn vector of 3 points
* @return An optional osg::Matrixd.
*/
optional<Matrixd> tls::matrixFrom3Points(const std::vector<Vec3d> &pointsIn)
{
  if (pointsIn.size() != 3)
    return boost::none;
  return tls::matrixFrom3Points(pointsIn.at(0), pointsIn.at(1), pointsIn.at(2));
}
