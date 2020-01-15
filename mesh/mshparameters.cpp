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

#include <cstddef> //null error from nglib

#include <boost/bimap.hpp>
#include <boost/assign/list_of.hpp>

#ifdef NETGEN_PRESENT
#define OCCGEOMETRY
namespace nglib //what the fuck is this nonsense!
{
#include <nglib.h>
}
using namespace nglib;
#endif


#include "project/serial/xsdcxxoutput/mesh.h"
#include "mesh/mshparameters.h"

using namespace msh::prm;

/*
    IMeshTools_Parameters defaults:
    Angle(0.5),
    Deflection(0.001),
    AngleInterior(-1.0),
    DeflectionInterior(-1.0),
    MinSize (-1.0),
    InParallel (Standard_False),
    Relative (Standard_False),
    InternalVerticesMode (Standard_True),
    ControlSurfaceDeflection (Standard_True),
    CleanModel (Standard_True),
    AdjustMinSize (Standard_False)
*/
OCCT::OCCT()
: IMeshTools_Parameters()
{
  //changing some defaults.
  Deflection = 0.25;
  InParallel = Standard_True;
}

void OCCT::serialIn(const prj::srl::msh::ParametersOCCT &psIn)
{
  Angle = psIn.angle();
  Deflection = psIn.deflection();
  AngleInterior = psIn.angleInterior();
  DeflectionInterior = psIn.deflectionInterior();
  MinSize = psIn.minSize();
  InParallel = psIn.inParallel();
  Relative = psIn.relative();
  InternalVerticesMode = psIn.internalVerticesMode();
  ControlSurfaceDeflection = psIn.controlSurfaceDeflection();
  CleanModel = psIn.cleanModel();
  AdjustMinSize = psIn.adjustMinSize();
}

prj::srl::msh::ParametersOCCT OCCT::serialOut() const
{
  return prj::srl::msh::ParametersOCCT
  (
    Angle
    , Deflection
    , AngleInterior
    , DeflectionInterior
    , MinSize
    , InParallel
    , Relative
    , InternalVerticesMode
    , ControlSurfaceDeflection
    , CleanModel
    , AdjustMinSize
  );
}

#ifdef NETGEN_PRESENT
nglib::Ng_Meshing_Parameters Netgen::convert() const
{
  Ng_Meshing_Parameters out;
  out.uselocalh = static_cast<int>(useLocalH);
  out.maxh = maxH;
  out.minh = minH;
  out.fineness = fineness;
  out.grading = grading;
  out.elementsperedge = elementsPerEdge;
  out.elementspercurve = elementsPerCurve;
  out.closeedgeenable = static_cast<int>(closeEdgeEnable);
  out.closeedgefact = closeEdgeFactor;
  out.minedgelenenable = static_cast<int>(minEdgeLenEnable);
  out.minedgelen = minEdgeLen;
  out.second_order = static_cast<int>(secondOrder);
  out.quad_dominated = static_cast<int>(quadDominated);
  out.optsurfmeshenable = static_cast<int>(optSurfMeshEnable);
  out.optvolmeshenable = static_cast<int>(optVolMeshEnable);
  out.optsteps_2d = optSteps2d;
  out.optsteps_3d = optSteps3d;
  out.invert_tets = static_cast<int>(invertTets);
  out.invert_trigs = static_cast<int>(invertTrigs);
  out.check_overlap = static_cast<int>(checkOverlap);
  out.check_overlapping_boundary = static_cast<int>(checkOverlappingBoundary);
  
  return out;
}
#endif

void Netgen::resetValues()
{
  *this = Netgen();
}

void Netgen::ensureValidValues()
{
  //this isn't complete. just fixing as I am getting problems.
  minH = std::max(0.0, minH);
  maxH = std::max(minH, maxH);
  fineness = std::min(std::max(0.0, fineness), 1.0);
  grading = std::min(std::max(0.1, grading), 1.0);
  elementsPerEdge = std::min(std::max(0.2, elementsPerEdge), 5.0);
  elementsPerCurve = std::min(std::max(0.2, elementsPerCurve), 5.0);
  closeEdgeFactor = std::min(std::max(0.2, closeEdgeFactor), 8.0);
  minEdgeLen = std::min(std::max(0.2, minEdgeLen), 5.0);
  optSteps2d = std::max(0, optSteps2d);
  optSteps3d = std::max(0, optSteps3d);
}

