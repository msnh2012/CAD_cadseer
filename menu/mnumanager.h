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

#ifndef MNU_MANAGER_H
#define MNU_MANAGER_H

#include <memory>

#include "message/msgmessage.h"

namespace mnu
{
  namespace srl{class Command; class Cue;}
  
  /*! @class Manager
   *  @brief manages the cadseer menu/toolbar/commands
   *
   * Upon construction the manager builds a complete and default
   * menu file and writes it to disk in the user home directory.
   * Then it reads a menu file according to preferences and builds
   * the commands/menus/toolbars. The menu file preference is yet to
   * be implemented, so for now we just read in the complete menu.
   * This should allow the user to copy complete menu file and
   * customize and then point preference to file.
   */
  class Manager
  {
  public:
    Manager();
    void loadMenu(const std::string&);
    msg::Mask getMask(std::size_t) const;
    const srl::Command& getCommand(std::size_t) const;
    const srl::Cue& getCueRead() const;
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
  };
  
  Manager& manager();
}

#endif //MNU_MANAGER_H
