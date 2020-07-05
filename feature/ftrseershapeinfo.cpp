/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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

#include <QTextStream>

#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <TopoDS.hxx>
#include <BRep_Tool.hxx>
#include <gp_Cylinder.hxx>
#include <gp_Pln.hxx>
#include <gp_Cone.hxx>
#include <gp_Sphere.hxx>
#include <gp_Torus.hxx>
#include <gp_Lin.hxx>
#include <gp_Circ.hxx>
#include <gp_Elips.hxx>
#include <gp_Hypr.hxx>
#include <gp_Parab.hxx>

#include <osg/Matrixd>

#include "globalutilities.h"
#include "tools/infotools.h"
#include "tools/occtools.h"
#include "tools/idtools.h"
#include "annex/annseershape.h"
#include "feature/ftrseershapeinfo.h"

namespace ftr
{
    class SeerShapeInfo::FunctionMapper
    {
        public:
            typedef std::function<void (QTextStream&, const TopoDS_Shape&)> InfoFunction;
            typedef std::map<TopAbs_ShapeEnum, InfoFunction> FunctionMap;
            FunctionMap functionMap;
    };
}

using namespace ftr;

SeerShapeInfo::SeerShapeInfo(const ann::SeerShape &shapeIn) : seerShape(shapeIn)
{
    functionMapper = std::move(std::unique_ptr<FunctionMapper>(new FunctionMapper()));
    functionMapper->functionMap.insert(std::make_pair(TopAbs_COMPOUND, std::bind(&SeerShapeInfo::compInfo, this, std::placeholders::_1, std::placeholders::_2)));
    functionMapper->functionMap.insert(std::make_pair(TopAbs_COMPSOLID, std::bind(&SeerShapeInfo::compSolidInfo, this, std::placeholders::_1, std::placeholders::_2)));
    functionMapper->functionMap.insert(std::make_pair(TopAbs_SOLID, std::bind(&SeerShapeInfo::solidInfo, this, std::placeholders::_1, std::placeholders::_2)));
    functionMapper->functionMap.insert(std::make_pair(TopAbs_SHELL, std::bind(&SeerShapeInfo::shellInfo, this, std::placeholders::_1, std::placeholders::_2)));
    functionMapper->functionMap.insert(std::make_pair(TopAbs_FACE, std::bind(&SeerShapeInfo::faceInfo, this, std::placeholders::_1, std::placeholders::_2)));
    functionMapper->functionMap.insert(std::make_pair(TopAbs_WIRE, std::bind(&SeerShapeInfo::wireInfo, this, std::placeholders::_1, std::placeholders::_2)));
    functionMapper->functionMap.insert(std::make_pair(TopAbs_EDGE, std::bind(&SeerShapeInfo::edgeInfo, this, std::placeholders::_1, std::placeholders::_2)));
    functionMapper->functionMap.insert(std::make_pair(TopAbs_VERTEX, std::bind(&SeerShapeInfo::vertexInfo, this, std::placeholders::_1, std::placeholders::_2)));
    
    //points handled at factory.
}
SeerShapeInfo::~SeerShapeInfo(){}

QTextStream& SeerShapeInfo::getShapeInfo(QTextStream &streamIn, const boost::uuids::uuid &idIn )
{
    assert(functionMapper);
    streamIn << endl << "Shape information: " << endl;
    if (seerShape.isNull())
    {
      streamIn << "    SeerShape is null" << endl;
      return streamIn;
    }
    assert(seerShape.hasId(idIn));
    if (!seerShape.hasId(idIn))
      return streamIn;
    const TopoDS_Shape &shape = seerShape.findShape(idIn);
    if (shape.IsNull())
    {
      streamIn << "    TopoDS_Shape is null" << endl;
      return streamIn;
    }
    
    functionMapper->functionMap.at(shape.ShapeType())(streamIn, shape);
    //common to all shapes.
    streamIn << "    Orientation: " << ((shape.Orientation() == TopAbs_FORWARD) ? ("Forward") : ("Reversed")) << endl
        << "    Hash code: " << occt::getShapeHash(shape) << endl
        << "    Shape id: " << QString::fromStdString(gu::idToString(idIn)) << endl;
    
    occt::BoundingBox bb(shape);    
    streamIn << "    Bounding Box: "
        << qSetRealNumberPrecision(12) << fixed << endl
        << "        Center: "; gu::gpPntOut(streamIn, bb.getCenter()); streamIn << endl
        << "        Length: " << bb.getLength() << endl
        << "        Width: " << bb.getWidth() << endl
        << "        Height: " << bb.getHeight() << endl;
    
    return streamIn;
}

