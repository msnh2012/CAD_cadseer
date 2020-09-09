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

#ifndef GU_INFOTOOLS_H
#define GU_INFOTOOLS_H

class QTextStream;
class QString;

namespace osg{class Matrixd; class Vec3d; class Quat;}
class gp_Pnt;
class gp_Dir;

namespace gu
{
    QTextStream& osgMatrixOut(QTextStream&, const osg::Matrixd&);
    QTextStream& osgQuatOut(QTextStream&, const osg::Quat&);
    QTextStream& osgVectorOut(QTextStream&, const osg::Vec3d&);
    QTextStream& gpPntOut(QTextStream&, const gp_Pnt&);
    QTextStream& gpDirOut(QTextStream&, const gp_Dir&);
    
    QString osgMatrixOut(const osg::Matrixd&);
    QString osgQuatOut(const osg::Quat&);
    QString osgVectorOut(const osg::Vec3d&);
    QString gpPntOut(const gp_Pnt&);
    QString gpDirOut(const gp_Dir&);
    
    static const std::vector<QString> surfaceTypeStrings = 
    {
        ("GeomAbs_Plane"),
        ("GeomAbs_Cylinder"),	
        ("GeomAbs_Cone"),
        ("GeomAbs_Sphere"),
        ("GeomAbs_Torus"),
        ("GeomAbs_BezierSurface"),
        ("GeomAbs_BSplineSurface"),
        ("GeomAbs_SurfaceOfRevolution"),
        ("GeomAbs_SurfaceOfExtrusion"),
        ("GeomAbs_OffsetSurface"),
        ("GeomAbs_OtherSurface ")
    };
    
    static const std::vector<QString> curveTypeStrings = 
    {
        ("GeomAbs_Line"),
        ("GeomAbs_Circle "),
        ("GeomAbs_Ellipse"),
        ("GeomAbs_Hyperbola"),
        ("GeomAbs_Parabola"),
        ("GeomAbs_BezierCurve"),
        ("GeomAbs_BSplineCurve"),
        ("GeomAbs_OffsetCurve"),
        ("GeomAbs_OtherCurve")
    };
    
    static const std::vector<QString> continuityStrings =
    {
      ("GeomAbs_C0"),
      ("GeomAbs_G1"),
      ("GeomAbs_C1"),
      ("GeomAbs_G2"),
      ("GeomAbs_C2"),
      ("GeomAbs_C3"),
      ("GeomAbs_CN")
    };
}

#endif // GU_INFOTOOLS_H