void Netgen::serialIn(const prj::srl::msh::ParametersNetgen &psIn)
{
    useLocalH = psIn.useLocalH();
    maxH = psIn.maxH();
    minH = psIn.minH();
    fineness = psIn.fineness();
    grading = psIn.grading();
    elementsPerEdge = psIn.elementsPerEdge();
    elementsPerCurve = psIn.elementsPerCurve();
    closeEdgeEnable = psIn.closeEdgeEnable();
    closeEdgeFactor = psIn.closeEdgeFactor();
    minEdgeLenEnable = psIn.minEdgeLenEnable();
    minEdgeLen = psIn.minEdgeLen();
    secondOrder = psIn.secondOrder();
    quadDominated = psIn.quadDominated();
    optSurfMeshEnable = psIn.optSurfMeshEnable();
    optVolMeshEnable = psIn.optVolMeshEnable();
    optSteps2d = psIn.optSteps2d();
    optSteps3d = psIn.optSteps3d();
    invertTets = psIn.invertTets();
    invertTrigs = psIn.invertTrigs();
    checkOverlap = psIn.checkOverlap();
    checkOverlappingBoundary = psIn.checkOverlappingBoundary();
}

prj::srl::msh::ParametersNetgen Netgen::serialOut() const
{
  return prj::srl::msh::ParametersNetgen
  (
    useLocalH
    , maxH
    , minH
    , fineness
    , grading
    , elementsPerEdge
    , elementsPerCurve
    , closeEdgeEnable
    , closeEdgeFactor
    , minEdgeLenEnable
    , minEdgeLen
    , secondOrder
    , quadDominated
    , optSurfMeshEnable
    , optVolMeshEnable
    , optSteps2d
    , optSteps3d
    , invertTets
    , invertTrigs
    , checkOverlap
    , checkOverlappingBoundary
  );
}

namespace msh
{
  namespace prm
  {
    struct GMSH::Option::Enumerate
    {
      const std::string& toString(double d)
      {
        return map.right.at(d);
      }
      double toDouble(const std::string &s)
      {
        return map.left.at(s);
      }
      typedef boost::bimap<std::string, double> EnumMap;
      EnumMap map;
    };
  }
}

GMSH::Option::Option(const std::string &kIn, const std::string &dIn, double defaultIn)
: key(kIn)
, description(dIn)
, value(defaultIn)
, defaultValue(defaultIn)
{
}

GMSH::Option::Option(const std::string &kIn, const std::string &dIn, double defaultIn, std::unique_ptr<Enumerate> eIn)
: key(kIn)
, description(dIn)
, value(defaultIn)
, defaultValue(defaultIn)
, enumerate(std::move(eIn))
{
}

GMSH::Option::Option(Option&& rhs)
: key(rhs.key)
, description(rhs.description)
, value(rhs.value)
, defaultValue(rhs.defaultValue)
, enumerate(std::forward<std::unique_ptr<Enumerate>>(rhs.enumerate))
{
}

GMSH::Option::~Option(){}

void GMSH::Option::setValue(double vIn)
{
  if (enumerate)
  {
    try
    {
      std::string dummy = enumerate->map.right.at(vIn); // throws std::out_of_range
      value = vIn;
    }
    catch(std::out_of_range &e){}
  }
  else
    value = vIn;
}

void GMSH::Option::setToDefault()
{
  value = defaultValue;
}

bool GMSH::Option::isDefault() const
{
  return value == defaultValue;
}

void GMSH::Option::setValue(const std::string &sIn)
{
  assert(enumerate);
  if (enumerate)
  {
    try
    {
      value = enumerate->map.left.at(sIn);
    }
    catch(std::out_of_range &e){}
  }
  //if we try to set value with a string and there is no enumerate. do nothing.
}

