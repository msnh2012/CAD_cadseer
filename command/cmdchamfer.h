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

#ifndef CMD_CHAMFER_H
#define CMD_CHAMFER_H

#include "command/cmdbase.h"

namespace dlg{class Chamfer;}
namespace ftr{class Chamfer;}

namespace cmd
{
  /**
  * @todo write docs
  */
  class Chamfer : public Base
  {
  public:
    Chamfer();
    ~Chamfer() override;
    
    std::string getCommandName() override{return "Chamfer";}
    std::string getStatusMessage() override;
    void activate() override;
    void deactivate() override;
  private:
    void go();
    bool firstRun = true;
    dlg::Chamfer *dialog = nullptr;
  };
  
  /**
  * @todo write docs
  */
  class ChamferEdit : public Base
  {
  public:
    ChamferEdit(ftr::Base*);
    ~ChamferEdit() override;
    
    std::string getCommandName() override{return "Chamfer Edit";}
    std::string getStatusMessage() override;
    void activate() override;
    void deactivate() override;
  private:
    dlg::Chamfer *dialog = nullptr;
    ftr::Chamfer *feature = nullptr;
  };
}
#endif // CMD_CHAMFER_H
