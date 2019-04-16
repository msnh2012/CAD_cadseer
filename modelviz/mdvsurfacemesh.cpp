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

#include <osg/Switch>
#include <osg/Geometry>
#include <osg/Depth>
#include <osg/LineWidth>
#include <osg/ShadeModel>
#include <osgUtil/SmoothingVisitor>
#include <osgUtil/UpdateVisitor>

#include "library/childnamevisitor.h"
#include "mesh/mesh.h"
#include "annex/surfacemesh.h"
#include "modelviz/nodemaskdefs.h"
#include "modelviz/surfacemesh.h"

void assignDepth(osg::Geometry *g, double low, double high)
{
  osg::Depth *depth = new osg::Depth();
  depth->setRange(low, high);
  g->getOrCreateStateSet()->setAttributeAndModes(depth);
}

osg::Switch* mdv::generate(const ann::SurfaceMesh &sfIn)
{
  using Mesh = msh::srf::Mesh;
  using Face = msh::srf::Mesh::Face_index;
  using Vertex = msh::srf::Mesh::Vertex_index;
  using Edge = msh::srf::Mesh::Edge_index;
  using Point = msh::srf::Mesh::Point;
  
  osg::ref_ptr<osg::Switch> out = new osg::Switch();
  out->setUpdateCallback(new mdv::MeshCallback());

  osg::Geometry *faceGeometry = new osg::Geometry();
  out->addChild(faceGeometry);
  osg::Vec3Array *faceVertices = new osg::Vec3Array();
  faceGeometry->setVertexArray(faceVertices);
  osg::Vec4Array *faceColors = new osg::Vec4Array();
  faceGeometry->setColorArray(faceColors);
  faceGeometry->setColorBinding(osg::Geometry::BIND_OVERALL);
  faceColors->push_back(osg::Vec4(1.0, 0.0, 0.0, 1.0));
  faceGeometry->setNodeMask(mdv::face);
  faceGeometry->setName("faces");
  assignDepth(faceGeometry, 0.004, 1.004);
  faceGeometry->getOrCreateStateSet()->setAttributeAndModes(new osg::ShadeModel(osg::ShadeModel::FLAT));
  faceGeometry->setDataVariance(osg::Object::STATIC);
  faceGeometry->setUseDisplayList(false);
//     faceGeometry->setUseVertexBufferObjects(true);
  faceGeometry->setNormalArray(new osg::Vec3Array());
  faceGeometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
  
  
  const Mesh &m = sfIn.getStow().mesh;
  osg::ref_ptr<osg::DrawElementsUInt> triangles = new osg::DrawElementsUInt(GL_TRIANGLES);
  for (Face f : m.faces())
  {
    for(Vertex vd : vertices_around_face(m.halfedge(f), m))
    {
      faceVertices->push_back(osg::Vec3(m.point(vd).x(), m.point(vd).y(), m.point(vd).z()));
      triangles->push_back(faceVertices->size() - 1);
    }
  }
  faceGeometry->addPrimitiveSet(triangles.get());
  //not sure, but it appears that the smoothing visitor is using duplicate
  //vertices as the same and screwing up mesh normal calculation. Hence the 0.0 parameter.
  osgUtil::SmoothingVisitor::smooth(*faceGeometry, 0.0); //build normals
  
  osg::Geometry *intEdges = new osg::Geometry();
  out->addChild(intEdges);
  osg::Vec3Array *intEdgeVertices = new osg::Vec3Array();
  intEdges->setVertexArray(intEdgeVertices);
  osg::ref_ptr<osg::Vec4Array> intEdgeColors = new osg::Vec4Array();
  intEdges->setColorArray(intEdgeColors.get());
  intEdgeColors->push_back(osg::Vec4(0.0, 0.0, 1.0, 1.0));
  intEdges->setColorBinding(osg::Geometry::BIND_OVERALL);
  intEdges->setName("intEdges");
  intEdges->setNodeMask(mdv::edge);
  intEdges->setDataVariance(osg::Object::STATIC);
  intEdges->setUseDisplayList(false);
  intEdges->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
  
  osg::Geometry *extEdges = static_cast<osg::Geometry*>(intEdges->clone(osg::CopyOp::DEEP_COPY_ALL));
  out->addChild(extEdges);
  extEdges->setName("extEdges");
  static_cast<osg::Vec4Array&>(*extEdges->getColorArray())[0] = osg::Vec4(0.0, 1.0, 0.0, 1.0);
  osg::Vec3Array *extEdgeVertices = static_cast<osg::Vec3Array*>(extEdges->getVertexArray());
  
  assignDepth(intEdges, 0.003, 1.003);
  assignDepth(extEdges, 0.002, 1.002);
  
  intEdges->getOrCreateStateSet()->setAttributeAndModes(new osg::LineWidth(2.0f));
  extEdges->getOrCreateStateSet()->setAttributeAndModes(new osg::LineWidth(5.0f));
  
  osg::DrawElementsUInt *intEdgeElements = new osg::DrawElementsUInt(GL_LINES);
  intEdges->addPrimitiveSet(intEdgeElements);
  osg::DrawElementsUInt *extEdgeElements = new osg::DrawElementsUInt(GL_LINES);
  extEdges->addPrimitiveSet(extEdgeElements);
  
  auto convertPoint = [&m](msh::srf::Vertex vIn) -> osg::Vec3d
  {
    Point p = m.point(vIn);
    return osg::Vec3d(p.x(), p.y(), p.z());
  };
  for (Edge e : m.edges())
  {
    osg::Vec3d start = convertPoint(m.vertex(e, 0));
    osg::Vec3d end = convertPoint(m.vertex(e, 1));
    
    intEdgeVertices->push_back(start);
    intEdgeElements->push_back(intEdgeVertices->size() - 1);
    intEdgeVertices->push_back(end);
    intEdgeElements->push_back(intEdgeVertices->size() - 1);
    
    if (m.is_border(e))
    {
      extEdgeVertices->push_back(start);
      extEdgeElements->push_back(extEdgeVertices->size() - 1);
      extEdgeVertices->push_back(end);
      extEdgeElements->push_back(extEdgeVertices->size() - 1);
    }
  }
  
  return out.release();
}

using namespace mdv;

void MeshCallback::showTriangles()
{
  dirty = true;
  triVis = true;
}

void MeshCallback::hideTriangles()
{
  dirty = true;
  triVis = false;
}

void MeshCallback::showEdges()
{
  dirty = true;
  intEdgeVis = true;
}

void MeshCallback::hideEdges()
{
  dirty = true;
  intEdgeVis = false;
}

void MeshCallback::showBorderEdges()
{
  dirty = true;
  extEdgeVis = true;
}

void MeshCallback::hideBorderEdges()
{
  dirty = true;
  extEdgeVis = false;
}

void MeshCallback::operator()(osg::Node *node, osg::NodeVisitor *visitor)
{
  osgUtil::UpdateVisitor *uVisitor = visitor->asUpdateVisitor();
  if (uVisitor)
  {
    if (dirty)
    {
      osg::Switch *theSwitch = dynamic_cast<osg::Switch*>(node);
      assert(theSwitch);
      
      auto go = [theSwitch](const std::string& nameIn, bool setting)
      {
        lbr::ChildNameVisitor v(nameIn.c_str());
        theSwitch->accept(v);
        assert(v.out);
        if (!v.out)
          return; //warning?
        theSwitch->setChildValue(v.out, setting);
      };
      
      go("faces", triVis);
      go("intEdges", intEdgeVis);
      go("extEdges", extEdgeVis);
      dirty = false;
    }
  }
  traverse(node, visitor);
}
