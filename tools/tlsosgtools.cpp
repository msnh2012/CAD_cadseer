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

#include <functional>

#include "tools/tlsosgtools.h"

/*! @brief Create a matrix based upon points.
* 
* @details needs to 2 points of x, y, z.
* points.at(0) is the optional origin, set to 0 0 0 if not present.
* points.at(1) is the optional x point
* points.at(2) is the optional y point
* points.at(3) is the optional z point
* @param pointsIn vector of 4 optional points
* 
* @return An optional osg::Matrixd.
*/
std::optional<osg::Matrixd> tls::matrixFromPoints(const std::array<std::optional<osg::Vec3d>, 4> &pointsIn)
{
  //looking goofy as points expects origin to be first while axes expects origin to be last.
  std::array<std::optional<osg::Vec3d>, 4> vectorsOut;
  
  vectorsOut[3] = osg::Vec3d();
  if (pointsIn.at(0))
    vectorsOut[3] = *pointsIn.at(0);
  if (pointsIn.at(1))
    vectorsOut[0] = *pointsIn.at(1) - *vectorsOut[3];
  if (pointsIn.at(2))
    vectorsOut[1] = *pointsIn.at(2) - *vectorsOut[3];
  if (pointsIn.at(3))
    vectorsOut[2] = *pointsIn.at(3) - *vectorsOut[3];
  
  return matrixFromAxes(vectorsOut);
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
std::optional<osg::Matrixd> tls::matrixFromAxes(const std::array<std::optional<osg::Vec3d>, 4> &oPointsIn)
{
  auto oxAxis = oPointsIn.at(0);
  auto oyAxis = oPointsIn.at(1);
  auto ozAxis = oPointsIn.at(2);
  auto oOrigin = oPointsIn.at(3);
  
  
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
    
    osg::Matrixd fm;
    fm(0,0) = lx.x(); fm(0,1) = lx.y(); fm(0,2) = lx.z();
    fm(1,0) = ly.x(); fm(1,1) = ly.y(); fm(1,2) = ly.z();
    fm(2,0) = lz.x(); fm(2,1) = lz.y(); fm(2,2) = lz.z();
    fm.setTrans(lo);
    
    return fm;
  }
  
  return std::nullopt;
}
