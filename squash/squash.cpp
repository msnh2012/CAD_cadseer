/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#include <boost/filesystem.hpp>

#include <igl/arap.h>
#include <igl/boundary_loop.h>
#include <igl/harmonic.h>
#include <igl/map_vertices_to_circle.h>
#include <igl/massmatrix.h>

#include <tools/idtools.h>
#include <squash/mesh.h>
#include <CGAL/Polygon_mesh_processing/triangulate_hole.h>
#include <CGAL/Polygon_mesh_processing/remesh.h>
#include <CGAL/Polygon_mesh_processing/border.h>
#include <CGAL/boost/graph/Euler_operations.h>
#include <squash/tools.h>
#include <squash/support.h>
#include <squash/igl.h>
#include <squash/squash.h>

#include "mesh/parameters.h"
#include "mesh/mesh.h"
#include "annex/surfacemesh.h"

#include <BRepTools.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepExtrema_DistShapeShape.hxx>

using namespace boost::filesystem;

using namespace sqs;

Parameters::~Parameters(){}

std::string writeShellOut(const TopoDS_Shell& sIn)
{
  path sPath = temp_directory_path() / (gu::idToString(gu::createRandomId()) + ".brep");
  BRepTools::Write(sIn, sPath.string().c_str());
  return sPath.string();
}

std::pair<int, int> fillHoles(msh::srf::Mesh &mIn)
{
  //try to fill holes.
  std::vector<HalfEdges> boundaries = sqs::getBoundaries(mIn);
  boundaries.pop_back(); //get boundaries puts the largest boundary at the end. so just remove it.
  int holesFilled = 0;
  int holesFailed = 0;
  for (const auto &b : boundaries)
  {
    Faces nf; //new faces
    Vertices nv; //new vertices
    bool success = CGAL::cpp11::get<0>
    (
      CGAL::Polygon_mesh_processing::triangulate_refine_and_fair_hole
      (
        mIn,
        b.front(),
        std::back_inserter(nf),
        std::back_inserter(nv)
      )
    );
    if (success)
      holesFilled++;
    else
      holesFailed++;
  }
  
  return std::make_pair(holesFilled, holesFailed);
}

void remesh(msh::srf::Mesh &mIn, int granularity = 0, int tCount = 0, double clength = 0.0)
{
  //remeshing.
  //get collapse length.
  auto rangeCLength = [](double vIn) -> double
  {
    vIn = std::max(vIn, 0.01);
    vIn = std::min(vIn, 100.0);
    return vIn;
  };
  
  auto calculateCLength = [&](int tc) -> double
  {
    double a = sqs::getMeshFaceArea(mIn);
    double ofa = a / static_cast<double>(tc); //one face area
    double l = std::sqrt(2 * ofa);
    l = rangeCLength(l);
    return l;
  };
  
  if (clength > 0.0)
    clength = rangeCLength(clength);
  else if (tCount > 0)
    clength = calculateCLength(tCount);
  else if (granularity >= 1 && granularity <= 5)
  {
    static const std::vector<int> vs = 
    {
      1000,
      3000,
      5000,
      7000,
      9000
    };
    clength = calculateCLength(vs.at(granularity - 1));
  }
  else
    clength = calculateCLength(5000);
  
  namespace pmp = CGAL::Polygon_mesh_processing;
  pmp::isotropic_remeshing
  (
    CGAL::faces(mIn),
    clength, //target edge length
    mIn,
    pmp::parameters::number_of_iterations(3)
    .protect_constraints(false)//false = don't protect border
  );

  Faces nf;
  Vertices nv;
  pmp::refine
  (
    mIn,
    sqs::obtuse(mIn, 170),
    std::back_inserter(nf),
    std::back_inserter(nv),
    pmp::parameters::density_control_factor(2.0)
  );
}

