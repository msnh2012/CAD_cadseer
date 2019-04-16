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

#ifndef LBR_RANGE_H
#define LBR_RANGE_H

namespace lbr
{
  /**
   * @struct RangeMask
   * @brief struct for testing of inclusion in range.
   */
  struct RangeMask
  {
    double start = 0.0;
    double finish = 0.0;
    RangeMask(){}
    RangeMask(double startIn, double finishIn):
    start(std::min(startIn, finishIn))
    ,finish(std::max(startIn, finishIn))
    {}
    bool isIn(double test) const
    {
      if (test >= start && test <= finish)
        return true;
      return false;
    }
    bool isOut(double test) const
    {
      return !isIn(test);
    }
    bool isBelow(double test) const
    {
      return test < start;
    }
    bool isAbove(double test) const
    {
      return test > finish;
    }
    bool isValid() const
    {
      return finish > start;
    }
  };
  
  inline bool operator==(const RangeMask& lhs, const RangeMask& rhs)
  {
    if (lhs.start == rhs.start && lhs.finish == rhs.finish)
      return true;
    return false;
  }
}

#endif // LBR_RANGE_H
