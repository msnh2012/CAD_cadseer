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

#ifndef FTR_REVOLVE_H
#define FTR_REVOLVE_H

#include "feature/ftrbase.h"

namespace prj{namespace srl{namespace rvls{class Revolve;}}}

namespace ftr
{
  namespace Revolve
  {
    namespace PrmTags
    {
      inline constexpr std::string_view axisType = "axisType";
      inline constexpr std::string_view profilePicks = "profilePicks";
      inline constexpr std::string_view axisPicks = "axisPicks";
      inline constexpr std::string_view axisOrigin = "axisOrigin";
      inline constexpr std::string_view axisDirection = "axisDirection";
    }
    class Feature : public Base
    {
    public:
      Feature();
      ~Feature() override;
      
      void updateModel(const UpdatePayload&) override;
      Type getType() const override {return Type::Revolve;}
      const std::string& getTypeString() const override {return toString(Type::Revolve);}
      const QIcon& getIcon() const override {return icon;}
      Descriptor getDescriptor() const override {return Descriptor::Create;}
      
      void serialWrite(const boost::filesystem::path&) override;
      void serialRead(const prj::srl::rvls::Revolve&);
    private:
      static QIcon icon;
      struct Stow;
      std::unique_ptr<Stow> stow;
    };
  }
}

#endif // FTR_REVOLVE_H
