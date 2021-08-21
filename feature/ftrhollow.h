/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef FTR_HOLLOW_H
#define FTR_HOLLOW_H

#include <TopTools_ListOfShape.hxx>

#include "feature/ftrpick.h"
#include "feature/ftrbase.h"

class BRepOffsetAPI_MakeThickSolid;

namespace prj{namespace srl{namespace hlls{class Hollow;}}}
namespace ftr
{
  namespace Hollow
  {
    namespace InputTags
    {
      inline constexpr std::string_view pick = "pick";
    }
    class Feature : public Base
    {
    public:
      Feature();
      ~Feature() override;
      void updateModel(const UpdatePayload&) override;
      Type getType() const override {return Type::Hollow;}
      const std::string& getTypeString() const override {return toString(Type::Hollow);}
      const QIcon& getIcon() const override {return icon;}
      Descriptor getDescriptor() const override {return Descriptor::Alter;}
      void serialWrite(const boost::filesystem::path&) override;
      void serialRead(const prj::srl::hlls::Hollow&);
    private:
      static QIcon icon;
      struct Stow;
      std::unique_ptr<Stow> stow;
    };
  }
}

#endif // FTR_HOLLOW_H
