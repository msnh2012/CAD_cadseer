/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2021 Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef CMV_PARAMETERBASE_H
#define CMV_PARAMETERBASE_H

#include <QWidget>
namespace cmv
{
  /**
  * @class ParameterBase @brief Base class for widgets that affect parameters.
  * 
  * @details We can't rely on prm::Observers inside dialogs and commandviews
  * to take actions like trigger feature or project updates. This is because
  * we have no idea who made the parameter change. So we need to be able
  * signal these changes purely through gui so commandviews and dialogs
  * no when it's widgets modified the parameter. Widgets will still need
  * a prm::Observer because we want them to stay up to date if the parameter
  * is changed from an external source.
  */
  class ParameterBase : public QWidget
  {
    Q_OBJECT
  public:
    ParameterBase() = delete;
    explicit ParameterBase(QWidget *p) : QWidget(p){}
  Q_SIGNALS:
    void prmValueChanged();
    void prmConstantChanged();
  };
}

#endif // CMV_PARAMETERBASE_H
