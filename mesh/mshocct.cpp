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

#include <cassert>
#include <cstddef> //null error from nglib

#include <boost/filesystem/operations.hpp>

#include <CGAL/Polygon_mesh_processing/repair.h>
#include <CGAL/Polygon_mesh_processing/repair_polygon_soup.h>
#include <CGAL/Polygon_mesh_processing/orient_polygon_soup.h>
#include <CGAL/Polygon_mesh_processing/polygon_soup_to_polygon_mesh.h>

#include <BRepBuilderAPI_Copy.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <BRepTools.hxx>
#include <TopoDS.hxx>
#include <Poly_Triangulation.hxx>

#include "tools/occtools.h"
#include "mesh/mshmesh.h"
#include "mesh/mshparameters.h"
#include "mesh/mshocct.h"

#ifdef NETGEN_PRESENT
#define OCCGEOMETRY
namespace nglib //what the fuck is this nonsense!
{
  //had to 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib/x86_64-linux-gnu/netgen'
  #include <netgen/nglib.h>
}
using namespace nglib;
#endif

#ifdef GMSH_PRESENT
  #include <gmsh.h>
#endif

static msh::srf::Stow generate(const TopoDS_Shape &shapeIn, const msh::prm::OCCT &prmsIn)
{
  assert(!shapeIn.IsNull());
  msh::srf::Stow out;
  if (shapeIn.IsNull())
    return out;
  
  //copy shape so we leave mesh intact in the original shape.
  //don't copy geometry or mesh.
  BRepBuilderAPI_Copy copier(shapeIn, Standard_False, Standard_False);
  TopoDS_Shape copy(copier.Shape());
  BRepMesh_IncrementalMesh(copy, prmsIn);
  
  std::vector<msh::srf::Point> points;
  std::vector<std::vector<std::size_t>> polygons;
  
  for (const auto &face : occt::mapShapes(copy))
  {
    if (face.ShapeType() != TopAbs_FACE)
      continue;

    //meshes are defined at geometry location and need to be transformed
    //into topological position
    TopLoc_Location location;
    const Handle(Poly_Triangulation) &triangulation = BRep_Tool::Triangulation(TopoDS::Face(face), location);
    if (triangulation.IsNull())
    {
      std::cout << "warning null triangulation in face. " << BOOST_CURRENT_FUNCTION << std::endl;
      continue;
    }
    gp_Trsf transformation = location.Transformation();
    
    std::size_t offset = points.size();
    const TColgp_Array1OfPnt& nodes = triangulation->Nodes();
    for (int index(nodes.Lower()); index <= nodes.Upper(); ++index)
    {
      gp_Pnt point = nodes.Value(index);
      point.Transform(transformation);
      points.emplace_back(msh::srf::Point(point.X(), point.Y(), point.Z()));
    }
    
    const Poly_Array1OfTriangle& triangles = triangulation->Triangles();
    for (int index(triangles.Lower()); index < triangles.Upper() + 1; ++index)
    {
      int N1, N2, N3;
      triangles(index).Get(N1, N2, N3);
      std::vector<std::size_t> polygon;
      if (face.Orientation() == TopAbs_FORWARD)
      {
        polygon.push_back(N1 - 1 + offset);
        polygon.push_back(N2 - 1 + offset);
        polygon.push_back(N3 - 1 + offset);
      }
      else
      {
        polygon.push_back(N1 - 1 + offset);
        polygon.push_back(N3 - 1 + offset);
        polygon.push_back(N2 - 1 + offset);
      }
      polygons.push_back(polygon);
    }
  }
  
  CGAL::Polygon_mesh_processing::repair_polygon_soup(points, polygons);
  CGAL::Polygon_mesh_processing::orient_polygon_soup(points, polygons);
  if (!CGAL::Polygon_mesh_processing::is_polygon_soup_a_polygon_mesh(polygons))
  {
    std::cout << "INVALID polygon soup" << std::endl;
    return out;
  }
  CGAL::Polygon_mesh_processing::polygon_soup_to_polygon_mesh(points, polygons, out.mesh);
  
  CGAL::Polygon_mesh_processing::remove_isolated_vertices(out.mesh);
  out.mesh.collect_garbage();
  return out;
}

/*! @brief Generate a mesh from a face with parameters.
 * 
 * @param faceIn The source face for a surface mesh. Expects not null
 * @param prmsIn The parameters controlling surface mesh generation.
 * 
 */
