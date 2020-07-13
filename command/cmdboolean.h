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

#include <variant>
#include <tuple>

#include "command/cmdleafmanager.h"
#include "command/cmdbase.h"

namespace ftr{class Base; class Intersect; class Subtract; class Union;}

namespace cmd
{
  /**
  * @todo write docs
  */
  class Boolean : public Base
  {
  public:
    using Variant = std::variant<ftr::Intersect*, ftr::Subtract*, ftr::Union*>;
    Variant feature;
    ftr::Base *basePtr = nullptr; //don't need a visit for some things.
    
    Boolean(ftr::Type); //new feature
    Boolean(ftr::Base*); //edit feature
    ~Boolean() override;
    
    std::string getCommandName() override;
    std::string getStatusMessage() override;
    void activate() override;
    void deactivate() override;
    
    void setSelections(const std::vector<slc::Message>&, const std::vector<slc::Message>&);
    std::tuple<std::vector<slc::Message>, std::vector<slc::Message>> getSelections();
    void localUpdate();
  private:
    cmd::LeafManager leafManager;
    void go();
    bool isValidSelection(const slc::Message&);
  };
}
#endif // CMD_BOOLEAN_H
