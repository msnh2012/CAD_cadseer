/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef CMD_STRIP_H
#define CMD_STRIP_H

#include "command/cmdbase.h"

namespace dlg{class Strip;}
namespace ftr{class Strip;}

namespace cmd
{
  class Strip : public Base
  {
  public:
    Strip();
    virtual ~Strip() override;
    
    virtual std::string getCommandName() override{return "Strip";}
    virtual std::string getStatusMessage() override;
    virtual void activate() override;
    virtual void deactivate() override;
  private:
    dlg::Strip *dialog = nullptr;
    void go();
  };
  
  class StripEdit : public Base
  {
  public:
    StripEdit(ftr::Base*);
    virtual ~StripEdit() override;
    
    virtual std::string getCommandName() override{return "Strip Edit";}
    virtual std::string getStatusMessage() override;
    virtual void activate() override;
    virtual void deactivate() override;
  private:
    dlg::Strip *dialog = nullptr;
    ftr::Strip *strip = nullptr;
    void go();
  };
}

#endif // CMD_STRIP_H