Vertices getBaseVertices(const Mesh &mIn, const occt::FaceVector &fvIn)
{
  //we are not checking the connection of the faces in.
  Bnd_Box bb;
  for (const auto &f : fvIn)
    BRepBndLib::Add(f, bb);
  Vertices out;
  for (const auto &v : CGAL::vertices(mIn))
  {
    gp_Pnt occp(mIn.point(v).x(), mIn.point(v).y(), mIn.point(v).z());
    if (bb.IsOut(occp))
      continue;
    TopoDS_Vertex vx = BRepBuilderAPI_MakeVertex(occp);
    for (const auto &f : fvIn)
    {
      BRepExtrema_DistShapeShape dss(vx, f);
      if (!dss.IsDone())
        continue;
      if (dss.Value() < 0.0001)
      {
        out.push_back(v);
        break;
      }
    }
  }
  
  return out;
}

static osg::BoundingSphered convertBoundSphere(msh::srf::BSphere &sIn) //sphere can't be const?
{
  const auto it = sIn.center_cartesian_begin();
  msh::srf::Point t(*it, *(it + 1), *(it + 2));
  osg::Vec3d center(t.x(), t.y(), t.z());
  double radius = sIn.radius();
  
  return osg::BoundingSphered(center, radius);
}

static osg::Vec3d convertPoint(const msh::srf::Point &pIn)
{
  return osg::Vec3d(pIn.x(), pIn.y(), pIn.z());
}

static msh::srf::Point convertPoint(const osg::Vec3d &pIn)
{
  return msh::srf::Point(pIn.x(), pIn.y(), pIn.z());
}

