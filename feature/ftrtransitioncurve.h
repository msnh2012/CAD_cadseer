/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2019 Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef FTR_TRANSITIONCURVE_H
#define FTR_TRANSITIONCURVE_H

#include "feature/ftrpick.h"
#include "feature/ftrbase.h"

class TopoDS_Edge;

namespace ann{class SeerShape;}
namespace lbr{class PLabel;}
namespace prj{namespace srl{namespace tscs{class TransitionCurve;}}}

namespace ftr
{
  namespace TransitionCurve
  {
    namespace InputTags
    {
      inline constexpr std::string_view pick = "pick";
    }
    namespace PrmTags
    {
      inline constexpr std::string_view direction0 = "direction0";
      inline constexpr std::string_view direction1 = "direction1";
      inline constexpr std::string_view magnitude0 = "magnitude0";
      inline constexpr std::string_view magnitude1 = "magnitude1";
      inline constexpr std::string_view autoScale = "autoScale";
    }
    class Feature : public Base
    {
    public:
      Feature();
      ~Feature() override;
      
      void updateModel(const UpdatePayload&) override;
      Type getType() const override {return Type::TransitionCurve;}
      const std::string& getTypeString() const override {return toString(Type::TransitionCurve);}
      const QIcon& getIcon() const override {return icon;}
      Descriptor getDescriptor() const override {return Descriptor::Create;}
      
      void serialWrite(const boost::filesystem::path&) override;
      void serialRead(const prj::srl::tscs::TransitionCurve&);
      
      void gleanDirection0(const TopoDS_Edge&);
      void gleanDirection1(const TopoDS_Edge&);
    private:
      static QIcon icon;
      struct Stow;
      std::unique_ptr<Stow> stow;
    };
  }
}

#endif //FTR_TRANSITIONCURVE_H
