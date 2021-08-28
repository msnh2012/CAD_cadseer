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

#ifndef CMV_QUOTE_H
#define CMV_QUOTE_H

#include <memory>

#include "commandview/cmvbase.h"

namespace cmd{class Quote;}

namespace cmv
{
  /**
  * @todo write docs
  */
  class Quote : public Base
  {
    Q_OBJECT
  public:
    Quote(cmd::Quote*);
    ~Quote() override;
  private Q_SLOTS:
//     void browseForTemplateSlot();
//     void browseForOutputSlot();
//     void takePictureSlot();
//     void loadLabelPixmapSlot();
//     void selectionsChanged();
//     void valueChanged();
//     void comboValueChanged(int);
//     void templateChanged(const QString&);
//     void outputChanged(const QString&);
    void modelChanged(const QModelIndex&, const QModelIndex&);
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
  };
}

#endif // CMV_QUOTE_H
