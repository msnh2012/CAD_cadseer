/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2020 Thomas S. Anderson blobfish.at.gmx.com
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

#include "subprojects/pmp-library/src/pmp/SurfaceMesh.h"

#include "mesh/mshconvert.h"

pmp::SurfaceMesh msh::srf::convert(const msh::srf::Mesh &mIn)
{
  msh::srf::Mesh workMesh = mIn;
  workMesh.collect_garbage();
  
  pmp::SurfaceMesh out;
  for (const auto &vIn : CGAL::vertices(workMesh))
  {
    const auto &pIn = workMesh.point(vIn);
    out.add_vertex(pmp::Point(pIn.x(), pIn.y(), pIn.z()));
  }
  for (const auto &fIn : CGAL::faces(workMesh))
  {
    std::vector<pmp::Vertex> ps;
    for (const auto &he : CGAL::halfedges_around_face(workMesh.halfedge(fIn), workMesh))
      ps.push_back(pmp::Vertex(static_cast<pmp::IndexType>(workMesh.source(he))));
    out.add_face(ps);
  }
  
  out.garbage_collection();
  return out;
}

msh::srf::Mesh msh::srf::convert(const pmp::SurfaceMesh &mIn)
{
  msh::srf::Mesh out;
  
  for (const auto &vIn : mIn.vertices())
  {
    pmp::Point pIn = mIn.position(vIn);
    msh::srf::Point pOut(pIn[0], pIn[1], pIn[2]);
    out.add_vertex(pOut);
  }
  for (const auto &fIn : mIn.faces())
  {
    std::vector<msh::srf::Mesh::vertex_index> verts;
    pmp::SurfaceMesh::VertexAroundFaceCirculator it(&mIn, fIn);
    pmp::SurfaceMesh::VertexAroundFaceCirculator itEnd;
    for (pmp::Vertex v : pmp::SurfaceMesh::VertexAroundFaceCirculator(&mIn, fIn))
      verts.push_back(static_cast<msh::srf::Mesh::vertex_index>(v.idx()));
    out.add_face(verts);
  }
  
  out.collect_garbage();
  return out;
}
