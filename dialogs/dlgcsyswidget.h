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

#ifndef DLG_CSYSWIDGET_H
#define DLG_CSYSWIDGET_H

#include <memory>

#include <QWidget>

namespace boost{namespace uuids{struct uuid;}}

namespace prm{class Parameter;}

namespace dlg
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
  class CSysWidget : public QWidget
  {
    Q_OBJECT
  public:
    CSysWidget(QWidget*, prm::Parameter*);
    ~CSysWidget();
    void syncLinks(); //!< call to commit link changes.
    void setCSysLinkId(const boost::uuids::uuid&); //!< pass nil for no feature link.
    boost::uuids::uuid getCSysLinkId() const; //!< returns nil id if no feature link.
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
    
    void radioSlot();
  };
}


#endif //DLG_CSYSWIDGET_H
