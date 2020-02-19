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

namespace lbr{class PLabel;}
namespace prj{namespace srl{namespace hlls{class Hollow;}}}
namespace ann{class SeerShape;}
namespace ftr
{
  class Hollow : public Base
  {
  public:
    Hollow();
    virtual ~Hollow() override;
    virtual void updateModel(const UpdatePayload&) override;
    virtual Type getType() const override {return Type::Hollow;}
    virtual const std::string& getTypeString() const override {return toString(Type::Hollow);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Alter;}
    virtual void serialWrite(const boost::filesystem::path&) override;
    void serialRead(const prj::srl::hlls::Hollow&);
    
    void setHollowPicks(const Picks&);
    const Picks& getHollowPicks() const {return hollowPicks;}
    void setOffset(double);
    prm::Parameter& getOffset() const;
  private:
    static QIcon icon;
    std::unique_ptr<prm::Parameter> offset;
    osg::ref_ptr<lbr::PLabel> label; //!< graphic icon
    Picks hollowPicks;
    std::unique_ptr<ann::SeerShape> sShape;
    
    /*! used to map new geometry to old geometry. this will end up
     * more complicated than this as hollow can make splits.
     */ 
    std::map<boost::uuids::uuid, boost::uuids::uuid> shapeMap;
    
    void generatedMatch(BRepOffsetAPI_MakeThickSolid&, const ann::SeerShape &);
  };
}

#endif // FTR_HOLLOW_H
