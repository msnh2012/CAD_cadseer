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
#include <Geom_BSplineCurve.hxx>

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

SeerShapeInfo::SeerShapeInfo(const ann::SeerShape &shapeIn)
: seerShape(shapeIn)
, functionMapper(std::make_unique<FunctionMapper>())
{
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
    streamIn << Qt::endl << "Shape information: " << Qt::endl;
    if (seerShape.isNull())
    {
      streamIn << "    SeerShape is null" << Qt::endl;
      return streamIn;
    }
    assert(seerShape.hasId(idIn));
    if (!seerShape.hasId(idIn))
      return streamIn;
    const TopoDS_Shape &shape = seerShape.findShape(idIn);
    if (shape.IsNull())
    {
      streamIn << "    TopoDS_Shape is null" << Qt::endl;
      return streamIn;
    }
    
    functionMapper->functionMap.at(shape.ShapeType())(streamIn, shape);
    //common to all shapes.
    streamIn << "    Orientation: " << ((shape.Orientation() == TopAbs_FORWARD) ? ("Forward") : ("Reversed")) << Qt::endl
        << "    Hash code: " << occt::getShapeHash(shape) << Qt::endl
        << "    Shape id: " << QString::fromStdString(gu::idToString(idIn)) << Qt::endl;
    
    occt::BoundingBox bb(shape);    
    streamIn << "    Bounding Box: "
        << qSetRealNumberPrecision(12) << Qt::fixed << Qt::endl
        << "        Center: "; gu::gpPntOut(streamIn, bb.getCenter()); streamIn << Qt::endl
        << "        Length: " << bb.getLength() << Qt::endl
        << "        Width: " << bb.getWidth() << Qt::endl
        << "        Height: " << bb.getHeight() << Qt::endl;
    
    return streamIn;
}

void SeerShapeInfo::compInfo(QTextStream &streamIn, const TopoDS_Shape &s)
{
    occt::ShapeVector subShapes = occt::ShapeVectorCast(TopoDS::Compound(s));
    streamIn << "    Shape type: compound" << Qt::endl
             << "    Sub-Shapes:" << Qt::endl;
    for (const auto &ss : subShapes)
      streamIn << "        " << QString::fromStdString(occt::getShapeTypeString(ss)) << Qt::endl;
}

void SeerShapeInfo::compSolidInfo(QTextStream &streamIn, const TopoDS_Shape&)
{
    streamIn << "    Shape type: compound solid" << Qt::endl;
}

void SeerShapeInfo::solidInfo(QTextStream &streamIn, const TopoDS_Shape&)
{
    streamIn << "    Shape type: solid" << Qt::endl;
}

