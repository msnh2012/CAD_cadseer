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

#include "command/cmdleafmanager.h"
#include "command/cmdbase.h"

namespace ftr{class Sweep;}

namespace cmd
{
  class Sweep : public Base
  {
  public:
    ftr::Sweep *feature = nullptr;
    
    using Profile = std::tuple<slc::Message, bool, bool>;
    using Profiles = std::vector<Profile>;
    
    using Auxiliary = std::tuple<const slc::Message&, bool, int>;
    
    Sweep();
    Sweep(ftr::Base*);
    ~Sweep() override;
    
    std::string getCommandName() override{return "Sweep";}
    std::string getStatusMessage() override;
    void activate() override;
    void deactivate() override;
    
    void setCommon(const slc::Message&, const Profiles&, int);
    void setBinormal(const slc::Message&, const Profiles&, const slc::Messages&);
    void setSupport(const slc::Message&, const Profiles&, const slc::Message&);
    void setAuxiliary(const slc::Message&, const Profiles&, const Auxiliary&);
    void localUpdate();
  private:
    cmd::LeafManager leafManager;
    void go();
    void connectCommon(const slc::Message&, const Profiles&);
    bool isValidSelection(const slc::Message&);
  };
}
#endif // CMD_SWEEP_H
