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

#ifndef CMD_SKETCH_H
#define CMD_SKETCH_H

#include "command/cmdleafmanager.h"
#include "command/cmdbase.h"

namespace ftr{class Sketch;}

namespace cmd
{
  /**
  * @todo write docs
  */
  class Sketch : public Base
  {
    public:
      ftr::Sketch *feature = nullptr;
      
      Sketch();
      Sketch(ftr::Base*);
      ~Sketch() override;
      
      std::string getCommandName() override{return "Sketch";}
      std::string getStatusMessage() override;
      void activate() override;
      void deactivate() override;
      
      void localUpdate();
    private:
      cmd::LeafManager leafManager;
      void go();
  };
}

#endif // CMD_SKETCH_H
