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

#include <cassert>
#include <functional>
#include <cstddef> //null error from nglib

#include <CGAL/Polygon_mesh_processing/remesh.h>

#include "subprojects/pmp-library/src/pmp/SurfaceMesh.h"
#include "subprojects/pmp-library/src/pmp/algorithms/SurfaceRemeshing.h"

#include "project/serial/generated/prjsrlsfmssurfacemesh.h"
#include "mesh/mshparameters.h"
#include "mesh/mshmesh.h"
#include "mesh/mshconvert.h"
#include "mesh/mshfillholescgal.h"
#include "mesh/mshfillholespmp.h"
#include "annex/annsurfacemesh.h"

using namespace ann;

SurfaceMesh::SurfaceMesh() : Base(), stow(new msh::srf::Stow()) {}

SurfaceMesh::SurfaceMesh(const msh::srf::Stow &stowIn)
: Base()
, stow(new msh::srf::Stow(stowIn))
{
}

SurfaceMesh& SurfaceMesh::operator=(const SurfaceMesh &rhs)
{
  stow->mesh = rhs.stow->mesh;
  return *this;
}

SurfaceMesh& SurfaceMesh::operator=(const msh::srf::Stow &sIn)
{
  stow->mesh = sIn.mesh;
  return *this;
}

SurfaceMesh::~SurfaceMesh() {}

const msh::srf::Stow& SurfaceMesh::getStow() const
{
  return *stow;
}

/*! @brief Add a mesh from an OFF formated file.
 * 
 * @param file ascii OFF file to read and append.
 * @return success state of operation.
 * @note does not clear any existing mesh data
 */
bool SurfaceMesh::readOFF(const boost::filesystem::path &file)
{
  std::ifstream stream(file.string(), std::ios_base::in);
  if (!stream.is_open())
    return false;
  return CGAL::read_off(stream, stow->mesh);
}

/*! @brief Write mesh to an OFF formated file.
 * 
 * @param file OFF file to write 'this' mesh.
 * @return success state of operation.
 */
bool SurfaceMesh::writeOFF(const boost::filesystem::path &file) const
{
  std::ofstream stream(file.string(), std::ios_base::out | std::ios_base::trunc);
  if (!stream.is_open())
    return false;
  return CGAL::write_off(stream, stow->mesh);
}

/*! @brief Add a mesh from an OFF formated file.
 * 
 * @param file ascii or binary PLY file to read and append.
 * @return success state of operation.
 * @note does not clear any existing mesh data
 */
bool SurfaceMesh::readPLY(const boost::filesystem::path &file)
{
  std::ifstream stream(file.string(), std::ios_base::in);
  if (!stream.is_open())
    return false;
  return CGAL::read_ply(stream, stow->mesh);
}

/*! @brief Write mesh to a PLY formated file.
 * 
 * @param file PLY file to write 'this' mesh.
 * @return success state of operation.
 */
bool SurfaceMesh::writePLY(const boost::filesystem::path &file) const
{
  std::ofstream stream(file.string(), std::ios_base::out | std::ios_base::trunc);
  if (!stream.is_open())
    return false;
  return CGAL::write_ply(stream, stow->mesh);
}


/*! @brief Remesh the contained mesh
 * 
 * @param edgeLength Is the target length for all edges.
 * @param iterations Is the number of algorithm iterations. 3 is a good start. cgal default is 1.
 */
void SurfaceMesh::remeshCGAL(double edgeLength, int iterations)
{
  namespace cpmp = CGAL::Polygon_mesh_processing;
  cpmp::isotropic_remeshing
  (
    CGAL::faces(stow->mesh),
    edgeLength,
    stow->mesh,
    cpmp::parameters::number_of_iterations(iterations)
    .protect_constraints(false)//false = don't protect border
  );
}

/*! @brief Remesh the contained mesh
 * 
 * @param edgeLength Is the target length for all edges.
 * @param iterations Is the number of algorithm iterations. 3 is a good start. cgal default is 1.
 */
void SurfaceMesh::remeshPMPUniform(double edgeLength, int iterations)
{
  pmp::SurfaceMesh pmpMesh = msh::srf::convert(stow->mesh);
  pmp::SurfaceRemeshing remesh(pmpMesh);
  remesh.uniform_remeshing(edgeLength, iterations);
  stow->mesh = msh::srf::convert(pmpMesh);
}

/*! @brief Remove holes from mesh using CGAL
 */
void SurfaceMesh::fillHolesCGAL()
{
  msh::srf::fillHoles(*stow);
}

/*! @brief Remove holes from mesh using pmp-lib
 */
void SurfaceMesh::fillHolesPMP()
{
  pmp::SurfaceMesh pmpMesh = msh::srf::convert(stow->mesh);
  msh::srf::fillHoles(pmpMesh);
  stow->mesh = msh::srf::convert(pmpMesh);
}

prj::srl::mshs::Surface SurfaceMesh::serialOut()
{
  msh::srf::Mesh m = stow->mesh;
  m.collect_garbage();
  
  auto convertOut = [&](const msh::srf::Vertex &vIn) -> prj::srl::spt::Vec3d
  {
    const msh::srf::Point &p = m.point(vIn);
    return prj::srl::spt::Vec3d(p.x(), p.y(), p.z());
  };
  
  prj::srl::mshs::Surface out;
  for (const msh::srf::Vertex &v : m.vertices())
    out.points().push_back(convertOut(v));
  
  for (const msh::srf::Face &f : m.faces())
  {
    prj::srl::mshs::Face fo;
    for(const msh::srf::Vertex &vd : vertices_around_face(m.halfedge(f), m))
      fo.indexes().push_back(vd);
    out.faces().push_back(fo);
  }
  
  return out;
}

void SurfaceMesh::serialIn(const prj::srl::mshs::Surface &smIn)
{
  msh::srf::Mesh &m = stow->mesh;
  
  for (const auto &pIn : smIn.points())
    m.add_vertex(msh::srf::Point(pIn.x(), pIn.y(), pIn.z()));
  
  for (const auto &fIn : smIn.faces())
  {
    msh::srf::Vertices vertices;
    for (const auto &i : fIn.indexes())
      vertices.push_back(static_cast<msh::srf::Vertex>(i));
    m.add_face(vertices);
  }
}
