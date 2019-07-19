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

#ifndef CMD_BOOLEAN_H
#define CMD_BOOLEAN_H

#include "command/cmdbase.h"

namespace dlg{class Boolean;}
namespace ftr{class Base; class Intersect; class Subtract; class Union;}

namespace cmd
{
  /**
  * @todo write docs
  */
  class Boolean : public Base
  {
  public:
    Boolean(const ftr::Type&);
    ~Boolean() override;
    
    std::string getCommandName() override;
    std::string getStatusMessage() override;
    void activate() override;
    void deactivate() override;
  private:
    void go();
    ftr::Type ftrType;
    bool firstRun = true;
    dlg::Boolean *dialog = nullptr;
  };
  
  /**
  * @todo write docs
  */
  class BooleanEdit : public Base
  {
  public:
    BooleanEdit(ftr::Base*);
    virtual ~BooleanEdit() override;
    
    virtual std::string getCommandName() override{return "Boolean Edit";}
    virtual std::string getStatusMessage() override;
    virtual void activate() override;
    virtual void deactivate() override;
  private:
    dlg::Boolean *dialog = nullptr;
    ftr::Intersect *iPtr = nullptr;
    ftr::Subtract *sPtr = nullptr;
    ftr::Union *uPtr = nullptr;
  };
}
#endif // CMD_BOOLEAN_H
