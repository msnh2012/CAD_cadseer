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


#ifndef TLS_SHAPEID_H
#define TLS_SHAPEID_H

#include <memory>

namespace boost{namespace uuids{struct uuid;}}
class TopoDS_Shape;

namespace tls
{
  /*!@class ShapeIdContainer
   * @brief Mapping between ids and shapes
   * 
   * Used for temporarily mapping between shapes and ids before
   * seershape can be involved.
   * 
   */
  class ShapeIdContainer
  {
  public:
    ShapeIdContainer();
    ~ShapeIdContainer();
    ShapeIdContainer(const ShapeIdContainer&); //copy
    ShapeIdContainer& operator=(const ShapeIdContainer&); //copy
    ShapeIdContainer(ShapeIdContainer&&) noexcept; //move
    ShapeIdContainer& operator=(ShapeIdContainer&&) noexcept; //move
    
    void insert(const boost::uuids::uuid&, const TopoDS_Shape&);
    bool has(const TopoDS_Shape&);
    const boost::uuids::uuid& find(const TopoDS_Shape&);
    const TopoDS_Shape& find(const boost::uuids::uuid&);
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
  };

}

#endif //TLS_SHAPEID_H
