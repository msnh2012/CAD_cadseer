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

#ifndef FTR_EXTRUDE_H
#define FTR_EXTRUDE_H

#include "feature/ftrpick.h"
#include "feature/ftrbase.h"

namespace ann{class SeerShape;}
namespace prm{struct Observer;}
namespace lbr{class IPGroup; class PLabel;}
namespace occt{class BoundingBox;}
namespace prj{namespace srl{namespace exrs{class Extrude;}}}

namespace ftr
{
  /**
  * @todo write docs
  */
  class Extrude : public Base
  {
  public:
    constexpr static const char *axisName = "axis";
    
    enum class DirectionType
    {
      Infer //!< Infer the direction from the geometry to extrude.
      , Picks //!< Direction is described by axisPicks and payload inputs.
      , Parameter //!< Direction is like a regular parameter
    };
    
    Extrude();
    virtual ~Extrude() override;
    
    virtual void updateModel(const UpdatePayload&) override;
    virtual Type getType() const override {return Type::Extrude;}
    virtual const std::string& getTypeString() const override {return toString(Type::Extrude);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
    
    virtual void serialWrite(const boost::filesystem::path&) override;
    void serialRead(const prj::srl::exrs::Extrude&);
    
    void setPicks(const Picks&);
    const Picks& getPicks() const {return picks;}
    void setAxisPicks(const Picks&);
    const Picks& getAxisPicks() {return axisPicks;}
    void setDirectionType(DirectionType);
    DirectionType getDirectionType(){return directionType;}
    
  protected:
    Picks picks;
    Picks axisPicks; // array because might be 2 points.
    std::unique_ptr<ann::SeerShape> sShape;
    std::unique_ptr<prm::Parameter> direction;
    std::unique_ptr<prm::Parameter> distance;
    std::unique_ptr<prm::Parameter> offset;
    
    std::unique_ptr<prm::Observer> directionObserver;
    
    osg::ref_ptr<lbr::PLabel> directionLabel;
    osg::ref_ptr<lbr::IPGroup> distanceLabel;
    osg::ref_ptr<lbr::IPGroup> offsetLabel;
    osg::ref_ptr<osg::Switch> internalSwitch; //separate switch from overlay switch.
    
    DirectionType directionType = DirectionType::Infer;
    
    std::map<boost::uuids::uuid, boost::uuids::uuid> originalMap; //map inputs to equivalents in the output.
    std::map<boost::uuids::uuid, boost::uuids::uuid> generatedMap; //map transition shapes.
    std::map<boost::uuids::uuid, boost::uuids::uuid> lastMap; //map 'top' shapes.
    std::map<boost::uuids::uuid, boost::uuids::uuid> oWireMap; //map new face to outer wire.
    
    void setupLabels();
    void updateLabels(occt::BoundingBox&);
    void updateLabelVisibility();
    
  private:
    static QIcon icon;
  };
}

#endif // FTR_EXTRUDE_H
