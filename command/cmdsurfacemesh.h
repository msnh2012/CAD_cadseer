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


#ifndef CMD_SURFACEMESH_H
#define CMD_SURFACEMESH_H

#include "command/cmdbase.h"

namespace ftr{class SurfaceMesh;}
namespace dlg{class SurfaceMesh;}

namespace cmd
{
  /**
  * @todo write docs
  */
  class SurfaceMesh : public Base
  {
  public:
    SurfaceMesh();
    virtual ~SurfaceMesh() override;
    
    virtual std::string getCommandName() override{return "Surface Mesh";}
    virtual std::string getStatusMessage() override;
    virtual void activate() override;
    virtual void deactivate() override;
  private:
    void go();
  };
  
  /**
  * @todo write docs
  */
  class SurfaceMeshEdit : public Base
  {
  public:
    SurfaceMeshEdit(ftr::Base*);
    ~SurfaceMeshEdit() override;
    
    std::string getCommandName() override{return "SurfaceMesh Edit";}
    std::string getStatusMessage() override;
    void activate() override;
    void deactivate() override;
  private:
    dlg::SurfaceMesh *dialog = nullptr;
    ftr::SurfaceMesh *feature = nullptr;
  };
}

#endif // CMD_SURFACEMESH_H
