/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2019 Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef FTR_TRANSITIONCURVE_H
#define FTR_TRANSITIONCURVE_H

#include "feature/pick.h"
#include "feature/base.h"

class TopoDS_Edge;

namespace ann{class SeerShape;}
namespace lbr{class PLabel;}
namespace prj{namespace srl{class FeatureTransitionCurve;}}

namespace ftr
{
  class TransitionCurve : public Base
  {
  public:
    constexpr static const char *pickZero = "pickZero";
    constexpr static const char *pickOne = "pickOne";
    
    TransitionCurve();
    ~TransitionCurve() override;
    
    void updateModel(const UpdatePayload&) override;
    Type getType() const override {return Type::TransitionCurve;}
    const std::string& getTypeString() const override {return toString(Type::TransitionCurve);}
    const QIcon& getIcon() const override {return icon;}
    Descriptor getDescriptor() const override {return Descriptor::Create;}
    
    void serialWrite(const boost::filesystem::path&) override;
    void serialRead(const prj::srl::FeatureTransitionCurve&);
    
    void setPicks(const Picks&);
    const Picks& getPicks() const {return picks;}
    
    void setDirection0(bool);
    void setDirection1(bool);
    void gleanDirection0(const TopoDS_Edge&);
    void gleanDirection1(const TopoDS_Edge&);
    
  private:
    std::unique_ptr<ann::SeerShape> sShape;
    std::unique_ptr<prm::Parameter> pick0Direction;
    std::unique_ptr<prm::Parameter> pick1Direction;
    std::unique_ptr<prm::Parameter> pick0Magnitude;
    std::unique_ptr<prm::Parameter> pick1Magnitude;
    Picks picks;
    
    osg::ref_ptr<lbr::PLabel> directionLabel0;
    osg::ref_ptr<lbr::PLabel> directionLabel1;
    osg::ref_ptr<lbr::PLabel> magnitudeLabel0;
    osg::ref_ptr<lbr::PLabel> magnitudeLabel1;
    
    boost::uuids::uuid curveId;
    boost::uuids::uuid vertex0Id;
    boost::uuids::uuid vertex1Id;
    
    static QIcon icon;
  };
}

#endif //FTR_TRANSITIONCURVE_H
