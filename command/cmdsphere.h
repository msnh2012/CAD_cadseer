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

#ifndef CMD_SPHERE_H
#define CMD_SPHERE_H

#include "command/cmdbase.h"

namespace dlg{class Sphere;}
namespace ftr{class Sphere;}

namespace cmd
{
  /**
  * @todo write docs
  */
  class Sphere : public Base
  {
  public:
    Sphere();
    ~Sphere() override;
    
    std::string getCommandName() override{return "Sphere";}
    std::string getStatusMessage() override;
    void activate() override;
    void deactivate() override;
  private:
    void go();
    bool firstRun = true;
    dlg::Sphere *dialog = nullptr;
  };
  
  /**
  * @todo write docs
  */
  class SphereEdit : public Base
  {
  public:
    SphereEdit(ftr::Base*);
    ~SphereEdit() override;
    
    std::string getCommandName() override{return "Sphere Edit";}
    std::string getStatusMessage() override;
    void activate() override;
    void deactivate() override;
  private:
    dlg::Sphere *dialog = nullptr;
    ftr::Sphere *feature = nullptr;
  };
}
#endif // CMD_SPHERE_H
