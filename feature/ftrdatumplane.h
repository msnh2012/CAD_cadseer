/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef FTR_DATUMPLANE_H
#define FTR_DATUMPLANE_H

#include <osg/ref_ptr>

#include "feature/ftrpick.h"
#include "feature/ftrbase.h"

namespace prj{namespace srl{namespace dtps{class DatumPlane;}}}

namespace ftr
{
  class UpdatePayload;
  class ShapeHistory;
  
  namespace DatumPlane
  {
    namespace InputTags
    {
      inline constexpr std::string_view axis = "axis";
      inline constexpr std::string_view center = "center";
      inline constexpr std::string_view plane = "plane";
      inline constexpr std::string_view point = "point";
    }
    namespace PrmTags
    {
      inline constexpr std::string_view datumPlaneType = "datumPlaneType";
      inline constexpr std::string_view offsetPicks = "offsetPicks";
      inline constexpr std::string_view centerPicks = "centerPicks";
      inline constexpr std::string_view axisAnglePicks = "axisAnglePicks";
      inline constexpr std::string_view averagePicks = "averagePicks";
      inline constexpr std::string_view pointsPicks = "pointsPicks";
    }
    
    enum class DPType //keep in sync with parameter strings in constructor.
    {
      Constant = 0 //!< no links
      , Linked //!< linked to input feature.
      , Offset //!< offset from 1 planar face.
      , Center //!< 2 planar inputs. center, bisect
      , AxisAngle //!< through axis angle to planar
      , Average //!< average of three planes.
      , Points //!< through 3 points
//       , Tangent //!< 1 face pick(use normal at uv), 1 face pick and 1 point pick
//       , PofC //!< plane of curve. xy plane of conics
    };
    
    class Feature : public Base
    {
    public:
      Feature();
      ~Feature();
      
      void updateModel(const UpdatePayload&) override;
      void updateVisual() override;
      Type getType() const override {return Type::DatumPlane;}
      const std::string& getTypeString() const override {return toString(Type::DatumPlane);}
      const QIcon& getIcon() const override {return icon;}
      Descriptor getDescriptor() const override {return Descriptor::Create;}
      void serialWrite(const boost::filesystem::path&) override;
      void serialRead(const prj::srl::dtps::DatumPlane&);
      QTextStream& getInfo(QTextStream &) const override;
    private:
      static QIcon icon;
      
      struct Stow;
      std::unique_ptr<Stow> stow;
    };
  }
}

#endif // FTR_DATUMPLANE_H
