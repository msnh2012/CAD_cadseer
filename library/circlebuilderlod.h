/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2018  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef LBR_CIRCLEBUILDERLOD_H
#define LBR_CIRCLEBUILDERLOD_H

namespace osg{class LOD;}

namespace lbr
{
  /**
  * @class CircleBuilderLOD
  * @brief constructs lod node for circles and arcs.
  * @details 3 levels with hard coded values.
  * This builder uses circlebuilder so has no
  * color set.
  */
  class CircleBuilderLOD
  {
  public:
    CircleBuilderLOD() = delete;
    CircleBuilderLOD(const CircleBuilderLOD&) = delete;
    CircleBuilderLOD(double, double);
    
    operator osg::LOD* () const;
    
  protected:
    double radius; //!< radius of arc or circle
    double angle; //!< angle that the circle or arc spans in radians
    double screenHeight; //!< height of the screen using osg::displaysettings
  };
}

#endif // LBR_CIRCLEBUILDERLOD_H
