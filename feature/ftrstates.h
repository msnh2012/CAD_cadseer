/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2015  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef FTR_STATES_H
#define FTR_STATES_H

#include <bitset>

namespace ftr
{
  //note feature state data is divided between each feature
  //and the project. project stores Inactive and NonLeaf.
  //Feature stores the rest.
  //dagview combines them.
  typedef std::bitset<16> State;
  namespace StateOffset
  {
    static const std::size_t ModelDirty =       0;
    static const std::size_t VisualDirty =      1;
    static const std::size_t Failure =          2;
    static const std::size_t Skipped =          3;
    static const std::size_t Editing =          4;
    static const std::size_t NotSelectable =    5;
    static const std::size_t Inactive =         13; //stored in project
    static const std::size_t NonLeaf =          14; //stored in project
    static const std::size_t Loading =          15; //used in project
    
    static const State FeatureMask("0000000000111111");
    static const State ProjectMask("0110000000000000");
  }
}

#endif // FTR_STATES_H
