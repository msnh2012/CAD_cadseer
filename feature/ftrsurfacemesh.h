/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2019  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef FTR_SURFACEMESH_H
#define FTR_SURFACEMESH_H

#include "mesh/mshparameters.h"
#include "feature/ftrbase.h"

namespace ann{class SurfaceMesh;}
namespace prj{namespace srl{class FeatureSurfaceMesh;}}

namespace ftr
{
  class SurfaceMesh : public Base
  {
  public:
    enum class MeshType
    {
      inert, //!< creation is done some where else. Think import.
      occt, //!< construct from parent feature shape and use occt mesher.
      netgen, //!< construct from parent feature shape and use netgen mesher.
      gmsh //!< construct from parent feature shape and use gmsh mesher.
    };
    
    SurfaceMesh();
    virtual ~SurfaceMesh() override;
    
    virtual void updateModel(const UpdatePayload&) override;
    virtual void updateVisual() override;
    virtual Type getType() const override {return Type::SurfaceMesh;}
    virtual const std::string& getTypeString() const override {return toString(Type::SurfaceMesh);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
    
    void setMeshType(MeshType);
    MeshType getMeshType() const {return meshType;}
    //use Base::getAnnex to get mesh.
    void setMesh(std::unique_ptr<ann::SurfaceMesh> mIn, bool setToInert = true);
    const msh::prm::OCCT& getOcctParameters() const {return occtParameters;}
    void setOcctParameters(const msh::prm::OCCT&);
    const msh::prm::Netgen& getNetgenParameters() const {return netgenParameters;}
    void setNetgenParameters(const msh::prm::Netgen&);
    
    virtual void serialWrite(const boost::filesystem::path&) override;
    void serialRead(const prj::srl::FeatureSurfaceMesh&);
    
  protected:
    MeshType meshType = MeshType::inert;
    std::unique_ptr<ann::SurfaceMesh> mesh;
    msh::prm::OCCT occtParameters;
    msh::prm::Netgen netgenParameters;
    msh::prm::GMSH gmshParameters;
    
  private:
    static QIcon icon;
  };
}

#endif //FTR_SURFACEMESH_H
