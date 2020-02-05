/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2020 Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef FTR_SURFACEMESHFILL_H
#define FTR_SURFACEMESHFILL_H

// #include "feature/ftrpick.h"
#include "feature/ftrbase.h"

namespace ann{class SurfaceMesh;}
namespace prj{namespace srl{class FeatureSurfaceMeshFill;}}
namespace lbr{class PLabel;}

namespace ftr
{
  class SurfaceMeshFill : public Base
  {
  public:
    SurfaceMeshFill();
    ~SurfaceMeshFill() override;
    
    void updateModel(const UpdatePayload&) override;
    void updateVisual() override;
    Type getType() const override {return Type::SurfaceMeshFill;}
    const std::string& getTypeString() const override {return toString(Type::SurfaceMeshFill);}
    const QIcon& getIcon() const override {return icon;}
    Descriptor getDescriptor() const override {return Descriptor::Create;}
    
    void serialWrite(const boost::filesystem::path&) override;
    void serialRead(const prj::srl::FeatureSurfaceMeshFill&);
    
  private:
    std::unique_ptr<ann::SurfaceMesh> mesh;
    std::unique_ptr<prm::Parameter> algorithm;
    osg::ref_ptr<lbr::PLabel> algorithmLabel;
    
    static QIcon icon;
  };
}

#endif //FTR_SURFACEMESHFILL_H
