/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2020 Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef CMD_LEAFMANAGER_H
#define CMD_LEAFMANAGER_H

#include <memory>

namespace boost{namespace uuids{struct uuid;}}
namespace ftr{class Base;}

namespace cmd
{
  /*! @struct LeafManager
   * @brief Managing active states of features
   * @details Editing features that have inputs that are not active
   * can be real confusing, as the selection geometry is hidden.
   * This type is meant to handle the situation by making the inputs
   * active for editing and later restoring to original state.
   * @note We can't automatically rewind in constructor because
   * a command gets constructed before currently running command
   * gets deactivated. So we would be taking a snap shot of another
   * commands rewound state.
   */
  struct LeafManager
  {
  public:
    LeafManager();
    LeafManager(const boost::uuids::uuid&);
    LeafManager(const ftr::Base*);
    ~LeafManager();
    void rewind();
    void fastForward();
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
  };
}
#endif //CMD_LEAFMANAGER_H
