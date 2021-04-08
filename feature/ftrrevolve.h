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

#ifndef FTR_REVOLVE_H
#define FTR_REVOLVE_H

#include "feature/ftrpick.h"
#include "feature/ftrbase.h"

namespace ann{class SeerShape;}
namespace lbr{class PLabel;}
namespace occt{class BoundingBox;}
namespace prj{namespace srl{namespace rvls{class Revolve;}}}
namespace prm{struct Observer;}

namespace ftr
{
  /**
  * @todo write docs
  */
  class Revolve : public Base
  {
  public:
    constexpr static const char *axisName = "axis";
    
    enum class AxisType
    {
      Picks //!< axis is described by axisPicks and payload inputs.
      , Parameter //!< axis is like a regular parameter
    };
    
    Revolve();
    virtual ~Revolve() override;
    
    virtual void updateModel(const UpdatePayload&) override;
    virtual Type getType() const override {return Type::Revolve;}
    virtual const std::string& getTypeString() const override {return toString(Type::Revolve);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
    
    virtual void serialWrite(const boost::filesystem::path&) override;
    void serialRead(const prj::srl::rvls::Revolve&);
    
    void setPicks(const Picks&);
    const Picks& getPicks() const {return picks;}
    void setAxisPicks(const Picks&);
    const Picks& getAxisPicks() const {return axisPicks;}
    void setAxisType(AxisType);
    AxisType getAxisType() const {return axisType;}
    
  protected:
    Picks picks; //!< geometry or feature to revolve.
    Picks axisPicks; // array because might be 2 points.
    std::unique_ptr<ann::SeerShape> sShape;
    std::unique_ptr<prm::Parameter> axisOrigin;
    std::unique_ptr<prm::Parameter> axisDirection;
    std::unique_ptr<prm::Parameter> angle;
    
    std::unique_ptr<prm::Observer> prmObserver;
    
    osg::ref_ptr<lbr::PLabel> axisOriginLabel;
    osg::ref_ptr<lbr::PLabel> axisDirectionLabel;
    osg::ref_ptr<lbr::PLabel> angleLabel;
    osg::ref_ptr<osg::Switch> internalSwitch;
    
    AxisType axisType = AxisType::Parameter;
    
    std::map<boost::uuids::uuid, boost::uuids::uuid> generatedMap; //map transition shapes.
    std::map<boost::uuids::uuid, boost::uuids::uuid> lastMap; //map 'top' shapes.
    std::map<boost::uuids::uuid, boost::uuids::uuid> oWireMap; //map new face to outer wire.
    
    void updateLabelVisibility();
    void updateLabels(occt::BoundingBox&);
    
  private:
    static QIcon icon;
  };
}

#endif // FTR_REVOLVE_H
