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

#include <memory>

namespace osg
{
  class MatrixTransform;
}

namespace gsn
{
  /*! @struct NodeBuilder
   * @brief Used to construct menu icons.
   * 
   * @details texture sizes are made to be a power of 2.
   * if size of icon radius is, for example, 48 we get
   * a bunch of osg notifications that the image is being scaled.
   * seems to be working right, so not looking into it any further.
   */
  struct NodeBuilder
  {
    NodeBuilder() = delete;
    NodeBuilder(double radiusIn, int sidesIn = -1);
    ~NodeBuilder();
    void initialize(); //called by default in constructor.
    osg::MatrixTransform* buildNode(const char*, unsigned int);
    struct Stow;
    std::unique_ptr<Stow> stow;
  };
  
}

#endif // GSN_GESTURENODE_H
