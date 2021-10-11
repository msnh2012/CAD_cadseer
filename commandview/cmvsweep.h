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

#ifndef CMV_SWEEP_H
#define CMV_SWEEP_H

#include <memory>

#include "commandview/cmvbase.h"

class QItemSelection;

namespace cmd{class Sweep;}

namespace cmv
{
  /**
  * @todo write docs
  */
  class Sweep : public Base
  {
    Q_OBJECT
  public:
    Sweep(cmd::Sweep*);
    ~Sweep() override;
  protected:
    bool eventFilter(QObject*, QEvent*) override;
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
  private Q_SLOTS:
    void lawDirtySlot();
    void lawValueChangedSlot();
    void profilesChanged(const QModelIndex&, const QModelIndex&);
    void profileSelectionChanged(const QItemSelection&, const QItemSelection&);
    void modelChanged(const QModelIndex&, const QModelIndex&);
  };
}

#endif // CMV_SWEEP_H
