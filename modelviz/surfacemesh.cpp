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

#include "mesh/mesh.h"
#include "annex/surfacemesh.h"
#include "modelviz/nodemaskdefs.h"
#include "modelviz/surfacemesh.h"

osg::Switch* mdv::generate(const ann::SurfaceMesh &sfIn)
{
  osg::ref_ptr<osg::Switch> out = new osg::Switch();

  osg::Geometry *faceGeometry = new osg::Geometry();
  out->addChild(faceGeometry);
  osg::Vec3Array *faceVertices = new osg::Vec3Array();
  faceGeometry->setVertexArray(faceVertices);
  osg::Vec4Array *faceColors = new osg::Vec4Array();
  faceGeometry->setColorArray(faceColors);
  faceGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
  faceGeometry->setNodeMask(mdv::face);
  faceGeometry->setName("faces");
  osg::Depth *faceDepth = new osg::Depth();
  faceDepth->setRange(0.002, 1.002);
  faceGeometry->getOrCreateStateSet()->setAttributeAndModes(faceDepth);
  faceGeometry->getOrCreateStateSet()->setAttributeAndModes(new osg::ShadeModel(osg::ShadeModel::FLAT));
  faceGeometry->setDataVariance(osg::Object::STATIC);
  faceGeometry->setUseDisplayList(false);
//     faceGeometry->setUseVertexBufferObjects(true);
  faceGeometry->setNormalArray(new osg::Vec3Array());
  faceGeometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
  
  using Mesh = msh::srf::Mesh;
  using Face = msh::srf::Mesh::Face_index;
  using Vertex = msh::srf::Mesh::Vertex_index;
  using Edge = msh::srf::Mesh::Edge_index;
  
  const Mesh &m = sfIn.getStow().mesh;
  osg::ref_ptr<osg::DrawElementsUInt> triangles = new osg::DrawElementsUInt(GL_TRIANGLES);
  for (Face f : m.faces())
  {
    for(Vertex vd : vertices_around_face(m.halfedge(f), m))
    {
      faceVertices->push_back(osg::Vec3(m.point(vd).x(), m.point(vd).y(), m.point(vd).z()));
      faceColors->push_back(osg::Vec4(1.0, 0.0, 0.0, 1.0));
      triangles->push_back(faceVertices->size() - 1);
    }
  }
  faceGeometry->addPrimitiveSet(triangles.get());
  //not sure, but it appears that the smoothing visitor is using duplicate
  //vertices as the same and screwing up mesh normal calculation. Hence the 0.0 parameter.
  osgUtil::SmoothingVisitor::smooth(*faceGeometry, 0.0); //build normals
  
  osg::Geometry *edgeGeometry = new osg::Geometry();
  out->addChild(edgeGeometry);
  osg::Vec3Array *edgeVertices = new osg::Vec3Array();
  edgeGeometry->setVertexArray(edgeVertices);
  osg::ref_ptr<osg::Vec4Array> edgeColors = new osg::Vec4Array();
  edgeGeometry->setColorArray(edgeColors.get());
  edgeGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
  edgeGeometry->setName("edges");
  edgeGeometry->setDataVariance(osg::Object::STATIC);
  edgeGeometry->setUseDisplayList(false);
  edgeGeometry->getOrCreateStateSet()->setAttributeAndModes(new osg::LineWidth(2.0f));
  osg::Depth *edgeDepth = new osg::Depth();
  edgeDepth->setRange(0.001, 1.001);
  edgeGeometry->getOrCreateStateSet()->setAttributeAndModes(edgeDepth);
  edgeGeometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
  
  osg::ref_ptr<osg::DrawElementsUInt> edges = new osg::DrawElementsUInt(GL_LINES);
  for (Edge e : m.edges())
  {
    Vertex start = m.vertex(e, 0);
    Vertex end = m.vertex(e, 1);
    edgeVertices->push_back(osg::Vec3(m.point(start).x(), m.point(start).y(), m.point(start).z()));
    edges->push_back(edgeVertices->size() - 1);
    edgeColors->push_back(osg::Vec4(0.0, 0.0, 0.0, 1.0));

    edgeVertices->push_back(osg::Vec3(m.point(end).x(), m.point(end).y(), m.point(end).z()));
    edges->push_back(edgeVertices->size() - 1);
    edgeColors->push_back(osg::Vec4(0.0, 0.0, 0.0, 1.0));
  }
  edgeGeometry->addPrimitiveSet(edges.get());
  
  return out.release();
}
