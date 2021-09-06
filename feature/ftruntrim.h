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

#ifndef FTR_UNTRIM_H
#define FTR_UNTRIM_H

#include "feature/ftrbase.h"

namespace prj{namespace srl{namespace utr{class Untrim;}}}

namespace ftr
{
  namespace Untrim
  {
    namespace InputTags
    {
      inline constexpr std::string_view pick = "pick";
    }
    namespace PrmTags
    {
      inline constexpr std::string_view closeU = "closeU"; //input face to deform, can be null
      inline constexpr std::string_view closeV = "closeV"; //input face to deform, can be null
      inline constexpr std::string_view makeSolid = "makeSolid"; //input face to deform, can be null
    }
    class Feature : public Base
    {
    public:
      Feature();
      ~Feature() override;
      
      void updateModel(const UpdatePayload&) override;
      Type getType() const override {return Type::Untrim;}
      const std::string& getTypeString() const override {return toString(Type::Untrim);}
      const QIcon& getIcon() const override {return icon;}
      Descriptor getDescriptor() const override {return Descriptor::Create;}
      
      void serialWrite(const boost::filesystem::path&) override;
      void serialRead(const prj::srl::utr::Untrim&);
    private:
      static QIcon icon;
      struct Stow;
      std::unique_ptr<Stow> stow;
    };
  }
}

#endif //FTR_UNTRIM_H
