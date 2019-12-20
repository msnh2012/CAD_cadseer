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

namespace mdv{class DatumSystem;}
namespace lbr{class PLabel;}
namespace ann{class CSysDragger;}
namespace prj{namespace srl{class FeatureDatumSystem;}}

/* right now the only offset is just a 3d vector offsetVector. This is only temporary.
 * We will add an orientation of promote it to a full blown matrixd.
 * This should be done after the expressions for those 2 are working.
 */

namespace ftr
{
  namespace DatumSystem
  {
    constexpr static const char *point0 = "point0";
    constexpr static const char *point1 = "point1";
    constexpr static const char *point2 = "point2";
    enum SystemType
    {
      Constant = 0
      , Linked
      , Through3Points
    };
    
    struct Cue
    {
      SystemType systemType;
      Picks picks;
    };
    
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
      void serialRead(const prj::srl::FeatureDatumSystem&);
      
      void setCue(const Cue&);
      const Cue& getCue() const {return cue;}
      void setCSys(const osg::Matrixd&); //converts to constant.
      osg::Matrixd getCSys() const;
      void setSize(double);
      void setAutoSize(bool);
      
    private:
      static QIcon icon;
      Cue cue;
      std::unique_ptr<prm::Parameter> csys;
      std::unique_ptr<prm::Parameter> autoSize;
      std::unique_ptr<prm::Parameter> size;
      std::unique_ptr<prm::Parameter> offsetVector;
      std::unique_ptr<ann::CSysDragger> csysDragger;
      
      osg::ref_ptr<mdv::DatumSystem> display;
      osg::ref_ptr<lbr::PLabel> autoSizeLabel;
      osg::ref_ptr<lbr::PLabel> sizeLabel;
      osg::ref_ptr<lbr::PLabel> offsetVectorLabel;
      osg::ref_ptr<osg::PositionAttitudeTransform> scale;
      
      void updateNone(const UpdatePayload&);
      void updateLinked(const UpdatePayload&);
      void update3Points(const UpdatePayload&);
      void updateVisualInternal();
    };
  }
}

#endif //FTR_DATUMSYSTEM_H
