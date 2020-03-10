/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#include "feature/ftrbase.h"
#include "feature/ftrupdatepayload.h"

using namespace ftr;

std::vector<const Base*> UpdatePayload::getFeatures(const std::string &tag) const
{
  return getFeatures(updateMap, tag);
}

std::vector<const Base*> UpdatePayload::getFeatures(const UpdateMap &updateMapIn, const std::string &tag)
{
  std::vector<const Base*> out;
  if (tag.empty())
  {
    for (const auto &p : updateMapIn)
      out.push_back(p.second);
  }
  else
  {
    for (auto its = updateMapIn.equal_range(tag); its.first != its.second; ++its.first)
      out.push_back(its.first->second);
  }
  
  std::sort(out.begin(), out.end());
  auto last = std::unique(out.begin(), out.end());
  out.erase(last, out.end());
  
  return out;
}
