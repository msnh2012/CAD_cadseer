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

#include "command/base.h"

namespace dlg{class Sketch;}
namespace ftr{class Sketch;}

namespace cmd
{
  /**
  * @todo write docs
  */
  class Sketch : public Base
  {
    public:
      Sketch();
      virtual ~Sketch() override;
      
      virtual std::string getCommandName() override{return "Sketch";}
      virtual std::string getStatusMessage() override;
      virtual void activate() override;
      virtual void deactivate() override;
    private:
      dlg::Sketch *dialog = nullptr;
      void go();
  };
  
  class SketchEdit : public Base
  {
  public:
    SketchEdit(ftr::Base*);
    virtual ~SketchEdit() override;
    virtual std::string getCommandName() override{return "Sketch Edit";}
    virtual std::string getStatusMessage() override;
    virtual void activate() override;
    virtual void deactivate() override;
  private:
    ftr::Sketch *feature = nullptr;
    dlg::Sketch *dialog = nullptr;
    void go();
  };
}

#endif // CMD_SKETCH_H
