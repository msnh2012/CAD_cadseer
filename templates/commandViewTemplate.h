/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) %YEAR% Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef CMV_%CLASSNAMEUPPERCASE%_H
#define CMV_%CLASSNAMEUPPERCASE%_H

#include <memory>

#include "commandview/cmvbase.h"

namespace cmd{class %CLASSNAME%;}

namespace cmv
{
  /**
  * @todo write docs
  */
  class %CLASSNAME% : public Base
  {
    Q_OBJECT
  public:
    %CLASSNAME%(cmd::%CLASSNAME%*);
    ~%CLASSNAME%() override;
  protected:
    bool eventFilter(QObject*, QEvent*) override;
  private Q_SLOTS:
    void modelChanged(const QModelIndex&, const QModelIndex&);
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
  };
}

#endif // CMV_%CLASSNAMEUPPERCASE%_H
