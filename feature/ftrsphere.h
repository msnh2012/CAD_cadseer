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

#ifndef FTR_SPHERE_H
#define FTR_SPHERE_H

#include <memory>

#include <osg/ref_ptr>

#include "feature/ftrbase.h"

class BRepPrimAPI_MakeSphere;
namespace lbr{class IPGroup;}
namespace prj{namespace srl{namespace sprs{class Sphere;}}}
namespace ann{class CSysDragger; class SeerShape;}

namespace ftr
{
  class Sphere : public Base
  {
  public:
    Sphere();
    virtual ~Sphere() override;
    void setRadius(double radiusIn);
    void setCSys(const osg::Matrixd&);
    const prm::Parameter& getRadius() const;
    osg::Matrixd getCSys() const;
    
    virtual void updateModel(const UpdatePayload&) override;
    virtual Type getType() const override {return Type::Sphere;}
    virtual const std::string& getTypeString() const override {return toString(Type::Sphere);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
    virtual void serialWrite(const boost::filesystem::path&) override; //!< write xml file. not const, might reset a modified flag.
    void serialRead(const prj::srl::sprs::Sphere &sSphere); //!<initializes this from sBox. not virtual, type already known.
    
  protected:
    std::unique_ptr<prm::Parameter> radius;
    std::unique_ptr<prm::Parameter> csys;
  
    std::unique_ptr<ann::CSysDragger> csysDragger;
    std::unique_ptr<ann::SeerShape> sShape;
    
    osg::ref_ptr<lbr::IPGroup> radiusIP;
    
    void initializeMaps();
    void updateResult(BRepPrimAPI_MakeSphere&);
    void setupIPGroup();
    void updateIPGroup();
    
  private:
    static QIcon icon;
  };
}

#endif // FTR_SPHERE_H