void SeerShapeInfo::shellInfo(QTextStream &streamIn, const TopoDS_Shape &shapeIn)
{
    streamIn << "    Shape type: shell." << Qt::endl
        << "    Closed is: " << ((shapeIn.Closed()) ? ("true") : ("false")) << Qt::endl;
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
    
    streamIn << qSetRealNumberPrecision(12) << Qt::fixed
    << "    Shape type: face" << Qt::endl
    << "    Tolerance: " << BRep_Tool::Tolerance(TopoDS::Face(shapeIn)) << Qt::endl;
    if (surfaceAdaptor.IsUPeriodic())
      streamIn << "    Is U periodic: True. Period = " << surfaceAdaptor.UPeriod() << Qt::endl;
    else
      streamIn << "    Is U periodic: False" << Qt::endl;
    if (surfaceAdaptor.IsVPeriodic())
      streamIn << "    Is V periodic: True. Period = " << surfaceAdaptor.VPeriod() << Qt::endl;
    else
      streamIn << "    Is V periodic: False" << Qt::endl;
    streamIn << "    Surface type: " << gu::surfaceTypeStrings.at(surfaceAdaptor.GetType()) << Qt::endl;
    streamIn << "    U Range: " << surfaceAdaptor.FirstUParameter() << "    "<< surfaceAdaptor.LastUParameter() << Qt::endl;
    streamIn << "    V Range: " << surfaceAdaptor.FirstVParameter() << "    "<< surfaceAdaptor.LastVParameter() << Qt::endl;
    if (surfaceAdaptor.GetType() == GeomAbs_Plane)
    {
      gp_Pln plane = surfaceAdaptor.Plane();
      
      streamIn
      << "        Placement: "; gu::osgMatrixOut(streamIn, gu::toOsg(surfaceSystem(plane))) << Qt::endl;
    }
    else if (surfaceAdaptor.GetType() == GeomAbs_Cylinder)
    {
      gp_Cylinder cyl = surfaceAdaptor.Cylinder();
      
      streamIn
      << "        Radius: " << cyl.Radius() << Qt::endl
      << "        Placement: "; gu::osgMatrixOut(streamIn, gu::toOsg(surfaceSystem(cyl))) << Qt::endl;
    }
    else if (surfaceAdaptor.GetType() == GeomAbs_Cone)
    {
      gp_Cone cone = surfaceAdaptor.Cone();
      
      streamIn
      << "        Radius: " << cone.RefRadius() << Qt::endl
      << "        Semi Angle: " << cone.SemiAngle() << Qt::endl
      << "        Placement: "; gu::osgMatrixOut(streamIn, gu::toOsg(surfaceSystem(cone))) << Qt::endl;
    }
    else if (surfaceAdaptor.GetType() == GeomAbs_Sphere)
    {
      gp_Sphere sphere = surfaceAdaptor.Sphere();
      
      streamIn
      << "        Radius: " << sphere.Radius() << Qt::endl
      << "        Placement: "; gu::osgMatrixOut(streamIn, gu::toOsg(surfaceSystem(sphere))) << Qt::endl;
    }
    else if (surfaceAdaptor.GetType() == GeomAbs_Torus)
    {
      gp_Torus torus = surfaceAdaptor.Torus();
      
      streamIn
      << "        Major Radius: " << torus.MajorRadius() << Qt::endl
      << "        Minor Radius: " << torus.MinorRadius() << Qt::endl
      << "        Placement: "; gu::osgMatrixOut(streamIn, gu::toOsg(surfaceSystem(torus))) << Qt::endl;
    }
}

void SeerShapeInfo::wireInfo(QTextStream &streamIn, const TopoDS_Shape&)
{
    streamIn << "    Shape type: wire" << Qt::endl;
}