void SeerShapeInfo::compInfo(QTextStream &streamIn, const TopoDS_Shape &s)
{
    occt::ShapeVector subShapes = occt::ShapeVectorCast(TopoDS::Compound(s));
    streamIn << "    Shape type: compound" << endl
             << "    Sub-Shapes:" << endl;
    for (const auto &ss : subShapes)
      streamIn << "        " << QString::fromStdString(occt::getShapeTypeString(ss)) << endl;
}

void SeerShapeInfo::compSolidInfo(QTextStream &streamIn, const TopoDS_Shape&)
{
    streamIn << "    Shape type: compound solid" << endl;
}

void SeerShapeInfo::solidInfo(QTextStream &streamIn, const TopoDS_Shape&)
{
    streamIn << "    Shape type: solid" << endl;
}

void SeerShapeInfo::shellInfo(QTextStream &streamIn, const TopoDS_Shape &shapeIn)
{
    streamIn << "    Shape type: shell." << endl
        << "    Closed is: " << ((shapeIn.Closed()) ? ("true") : ("false")) << endl;
}

template<typename T>
static gp_Ax2 surfaceSystem(const T& surface)
{
  //possible conversion from left handed to right handed system.
  return surface.Position().Ax2();
}

void SeerShapeInfo::faceInfo(QTextStream &streamIn, const TopoDS_Shape &shapeIn)
{
    assert(shapeIn.ShapeType() == TopAbs_FACE);
    BRepAdaptor_Surface surfaceAdaptor(TopoDS::Face(shapeIn));
    
    streamIn << qSetRealNumberPrecision(12) << fixed
    << "    Shape type: face" << endl
    << "    Tolerance: " << BRep_Tool::Tolerance(TopoDS::Face(shapeIn)) << endl;
    if (surfaceAdaptor.IsUPeriodic())
      streamIn << "    Is U periodic: True. Period = " << surfaceAdaptor.UPeriod() << endl;
    else
      streamIn << "    Is U periodic: False" << endl;
    if (surfaceAdaptor.IsVPeriodic())
      streamIn << "    Is V periodic: True. Period = " << surfaceAdaptor.VPeriod() << endl;
    else
      streamIn << "    Is V periodic: False" << endl;
    streamIn << "    Surface type: " << gu::surfaceTypeStrings.at(surfaceAdaptor.GetType()) << endl;
    if (surfaceAdaptor.GetType() == GeomAbs_Plane)
    {
      gp_Pln plane = surfaceAdaptor.Plane();
      
      streamIn
      << "        Placement: "; gu::osgMatrixOut(streamIn, gu::toOsg(surfaceSystem(plane))) << endl;
    }
    else if (surfaceAdaptor.GetType() == GeomAbs_Cylinder)
    {
      gp_Cylinder cyl = surfaceAdaptor.Cylinder();
      
      streamIn
      << "        Radius: " << cyl.Radius() << endl
      << "        Placement: "; gu::osgMatrixOut(streamIn, gu::toOsg(surfaceSystem(cyl))) << endl;
    }
    else if (surfaceAdaptor.GetType() == GeomAbs_Cone)
    {
      gp_Cone cone = surfaceAdaptor.Cone();
      
      streamIn
      << "        Radius: " << cone.RefRadius() << endl
      << "        Semi Angle: " << cone.SemiAngle() << endl
      << "        Placement: "; gu::osgMatrixOut(streamIn, gu::toOsg(surfaceSystem(cone))) << endl;
    }
    else if (surfaceAdaptor.GetType() == GeomAbs_Sphere)
    {
      gp_Sphere sphere = surfaceAdaptor.Sphere();
      
      streamIn
      << "        Radius: " << sphere.Radius() << endl
      << "        Placement: "; gu::osgMatrixOut(streamIn, gu::toOsg(surfaceSystem(sphere))) << endl;
    }
    else if (surfaceAdaptor.GetType() == GeomAbs_Torus)
    {
      gp_Torus torus = surfaceAdaptor.Torus();
      
      streamIn
      << "        Major Radius: " << torus.MajorRadius() << endl
      << "        Minor Radius: " << torus.MinorRadius() << endl
      << "        Placement: "; gu::osgMatrixOut(streamIn, gu::toOsg(surfaceSystem(torus))) << endl;
    }
}

void SeerShapeInfo::wireInfo(QTextStream &streamIn, const TopoDS_Shape&)
{
    streamIn << "    Shape type: wire" << endl;
}

