/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2022 Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef CMV_LAWSPINE_H
#define CMV_LAWSPINE_H

#include <memory>

#include "commandview/cmvbase.h"

namespace cmd{class LawSpine;}

namespace cmv
{
  /**
  * @todo write docs
  */
  class LawSpine : public Base
  {
    Q_OBJECT
  public:
    LawSpine(cmd::LawSpine*);
    ~LawSpine() override;
  protected:
    bool eventFilter(QObject*, QEvent*) override;
  private Q_SLOTS:
    void modelChanged(const QModelIndex&, const QModelIndex&);
    void pointModelChanged(const QModelIndex&, const QModelIndex&);
    void lawsChanged(int);
    void pointSelectionChanged();
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
  };
}

#endif // CMV_LAWSPINE_H
