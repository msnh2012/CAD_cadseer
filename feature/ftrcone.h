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

#ifndef FTR_CONE_H
#define FTR_CONE_H

#include <memory>

#include <osg/ref_ptr>

#include <feature/base.h>

namespace lbr{class IPGroup;}
namespace prj{namespace srl{class FeatureCone;}}
namespace ann{class CSysDragger;}

namespace ftr
{
  class ConeBuilder;
  
  class Cone : public Base
  {
  public:
    Cone();
    virtual ~Cone() override;
    void setRadius1(const double &radius1In);
    void setRadius2(const double &radius2In);
    void setHeight(const double &heightIn);
    void setParameters(const double &radius1In, const double &radius2In, const double &heightIn);
    void setCSys(const osg::Matrixd&);
    double getRadius1() const;
    double getRadius2() const;
    double getHeight() const;
    void getParameters (double &radius1Out, double &radius2Out, double &heightOut) const;
    osg::Matrixd getCSys() const;
    
    virtual void updateModel(const UpdatePayload&) override;
    virtual Type getType() const override {return Type::Cone;}
    virtual const std::string& getTypeString() const override {return toString(Type::Cone);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
    virtual void serialWrite(const boost::filesystem::path&) override; //!< write xml file. not const, might reset a modified flag.
    void serialRead(const prj::srl::FeatureCone &sCone); //!<initializes this from sBox. not virtual, type already known.
    
  protected:
    std::unique_ptr<prm::Parameter> radius1;
    std::unique_ptr<prm::Parameter> radius2; //!< maybe zero.
    std::unique_ptr<prm::Parameter> height;
    std::unique_ptr<prm::Parameter> csys;
  
    std::unique_ptr<ann::CSysDragger> csysDragger;
    std::unique_ptr<ann::SeerShape> sShape;
    
    osg::ref_ptr<lbr::IPGroup> heightIP;
    osg::ref_ptr<lbr::IPGroup> radius1IP;
    osg::ref_ptr<lbr::IPGroup> radius2IP;
    
    void initializeMaps();
    void updateResult(const ConeBuilder &);
    void setupIPGroup();
    void updateIPGroup();
    
  private:
    static QIcon icon;
  };
}

#endif // FTR_CONE_H