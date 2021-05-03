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

#ifndef FTR_DATUMSYSTEM_H
#define FTR_DATUMSYSTEM_H

#include "feature/ftrpick.h"
#include "feature/ftrbase.h"

namespace osg{class Matrixd; class PositionAttitudeTransform;}

namespace prm{struct Observer;}
namespace mdv{class DatumSystem;}
namespace lbr{class PLabel;}
namespace ann{class CSysDragger;}
namespace prj{namespace srl{namespace dtms{class DatumSystem;}}}

/* right now the only offset is just a 3d vector offsetVector. This is only temporary.
 * We will add an orientation or promote it to a full blown matrixd.
 * This should be done after the expressions for those 2 are working.
 */

namespace ftr
{
  namespace DatumSystem
  {
    enum SystemType
    {
      Constant = 0
      , Linked
      , Through3Points
    };
    namespace Tags
    {
      inline constexpr std::string_view SystemType = "SystemType";
      inline constexpr std::string_view Linked = "Linked";
      inline constexpr std::string_view Points = "Points";
    }
    
    class Feature : public Base
    {
    public:
      Feature();
      ~Feature() override;
      
      void updateModel(const UpdatePayload&) override;
      void updateVisual() override;
      Type getType() const override {return Type::DatumSystem;}
      const std::string& getTypeString() const override {return toString(Type::DatumSystem);}
      const QIcon& getIcon() const override {return icon;}
      Descriptor getDescriptor() const override {return Descriptor::Create;}
      
      void serialWrite(const boost::filesystem::path&) override;
      void serialRead(const prj::srl::dtms::DatumSystem&);
    private:
      static QIcon icon;
      struct Stow;
      std::unique_ptr<Stow> stow;
    };
  }
}

#endif //FTR_DATUMSYSTEM_H
