/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2018  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef TLS_SHAPEVECTOR_H
#define TLS_SHAPEVECTOR_H

#include <vector>

#include <NCollection_StdAllocator.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Shell.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>

namespace occt
{
  typedef std::vector<TopoDS_Shape, NCollection_StdAllocator<TopoDS_Shape>> ShapeVector;
  typedef std::vector<TopoDS_Solid, NCollection_StdAllocator<TopoDS_Solid>> SolidVector;
  typedef std::vector<TopoDS_Shell, NCollection_StdAllocator<TopoDS_Shell>> ShellVector;
  typedef std::vector<TopoDS_Face, NCollection_StdAllocator<TopoDS_Face>> FaceVector;
  typedef std::vector<TopoDS_Wire, NCollection_StdAllocator<TopoDS_Wire>> WireVector;
  typedef std::vector<TopoDS_Edge, NCollection_StdAllocator<TopoDS_Edge>> EdgeVector;
  typedef std::vector<TopoDS_Vertex, NCollection_StdAllocator<TopoDS_Vertex>> VertexVector;
}

#endif //TLS_SHAPEVECTOR_H
