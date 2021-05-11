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

#ifndef FTR_SKETCH_H
#define FTR_SKETCH_H

#include "feature/ftrbase.h"

namespace prj{namespace srl{namespace skts{class Sketch;}}}
namespace skt{class Visual;}

namespace ftr
{
  namespace Sketch
  {
    class Feature : public Base
    {
    public:
      Feature();
      ~Feature() override;
      
      void updateModel(const UpdatePayload&) override;
      Type getType() const override {return Type::Sketch;}
      const std::string& getTypeString() const override {return toString(Type::Sketch);}
      const QIcon& getIcon() const override {return icon;}
      Descriptor getDescriptor() const override {return Descriptor::Create;}
      void serialWrite(const boost::filesystem::path&) override;
      void serialRead(const prj::srl::skts::Sketch&);
      
      void buildDefault(const osg::Matrixd&, double); //used in command
      skt::Visual* getVisual(); //commandview
      void addHPPair(uint32_t, const std::shared_ptr<prm::Parameter>&); //commandview
      void removeHPPair(const prm::Parameter*); //commandview
      void updateConstraintValue(const prm::Parameter*); //commandview
      
      void draggerShow();
      void draggerHide();
    private:
      static QIcon icon;
      struct Stow;
      std::unique_ptr<Stow> stow;
    };
  }
}

#endif // FTR_SKETCH_H
