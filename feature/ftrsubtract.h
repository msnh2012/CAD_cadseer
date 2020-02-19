/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2015  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef FTR_SUBTRACT_H
#define FTR_SUBTRACT_H

#include "feature/ftrpick.h"
#include "feature/ftrbase.h"

namespace prj{namespace srl{namespace sbts{class Subtract;}}}
namespace ann{class SeerShape; class IntersectionMapper;}

namespace ftr
{
  class Subtract : public Base
  {
  public:
    Subtract();
    virtual ~Subtract() override;
    virtual void updateModel(const UpdatePayload&) override;
    virtual Type getType() const override {return Type::Subtract;}
    virtual const std::string& getTypeString() const override {return toString(Type::Subtract);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Alter;}
    
    virtual void serialWrite(const boost::filesystem::path&) override;
    void serialRead(const prj::srl::sbts::Subtract&);
    
    void setTargetPicks(const Picks&);
    const Picks& getTargetPicks(){return targetPicks;}
    void setToolPicks(const Picks&);
    const Picks& getToolPicks(){return toolPicks;}
    
  protected:
    std::unique_ptr<ann::SeerShape> sShape;
    std::unique_ptr<ann::IntersectionMapper> iMapper;
    
    Picks targetPicks;
    Picks toolPicks;
    
  private:
    static QIcon icon;
  };
}

#endif // FTR_SUBTRACT_H