msh::srf::Stow msh::srf::generate(const TopoDS_Face &faceIn, const msh::prm::OCCT &prmsIn)
{
  return generate(dynamic_cast<const TopoDS_Shape&>(faceIn), prmsIn);
}

/*! @brief Generate a mesh from a shell with parameters.
 * 
 * @param shellIn The source shell for a surface mesh. Expects not null
 * @param prmsIn The parameters controlling surface mesh generation.
 * 
 */
msh::srf::Stow msh::srf::generate(const TopoDS_Shell &shellIn, const msh::prm::OCCT &prmsIn)
{
  return generate(dynamic_cast<const TopoDS_Shape&>(shellIn), prmsIn);
}


//****************************************** Begin netgen ***********************************************
#ifdef NETGEN_PRESENT
typedef std::unique_ptr<Ng_OCC_Geometry, std::function<Ng_Result(Ng_OCC_Geometry*)> > OccGeomPtr;
typedef std::unique_ptr<Ng_Mesh, std::function<void(Ng_Mesh *)> > NgMeshPtr;

struct NgManager
{
  NgManager(){Ng_Init();}
  ~NgManager(){Ng_Exit();}
};
typedef std::unique_ptr<NgManager> NgManagerPtr;
#endif

static msh::srf::Stow generate(const TopoDS_Shape &shapeIn, const msh::prm::Netgen &prmsIn)
{
  assert(!shapeIn.IsNull());
  msh::srf::Stow out;
  if (shapeIn.IsNull())
    return out;
  
#ifdef NETGEN_PRESENT
  //netgen is structured around a file, so write out a temp brep so netgen can read it.
  assert(boost::filesystem::exists(prmsIn.filePath.parent_path()));
  BRepTools::Write(shapeIn, prmsIn.filePath.string().c_str());
  
  NgManagerPtr manager(new NgManager());
  OccGeomPtr gPtr(Ng_OCC_Load_BREP(prmsIn.filePath.string().c_str()), std::bind(&Ng_OCC_DeleteGeometry, std::placeholders::_1));
  if (!gPtr)
    return out; //return empty mesh
  
  NgMeshPtr ngMeshPtr(Ng_NewMesh(), std::bind(&Ng_DeleteMesh, std::placeholders::_1));
  assert(ngMeshPtr);
  Ng_Meshing_Parameters mp = prmsIn.convert();
  try
  {
    Ng_Result r;
    r = Ng_OCC_SetLocalMeshSize(gPtr.get(), ngMeshPtr.get(), &mp);
    if (r != NG_OK)
      std::cout << "error in Ng_OCC_SetLocalMeshSize. code: " << r << ". " << BOOST_CURRENT_FUNCTION << std::endl;
    r = Ng_OCC_GenerateEdgeMesh(gPtr.get(), ngMeshPtr.get(), &mp);
    if (r != NG_OK)
      std::cout << "error in Ng_OCC_GenerateEdgeMesh. code: " << r << ". " << BOOST_CURRENT_FUNCTION << std::endl;
    r = Ng_OCC_GenerateSurfaceMesh(gPtr.get(), ngMeshPtr.get(), &mp);
    if (r != NG_OK)
      std::cout << "error in Ng_OCC_GenerateSurfaceMesh. code: " << r << ". " << BOOST_CURRENT_FUNCTION << std::endl;
    if (prmsIn.optSurfMeshEnable)
      Ng_OCC_Uniform_Refinement (gPtr.get(), ngMeshPtr.get());
  }
  catch (...)
  {
    //netgen is throwing an exception about faces not meshed.
    //I would like to warn the user, but try and use what has been meshed.
    std::cout << "Unknown exception in: " << BOOST_CURRENT_FUNCTION << std::endl;
  }
  
  using Vertex = msh::srf::Mesh::Vertex_index;
  using Point = msh::srf::Point;
  auto &m = out.mesh;
  
  //apparently netgen is using 1 based arrays.
  int pointCount = Ng_GetNP(ngMeshPtr.get());
  for (int i = 0; i < pointCount; ++i)
  {
    double coord[3];
    Ng_GetPoint(ngMeshPtr.get(), i + 1, coord);
    m.add_vertex(Point(coord[0], coord[1], coord[2]));
  }
  
  int faceCount = Ng_GetNSE(ngMeshPtr.get());
  for (int i = 0; i < faceCount; ++i)
  {
    Ng_Surface_Element_Type type;
    int pIndex[8];
    type = Ng_GetSurfaceElement (ngMeshPtr.get(), i + 1, pIndex);
    assert(type == NG_TRIG);
    if (type != NG_TRIG)
      throw std::runtime_error("wrong element type in netgen read");
    
    m.add_face(static_cast<Vertex>(pIndex[0] - 1), static_cast<Vertex>(pIndex[1] - 1), static_cast<Vertex>(pIndex[2] - 1));
  }
#endif
  
  return out;
}

