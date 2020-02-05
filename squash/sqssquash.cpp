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
#include <boost/next_prior.hpp>

#include <igl/arap.h>
#include <igl/boundary_loop.h>
#include <igl/harmonic.h>
#include <igl/map_vertices_to_circle.h>
#include <igl/massmatrix.h>

#include <CGAL/Polygon_mesh_processing/border.h>

#include "tools/idtools.h"
#include "squash/sqsigl.h"
#include "squash/sqssquash.h"
#include "mesh/mshparameters.h"
#include "mesh/mshmesh.h"
#include "mesh/mshocct.h"
#include "annex/annsurfacemesh.h"
#include "globalutilities.h"

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

using Mesh = msh::srf::Mesh;
using Face = msh::srf::Face;
using Faces = msh::srf::Faces;
using Edge = msh::srf::Edge;
using HalfEdge = msh::srf::HalfEdge;
using Vertex = msh::srf::Vertex;
using HalfEdges = msh::srf::HalfEdges;
using Vertices = msh::srf::Vertices;
using BSphere = msh::srf::BSphere;
using Point = msh::srf::Point;
using Kernel = msh::srf::Kernel;

Parameters::~Parameters(){}

static double getBSphereRadius(const Mesh &mIn, const HalfEdges &hesIn)
{
  BSphere bs; //bounding sphere.
  for (const auto &he : hesIn)
    bs.insert(mIn.point(mIn.source(he)));
  return bs.radius();
}

static std::vector<HalfEdges> getBoundaries(const msh::srf::Mesh &mIn)
{
  HalfEdges abhes; //all border half edges
  for (const auto &he : mIn.halfedges())
  {
    if (mIn.is_border(he))
      abhes.push_back(he);
  }
  gu::uniquefy(abhes);
  
  std::vector<HalfEdges> bheso; //border half edges out
  while(!abhes.empty())
  {
    HalfEdges cb; //current border.
    cb.push_back(abhes.back());
    abhes.pop_back();
    for (auto it = abhes.begin(); it != abhes.end();)
    {
      if (mIn.target(cb.back()) == mIn.source(*it))
      {
        cb.push_back(*it);
        abhes.erase(it);
        it = abhes.begin();
      }
      else
       ++it;
      
      if (mIn.source(cb.front()) == mIn.target(cb.back()))
      {
        break;
      }
    }
    bheso.push_back(cb);
  }
  
  std::vector<HalfEdges>::iterator lrit = bheso.end(); //largest radius iterator
  double largestRadius = -1.0;
  for (auto it = bheso.begin(); it != bheso.end(); ++it)
  {
    double cr = getBSphereRadius(mIn, *it);
    if (cr > largestRadius)
    {
      largestRadius = cr;
      lrit = it;
    }
  }
  if (lrit == bheso.end() || largestRadius <= 0.0)
    throw std::runtime_error("no largest boundary");
  
  HalfEdges temp = *lrit;
  bheso.erase(lrit);
  bheso.push_back(temp);
  
  return bheso;
}

static double getFaceArea(const msh::srf::Mesh &mIn, const Face &fIn)
{
  std::vector<Point> ps;
  for (const auto &he : CGAL::halfedges_around_face(mIn.halfedge(fIn), mIn))
    ps.push_back(mIn.point(mIn.source(he)));
  assert(ps.size() == 3);
  return Kernel::Compute_area_3()(ps[0], ps[1], ps[2]);
}

static double getMeshFaceArea(const msh::srf::Mesh &mIn)
{
  double out = 0.0;
  for (const auto &f : mIn.faces())
    out += getFaceArea(mIn, f);
  
  return out;
}

static double getMeshEdgeLength(const msh::srf::Mesh &mIn)
{
  auto getEdgeLength = [&](const Edge &eIn) -> double
  {
    std::vector<msh::srf::Point> ps;
    ps.push_back(mIn.point(mIn.vertex(eIn, 0)));
    ps.push_back(mIn.point(mIn.vertex(eIn, 1)));
    
    return std::sqrt(Kernel::Compute_squared_distance_3()(ps[0], ps[1]));
  };
  
  double out = 0.0;
  for (const auto &e : mIn.edges())
    out += getEdgeLength(e);
  
  return out;
}

double inferEdgeLength(const msh::srf::Mesh &mIn, int granularity = 0, int tCount = 0, double clength = 0.0)
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
    double a = getMeshFaceArea(mIn);
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
  
  return clength;
}

Vertices getBaseVertices(const msh::srf::Mesh &mIn, const occt::FaceVector &fvIn)
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
  ps.mesh3d = std::make_shared<ann::SurfaceMesh>();
  
  msh::prm::OCCT occtSettings;
  *ps.mesh3d = msh::srf::generate(ps.s, occtSettings).mesh;
  ps.message = "Last operation: occt mesh\n";
  
  ps.mesh3d->remeshCGAL(inferEdgeLength(ps.mesh3d->getStow().mesh, ps.granularity), 3);
  ps.message = "Last operation: isotropic remesh\n";
  ps.mesh3d->fillHolesPMP();
  ps.message = "Last operation: fill holes\n";
  
  //we alter the mesh, so make a copy.
  msh::srf::Mesh workMesh = ps.mesh3d->getStow().mesh;
  
  msh::srf::Vertices baseVertices = getBaseVertices(workMesh, ps.fs);
  
  /* going to scale and translate mesh so it fits in parametric
   * range (0,1). arap solver appears to need it.
   */
  osg::Matrixd bmpt;
  {
    msh::srf::BSphere tbmbs; //temp base mesh bounding sphere.
    for (const auto &v : workMesh.vertices())
      tbmbs.insert(workMesh.point(v));
//     sqs::getBSphere(workMesh, tbmbs);
    osg::BoundingSphered bmbs = convertBoundSphere(tbmbs); //base mesh bounding sphere.
    osg::Vec3d ap = bmbs.center(); //anchor point.
    ap.z() = 0.0;
    osg::Matrixd bmt = osg::Matrixd::translate(-ap);
    double bmsv = 1 / (2 * osg::PI * bmbs.radius());
    osg::Matrixd bms = osg::Matrixd::scale(bmsv, bmsv, bmsv);
    bmpt = bmt * bms; //base mesh parametric transformation
  }

  //transform base mesh.
  for (const auto &v : CGAL::vertices(workMesh))
  {
    osg::Vec3d op = convertPoint(workMesh.point(v));
    op = op * bmpt;
    workMesh.point(v) = convertPoint(op);
  }
  
  Eigen::MatrixXd iglVs;
  Eigen::MatrixXi iglFs;
  std::tie(iglVs, iglFs) = sqs::toIgl(workMesh);
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
    bc(brow, 0) = workMesh.point(v).x();
    bc(brow, 1) = workMesh.point(v).y();
    brow++;
  }

  // Initialize ARAP
  arap_data.max_iter = 1600;
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
  
  double baseMeshArea = getMeshFaceArea(ps.mesh3d->getStow().mesh);
  double baseMeshEdgeLength = getMeshEdgeLength(ps.mesh3d->getStow().mesh);
  double faceError = std::fabs(baseMeshArea - getMeshFaceArea(uvMesh)) / baseMeshArea;
  double edgeError = std::fabs(baseMeshEdgeLength - getMeshEdgeLength(uvMesh)) / baseMeshEdgeLength;
  std::ostringstream stream;
  stream
  << "squash results: " << std::endl
  << "face error: " << faceError << std::endl
  << "edge error: " << edgeError << std::endl;
  ps.message = stream.str();
  
  std::vector<HalfEdges> fbs = getBoundaries(uvMesh);
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

