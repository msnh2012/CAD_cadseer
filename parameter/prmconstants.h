/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2021  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef PRM_CONSTANTS_H
#define PRM_CONSTANTS_H

#include <string_view>

#include <QObject>
#include <QString>

namespace prm
{
  /*! Common default names. subjected to locale. */
  namespace Names
  {
    static const QString Radius = QObject::tr("Radius");
    static const QString Height = QObject::tr("Height");
    static const QString Length = QObject::tr("Length");
    static const QString Width = QObject::tr("Width");
    static const QString Radius1 = QObject::tr("Radius1");
    static const QString Radius2 = QObject::tr("Radius2");
    static const QString Position = QObject::tr("Position");
    static const QString Distance = QObject::tr("Distance");
    static const QString Angle = QObject::tr("Angle");
    static const QString Offset = QObject::tr("Offset");
    static const QString CSys = QObject::tr("CSys");
    static const QString Diameter = QObject::tr("Diameter");
    static const QString Origin = QObject::tr("Origin");
    static const QString Direction = QObject::tr("Direction");
    static const QString Scale = QObject::tr("Scale");
    static const QString Path = QObject::tr("Path");
    static const QString Size = QObject::tr("Size");
    static const QString AutoSize = QObject::tr("AutoSize");
    static const QString Pitch = QObject::tr("Pitch");
    static const QString Picks = QObject::tr("Picks");
  }
  
  /*! Common tags for parameters not subjected to locale */
  namespace Tags
  {
    constexpr std::string_view Radius = "Radius"; //!< cylinder, sphere, blend
    constexpr std::string_view Height = "Height"; //!< cylinder, box, cone
    constexpr std::string_view Length = "Length"; //!< box
    constexpr std::string_view Width = "Width"; //!< box
    constexpr std::string_view Position = "Position"; //!< blend
    constexpr std::string_view Distance = "Distance"; //!< chamfer, sketch
    constexpr std::string_view Angle = "Angle"; //!< draft
    constexpr std::string_view Offset = "Offset"; //!< datum plane
    constexpr std::string_view CSys = "CSys"; //!< feature with a coordinate system.
    constexpr std::string_view Diameter = "Diameter"; //!< sketch
    constexpr std::string_view Origin = "Origin"; //!< revolve, dieset
    constexpr std::string_view Direction = "Direction"; //!< extrude.
    constexpr std::string_view Scale = "Scale"; //!< image plane.
    constexpr std::string_view Path = "Path"; //!< image plane.
    constexpr std::string_view Size = "Size"; //!< datums.
    constexpr std::string_view AutoSize = "AutoSize"; //!< datums.
    constexpr std::string_view Pitch = "Pitch"; //!< thread, strip
    constexpr std::string_view Picks = "Picks"; //!< Datum plane
  }
}

#endif //PRM_CONSTANTS_H
