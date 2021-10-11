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

namespace ftr{namespace Sweep{class Feature;}}

namespace cmd
{
  class Sweep : public Base
  {
  public:
    using Profiles = std::vector<slc::Messages>;
    ftr::Sweep::Feature *feature = nullptr;
    
    Sweep();
    Sweep(ftr::Base*);
    ~Sweep() override;
    
    std::string getCommandName() override{return "Sweep";}
    std::string getStatusMessage() override;
    void activate() override;
    void deactivate() override;
    
    void setCommon(const slc::Messages&, const Profiles&);
    void setBinormal(const slc::Messages&, const Profiles&, const slc::Messages&);
    void setSupport(const slc::Messages&, const Profiles&, const slc::Messages&);
    void setAuxiliary(const slc::Messages&, const Profiles&, const slc::Messages&);
    void localUpdate();
  private:
    cmd::LeafManager leafManager;
    void go();
    void connectCommon(const slc::Messages&, const Profiles&);
    bool isValidSelection(const slc::Message&);
  };
}
#endif // CMD_SWEEP_H
