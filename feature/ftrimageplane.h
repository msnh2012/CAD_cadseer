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

#ifndef FTR_IMAGEPLANE_H
#define FTR_IMAGEPLANE_H

#include <osg/ref_ptr>

#include "feature/ftrbase.h"

namespace osg{class Geometry; class PositionAttitudeTransform;}

namespace ann{class CSysDragger;}
namespace lbr{class PLabel;}
namespace prj{namespace srl{namespace imps{class ImagePlane;}}}

namespace ftr
{
  class ImagePlane : public Base
  {
  public:
    ImagePlane();
    ~ImagePlane() override;
    
    void updateModel(const UpdatePayload&) override;
    void updateVisual() override;
    Type getType() const override {return Type::ImagePlane;}
    const std::string& getTypeString() const override {return toString(Type::ImagePlane);}
    const QIcon& getIcon() const override {return icon;}
    Descriptor getDescriptor() const override {return Descriptor::Create;}
    
    void serialWrite(const boost::filesystem::path&) override;
    void serialRead(const prj::srl::imps::ImagePlane&);
    
    std::string setImage(const boost::filesystem::path&);
    void setScale(double);
    double getScale() const;
    
  private:
    std::unique_ptr<prm::Parameter> scale;
    std::unique_ptr<prm::Parameter> csys; //!< exists just to pass to dragger
    std::unique_ptr<ann::CSysDragger> csysDragger;
    osg::ref_ptr<osg::PositionAttitudeTransform> transform; //used for scale only
    osg::ref_ptr<osg::MatrixTransform> labelDraggerTransform; //so dragger can connect.
    osg::ref_ptr<lbr::PLabel> scaleLabel;
    osg::ref_ptr<osg::Geometry> geometry;
    osg::Vec3d cornerVec; //!< needed for label placement
    boost::filesystem::path imagePath;
    std::string buildGeometry(); //!< build geometry node from current image or return error string.
    void updateVisualPrivate();
    static QIcon icon;
  };
}

#endif //FTR_IMAGEPLANE_H
