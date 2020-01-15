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

#ifndef DLG_SURFACEMESH_H
#define DLG_SURFACEMESH_H

#include <memory>

#include "dialogs/dlgbase.h"

class QGridLayout;
class QPushButton;

namespace ftr{class SurfaceMesh;}
namespace msh{namespace prm{struct OCCT;}}

namespace dlg
{
  /**
  * @todo write docs
  */
  class SurfaceMesh : public Base
  {
    Q_OBJECT
  public:
    SurfaceMesh(ftr::SurfaceMesh*, QWidget*, bool = false);
    ~SurfaceMesh() override;
  public Q_SLOTS:
    void reject() override;
    void accept() override;
    void comboChanged(int);
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
    
    void init();
    void loadFeatureData();
    void finishDialog();
  };
}

#endif // DLG_SURFACEMESH_H
