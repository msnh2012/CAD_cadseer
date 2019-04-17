/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2019  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef FTR_LINE_H
#define FTR_LINE_H

#include "feature/ftrpick.h"
#include "feature/ftrbase.h"

namespace ann{class SeerShape;}
namespace prj{namespace srl{class FeatureLine;}}

namespace ftr
{
  class Line : public Base
  {
  public:
    constexpr static const char *pickZero = "pickZero";
    constexpr static const char *pickOne = "pickOne";
    
    Line();
    virtual ~Line() override;
    
    virtual void updateModel(const UpdatePayload&) override;
    virtual Type getType() const override {return Type::Line;}
    virtual const std::string& getTypeString() const override {return toString(Type::Line);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
    
    virtual void serialWrite(const boost::filesystem::path&) override;
    void serialRead(const prj::srl::FeatureLine&);
    
    void setPicks(const Picks&);
    const Picks& getPicks() const {return picks;}
    
  protected:
    Picks picks;
    std::unique_ptr<ann::SeerShape> sShape;
    boost::uuids::uuid lineId;
  private:
    static QIcon icon;
  };
}



#endif //FTR_LINE_H
