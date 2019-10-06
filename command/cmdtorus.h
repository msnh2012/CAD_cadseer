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

#ifndef CMD_TORUS_H
#define CMD_TORUS_H

#include "command/cmdbase.h"

namespace dlg{class Torus;}
namespace ftr{class Torus;}

namespace cmd
{
  /**
  * @todo write docs
  */
  class Torus : public Base
  {
  public:
    Torus();
    ~Torus() override;
    
    std::string getCommandName() override{return "Torus";}
    std::string getStatusMessage() override;
    void activate() override;
    void deactivate() override;
  private:
    void go();
    bool firstRun = true;
    dlg::Torus *dialog = nullptr;
  };
  
  /**
  * @todo write docs
  */
  class TorusEdit : public Base
  {
  public:
    TorusEdit(ftr::Base*);
    ~TorusEdit() override;
    
    std::string getCommandName() override{return "Torus Edit";}
    std::string getStatusMessage() override;
    void activate() override;
    void deactivate() override;
  private:
    dlg::Torus *dialog = nullptr;
    ftr::Torus *feature = nullptr;
  };
}
#endif // CMD_TORUS_H
