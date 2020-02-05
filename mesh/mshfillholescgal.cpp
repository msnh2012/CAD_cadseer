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

#define CGAL_NO_ASSERTIONS
#include <CGAL/Polygon_mesh_processing/triangulate_hole.h>
#include <CGAL/Polygon_mesh_processing/border.h>

#include "globalutilities.h"
#include "mesh/mshmesh.h"
#include "mesh/mshfillholescgal.h"

using Mesh = msh::srf::Mesh;
using Faces = msh::srf::Faces;
using HalfEdges = msh::srf::HalfEdges;
using Vertices = msh::srf::Vertices;
using BSphere = msh::srf::BSphere;

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

void msh::srf::fillHoles(msh::srf::Stow &sIn)
{
  //with this loop structure, we might try boundaries more than once.
  //so these stats could be wrong.
  int holesFilled = 0;
  int holesFailed = 0;
  
  auto mIn = sIn.mesh;
  
  std::vector<HalfEdges> boundaries = getBoundaries(mIn);
  boundaries.pop_back(); //remove outer boundary.
  
  while(!boundaries.empty())
  {
    std::size_t sizePrior = boundaries.size();
    
    Faces nf; //new faces
    Vertices nv; //new vertices
    bool success = CGAL::cpp11::get<0>
    (
      CGAL::Polygon_mesh_processing::triangulate_refine_and_fair_hole
      (
        mIn,
        //this is weird. if I use 'boundaries.back().front()' I get holes that don't fill.
        //I believe this is a bug in cgal as I have visually inspected the boundaries.
        boundaries.back().front(),
        std::back_inserter(nf),
        std::back_inserter(nv)
      )
    );
    if (success)
    {
      std::vector<HalfEdges> tb = getBoundaries(mIn); //temp boundaries
      tb.pop_back(); //remove outer boundary.
      if (tb.size() != sizePrior)
      {
        //worked.
        holesFilled++;
        boundaries = tb;
      }
      else
      {
        boundaries.pop_back(); //remove boundary that didn't work
      }
    }
    else
    {
      holesFailed++;
      boundaries.pop_back();
    }
  }
  
//   return std::make_pair(holesFilled, holesFailed);
}
