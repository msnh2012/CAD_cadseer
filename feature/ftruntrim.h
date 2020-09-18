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

#include "feature/ftrpick.h"
#include "feature/ftrbase.h"

namespace ann{class SeerShape;}
namespace prj{namespace srl{namespace utr{class Untrim;}}}
namespace lbr{class IPGroup; class PLabel;}

namespace ftr
{
  class Untrim : public Base
  {
  public:
    Untrim();
    ~Untrim() override;
    
    void updateModel(const UpdatePayload&) override;
    Type getType() const override {return Type::Untrim;}
    const std::string& getTypeString() const override {return toString(Type::Untrim);}
    const QIcon& getIcon() const override {return icon;}
    Descriptor getDescriptor() const override {return Descriptor::Create;}
    
    void serialWrite(const boost::filesystem::path&) override;
    void serialRead(const prj::srl::utr::Untrim&);
    
    void setPick(const Pick&);
    const Pick& getPick() const {return pick;}
    
  private:
    std::unique_ptr<ann::SeerShape> sShape;
    std::unique_ptr<prm::Parameter> offset;
    std::unique_ptr<prm::Parameter> closeU;
    std::unique_ptr<prm::Parameter> closeV;
    std::unique_ptr<prm::Parameter> makeSolid;
    Pick pick;
    
    osg::ref_ptr<lbr::PLabel> offsetLabel;
    osg::ref_ptr<lbr::PLabel> closeULabel;
    osg::ref_ptr<lbr::PLabel> closeVLabel;
    osg::ref_ptr<lbr::PLabel> makeSolidLabel;
    
    using uuid = boost::uuids::uuid;
    uuid solidId;
    uuid shellId;
    std::vector<uuid> uvEdgeIds; //ids from creating face from surface and u v ranges.
    std::map<uuid, std::pair<uuid, uuid>> edgeToCap; //first is face, second is wire.
    
    static QIcon icon;
  };
}

#endif //FTR_UNTRIM_H
