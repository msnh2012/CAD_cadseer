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

#ifndef CMV_MESSAGE_H
#define CMV_MESSAGE_H

class QWidget;

namespace cmv
{
  class Base;
  /*! @struct Message
   * @brief Messages related to command views
   * @details Slight duplication of info because Base has pane width, but
   * we need width separate in message because main window sends pane width
   * when splitter is dragged. MainWindow doesn't have a clean way to get the
   * current command view ... although that would be pretty easy to add...?
   * 
   */
  struct Message
  {
    Base *widget = nullptr;
    int paneWidth = 100;
    Message() = default;
    Message(Base*, int);
  };
}

#endif //CMV_MESSAGE_H
