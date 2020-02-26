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

#ifndef CMV_PANE_H
#define CMV_PANE_H

#include <memory>

#include <QWidget>

namespace cmv
{
  /*! @class Pane 
   * @brief Widget for containing command views.
   * @details controlled by command manager and displayed
   * in the main window under the splitter. No parent widget
   * for constructor as the splitter will take ownership 
   * of the one, and only one, instance of this object. 
   */
  class Pane : public QWidget
  {
    Q_OBJECT
  public:
    Pane();
    ~Pane() override;
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
    
    void buildGui();
    void doneSlot();
  };
}

#endif //CMV_PANE_H
