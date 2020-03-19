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

namespace boost{namespace filesystem{class path;}}

namespace prj{namespace srl{namespace mshs{class Surface;}}}

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
    SurfaceMesh& operator=(const SurfaceMesh&);
    SurfaceMesh& operator=(const msh::srf::Stow&);
    virtual ~SurfaceMesh() override;
    virtual Type getType() override {return Type::SurfaceMesh;}
    
    const msh::srf::Stow& getStow() const;
    bool readOFF(const boost::filesystem::path&);
    bool writeOFF(const boost::filesystem::path&) const;
    bool readPLY(const boost::filesystem::path&);
    bool writePLY(const boost::filesystem::path&) const;
    bool readSTL(const boost::filesystem::path&);
    bool writeSTL(const boost::filesystem::path&) const;
    
    void remeshCGAL(double, int);
    void remeshPMPUniform(double, int);
    
    void fillHolesCGAL();
    void fillHolesPMP();
    
    prj::srl::mshs::Surface serialOut();
    void serialIn(const prj::srl::mshs::Surface&);
  private:
    std::unique_ptr<msh::srf::Stow> stow;
  };
}

#endif // ANN_SURFACE_MESH_H