/*! @brief Generate a mesh from a face with parameters.
 * 
 * @param faceIn The source face for a surface mesh. Expects not null
 * @param prmsIn The parameters controlling surface mesh generation.
 * 
 */
msh::srf::Stow msh::srf::generate(const TopoDS_Face &faceIn, const msh::prm::Netgen &prmsIn)
{
  return generate(dynamic_cast<const TopoDS_Shape&>(faceIn), prmsIn);
}

/*! @brief Generate a mesh from a shell with parameters.
 * 
 * @param shellIn The source shell for a surface mesh. Expects not null
 * @param prmsIn The parameters controlling surface mesh generation. Expects fileName to be set.
 * 
 */
msh::srf::Stow msh::srf::generate(const TopoDS_Shell &shellIn, const msh::prm::Netgen &prmsIn)
{
  return generate(dynamic_cast<const TopoDS_Shape&>(shellIn), prmsIn);
}

//**************************************** begin gmsh *************************************************

#ifdef GMSH_PRESENT
struct GmshManager
{
  GmshManager(){gmsh::initialize();}
  ~GmshManager(){gmsh::finalize();}
};
#endif

static msh::srf::Stow generate(const TopoDS_Shape &shapeIn, const msh::prm::GMSH &prmsIn)
{
  assert(!shapeIn.IsNull());
  msh::srf::Stow out;
  if (shapeIn.IsNull())
    return out;
  
#ifdef GMSH_PRESENT
  using Vertex = msh::srf::Mesh::Vertex_index;
  using Point = msh::srf::Point;
  auto &m = out.mesh;
  
  //gmsh is structured around a file, so write out a temp brep so gmsh can read it.
  assert(boost::filesystem::exists(prmsIn.filePath.parent_path()));
  BRepTools::Write(shapeIn, prmsIn.filePath.string().c_str());
  
  GmshManager manager;
//   gmsh::option::setNumber("General.Terminal", 1); //good for log info on terminal
  gmsh::open(prmsIn.filePath.string().c_str());
  for (const auto &option : prmsIn.options)
  {
    if (option.isDefault())
      continue;
    gmsh::option::setNumber(option.getKey(), option.getValue());
    std::cout << "setting option: " << option.getKey() << " to: " << option.getValue() << std::endl;
  }
  gmsh::model::mesh::generate(2);
  if (prmsIn.refine)
    gmsh::model::mesh::refine();

  std::vector<int> nodes;
  std::vector<double> coords;
  std::vector<double> pCoords; //not used at this time.
  gmsh::model::mesh::getNodes(nodes, coords, pCoords);
  
  if (nodes.empty() || coords.size() < 3)
    return out;
  
  //gmsh tags are 1 based.
  assert(!(coords.size() % 3));
  for (std::size_t i = 0; i < coords.size(); i += 3)
    m.add_vertex(Point(coords[i], coords[i+1], coords[i+2]));

  std::vector<int> elementTypes;
  std::vector<std::vector<int>> elementTags;
  std::vector<std::vector<int>> nodeTags;
  gmsh::model::mesh::getElements(elementTypes, elementTags, nodeTags);
  for (std::size_t i = 0; i < elementTypes.size(); ++i)
  {
    if (elementTypes[i] == 2) //triangles
    {
      assert(!(nodeTags[i].size() % 3));
      for (std::size_t ic = 0; ic < nodeTags[i].size(); ic += 3)
      {
        m.add_face
        (
          static_cast<Vertex>(nodeTags[i][ic] - 1)
          , static_cast<Vertex>(nodeTags[i][ic+1] - 1)
          , static_cast<Vertex>(nodeTags[i][ic+2] - 1)
        );
      }
    }
  }
#endif //GMSH_PRESENT
  
  return out;
}

msh::srf::Stow msh::srf::generate(const TopoDS_Face &faceIn, const msh::prm::GMSH &prmsIn)
{
  return generate(dynamic_cast<const TopoDS_Shape&>(faceIn), prmsIn);
}

msh::srf::Stow msh::srf::generate(const TopoDS_Shell &faceIn, const msh::prm::GMSH &prmsIn)
{
  return generate(dynamic_cast<const TopoDS_Shape&>(faceIn), prmsIn);
}
