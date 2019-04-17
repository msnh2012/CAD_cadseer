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

#include <iostream>
#include <algorithm>

#include "message/msgsift.h"

using namespace msg;

Sift::Sift(std::initializer_list<MapPair> mIn)
{
  std::copy(mIn.begin(), mIn.end(), std::back_inserter(mapPairs));
}

void Sift::insert(Mask m, Handler h)
{
  mapPairs.push_back(std::make_pair(m, h));
  isSorted = false;
}

void Sift::insert(std::initializer_list<MapPair> mIn)
{
  std::copy(mIn.begin(), mIn.end(), std::back_inserter(mapPairs));
}

void Sift::sort()
{
  // yuck on the whole string conversion!
  std::sort
  (
    mapPairs.begin()
    , mapPairs.end()
    , [](const MapPair &lhs, const MapPair &rhs)
    {
      return lhs.first.to_string() < rhs.first.to_string();
    }
  );
  isSorted = true;
}

void Sift::receive(const Message &mIn) const
{
  Handler handler;
  if (isSorted)
  {
    auto it = std::lower_bound
    (
      mapPairs.begin()
      , mapPairs.end()
      , std::make_pair(mIn.mask, Handler())
      , [](const MapPair &lhs, const MapPair &rhs)
      {
        return lhs.first.to_string() < rhs.first.to_string();
      }
    );
    
    if (it != mapPairs.end() && it->first == mIn.mask)
      handler = it->second;
  }
  else
  {
    for (const auto &p : mapPairs)
    {
      if (p.first == mIn.mask)
      {
        handler = p.second;
        break; //only one function per mask.
      }
    }
  }
  
  if (!handler)
    return;
  if (stackDepth)
    std::cout << "WARNING: " << name << " stack depth: " << stackDepth << std::endl;
  stackDepth++;
  handler(mIn);
  stackDepth--;
}
