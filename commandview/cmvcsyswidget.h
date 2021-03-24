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

#ifndef CMV_CSYSWIDGET_H
#define CMV_CSYSWIDGET_H

#include <memory>

#include <QWidget>

#include "commandview/cmvparameterbase.h"

namespace boost{namespace uuids{struct uuid;}}

namespace prm{class Parameter;}

namespace cmv
{
  /*! @class CSysWidget
   * @brief Widget for controlling CSys parameters
   * 
   * Checkbox for csys options constant/expression linked
   * or feature linked.
   * 
   * @note this uses a selectionwidget. selectionwidget assumes
   * only one use per dialog/tab. If csyswidget exists in the same
   * dialog/tab as another selectionwidget, I think it will be chaos.
   */
  class CSysWidget : public ParameterBase
  {
    Q_OBJECT
  public:
    CSysWidget(QWidget*, prm::Parameter*);
    ~CSysWidget();
    void setCSysLinkId(const boost::uuids::uuid&); //!< pass nil for no feature link.
    boost::uuids::uuid getCSysLinkId() const; //!< returns nil id if no feature link.
  Q_SIGNALS:
    void dirty(); //!< called when something has changed.
  public Q_SLOTS:
    void statusChanged(); //!< called when shown or enabled through viz filter.
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
    
    void radioSlot();
  };
}


#endif //CMV_CSYSWIDGET_H