std::vector<std::string> GMSH::Option::getEnumerateStrings() const
{
  std::vector<std::string> out;
  
  auto it = enumerate->map.left.begin();
  auto itEnd = enumerate->map.left.end();
  for (; it != itEnd; ++it)
    out.push_back(it->first);
  
  return out;
}

GMSH::GMSH()
{
  // http://gmsh.info/doc/texinfo/gmsh.html#Mesh-options-list
  {
    std::unique_ptr<Option::Enumerate> enumerate(new Option::Enumerate());
    enumerate->map = boost::assign::list_of<Option::Enumerate::EnumMap::relation>
      ("MeshAdapt", 1)
      ("Automatic", 2)
      ("Delaunay", 5)
      ("Frontal", 6)
      ("BAMG", 7)
      ("DelQuad", 8);
    options.push_back(Option("Mesh.Algorithm", "2D mesh algorithm", 2, std::move(enumerate)));
  }
  {
    std::unique_ptr<Option::Enumerate> enumerate(new Option::Enumerate());
    enumerate->map = boost::assign::list_of<Option::Enumerate::EnumMap::relation>
      ("Delaunay", 1)
      ("Frontal", 4)
      ("Frontal Delaunay", 5)
      ("Frontal Hex", 6)
      ("MMG3D", 7)
      ("R-tree", 9)
      ("HXT", 10);
    options.push_back(Option("Mesh.Algorithm3D", "3D mesh algorithm", 1, std::move(enumerate)));
  }
  options.push_back
  (
    Option
    (
      "Mesh.AngleSmoothNormals"
      , "Threshold angle below which normals are not smoothed"
      , 30
    )
  );
  options.push_back
  (
    Option
    (
      "Mesh.AngleToleranceFacetOverlap"
      , "Consider connected facets as overlapping when the dihedral angle between the facets is smaller than the user’s defined tolerance"
      , 0.1
    )
  );
  options.push_back
  (
    Option
    (
      "Mesh.AnisoMax"
      , "Maximum anisotropy of the mesh"
      , 1e+33
    )
  );
  options.push_back
  (
    Option
    (
      "Mesh.AllowSwapAngle"
      , "Threshold angle (in degrees) between faces normals under which we allow an edge swap"
      , 10.0
    )
  );
  //skip Mesh.BdfFieldFormat
  //skip Mesh.Binary
  options.push_back
  (
    Option
    (
      "Mesh.BoundaryLayerFanPoints"
      , "Number of points (per Pi rad) for 2D boundary layer fans"
      , 5
    )
  );
  //skip Mesh.CgnsImportOrder
  //skip Mesh.CgnsConstructTopology
  {
    std::unique_ptr<Option::Enumerate> enumerate(new Option::Enumerate());
    enumerate->map = boost::assign::list_of<Option::Enumerate::EnumMap::relation>
      ("Longest", 1)
      ("Shortest", 2);
    options.push_back
    (
      Option
      (
        "Mesh.CharacteristicLengthExtendFromBoundary"
        , "Extend computation of mesh element sizes from the boundaries into the interior(3D Delaunay)"
        , 1
        , std::move(enumerate)
      )
    );
  }
  options.push_back
  (
    Option
    (
      "Mesh.CharacteristicLengthFactor"
      , "Factor applied to all mesh element sizes"
      , 1
    )
  );
  options.push_back
  (
    Option
    (
      "Mesh.CharacteristicLengthMin"
      , "Minimum mesh element size"
      , 0
    )
  );
  options.push_back
  (
    Option
    (
      "Mesh.CharacteristicLengthMax"
      , "Maximum mesh element size"
      , 1e+22
    )
  );
  {
    std::unique_ptr<Option::Enumerate> enumerate(new Option::Enumerate());
    enumerate->map = boost::assign::list_of<Option::Enumerate::EnumMap::relation>
      ("Disabled", 0)
      ("Enabled", 1);
    options.push_back
    (
      Option
      (
        "Mesh.CharacteristicLengthFromCurvature"
        , "Automatically compute mesh element sizes from curvature (experimental)"
        , 0
        , std::move(enumerate)
      )
    );
  }
  //skip Mesh.CharacteristicLengthFromPoints
  //skip Mesh.Clip
  //skip Mesh.ColorCarousel
  //skip Mesh.CpuTime
  //skip Mesh.DrawSkinOnly
  //skip Mesh.Dual
  //skip Mesh.ElementOrder
  //skip Mesh.Explode
  //skip Mesh.FlexibleTransfinite
  //skip Mesh.NewtonConvergenceTestXYZ
  //skip Mesh.Format
  //skip Mesh.Hexahedra
  //skip Mesh.HighOrderNumLayers
  //skip Mesh.HighOrderOptimize
  //skip Mesh.HighOrderPeriodic
  //skip Mesh.HighOrderPoissonRatio
  //skip Mesh.HighOrderThresholdMin
  //skip Mesh.HighOrderThresholdMax
  //skip Mesh.HighOrderOptPrimSurfMesh
  //skip Mesh.LabelSampling
  //skip Mesh.LabelType
  options.push_back
  (
    Option
    (
      "Mesh.LcIntegrationPrecision"
      , "Accuracy of evaluation of the LC field for 1D mesh generation"
      , 1e-09
    )
  );
  //skip Mesh.Light
  //skip Mesh.LightLines
  //skip Mesh.LightTwoSide
  //skip Mesh.Lines
  //skip Mesh.LineNumbers
  //skip Mesh.LineWidth
  options.push_back
  (
    Option
    (
      "Mesh.MaxNumThreads1D"
      , "Maximum number of threads for 1D meshing (0: use default)"
      , 0
    )
  );
  options.push_back
  (
    Option
    (
      "Mesh.MaxNumThreads2D"
      , "Maximum number of threads for 2D meshing (0: use default)"
      , 0
    )
  );
  options.push_back
  (
    Option
    (
      "Mesh.MaxNumThreads3D"
      , "Maximum number of threads for 3D meshing (0: use default)"
      , 0
    )
  );
  //skip Mesh.MeshOnlyVisible
  {
    std::unique_ptr<Option::Enumerate> enumerate(new Option::Enumerate());
    enumerate->map = boost::assign::list_of<Option::Enumerate::EnumMap::relation>
      ("Recursive", 1)
      ("K-way", 2);
    options.push_back
    (
      Option
      (
        "Mesh.MetisAlgorithm"
        , "METIS partitioning algorithm"
        , 1
        , std::move(enumerate)
      )
    );
  }
  {
    std::unique_ptr<Option::Enumerate> enumerate(new Option::Enumerate());
    enumerate->map = boost::assign::list_of<Option::Enumerate::EnumMap::relation>
      ("Random", 1)
      ("Sorted Heavy-Edge", 2);
    options.push_back
    (
      Option
      (
        "Mesh.MetisEdgeMatching"
        , "METIS edge matching type"
        , 2
        , std::move(enumerate)
      )
    );
  }
  {
    std::unique_ptr<Option::Enumerate> enumerate(new Option::Enumerate());
    enumerate->map = boost::assign::list_of<Option::Enumerate::EnumMap::relation>
      ("FM-based cut", 1)
      ("Greedy", 2)
      ("Two-sided node FM", 3)
      ("One-sided node FM", 4);
    options.push_back
    (
      Option
      (
        "Mesh.MetisRefinementAlgorithm"
        , "METIS algorithm for k-way refinement"
        , 2
        , std::move(enumerate)
      )
    );
  }
  options.push_back
  (
    Option
    (
      "Mesh.MinimumCirclePoints"
      , "Minimum number of nodes used to mesh a circle (and number of nodes per 2*pi radians when the mesh size of adapted to the curvature)"
      , 7
    )
  );
  options.push_back
  (
    Option
    (
      "Mesh.MinimumCurvePoints"
      , "Minimum number of points used to mesh a (non-straight) curve"
      , 3
    )
  );
  //skip Mesh.MshFileVersion
  //skip Mesh.MedFileMinorVersion
  //skip Mesh.PartitionHexWeight
  //skip Mesh.PartitionLineWeight
  //skip Mesh.PartitionPrismWeight
  //skip Mesh.PartitionPyramidWeight
  //skip Mesh.PartitionQuadWeight
  //skip Mesh.PartitionTrihedronWeight
  //skip Mesh.PartitionTetWeight
  //skip Mesh.PartitionTriWeight
  //skip Mesh.PartitionCreateTopology
  //skip Mesh.PartitionCreatePhysicals
  //skip Mesh.PartitionCreateGhostCells
  //skip Mesh.PartitionSplitMeshFiles
  //skip Mesh.PartitionTopologyFile
  //skip Mesh.PartitionOldStyleMsh2
  //skip Mesh.NbHexahedra
  //skip Mesh.NbNodes
  //skip Mesh.NbPartitions
  //skip Mesh.NbPrisms
  //skip Mesh.NbPyramids
  //skip Mesh.NbTrihedra
  //skip Mesh.NbQuadrangles
  //skip Mesh.NbTetrahedra
  //skip Mesh.NbTriangles
  //skip Mesh.Normals
  //skip Mesh.NumSubEdges
  {
    std::unique_ptr<Option::Enumerate> enumerate(new Option::Enumerate());
    enumerate->map = boost::assign::list_of<Option::Enumerate::EnumMap::relation>
      ("Disabled", 0)
      ("Enabled", 1);
    options.push_back
    (
      Option
      (
        "Mesh.Optimize"
        , "Optimize the mesh to improve the quality of tetrahedral elements"
        , 1
        , std::move(enumerate)
      )
    );
  }
  options.push_back
  (
    Option
    (
      "Mesh.OptimizeThreshold"
      , "Optimize tetrahedra that have a quality below value"
      , 0.3
    )
  );
  {
    std::unique_ptr<Option::Enumerate> enumerate(new Option::Enumerate());
    enumerate->map = boost::assign::list_of<Option::Enumerate::EnumMap::relation>
      ("Disabled", 0)
      ("Enabled", 1);
    options.push_back
    (
      Option
      (
        "Mesh.OptimizeNetgen"
        , "Optimize the mesh using Netgen to improve the quality of tetrahedral elements"
        , 0
        , std::move(enumerate)
      )
    );
  }
  //skip Mesh.Points
  //skip Mesh.PointNumbers
  //skip Mesh.PointSize
  //skip Mesh.PointType
  //skip Mesh.Prisms
  //skip Mesh.Pyramids
  //skip Mesh.Trihedra
  //skip Mesh.Quadrangles
  //skip Mesh.QualityInf
  //skip Mesh.QualitySup
  //skip Mesh.QualityType
  //skip Mesh.RadiusInf
  //skip Mesh.RadiusSup
  options.push_back
  (
    Option
    (
      "Mesh.RandomFactor"
      , "Random factor used in the 2D meshing algorithm (should be increased if RandomFactor * size(triangle)/size(model) approaches machine accuracy)"
      , 1e-09
    )
  );
  options.push_back
  (
    Option
    (
      "Mesh.RandomFactor3D"
      , "Random factor used in the 3D meshing algorithm"
      , 1e-12
    )
  );
  //skip Mesh.PreserveNumberingMsh2
  {
    std::unique_ptr<Option::Enumerate> enumerate(new Option::Enumerate());
    enumerate->map = boost::assign::list_of<Option::Enumerate::EnumMap::relation>
      ("Disabled", 0)
      ("Enabled", 1);
    options.push_back
    (
      Option
      (
        "Mesh.IgnorePeriodicity"
        , "Ignore alignment of periodic boundaries when reading the mesh (used by ParaView plugin)"
        , 0
        , std::move(enumerate)
      )
    );
  }
  {
    std::unique_ptr<Option::Enumerate> enumerate(new Option::Enumerate());
    enumerate->map = boost::assign::list_of<Option::Enumerate::EnumMap::relation>
      ("Simple", 0)
      ("Blossom", 1)
      ("simple full-quad", 2)
      ("blossom full-quad", 3);
    options.push_back
    (
      Option
      (
        "Mesh.RecombinationAlgorithm"
        , "Mesh recombination algorithm"
        , 1
        , std::move(enumerate)
      )
    );
  }
  {
    std::unique_ptr<Option::Enumerate> enumerate(new Option::Enumerate());
    enumerate->map = boost::assign::list_of<Option::Enumerate::EnumMap::relation>
      ("Disabled", 0)
      ("Enabled", 1);
    options.push_back
    (
      Option
      (
        "Mesh.RecombineAll"
        , "Apply recombination algorithm to all surfaces, ignoring per-surface spec"
        , 0
        , std::move(enumerate)
      )
    );
  }
  options.push_back
  (
    Option
    (
      "Mesh.RecombineOptimizeTopology"
      , "Number of topological optimization passes (removal of diamonds, ...) of recombined surface meshes"
      , 5
    )
  );
  {
    std::unique_ptr<Option::Enumerate> enumerate(new Option::Enumerate());
    enumerate->map = boost::assign::list_of<Option::Enumerate::EnumMap::relation>
      ("Disabled", 0)
      ("Enabled", 1);
    options.push_back
    (
      Option
      (
        "Mesh.Recombine3DAll"
        , "Apply recombination3D algorithm to all volumes, ignoring per-volume spec"
        , 0
        , std::move(enumerate)
      )
    );
  }
  {
    std::unique_ptr<Option::Enumerate> enumerate(new Option::Enumerate());
    enumerate->map = boost::assign::list_of<Option::Enumerate::EnumMap::relation>
      ("Hex", 0)
      ("hex+prisms", 1)
      ("hex+prism+pyramids", 2);
    options.push_back
    (
      Option
      (
        "Mesh.Recombine3DLevel"
        , "3d recombination level"
        , 0
        , std::move(enumerate)
      )
    );
  }
  {
    std::unique_ptr<Option::Enumerate> enumerate(new Option::Enumerate());
    enumerate->map = boost::assign::list_of<Option::Enumerate::EnumMap::relation>
      ("nonconforming", 0)
      ("trihedra", 1)
      ("pyramids+trihedra", 2)
      ("pyramids+hexSplit+trihedra", 3)
      ("hexSplit+trihedra", 4);
    options.push_back
    (
      Option
      (
        "Mesh.Recombine3DConformity"
        , "3d recombination conformity type"
        , 0
        , std::move(enumerate)
      )
    );
  }
  options.push_back
  (
    Option
    (
      "Mesh.RefineSteps"
      , "Number of refinement steps in the MeshAdapt-based 2D algorithms"
      , 10
    )
  );
  //skip Mesh.Renumber
  //skip Mesh.SaveAll
  //skip Mesh.SaveElementTagType
  //skip Mesh.SaveTopology
  //skip Mesh.SaveParametric
  //skip Mesh.SaveGroupsOfNodes
  //skip Mesh.ScalingFactor
  //skip Mesh.SecondOrderExperimental
  //skip Mesh.SecondOrderIncomplete
  //skip Mesh.SecondOrderLinear
  options.push_back
  (
    Option
    (
      "Mesh.Smoothing"
      , "Number of smoothing steps applied to the final mesh"
      , 1
    )
  );
  options.push_back
  (
    Option
    (
      "Mesh.SmoothCrossField"
      , "Apply n barycentric smoothing passes to the 3D cross field"
      , 0
    )
  );
  {
    std::unique_ptr<Option::Enumerate> enumerate(new Option::Enumerate());
    enumerate->map = boost::assign::list_of<Option::Enumerate::EnumMap::relation>
      ("Disabled", 0)
      ("Enabled", 1);
    options.push_back
    (
      Option
      (
        "Mesh.CrossFieldClosestPoint"
        , "Use closest point to compute 2D crossfield"
        , 1
        , std::move(enumerate)
      )
    );
  }
  {
    std::unique_ptr<Option::Enumerate> enumerate(new Option::Enumerate());
    enumerate->map = boost::assign::list_of<Option::Enumerate::EnumMap::relation>
      ("Disabled", 0)
      ("Enabled", 1);
    options.push_back
    (
      Option
      (
        "Mesh.SmoothNormals"
        , "Smooth the mesh normals"
        , 0
        , std::move(enumerate)
      )
    );
  }
  options.push_back
  (
    Option
    (
      "Mesh.SmoothRatio"
      , "Ratio between mesh sizes at nodes of a same edge (used in BAMG)"
      , 1.8
    )
  );
  {
    std::unique_ptr<Option::Enumerate> enumerate(new Option::Enumerate());
    enumerate->map = boost::assign::list_of<Option::Enumerate::EnumMap::relation>
      ("none", 0)
      ("all quadrangles", 1)
      ("all hexahedra", 2);
    options.push_back
    (
      Option
      (
        "Mesh.SubdivisionAlgorithm"
        , "Mesh subdivision algorithm"
        , 0
        , std::move(enumerate)
      )
    );
  }
  //skip Mesh.SurfaceEdges
  //skip Mesh.SurfaceFaces
  //skip Mesh.SurfaceNumbers
  //skip Mesh.SwitchElementTags
  //skip Mesh.Tangents
  //skip Mesh.Tetrahedra
  {
    std::unique_ptr<Option::Enumerate> enumerate(new Option::Enumerate());
    enumerate->map = boost::assign::list_of<Option::Enumerate::EnumMap::relation>
      ("Disabled", 0)
      ("Enabled", 1);
    options.push_back
    (
      Option
      (
        "Mesh.ToleranceEdgeLength"
        , "Skip a model edge in mesh generation if its length is less than user’s defined tolerance"
        , 0
        , std::move(enumerate)
      )
    );
  }
  options.push_back
  (
    Option
    (
      "Mesh.ToleranceInitialDelaunay"
      , "Tolerance for initial 3D Delaunay mesher"
      , 1e-08
    )
  );
  //skip Mesh.Triangles
  //skip Mesh.VolumeEdges
  //skip Mesh.VolumeFaces
  //skip Mesh.VolumeNumbers
  //skip Mesh.Voronoi
  //skip Mesh.ZoneDefinition
  //skip Mesh.Color.Points
  //skip Mesh.Color.PointsSup
  //skip Mesh.Color.Lines
  //skip Mesh.Color.Triangles
  //skip Mesh.Color.Quadrangles
  //skip Mesh.Color.Tetrahedra
  //skip Mesh.Color.Hexahedra
  //skip Mesh.Color.Prisms
  //skip Mesh.Color.Pyramids
  //skip Mesh.Color.Trihedra
  //skip Mesh.Color.Tangents
  //skip Mesh.Color.Normals
  //skip Mesh.Color.Zero - Mesh.Color.Nineteen
}

void GMSH::setOption(const std::string &keyIn, double valueIn)
{
  for (auto &option : options)
  {
    if (option.getKey() == keyIn)
      option.setValue(valueIn);
  }
}

void GMSH::setOption(const std::string &keyIn, const std::string &eIn)
{
  for (auto &option : options)
  {
    if (option.getKey() == keyIn)
      option.setValue(eIn);
  }
}

void GMSH::serialIn(const prj::srl::msh::ParametersGMSH &psIn)
{
  refine = psIn.refine();
  for (const auto &p : psIn.options().array())
    setOption(p.key(), p.value());
}

prj::srl::msh::ParametersGMSH GMSH::serialOut() const
{
  prj::srl::msh::ParametersGMSHOptions serialOptions;
  for (const auto &option : options)
    serialOptions.array().push_back(prj::srl::msh::ParametersGMSHOption(option.getKey(), option.getValue()));
  return prj::srl::msh::ParametersGMSH
  (
    serialOptions
    , refine
  );
}
