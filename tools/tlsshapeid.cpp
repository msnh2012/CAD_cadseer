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

#include <boost/uuid/uuid.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <TopoDS_Shape.hxx>

#include "tools/idtools.h"
#include "tools/occtools.h"
#include "tools/tlsshapeid.h"

using boost::uuids::uuid;

namespace
{
  namespace BMI = boost::multi_index;
  
  struct Record
  {
    uuid id;
    TopoDS_Shape shape;
    
    Record() :
      id(gu::createNilId()),
      shape(TopoDS_Shape())
      {}
      
    Record(const uuid &idIn, const TopoDS_Shape &shapeIn) :
      id(idIn),
      shape(shapeIn)
      {}
    
    //@{
    //! for tags
    struct ById{};
    struct ByShape{};
    //@}
  };
  
  struct Hash
  {
    std::size_t operator()(const TopoDS_Shape& shape) const
    {
      return static_cast<std::size_t>(occt::getShapeHash(shape));
    }
  };
  
  struct Equality
  {
    bool operator()(const TopoDS_Shape &shape1, const TopoDS_Shape &shape2) const
    {
      return shape1.IsSame(shape2);
    }
  };
  
  typedef boost::multi_index_container
  <
    Record,
    BMI::indexed_by
    <
      BMI::ordered_non_unique
      <
        BMI::tag<Record::ById>,
        BMI::member<Record, uuid, &Record::id>
      >,
      BMI::hashed_non_unique
      <
        BMI::tag<Record::ByShape>,
        BMI::member<Record, TopoDS_Shape, &Record::shape>,
        Hash,
        Equality
      >
    >
  > Container;
}

using namespace tls;

struct ShapeIdContainer::Stow : public Container{};

ShapeIdContainer::ShapeIdContainer()
: stow(std::make_unique<Stow>())
{}

ShapeIdContainer::~ShapeIdContainer() = default;

ShapeIdContainer::ShapeIdContainer(const ShapeIdContainer &rhs)
: stow(std::make_unique<Stow>())
{
  *stow = *rhs.stow;
}

ShapeIdContainer& ShapeIdContainer::operator=(const ShapeIdContainer &rhs)
{
  *stow = *rhs.stow;
  return *this;
}

ShapeIdContainer::ShapeIdContainer(ShapeIdContainer &&rhs) noexcept
: stow(std::move(rhs.stow))
{}

ShapeIdContainer& ShapeIdContainer::operator=(ShapeIdContainer &&rhs) noexcept
{
  stow = std::move(rhs.stow);
  return *this;
}

void ShapeIdContainer::insert(const boost::uuids::uuid &idIn, const TopoDS_Shape &ShapeIn)
{
  stow->insert(Record(idIn, ShapeIn));
}

bool ShapeIdContainer::has(const TopoDS_Shape &shapeIn)
{
  typedef Container::index<Record::ByShape>::type List;
  const List &list = stow->get<Record::ByShape>();
  List::const_iterator it = list.find(shapeIn);
  return it != list.end();
}

const boost::uuids::uuid& ShapeIdContainer::find(const TopoDS_Shape &shapeIn)
{
  typedef Container::index<Record::ByShape>::type List;
  const List &list = stow->get<Record::ByShape>();
  List::const_iterator it = list.find(shapeIn);
  assert(it != list.end());
  return it->id;
}

const TopoDS_Shape& ShapeIdContainer::find(const boost::uuids::uuid &idIn)
{
  typedef Container::index<Record::ById>::type List;
  const List &list = stow->get<Record::ById>();
  List::const_iterator it = list.find(idIn);
  assert(it != list.end());
  return it->shape;
}
