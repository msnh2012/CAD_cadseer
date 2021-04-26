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

/*! @brief Create a matrix from optional axes and optional origin
* 
* @details At least two non-parallel axes are needed to be successful.
* When all three axes are passed in, the most orthogonal pair will be master.
* That means, in that case, the passed in x axis might be changed. If
* that is not desired just pass two vectors in instead. If the origin is nullopt
* the matrix origin will be 0 0 0 as expected.
* @return a osg::Matrixd if successful otherwise nullopt
*/
std::optional<osg::Matrixd> tls::matrixFromAxes
(
  std::optional<osg::Vec3d> oxAxis
  , std::optional<osg::Vec3d> oyAxis
  , std::optional<osg::Vec3d> ozAxis
  , std::optional<osg::Vec3d> oOrigin
)
{
  //helper lambdas
  auto isBad = [](const std::optional<osg::Vec3d> &oAxis) -> bool
  {
    return !oAxis || !oAxis->valid() || oAxis->length() < std::numeric_limits<float>::epsilon();
  };
  
  auto areParallel = [](const osg::Vec3d &v0, const osg::Vec3d &v1) -> bool
  {
    if ((1 - std::fabs(v0 * v1)) < std::numeric_limits<float>::epsilon())
      return true;
    return false;
  };
  
  //assumes other axes are present and normalized. resultant is normalized.
  auto crossToX = [&]() {oxAxis = *oyAxis ^ *ozAxis; oxAxis->normalize();};
  auto crossToY = [&]() {oyAxis = *ozAxis ^ *oxAxis; oyAxis->normalize();};
  auto crossToZ = [&]() {ozAxis = *oxAxis ^ *oyAxis; ozAxis->normalize();};
  
  //verify we have enough to get started
  int invalidCount = 0;
  if (isBad(oxAxis))
  {
    oxAxis = std::nullopt;
    invalidCount++;
  }
  if (isBad(oyAxis))
  {
    oyAxis = std::nullopt;
    invalidCount++;
  }
  if (isBad(ozAxis))
  {
    ozAxis = std::nullopt;
    invalidCount++;
  }
  int axesCount = 3 - invalidCount; //easier to reason
  if (axesCount < 2)
    return std::nullopt;
  
  //make sure inputs are normalized
  if (oxAxis) oxAxis->normalize();
  if (oyAxis) oyAxis->normalize();
  if (ozAxis) ozAxis->normalize();
  
  std::function<void()> op1;
  std::function<void()> op2;
  
  //condition 1 all 3 axes are good to go.
  if (oxAxis && oyAxis && ozAxis)
  {
    //going to test dot and cross pair with closet to normal
    double dotMin = std::fabs(*oxAxis * *oyAxis);
    op1 = crossToZ;
    op2 = crossToY;
    
    double tempDot = std::fabs(*oxAxis * *ozAxis);
    if (tempDot < dotMin)
    {
      dotMin = tempDot;
      op1 = crossToY;
      op2 = crossToZ;
    }
    
    tempDot = std::fabs(*oyAxis * *ozAxis);
    if (tempDot < dotMin)
    {
      dotMin = tempDot;
      op1 = crossToX;
      op2 = crossToZ;
    }
  }
  else if (oxAxis && oyAxis && !areParallel(*oxAxis, *oyAxis)) //just x and y are good to go.
  {
    op1 = crossToZ;
    op2 = crossToY;
  }
  else if (oxAxis && ozAxis && !areParallel(*oxAxis, *ozAxis)) //just x and z are good to go.
  {
    op1 = crossToY;
    op2 = crossToZ;
  }
  else if (oyAxis && ozAxis && !areParallel(*oyAxis, *ozAxis)) //just y and z are good to go.
  {
    op1 = crossToX;
    op2 = crossToZ;
  }

  if (op1 && op2)
  {
    op1();
    op2();
    
    osg::Vec3d lx = *oxAxis; //local x axis
    osg::Vec3d ly = *oyAxis; //local y axis
    osg::Vec3d lz = *ozAxis; //local z axis
    osg::Vec3d lo; //local origin
    if (oOrigin)
      lo = *oOrigin;
    
    Matrixd fm;
    fm(0,0) = lx.x(); fm(0,1) = lx.y(); fm(0,2) = lx.z();
    fm(1,0) = ly.x(); fm(1,1) = ly.y(); fm(1,2) = ly.z();
    fm(2,0) = lz.x(); fm(2,1) = lz.y(); fm(2,2) = lz.z();
    fm.setTrans(lo);
    
    return fm;
  }
  
  return std::nullopt;
}
