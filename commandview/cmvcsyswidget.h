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
   * @details 2 modes of operation: 'User Mode' and 'Prm Mode'
   * 
   * User Mode(the default):
   * User selectable radio buttons for controlling csys parameter state.
   * state 1 (byConstant): the csys parameter is a matrix value or linked to a matrix expression.
   * state 2 (byFeature): the csys parameter is linked to an input parent feature.
   *
   * Prm Mode:
   * Has the 2 states of 'User Mode': 'byConstant' and 'byFeature' with an additional
   *     state of 'byNone'.
   * Radio buttons are hidden and user has no direct control of byConstant or byFeature.
   * Established post construction with @setPrmMode.
   * We will call the parameter passed into the constructor 'csysParameter'
   * The parameter passed in setPrmMode we will call 'linkParameter'
   * Both parameter's active states are observed.
   * When the csysParameter is active and the linkParameter is not
   *     Then the typical parameter edit will be shown. Equal to 'byConstant'
   * Inversely, when the csysParameter is not active and linkParameter is active
   *     Then a selection widget is shown for the user to select input feature.
   *     Equal to 'byFeature'
   * When both parameters are inactive the entire widget is disabled.
   *     'byNone' state.
   * Both parameters active, is undefined and will be ignored.
   *     This should only be a temporary, while switching states atomically.
   * 
   * @note Our whole widget factory is predicated on one parameter being passed in.
   * So we couldn't just add a constructor taking 2 parameters to get 'Prm Mode'.
   * We don't actually modify the linkParameter as 'csys linking' actually happens
   * in the project graph. Tangent: This long drawn out explanation, is a testament
   * to how 'csys linking' between features has broken my architecture and is
   * a complete hack! It is a hack I hope I can live with, because, as a user,
   * I love the feature!
   * 
   * @warning this uses a selectionwidget. selectionwidget assumes
   * only one use per commandView. If csyswidget exists in the same
   * commandView as another selectionwidget, I think it will be chaos.
   * @see dlg::SelectionWidget
   */
  class CSysWidget : public ParameterBase
  {
    Q_OBJECT
  public:
    CSysWidget(QWidget*, prm::Parameter*);
    ~CSysWidget();
    void setPrmMode(prm::Parameter*);
    void setCSysLinkId(const boost::uuids::uuid&); //!< pass nil for no feature link.
    boost::uuids::uuid getCSysLinkId() const; //!< returns nil id if no feature link.
  Q_SIGNALS:
    void dirty(); //!< called when something has changed.
  private Q_SLOTS:
    void statusChanged(); //!< called when shown or enabled through viz filter.
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
    
    void radioSlot();
  };
}


#endif //CMV_CSYSWIDGET_H
