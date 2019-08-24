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

#ifndef FTR_EXTRACT_H
#define FTR_EXTRACT_H

#include <osg/ref_ptr>

#include "tools/idtools.h"
#include "library/lbrplabel.h"
#include "feature/ftrpick.h"
#include "feature/ftrbase.h"

namespace prj{namespace srl{class FeatureExtract;}}
namespace ann{class SeerShape;}
namespace prm{class Parameter;}

namespace ftr
{
  class Extract : public Base
  {
  public:
    static std::shared_ptr<prm::Parameter> buildAngleParameter(double deg = 0.0); //set up default.
    std::shared_ptr<prm::Parameter> getAngleParameter() const {return angle;}
    void setAngleParameter(std::shared_ptr<prm::Parameter>);
    
    Extract();
    virtual ~Extract() override;
    virtual void updateModel(const UpdatePayload&) override;
    virtual Type getType() const override {return Type::Extract;}
    virtual const std::string& getTypeString() const override {return toString(Type::Extract);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
    virtual void serialWrite(const boost::filesystem::path&) override;
    void serialRead(const prj::srl::FeatureExtract &);

    void setPicks(const Picks&);
    const Picks& getPicks(){return picks;}
  private:
    static QIcon icon;
    Picks picks;
    
    std::unique_ptr<ann::SeerShape> sShape;
    std::shared_ptr<prm::Parameter> angle; //!< parameter containing tangent angle.
    osg::ref_ptr<lbr::PLabel> label;
  };
}

#endif // FTR_EXTRACT_H
