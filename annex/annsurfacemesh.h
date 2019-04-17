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

#ifndef ANN_SURFACE_MESH_H
#define ANN_SURFACE_MESH_H

#include <memory>

#include "annex/annbase.h"

class TopoDS_Shape;
class TopoDS_Shell;
class TopoDS_Face;

namespace prj{namespace srl{namespace msh{class Surface;}}}

namespace msh
{
  namespace srf{struct Stow;}
  namespace prm
  {
    struct OCCT;
    struct Netgen;
    struct GMSH;
  }
}

namespace ann
{
  /**
  * @class SurfaceMesh
  * @brief Annex class to hold a surface mesh.
  */
  class SurfaceMesh : public Base
  {
  public:
    SurfaceMesh();
    SurfaceMesh(const msh::srf::Stow&);
    virtual ~SurfaceMesh() override;
    virtual Type getType() override {return Type::SurfaceMesh;}
    
    const msh::srf::Stow& getStow() const;
    
    static std::unique_ptr<SurfaceMesh> generate(const TopoDS_Face&, const msh::prm::OCCT&);
    static std::unique_ptr<SurfaceMesh> generate(const TopoDS_Shell&, const msh::prm::OCCT&);
    static std::unique_ptr<SurfaceMesh> generate(const TopoDS_Face&, const msh::prm::Netgen&);
    static std::unique_ptr<SurfaceMesh> generate(const TopoDS_Shell&, const msh::prm::Netgen&);
    static std::unique_ptr<SurfaceMesh> generate(const TopoDS_Face&, const msh::prm::GMSH&);
    static std::unique_ptr<SurfaceMesh> generate(const TopoDS_Shell&, const msh::prm::GMSH&);
    
    prj::srl::msh::Surface serialOut();
    void serialIn(const prj::srl::msh::Surface&);
  private:
    std::unique_ptr<msh::srf::Stow> stow;
    static std::unique_ptr<SurfaceMesh> generate(const TopoDS_Shape&, const msh::prm::OCCT&);
    static std::unique_ptr<SurfaceMesh> generate(const TopoDS_Shape&, const msh::prm::Netgen&);
    static std::unique_ptr<SurfaceMesh> generate(const TopoDS_Shape&, const msh::prm::GMSH&);
  };
}

#endif // ANN_SURFACE_MESH_H
