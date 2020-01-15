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

#ifndef DLG_OCCTPARAMETERS_H
#define DLG_OCCTPARAMETERS_H

#include <memory>

#include <QWidget>

class QGridLayout;
class QPushButton;
class QScrollArea;

namespace msh{namespace prm{struct OCCT;}}

namespace dlg
{
  /**
  * @todo write docs
  */
  class OCCTParameters : public QWidget
  {
    Q_OBJECT
  public:
    OCCTParameters(QWidget*);
    void setOCCT(const msh::prm::OCCT &prms);
    msh::prm::OCCT getParameters() const;
    
  public Q_SLOTS:
    void resetParameters();
    
  private:
    void buildGui(const msh::prm::OCCT&);
    QGridLayout *layout = nullptr;
    QPushButton *resetButton = nullptr;
    QScrollArea *scrollArea = nullptr;
  };
}

#endif // DLG_OCCTPARAMETERS_H
