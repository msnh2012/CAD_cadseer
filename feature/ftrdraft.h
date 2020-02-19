/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef FTR_DRAFT_H
#define FTR_DRAFT_H

#include <osg/ref_ptr>

#include "feature/ftrpick.h"
#include "feature/ftrbase.h"

class BRepOffsetAPI_DraftAngle;
class TopoDS_Face;
class gp_Pln;

namespace lbr{class PLabel;}
namespace prj{namespace srl{namespace drfs{class Draft;}}}
namespace ann{class SeerShape;}
namespace ftr
{
  class Draft : public Base
  {
    public:
      constexpr static const char *neutral = "neutral";
      
      Draft();
      ~Draft();
      
      virtual void updateModel(const UpdatePayload&) override;
      virtual Type getType() const override {return Type::Draft;}
      virtual const std::string& getTypeString() const override {return toString(Type::Draft);}
      virtual const QIcon& getIcon() const override {return icon;}
      virtual Descriptor getDescriptor() const override {return Descriptor::Alter;}
      virtual void serialWrite(const boost::filesystem::path&) override;
      void serialRead(const prj::srl::drfs::Draft&);
      
      void setTargetPicks(const Picks&);
      const Picks& getTargetPicks() const {return targetPicks;}
      void setNeutralPick(const Pick&);
      const Pick& getNeutralPick() const {return neutralPick;}
      
      std::shared_ptr<prm::Parameter> getAngleParameter() const {return angle;}
      void setAngleParameter(std::shared_ptr<prm::Parameter>);
      
    protected:
      Picks targetPicks;
      Pick neutralPick;
      
      std::unique_ptr<ann::SeerShape> sShape;
      
      std::shared_ptr<prm::Parameter> buildAngleParameter();
      std::shared_ptr<prm::Parameter> angle; //!< parameter containing draft angle.
      osg::ref_ptr<lbr::PLabel> label; //!< graphic icon
    private:
      static QIcon icon;
      void generatedMatch(BRepOffsetAPI_DraftAngle&, const ann::SeerShape &);
      gp_Pln derivePlaneFromShape(const TopoDS_Shape &);
  };
}

#endif // FTR_DRAFT_H