void sqs::squash(sqs::Parameters &ps)
{
  //Just going to use a fine setting for occt.
  msh::prm::OCCT occtSettings;
  occtSettings.linearDeflection = 0.05;
  occtSettings.angularDeflection = 0.1;
  auto surfaceMeshPtr = ann::SurfaceMesh::generate(ps.s, occtSettings);
  msh::srf::Mesh baseMesh = surfaceMeshPtr->getStow().mesh;
  
//   fillHoles(baseMesh);
//   remesh(baseMesh, ps.granularity);
  
  baseMesh.collect_garbage();
  msh::srf::Mesh baseMeshCopy = baseMesh; //we alter the base mesh, so make a copy for use later.
  
  ps.mesh3d = std::make_shared<ann::SurfaceMesh>(msh::srf::Stow(baseMesh));
  
  msh::srf::Vertices baseVertices = getBaseVertices(baseMesh, ps.fs);
  
  /* going to scale and translate mesh so it fits in parametric
   * range (0,1). arap solver appears to need it.
   */
  osg::Matrixd bmpt;
  {
    msh::srf::BSphere tbmbs; //temp base mesh bounding sphere.
    for (const auto &v : baseMesh.vertices())
      tbmbs.insert(baseMesh.point(v));
//     sqs::getBSphere(baseMesh, tbmbs);
    osg::BoundingSphered bmbs = convertBoundSphere(tbmbs); //base mesh bounding sphere.
    osg::Vec3d ap = bmbs.center(); //anchor point.
    ap.z() = 0.0;
    osg::Matrixd bmt = osg::Matrixd::translate(-ap);
    double bmsv = 1 / (2 * osg::PI * bmbs.radius());
    osg::Matrixd bms = osg::Matrixd::scale(bmsv, bmsv, bmsv);
    bmpt = bmt * bms; //base mesh parametric transformation
  }

  //transform base mesh.
  for (const auto &v : CGAL::vertices(baseMesh))
  {
    osg::Vec3d op = convertPoint(baseMesh.point(v));
    op = op * bmpt;
    baseMesh.point(v) = convertPoint(op);
  }
  
  Eigen::MatrixXd iglVs;
  Eigen::MatrixXi iglFs;
  std::tie(iglVs, iglFs) = sqs::toIgl(baseMesh);
  Eigen::SparseMatrix<double> massIgl;
  igl::massmatrix(iglVs,iglFs,igl::MASSMATRIX_TYPE_DEFAULT,massIgl);
  
  // Compute the initial solution for ARAP (harmonic parametrization)
  Eigen::VectorXi bnd;
  igl::boundary_loop(iglFs,bnd);
  Eigen::MatrixXd bnd_uv;
  igl::map_vertices_to_circle(iglVs,bnd,bnd_uv);
  
  //rotation is coming from igl::map_vertices_to_circle
  
  Eigen::MatrixXd initial_guess;
  igl::harmonic(iglVs,iglFs,bnd,bnd_uv,1,initial_guess);
  
  // Add dynamic regularization to avoid to specify boundary conditions
  igl::ARAPData arap_data;
  Eigen::VectorXi b  = Eigen::VectorXi::Zero(0);
  Eigen::MatrixXd bc = Eigen::MatrixXd::Zero(0,0);
  
  b.resize(baseVertices.size());
  bc.resize(baseVertices.size(), 2);
  int brow = 0;
  for (const auto &v : baseVertices)
  {
    b(brow) = static_cast<int>(v);
    bc(brow, 0) = baseMesh.point(v).x();
    bc(brow, 1) = baseMesh.point(v).y();
    brow++;
  }

  // Initialize ARAP
  arap_data.max_iter = 400;
  arap_data.with_dynamics = true;
  arap_data.M = massIgl;
  arap_data.ym = 203395.33976;
  // 2 means that we're going to *solve* in 2d
  if (!arap_precomputation(iglVs, iglFs, 2, b, arap_data))
    throw std::runtime_error("arap_precomputation failed");

  // Solve arap using the harmonic map as initial guess
  Eigen::MatrixXd V_uv;
  V_uv = initial_guess;

  if (!arap_solve(bc,arap_data,V_uv))
    throw std::runtime_error("arap_solve failed");
  
  msh::srf::Mesh uvMesh = sqs::toCgal(V_uv, iglFs);
  for (const auto &v : CGAL::vertices(uvMesh))
    uvMesh.point(v) = convertPoint(convertPoint(uvMesh.point(v)) * osg::Matrixd::inverse(bmpt));
  
  ps.mesh2d = std::make_shared<ann::SurfaceMesh>(msh::srf::Stow(uvMesh));
  
  double baseMeshArea = sqs::getMeshFaceArea(baseMeshCopy);
  double baseMeshEdgeLength = sqs::getMeshEdgeLength(baseMeshCopy);
  double faceError = std::fabs(baseMeshArea - sqs::getMeshFaceArea(uvMesh)) / baseMeshArea;
  double edgeError = std::fabs(baseMeshEdgeLength - sqs::getMeshEdgeLength(uvMesh)) / baseMeshEdgeLength;
  std::ostringstream stream;
  stream
  << "squash results: " << std::endl
  << "face error: " << faceError << std::endl
  << "edge error: " << edgeError << std::endl;
  ps.message = stream.str();
  
  std::vector<HalfEdges> fbs = sqs::getBoundaries(uvMesh);
  assert(fbs.size() == 1);
  
  TopoDS_Vertex fv; //first vertex.
  TopoDS_Vertex cv; //current vertex.
  BRepBuilderAPI_MakeWire wm; //wire maker
  TopoDS_Face out; 
  for (const auto &he : fbs.back())
  {
    Vertex tvx = uvMesh.source(he);
    gp_Pnt tp(uvMesh.point(tvx).x(), uvMesh.point(tvx).y(), uvMesh.point(tvx).z());
    TopoDS_Vertex tv = BRepBuilderAPI_MakeVertex(tp);
    if (cv.IsNull()) //first vertex
    {
      cv = tv;
      fv = tv;
      continue;
    }
    BRepBuilderAPI_MakeEdge em(cv, tv); //edge maker.
    if (!em.IsDone())
      continue;
    ps.es.push_back(em);
    wm.Add(em);
    if (!wm.IsDone())
      continue;
    
    cv = tv;
  }
  ps.es.push_back(BRepBuilderAPI_MakeEdge(cv, fv));
  wm.Add(ps.es.back());
  if (!wm.IsDone())
    return;
    
  TopoDS_Wire w = wm;
  w.Reverse(); //half edges from mesh are on the outside going opposite orientation.
  
  BRepBuilderAPI_MakeFace fm(w);
  if (!fm.IsDone())
    return;
  
  ps.ff = fm;
}

