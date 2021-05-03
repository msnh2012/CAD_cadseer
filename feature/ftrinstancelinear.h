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

#ifndef FTR_INSTANCELINEAR_H
#define FTR_INSTANCELINEAR_H

#include <osg/ref_ptr>

#include "feature/ftrbase.h"

namespace prj{namespace srl{namespace inls{class InstanceLinear;}}}

namespace ftr
{
  namespace InstanceLinear
  {
    namespace Tags
    {
      inline constexpr std::string_view Source = "Source";
      inline constexpr std::string_view XOffset = "XOffset";
      inline constexpr std::string_view YOffset = "YOffset";
      inline constexpr std::string_view ZOffset = "ZOffset";
      inline constexpr std::string_view XCount = "XCount";
      inline constexpr std::string_view YCount = "YCount";
      inline constexpr std::string_view ZCount = "ZCount";
      inline constexpr std::string_view IncludeSource = "IncludeSource";
    }
    
    class Feature : public Base
    {
    public:
      Feature();
      ~Feature() override;
      
      void updateModel(const UpdatePayload&) override;
      Type getType() const override {return Type::InstanceLinear;}
      const std::string& getTypeString() const override {return toString(Type::InstanceLinear);}
      const QIcon& getIcon() const override {return icon;}
      Descriptor getDescriptor() const override {return Descriptor::Create;}
      
      void serialWrite(const boost::filesystem::path&) override;
      void serialRead(const prj::srl::inls::InstanceLinear&);
    private:
      static QIcon icon;
      struct Stow;
      std::unique_ptr<Stow> stow;
    };
  }
}

#endif // FTR_INSTANCELINEAR_H
