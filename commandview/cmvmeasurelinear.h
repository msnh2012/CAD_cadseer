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

#ifndef CMV_MEASURELINEAR_H
#define CMV_MEASURELINEAR_H

#include <memory>

#include "commandview/cmvbase.h"

namespace osg{class Vec3d;}
namespace dlg{class SelectionWidget;}

namespace cmv
{
  class MeasureLinear : public Base
  {
    Q_OBJECT
  public:
    MeasureLinear();
    ~MeasureLinear() override;
  private:
    dlg::SelectionWidget *selectionWidget = nullptr;
    
    void buildGui();
    void selectionDone(); //!< when second selection is made.
    void go();
    void build(const osg::Vec3d&, const osg::Vec3d&);
  };
}

#endif //CMV_MEASURELINEAR_H
