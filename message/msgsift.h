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

#ifndef MSG_SIFT_H
#define MSG_SIFT_H

#include "message/message.h"

namespace msg
{
  /*! @struct Sift 
   * @brief Message filter.
   * 
   * @details This structure is used to filter
   * incoming messages so objects can respond
   * to only interested messages.
   * 
   * If sort is called after all inserts are made
   * then any received messages will cause a binary
   * search in mapPairs. Otherwise a linear search
   * will be performed. Probably any performance
   * gained from the binary search will be lost in
   * the need for std::string key values. 
   */
  struct Sift
  {
  public:
    typedef std::pair<Mask, Handler> MapPair;
    
    Sift() = default;
    Sift(std::initializer_list<MapPair>);
    void insert(Mask, Handler);
    void insert(std::initializer_list<MapPair>);
    void sort();
    void receive(const Message&) const;
    std::string name = "no name"; //used for any error messages.
  private:
    std::vector<MapPair> mapPairs;
    bool isSorted = false;
    mutable std::size_t stackDepth = 0;
  };
}

#endif // MSG_SIFT_H