void SeerShapeInfo::edgeInfo(QTextStream &streamIn, const TopoDS_Shape &shapeIn)
{
    assert(shapeIn.ShapeType() == TopAbs_EDGE);
    const auto &edge = TopoDS::Edge(shapeIn);
    BRepAdaptor_Curve curveAdaptor(edge);
    
    auto boolText = [&](bool value) -> QString
    {
      if (value) return QObject::tr("True");
      return QObject::tr("False");
    };
    
    streamIn << qSetRealNumberPrecision(12) << Qt::fixed
    << "    Shape type: edge" << Qt::endl
    << "    Tolerance Brep_Tool: " << BRep_Tool::Tolerance(edge) << Qt::endl
    << "    Tolerance BRepAdaptor_Curve: " << curveAdaptor.Tolerance() << Qt::endl
    << "    Same parameter flag: " << boolText(BRep_Tool::SameParameter(edge)) << Qt::endl
    << "    Same range flag: " << boolText(BRep_Tool::SameRange(edge)) << Qt::endl
    << "    Curve type: " << gu::curveTypeStrings.at(curveAdaptor.GetType()) << Qt::endl;
    
    if (curveAdaptor.GetType() == GeomAbs_Line)
    {
      gp_Lin line = curveAdaptor.Line();
      
      streamIn
      << "        Location: "; gu::gpPntOut(streamIn, line.Location()) << Qt::endl
      << "        Direction: "; gu::gpDirOut(streamIn, line.Direction()) << Qt::endl
      << "        Start Point: "; gu::gpPntOut(streamIn, curveAdaptor.Value(curveAdaptor.FirstParameter())) << Qt::endl
      << "        End Point: "; gu::gpPntOut(streamIn, curveAdaptor.Value(curveAdaptor.LastParameter())) << Qt::endl;
    }
    else if (curveAdaptor.GetType() == GeomAbs_Circle)
    {
      gp_Circ circle = curveAdaptor.Circle();
      
      streamIn
      << "        Radius: " << circle.Radius() << Qt::endl
      << "        Placement: "; gu::osgMatrixOut(streamIn, gu::toOsg(circle.Position())) << Qt::endl
      << "        Start Point: "; gu::gpPntOut(streamIn, curveAdaptor.Value(curveAdaptor.FirstParameter())) << Qt::endl
      << "        End Point: "; gu::gpPntOut(streamIn, curveAdaptor.Value(curveAdaptor.LastParameter())) << Qt::endl;
    }
    else if (curveAdaptor.GetType() == GeomAbs_Ellipse)
    {
      gp_Elips ellipse = curveAdaptor.Ellipse();
      
      streamIn
      << "        Major Radius: " << ellipse.MajorRadius() << Qt::endl
      << "        Minor Radius: " << ellipse.MinorRadius() << Qt::endl
      << "        Focus 1: "; gu::gpPntOut(streamIn, ellipse.Focus1()) << Qt::endl
      << "        Focus 2: "; gu::gpPntOut(streamIn, ellipse.Focus2()) << Qt::endl
      << "        Placement: "; gu::osgMatrixOut(streamIn, gu::toOsg(ellipse.Position())) << Qt::endl
      << "        Start Point: "; gu::gpPntOut(streamIn, curveAdaptor.Value(curveAdaptor.FirstParameter())) << Qt::endl
      << "        End Point: "; gu::gpPntOut(streamIn, curveAdaptor.Value(curveAdaptor.LastParameter())) << Qt::endl;
    }
    else if (curveAdaptor.GetType() == GeomAbs_Hyperbola)
    {
      gp_Hypr hyp = curveAdaptor.Hyperbola();
      
      streamIn
      << "        Major Radius: " << hyp.MajorRadius() << Qt::endl
      << "        Minor Radius: " << hyp.MinorRadius() << Qt::endl
      << "        Focus 1: "; gu::gpPntOut(streamIn, hyp.Focus1()) << Qt::endl
      << "        Focus 2: "; gu::gpPntOut(streamIn, hyp.Focus2()) << Qt::endl
      << "        Placement: "; gu::osgMatrixOut(streamIn, gu::toOsg(hyp.Position())) << Qt::endl
      << "        Start Point: "; gu::gpPntOut(streamIn, curveAdaptor.Value(curveAdaptor.FirstParameter())) << Qt::endl
      << "        End Point: "; gu::gpPntOut(streamIn, curveAdaptor.Value(curveAdaptor.LastParameter())) << Qt::endl;
    }
    else if (curveAdaptor.GetType() == GeomAbs_Parabola)
    {
      //not sure about start and end points.
      gp_Parab par = curveAdaptor.Parabola();
      
      streamIn
      << "        Focus: "; gu::gpPntOut(streamIn, par.Focus()) << Qt::endl
      << "        Placement: "; gu::osgMatrixOut(streamIn, gu::toOsg(par.Position())) << Qt::endl
      << "        Start Point: "; gu::gpPntOut(streamIn, curveAdaptor.Value(curveAdaptor.FirstParameter())) << Qt::endl
      << "        End Point: "; gu::gpPntOut(streamIn, curveAdaptor.Value(curveAdaptor.LastParameter())) << Qt::endl;
    }
    else if (curveAdaptor.GetType() == GeomAbs_BSplineCurve)
    {
      //not sure about start and end points.
      opencascade::handle<Geom_BSplineCurve> spline = curveAdaptor.BSpline();
      
      streamIn
      << "        Is Closed: " << ((spline->IsClosed()) ? "True" : "False") << Qt::endl
      << "        Is Periodic: " << ((spline->IsPeriodic()) ? "True" : "False") << Qt::endl
      << "        Is Rational: " << ((spline->IsRational()) ? "True" : "False") << Qt::endl
      << "        Continuity: " << gu::continuityStrings.at(static_cast<int>(spline->Continuity())) << Qt::endl
      << "        Pole Count: " << spline->NbPoles() << Qt::endl;
//       << "        Weight Count: " << spline->Weights()->Length() << endl;
    }
}

void SeerShapeInfo::vertexInfo(QTextStream &streamIn, const TopoDS_Shape &shapeIn)
{
    assert(shapeIn.ShapeType() == TopAbs_VERTEX);
    gp_Pnt vPoint = BRep_Tool::Pnt(TopoDS::Vertex(shapeIn));
    Qt::forcepoint(streamIn) << qSetRealNumberPrecision(12) << Qt::fixed
    << "    Shape type: vertex" << Qt::endl
      << "    location: "; gu::gpPntOut(streamIn, vPoint); streamIn << Qt::endl
      << "    Tolerance: " << BRep_Tool::Tolerance(TopoDS::Vertex(shapeIn)) << Qt::endl;
}
