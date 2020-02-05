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

#include <iostream>
#include <cassert>
#include <memory>
#include <functional>

#include "subprojects/pmp-library/src/pmp/SurfaceMesh.h"
#include "subprojects/pmp-library/src/pmp/algorithms/SurfaceFeatures.h"
#include "subprojects/pmp-library/src/pmp/algorithms/SurfaceRemeshing.h"
#include "subprojects/pmp-library/src/pmp/algorithms/SurfaceHoleFilling.h"

#include "mesh/mshfillholespmp.h"

class Boundaries
{
public:
  Boundaries(pmp::SurfaceMesh &smIn) : sm(smIn)
  {
    //setup half edge boundary property.
    if (sm.has_halfedge_property("he:boundary"))
    {
      auto dummy = sm.halfedge_property<int>("he:boundary");
      sm.remove_halfedge_property(dummy);
    }
    auto hebp = sm.halfedge_property<int>("he:boundary", -1);
    
    //setup mesh property to designate outer boundary.
    //JFC. of course object property doesn't match the entity property interface.
    auto obip = sm.get_object_property<int>("o:outerBoundaryIndex");
    if (obip)
      sm.remove_object_property(obip);
    obip = sm.object_property("o:outerBoundaryIndex", -1);
    
    auto obsp = sm.get_object_property<pmp::Scalar>("o:outerBoundarySize"); //outer boundary property
    if (obsp)
      sm.remove_object_property(obsp);
    obsp = sm.object_property("o:outerBoundarySize", 0.0f);
    
    auto getUnassignedBoundaryHalfedge = [&]() -> pmp::Halfedge
    {
      for (auto he : sm.halfedges())
      {
        if (sm.is_boundary(he) && hebp[he] == -1)
          return he;
      }
      return pmp::Halfedge();
    };
    
    auto walkBoundary = [&](pmp::Halfedge he, int boundaryIndex)
    {
      std::vector<pmp::Halfedge> halfedges;
      pmp::BoundingBox bb;
      halfedges.push_back(he);
      bb += sm.position(sm.from_vertex(he));
      hebp[he] = boundaryIndex;
      he = sm.next_halfedge(he);
      while (he.is_valid() && he != halfedges.front())
      {
        halfedges.push_back(he);
        hebp[he] = boundaryIndex;
        bb += sm.position(sm.from_vertex(he));
        he = sm.next_halfedge(he);
      }
      if (obip[0] == -1 || bb.size() > obsp[0])
      {
        obip[0] = boundaryIndex;
        obsp[0] = bb.size();
      };
      
      //some debug messages.
//       std::cout << "half edge boundary size: " << halfedges.size() << std::endl;
//       std::cout << "bounding box size of boundary: " << bb.size() << std::endl;
    };
    
    int index = 0;
    auto che = getUnassignedBoundaryHalfedge(); //current half edge
    while (che.is_valid())
    {
//       std::cout << "index: " << index << std::endl;
      walkBoundary(che, index);
      che = getUnassignedBoundaryHalfedge();
      index++;
    }
  }
  
  /*! @brief Get interior boundary seeds.
   * 
   * @return A vector of halfedges. One for each interior boundary.
   * @note good input for fillHoles.
   * 
   */
  std::vector<pmp::Halfedge> getInteriorBoundarySeeds()
  {
    assert(sm.has_halfedge_property("he:boundary"));
    auto hebp = sm.halfedge_property<int>("he:boundary");
    auto obip = sm.get_object_property<int>("o:outerBoundaryIndex");
    assert(obip);
    std::vector<pmp::Halfedge> out;
    
    int boundaryIndex = 0;
    bool foundOne = true;
    while(foundOne)
    {
      foundOne = false;
      if (obip[0] == boundaryIndex)
        boundaryIndex++;
      for (auto he : sm.halfedges())
      {
        if (hebp[he] == boundaryIndex)
        {
          out.push_back(he);
          foundOne = true;
          break;
        }
      }
      boundaryIndex++;
    }
    
    return out;
  }
  
private:
  pmp::SurfaceMesh &sm;
};

void msh::srf::fillHoles(pmp::SurfaceMesh &mIn)
{
  Boundaries bs(mIn);
  auto seeds = bs.getInteriorBoundarySeeds();
  pmp::SurfaceHoleFilling hf(mIn);
  for (auto he : seeds)
    hf.fill_hole(he);
  
//   auto obip = mIn.get_object_property<int>("o:outerBoundaryIndex");
//   std::cout << "outer boundary index: " << obip[0] << std::endl;
}
