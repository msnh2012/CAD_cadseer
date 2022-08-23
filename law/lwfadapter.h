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

#ifndef LWF_ADAPTER_H
#define LWF_ADAPTER_H

#include <memory>

namespace lwf
{
  /*! @struct Adapter
   * @brief Bridge between seer shape occt fillet spine.
   */
  struct Adapter
  {
    Adapter() = delete;
    Adapter(const ann::SeerShape&, const boost::uuids::uuid&);
    Adapter(Adapter&&) noexcept;
//     Adapter(const ann::SeerShape&, const TopoDS_Edge&);
    ~Adapter();
    Adapter& operator=(Adapter &&) noexcept;
    
    bool isValid() const; //call after construction to make sure everything is good.
    std::pair<double, double> getRange() const;
    bool isPeriodic() const;
    gp_Pnt location(double) const;
    std::optional<double> toSpineParameter(const TopoDS_Vertex&) const;
    std::optional<double> toSpineParameter(const TopoDS_Edge&, double) const;
    std::pair<TopoDS_Shape, double> fromSpineParameter(double) const;
    TopoDS_Wire buildWire() const;
    bool isSpineEdge(const TopoDS_Edge&) const;
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
  };
}

#endif
