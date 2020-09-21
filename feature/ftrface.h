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

#ifndef FTR_FACE_H
#define FTR_FACE_H

#include "feature/ftrpick.h"
#include "feature/ftrbase.h"

namespace ann{class SeerShape;}
namespace prj{namespace srl{namespace fce{class Face;}}}

namespace ftr
{
  class Face : public Base
  {
  public:
    Face();
    ~Face() override;
    
    void updateModel(const UpdatePayload&) override;
    Type getType() const override {return Type::Face;}
    const std::string& getTypeString() const override {return toString(Type::Face);}
    const QIcon& getIcon() const override {return icon;}
    Descriptor getDescriptor() const override {return Descriptor::Create;}
    
    void serialWrite(const boost::filesystem::path&) override;
    void serialRead(const prj::srl::fce::Face&);
    
    void setPicks(const Picks&);
    const Picks& getPicks() const {return picks;}
    
  private:
    std::unique_ptr<ann::SeerShape> sShape;
    Picks picks;
    
    using uuid = boost::uuids::uuid;
    uuid faceId;
    uuid wireId;
    
    static QIcon icon;
  };
}

#endif //FTR_FACE_H