void SeerShapeInfo::edgeInfo(QTextStream &streamIn, const TopoDS_Shape &shapeIn)
{
    assert(shapeIn.ShapeType() == TopAbs_EDGE);
    BRepAdaptor_Curve curveAdaptor(TopoDS::Edge(shapeIn));
    
    streamIn << qSetRealNumberPrecision(12) << fixed
    << "    Shape type: edge" << endl
    << "    Tolerance Brep_Tool: " << BRep_Tool::Tolerance(TopoDS::Edge(shapeIn)) << endl
    << "    Tolerance BRepAdaptor_Curve: " << curveAdaptor.Tolerance() << endl
    << "    Curve type: " << gu::curveTypeStrings.at(curveAdaptor.GetType()) << endl;
    
    if (curveAdaptor.GetType() == GeomAbs_Line)
    {
      gp_Lin line = curveAdaptor.Line();
      
      streamIn
      << "        Location: "; gu::gpPntOut(streamIn, line.Location()) << endl
      << "        Direction: "; gu::gpDirOut(streamIn, line.Direction()) << endl
      << "        Start Point: "; gu::gpPntOut(streamIn, curveAdaptor.Value(curveAdaptor.FirstParameter())) << endl
      << "        End Point: "; gu::gpPntOut(streamIn, curveAdaptor.Value(curveAdaptor.LastParameter())) << endl;
    }
    else if (curveAdaptor.GetType() == GeomAbs_Circle)
    {
      gp_Circ circle = curveAdaptor.Circle();
      
      streamIn
      << "        Radius: " << circle.Radius() << endl
      << "        Placement: "; gu::osgMatrixOut(streamIn, gu::toOsg(circle.Position())) << endl
      << "        Start Point: "; gu::gpPntOut(streamIn, curveAdaptor.Value(curveAdaptor.FirstParameter())) << endl
      << "        End Point: "; gu::gpPntOut(streamIn, curveAdaptor.Value(curveAdaptor.LastParameter())) << endl;
    }
    else if (curveAdaptor.GetType() == GeomAbs_Ellipse)
    {
      gp_Elips ellipse = curveAdaptor.Ellipse();
      
      streamIn
      << "        Major Radius: " << ellipse.MajorRadius() << endl
      << "        Minor Radius: " << ellipse.MinorRadius() << endl
      << "        Focus 1: "; gu::gpPntOut(streamIn, ellipse.Focus1()) << endl
      << "        Focus 2: "; gu::gpPntOut(streamIn, ellipse.Focus2()) << endl
      << "        Placement: "; gu::osgMatrixOut(streamIn, gu::toOsg(ellipse.Position())) << endl
      << "        Start Point: "; gu::gpPntOut(streamIn, curveAdaptor.Value(curveAdaptor.FirstParameter())) << endl
      << "        End Point: "; gu::gpPntOut(streamIn, curveAdaptor.Value(curveAdaptor.LastParameter())) << endl;
    }
    else if (curveAdaptor.GetType() == GeomAbs_Hyperbola)
    {
      gp_Hypr hyp = curveAdaptor.Hyperbola();
      
      streamIn
      << "        Major Radius: " << hyp.MajorRadius() << endl
      << "        Minor Radius: " << hyp.MinorRadius() << endl
      << "        Focus 1: "; gu::gpPntOut(streamIn, hyp.Focus1()) << endl
      << "        Focus 2: "; gu::gpPntOut(streamIn, hyp.Focus2()) << endl
      << "        Placement: "; gu::osgMatrixOut(streamIn, gu::toOsg(hyp.Position())) << endl
      << "        Start Point: "; gu::gpPntOut(streamIn, curveAdaptor.Value(curveAdaptor.FirstParameter())) << endl
      << "        End Point: "; gu::gpPntOut(streamIn, curveAdaptor.Value(curveAdaptor.LastParameter())) << endl;
    }
    else if (curveAdaptor.GetType() == GeomAbs_Parabola)
    {
      //not sure about start and end points.
      gp_Parab par = curveAdaptor.Parabola();
      
      streamIn
      << "        Focus: "; gu::gpPntOut(streamIn, par.Focus()) << endl
      << "        Placement: "; gu::osgMatrixOut(streamIn, gu::toOsg(par.Position())) << endl
      << "        Start Point: "; gu::gpPntOut(streamIn, curveAdaptor.Value(curveAdaptor.FirstParameter())) << endl
      << "        End Point: "; gu::gpPntOut(streamIn, curveAdaptor.Value(curveAdaptor.LastParameter())) << endl;
    }
}

void SeerShapeInfo::vertexInfo(QTextStream &streamIn, const TopoDS_Shape &shapeIn)
{
    assert(shapeIn.ShapeType() == TopAbs_VERTEX);
    gp_Pnt vPoint = BRep_Tool::Pnt(TopoDS::Vertex(shapeIn));
    forcepoint(streamIn) << qSetRealNumberPrecision(12) << fixed
    << "    Shape type: vertex" << endl
      << "    location: "; gu::gpPntOut(streamIn, vPoint); streamIn << endl
      << "    Tolerance: " << BRep_Tool::Tolerance(TopoDS::Vertex(shapeIn)) << endl;
}
