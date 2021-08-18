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

#ifndef CMV_BASE_H
#define CMV_BASE_H

#include <memory>

#include <QWidget>

#include "selection/slcdefinitions.h"

namespace ftr{class Base;}
namespace msg{struct Node; struct Sift;}
namespace prj{class Project;}

namespace cmv
{
  /*! @class Base 
   * @brief Base Widget for command views.
   * @details No parent widget for constructor as the 
   * command will own them.
   * 
   * Managing feature states to be handled by editing commands not gui views.
   */
  class Base : public QWidget
  {
    Q_OBJECT
  public:
    Base() = delete;
    Base(const QString&);
    ~Base() override;
    void setPaneWidth(int);
    int getPaneWidth(){return paneWidth;};
    void goMaskDefault(bool = true); //!<qtimer delayed by default
    void goSelectionToolbar();
    
    static void clearContentMargins(QWidget*);
  protected:
    std::unique_ptr<msg::Node> node;
    std::unique_ptr<msg::Sift> sift;
    prj::Project *project = nullptr;
    QString name; //<! should be unique
    int paneWidth = 100;
    slc::Mask maskDefault = slc::None;
  };
}

#endif //CMV_BASE_H

