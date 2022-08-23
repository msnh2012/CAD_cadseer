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

#ifndef LBR_PLABELGRID_H
#define LBR_PLABELGRID_H

#include <memory>

#include <osg/AutoTransform>
#include <osg/Callback>

namespace lbr
{
  class PLabel;
  
  class PLabelGridCallback : public osg::Callback
  {
  public:
    struct Stow;
    std::unique_ptr<Stow> stow;
    
    PLabelGridCallback();
    ~PLabelGridCallback();
    void setDirtyLayout(); //call this to force an update of the layout.
    void setDirtyParameters(); //call this when child structure has changed.
    void setColumns(int); //>=1
    
    bool run(osg::Object*, osg::Object*) override;
  };
  
  osg::AutoTransform* buildGrid(int = 1);
}

#endif //LBR_PLABELGRID_H
