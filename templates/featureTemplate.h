/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) %YEAR% Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef FTR_%CLASSNAMEUPPERCASE%_H
#define FTR_%CLASSNAMEUPPERCASE%_H

// #include "feature/ftrpick.h"
#include "feature/ftrbase.h"

// namespace ann{class SeerShape;}
// namespace prj{namespace srl{namespace FIXME{class %CLASSNAME%;}}}
// namespace lbr{class IPGroup; class PLabel;}
// namespace prm{struct Observer;}

namespace ftr
{
  class %CLASSNAME% : public Base
  {
  public:
    %CLASSNAME%();
    ~%CLASSNAME%() override;
    
    void updateModel(const UpdatePayload&) override;
    Type getType() const override {return Type::%CLASSNAME%;}
    const std::string& getTypeString() const override {return toString(Type::%CLASSNAME%);}
    const QIcon& getIcon() const override {return icon;}
    Descriptor getDescriptor() const override {return Descriptor::Create;}
    
    void serialWrite(const boost::filesystem::path&) override;
//     void serialRead(const prj::srl::FIXME::%CLASSNAME%&);
    
//     void setPicks(const Picks&);
//     const Picks& getPicks() const {return picks;}
    
  private:
//     std::unique_ptr<ann::SeerShape> sShape;
//     std::unique_ptr<prm::Parameter> direction;
//     std::unique_ptr<prm::Parameter> distance;
//     std::unique_ptr<prm::Parameter> csys;
//     std::unique_ptr<prm::Observer> prmObserver;
//     Picks picks;
//     std::unique_ptr<prm::Parameter> picks; //experimental 
    
//     std::unique_ptr<ann::CSysDragger> csysDragger;
    
//     osg::ref_ptr<lbr::PLabel> directionLabel;
//     osg::ref_ptr<lbr::IPGroup> distanceIPGroup;
    
    static QIcon icon;
  };
}

#endif //FTR_%CLASSNAMEUPPERCASE%_H
