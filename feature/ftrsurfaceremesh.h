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

#ifndef FTR_SURFACEREMESH_H
#define FTR_SURFACEREMESH_H

#include "feature/ftrbase.h"

namespace ann{class SurfaceMesh;}
namespace prj{namespace srl{namespace srms{class SurfaceReMesh;}}}
namespace lbr{class PLabel;}

namespace ftr
{
  class SurfaceReMesh : public Base
  {
  public:
    SurfaceReMesh();
    ~SurfaceReMesh() override;
    
    void updateModel(const UpdatePayload&) override;
    void updateVisual() override;
    Type getType() const override {return Type::SurfaceReMesh;}
    const std::string& getTypeString() const override {return toString(Type::SurfaceReMesh);}
    const QIcon& getIcon() const override {return icon;}
    Descriptor getDescriptor() const override {return Descriptor::Create;}
    
    void serialWrite(const boost::filesystem::path&) override;
    void serialRead(const prj::srl::srms::SurfaceReMesh&);
    
  private:
    std::unique_ptr<ann::SurfaceMesh> mesh;
    std::unique_ptr<prm::Parameter> reMeshType;
    std::unique_ptr<prm::Parameter> minEdgeLength; //also used for uniform edge length
    std::unique_ptr<prm::Parameter> maxEdgeLength;
    std::unique_ptr<prm::Parameter> iterations;
    
    osg::ref_ptr<lbr::PLabel> reMeshTypeLabel;
    osg::ref_ptr<lbr::PLabel> minEdgeLengthLabel;
    osg::ref_ptr<lbr::PLabel> maxEdgeLengthLabel;
    osg::ref_ptr<lbr::PLabel> iterationsLabel;
    
    static QIcon icon;
  };
}

#endif //FTR_SURFACEREMESH_H
