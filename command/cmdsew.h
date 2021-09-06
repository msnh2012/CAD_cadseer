/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2018  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef CMD_SEW_H
#define CMD_SEW_H

#include "command/cmdleafmanager.h"
#include "command/cmdbase.h"

namespace ftr{namespace Sew{class Feature;}}

namespace cmd
{
  /**
  * @todo write docs
  */
  class Sew : public Base
  {
    public:
      ftr::Sew::Feature *feature = nullptr;
      
      Sew();
      Sew(ftr::Base*);
      ~Sew() override;
      
      std::string getCommandName() override{return "Sew";}
      std::string getStatusMessage() override;
      void activate() override;
      void deactivate() override;
      
      void setSelections(const std::vector<slc::Message>&);
      void localUpdate();
    private:
      cmd::LeafManager leafManager;
      void go();
      bool isValidSelection(const slc::Message&);
  };
}

#endif // CMD_SEW_H
