/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2022 Thomas S. Anderson blobfish.at.gmx.com
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

#include <BRep_Tool.hxx>

#include <ChFi3d_FilBuilder.hxx>
#include <ChFiDS_FilSpine.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>

#include "tools/occtools.h"
#include "annex/annseershape.h"
#include "law/lwfadapter.h"

using namespace lwf;

struct Adapter::Stow
{
  const ann::SeerShape &sShape;
  boost::uuids::uuid edgeId;
  opencascade::handle<ChFiDS_FilSpine> spine;
  
  Stow() = delete;
  Stow(const ann::SeerShape &ssIn, const boost::uuids::uuid &edgeIdIn)
  : sShape(ssIn)
  , edgeId(edgeIdIn)
  {
    //don't set me up
    assert(sShape.hasId(edgeId));
    assert(sShape.getOCCTShape(edgeId).ShapeType() == TopAbs_EDGE);
    
    auto parents = sShape.useGetParentsOfType(edgeId, TopAbs_SOLID);
    if (parents.empty())
      parents = sShape.useGetParentsOfType(edgeId, TopAbs_SHELL);
    if (parents.size() != 1)
      return;
    ChFi3d_FilBuilder tempBuilder(sShape.getOCCTShape(parents.front()));
    tempBuilder.Add(TopoDS::Edge(sShape.getOCCTShape(edgeId)));
    spine = dynamic_cast<ChFiDS_FilSpine*>(tempBuilder.Value(1).get());
  }
};

Adapter::Adapter(const ann::SeerShape &ssIn, const boost::uuids::uuid &eIdIn)
: stow(std::make_unique<Stow>(ssIn, eIdIn))
{}

Adapter::Adapter(Adapter&&) noexcept = default;

Adapter::~Adapter() = default;

Adapter& Adapter::operator=(Adapter &&) noexcept = default;

bool Adapter::isValid() const
{
  return static_cast<bool>(stow->spine);
}

std::pair<double, double> Adapter::getRange() const
{
  assert(isValid());
  return std::make_pair(stow->spine->FirstParameter(), stow->spine->LastParameter());
}

bool Adapter::isPeriodic() const
{
  assert(isValid());
  return stow->spine->IsPeriodic();
}

/*!@brief Get location from spine parameter.
 */
gp_Pnt Adapter::location(double spineParameter) const
{
  assert(isValid());
  //It appears 'Value' constrains the point within range.
  return stow->spine->Value(spineParameter);
}

/*!@brief Get parameter along spine for vertex
 * @details Get parameter along spine for vertex
 * @param[in] vIn is a vertex on the spine.
 * @return Spine parameter or nullopt if vertex is not on spine.
 */
std::optional<double> Adapter::toSpineParameter(const TopoDS_Vertex &vIn) const
{
  assert(isValid());
  double out = stow->spine->Absc(vIn);
  if (out < 0.0) return std::nullopt;
  return out;
}

/*!@brief Get parameter along spine for edge and edge parameter
 * @details Get parameter along spine for edge and edge parameter
 * @param[in] edgeIn is an edge on the spine.
 * @param[in] prmIn is a parameter on edge.
 * @return Spine parameter or nullopt is edge is not on spine or prmIn out of edge range.
 */
std::optional<double> Adapter::toSpineParameter(const TopoDS_Edge &edgeIn, double prmIn) const
{
  assert(isValid());
  //Are we going to be fighting orientation between pick edge and spine edge?
  int index = stow->spine->Index(edgeIn);
  if (index == 0) return std::nullopt; //occt is 1 based.
   
  //spine doesn't check edge range vs prmIn, so we do.
  BRepAdaptor_Curve ac(edgeIn);
  if (prmIn < ac.FirstParameter() || prmIn > ac.LastParameter()) return std::nullopt;
  
  return stow->spine->Absc(prmIn, index);
}

/*!@brief Get shape relative to spine parameter.
 * @param[in] prmIn is a parameter on spine.
 * @return Shape related to spine parameter.
 * @details Get shape relative to spine parameter.
 * If prmIn is out of spine range then first or last vertex
 * will be returned. Vertex shape is preferred.
 */
std::pair<TopoDS_Shape, double> Adapter::fromSpineParameter(double prmIn) const
{
  assert(isValid());
  //check if out of range.
  if (prmIn <= stow->spine->FirstParameter())
    return std::make_pair(stow->spine->FirstVertex(), 0.0);
  if (prmIn >= stow->spine->LastParameter())
    return std::make_pair(stow->spine->LastVertex(), 0.0);
  
  //ChFiDS_FilSpine isn't set up to convert spine parameters to vertices. so we do it ourselves.
  occt::ShapeVector vertices;
  for (int index = 1; index <= stow->spine->NbEdges(); ++index)
  {
    auto ce = stow->spine->Edges(index);
    assert(stow->sShape.hasShape(ce));
    auto verts = stow->sShape.useGetChildrenOfType(ce, TopAbs_VERTEX);
    vertices.insert(vertices.end(), verts.begin(), verts.end());
  }
  occt::uniquefy(vertices);
  
  auto prmPoint = stow->spine->Value(prmIn);
  for (const auto &v : vertices)
  {
    auto vtxPoint = BRep_Tool::Pnt(TopoDS::Vertex(v));
    auto d = prmPoint.Distance(vtxPoint);
    if (d <= Precision::Confusion())
      return std::make_pair(v, 0.0);
  }
  
  //still here? just get the edge a parameter.
  auto edgeIndex = stow->spine->Index(prmIn);
  double edgePrm;
  stow->spine->Parameter(edgeIndex, prmIn, edgePrm);
  return std::make_pair(stow->spine->Edges(edgeIndex), edgePrm);
}

TopoDS_Wire Adapter::buildWire() const
{
  assert(isValid());
  BRepBuilderAPI_MakeWire wm;
  for (int index = 1; index <= stow->spine->NbEdges(); ++index)
    wm.Add(stow->spine->Edges(index));
  assert(wm.IsDone());
  return wm;
}

bool Adapter::isSpineEdge(const TopoDS_Edge &eIn) const
{
  return stow->spine->Index(eIn) != 0;
}
