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

#ifndef CMD_SWEEP_H
#define CMD_SWEEP_H

#include "command/cmdbase.h"

namespace dlg{class Sweep;}
namespace ftr{class Sweep;}

namespace cmd
{
  /**
  * @todo write docs
  */
  class Sweep : public Base
  {
  public:
    Sweep();
    ~Sweep() override;
    
    std::string getCommandName() override{return "Sweep";}
    std::string getStatusMessage() override;
    void activate() override;
    void deactivate() override;
  private:
    void go();
    bool firstRun = true;
    dlg::Sweep *dialog = nullptr;
  };
  
  class SweepEdit : public Base
  {
  public:
    SweepEdit(ftr::Base*);
    virtual ~SweepEdit() override;
    
    virtual std::string getCommandName() override{return "Sweep Edit";}
    virtual std::string getStatusMessage() override;
    virtual void activate() override;
    virtual void deactivate() override;
    
  private:
    dlg::Sweep *sweepDialog = nullptr;
    ftr::Sweep *sweep;
  };
  
  
}
#endif // CMD_SWEEP_H
