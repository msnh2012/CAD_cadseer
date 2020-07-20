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

#ifndef CMD_HOLLOW_H
#define CMD_HOLLOW_H

#include "command/cmdleafmanager.h"
#include "command/cmdbase.h"

namespace ftr{class Hollow;}

namespace cmd
{
  /**
  * @todo write docs
  */
  class Hollow : public Base
  {
  public:
    ftr::Hollow *feature = nullptr;
    
    Hollow();
    Hollow(ftr::Base*);
    ~Hollow() override;
    
    std::string getCommandName() override{return "Hollow";}
    std::string getStatusMessage() override;
    void activate() override;
    void deactivate() override;
    
    bool isValidSelection(const slc::Message&);
    void setSelections(const std::vector<slc::Message>&);
    void localUpdate();
  private:
    cmd::LeafManager leafManager;
    void go();
  };
}
#endif // CMD_HOLLOW_H
