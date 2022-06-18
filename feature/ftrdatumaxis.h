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

#ifndef FTR_DATUMAXIS_H
#define FTR_DATUMAXIS_H

#include "feature/ftrbase.h"

namespace prj{namespace srl{namespace dtas{class DatumAxis;}}}

namespace ftr
{
  class UpdatePayload;
  namespace DatumAxis
  {
    enum AxisType //keep in sync prm::Parameter enum.
    {
      Constant, //!< doesn't reference anything
      Parameters, //!< origin and direction available to edit or expression link.
      Linked, //!< linked to axis of input feature csys. X, Y or Z.
      Points, //!< 2 points
      Intersection, //!< intersection of 2 planes.
      Geometry, //!< 1 pick of geometry. ie. cylindrical face.
      PointNormal //!< Through point normal to face
    };
    namespace Tags
    {
      inline constexpr std::string_view AxisType = "AxisType";
      inline constexpr std::string_view Linked = "Linked";
      inline constexpr std::string_view Points = "Points";
      inline constexpr std::string_view Intersection = "Intersection";
      inline constexpr std::string_view Geometry = "Geometry";
      inline constexpr std::string_view LinkedAxis = "LinkedAxis"; //enum
      inline constexpr std::string_view PointNormal = "PointNormal";
    }
    
    class Feature : public Base
    {
    public:
      Feature();
      ~Feature() override;
      
      void updateModel(const UpdatePayload&) override;
      void updateVisual() override;
      Type getType() const override {return Type::DatumAxis;}
      const std::string& getTypeString() const override {return toString(Type::DatumAxis);}
      const QIcon& getIcon() const override {return icon;}
      Descriptor getDescriptor() const override {return Descriptor::Create;}
      void serialWrite(const boost::filesystem::path&) override;
      void serialRead(const prj::srl::dtas::DatumAxis &);
    private:
      static QIcon icon;
      struct Stow;
      std::unique_ptr<Stow> stow;
    };
  }
}

#endif // FTR_DATUMAXIS_H
