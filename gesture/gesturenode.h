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

#ifndef GSN_GESTURENODE_H
#define GSN_GESTURENODE_H

#include <osg/ref_ptr>

namespace osg
{
  class MatrixTransform;
  class Texture2D;
  class TexEnv;
  class Depth;
}

namespace gsn
{
  //! used to cache common data during menu construction. all for icon.
  struct NodeCue
  {
    NodeCue() = delete;
    NodeCue(double radiusIn, int sidesIn = -1);
    ~NodeCue();
    double radius;
    int sides;
    osg::ref_ptr<osg::Vec3Array> vertices;
    osg::ref_ptr<osg::Vec3Array> normals; //only 1 bind overall
    osg::ref_ptr<osg::Vec4Array> colors; //only 1 bind overall
    osg::ref_ptr<osg::Texture2D> backTexture;
    osg::ref_ptr<osg::TexEnv> texenv;
    osg::ref_ptr<osg::Vec2Array> tCoords0;
    osg::ref_ptr<osg::Vec2Array> tCoords1;
    osg::ref_ptr<osg::Depth> depth;
  };
  
  osg::MatrixTransform* buildNode(const char*, unsigned int, const NodeCue&);
}

#endif // GSN_GESTURENODE_H
