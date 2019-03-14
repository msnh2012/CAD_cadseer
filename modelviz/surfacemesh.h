/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2019  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef MDV_SURFACEMESH_H
#define MDV_SURFACEMESH_H

#include <osg/NodeCallback>

namespace osg{class Switch;}
namespace ann{class SurfaceMesh;}

namespace mdv
{
  osg::Switch* generate(const ann::SurfaceMesh&);
  
  class MeshCallback : public osg::NodeCallback
  {
  public:
    void showTriangles();
    void hideTriangles();
    void showEdges();
    void hideEdges();
    void showBorderEdges();
    void hideBorderEdges();
    
    void operator()(osg::Node*, osg::NodeVisitor*) override;
  private:
    bool dirty = true;
    bool triVis = true;
    bool intEdgeVis = true;
    bool extEdgeVis = true;
  };
}

#endif //MDV_SURFACEMESH_H
