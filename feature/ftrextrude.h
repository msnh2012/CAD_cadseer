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

#ifndef FTR_EXTRUDE_H
#define FTR_EXTRUDE_H

#include "feature/ftrbase.h"

namespace prj{namespace srl{namespace exrs{class Extrude;}}}

namespace ftr
{
  namespace Extrude
  {
    namespace InputTags
    {
      inline constexpr std::string_view axis = "axis";
      inline constexpr std::string_view profile = "profile";
    }
    namespace PrmTags
    {
      inline constexpr std::string_view extrusionType = "extrusionType";
      inline constexpr std::string_view solid = "solid";
      inline constexpr std::string_view reverse = "reverse";
      inline constexpr std::string_view profilePicks = "profilePicks";
      inline constexpr std::string_view axisPicks = "axisPicks";
    }
    enum class ExtrusionType
    {
      Constant //!< vector parameter
      , Infer //!< Infer the direction from the geometry to extrude.
      , Picks //!< Direction is described by axisPicks and payload inputs.
    };
    
    class Feature : public Base
    {
    public:
      Feature();
      ~Feature() override;
      
      void updateModel(const UpdatePayload&) override;
      Type getType() const override {return Type::Extrude;}
      const std::string& getTypeString() const override {return toString(Type::Extrude);}
      const QIcon& getIcon() const override {return icon;}
      Descriptor getDescriptor() const override {return Descriptor::Create;}
      
      void serialWrite(const boost::filesystem::path&) override;
      void serialRead(const prj::srl::exrs::Extrude&);
    private:
      static QIcon icon;
      struct Stow;
      std::unique_ptr<Stow> stow;
    };
  }
}

#endif // FTR_EXTRUDE_H
