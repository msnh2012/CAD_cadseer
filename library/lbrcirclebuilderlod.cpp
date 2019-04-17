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

#include <limits>

#include <osg/DisplaySettings>
#include <osg/LOD>
#include <osg/Geometry>

#include "library/lbrcirclebuilder.h"
#include "library/lbrcirclebuilderlod.h"

using namespace lbr;

/*! @brief construct the CircleBuilderLOD
 * 
 * @param radiusIn is the radius of circle or arc.
 * @param angleIn is the angle that the arc will span in radians.
 * 
 */
CircleBuilderLOD::CircleBuilderLOD(double radiusIn, double angleIn):
radius(radiusIn)
, angle(angleIn)
, screenHeight(osg::DisplaySettings::instance()->getScreenHeight())
{
}

CircleBuilderLOD::operator osg::LOD* () const
{
  osg::LOD *out = new osg::LOD();
  out->setRangeMode(osg::LOD::PIXEL_SIZE_ON_SCREEN);
  
  CircleBuilder b;
  b.setRadius(radius);
  b.setAngularSpanRadians(angle);
  {
    //lowest resolution to have 16 sides for a full circle.
    //and goes from 0 projection size to half of screen height.
    int sides = static_cast<int>(16.0 * angle / (osg::PI * 2.0));
    b.setSegments(sides);
    out->addChild(static_cast<osg::Geometry*>(b), 0.0, screenHeight / 4.0);
  }
  {
    //middle resolution to have 64 sides for a full circle.
    //and goes from half of screen height to 2 * screen height.
    int sides = static_cast<int>(64.0 * angle / (osg::PI * 2.0));
    b.setSegments(sides);
    out->addChild(static_cast<osg::Geometry*>(b), screenHeight / 4.0, screenHeight * 2.0);
  }
  {
    //highest resolution to have 512 sides for a full circle.
    //and goes from 2 * screen height to float max.
    int sides = static_cast<int>(512.0 * angle / (osg::PI * 2.0));
    b.setSegments(sides);
    out->addChild(static_cast<osg::Geometry*>(b), screenHeight * 2.0, std::numeric_limits<float>::max());
  }
  
  return out;
}
