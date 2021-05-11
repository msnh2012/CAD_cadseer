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

#ifndef CMV_SKETCH_H
#define CMV_SKETCH_H

#include <memory>

#include "commandview/cmvbase.h"

namespace cmd{class Sketch;}

namespace cmv
{
  /**
  * @todo write docs
  */
  class Sketch : public Base
  {
    Q_OBJECT
  public:
    Sketch(cmd::Sketch*);
    ~Sketch() override;
    
    void activate();
    void deactivate();
  private Q_SLOTS:
    void buttonToggled(int, bool);
    void modelChanged(const QModelIndex&, const QModelIndex&);
    
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
    
    void chainToggled(bool);
    void addPoint(bool);
    void addLine(bool);
    void addArc(bool);
    void addCircle(bool);
    void addBezier(bool);
    
    void remove();
    void cancel();
    void toggleConstruction();
    
    void addCoincident();
    void addHorizontal();
    void addVertical();
    void addTangent();
    void addDistance();
    void addDiameter();
    void addAngle();
    void addSymmetric();
    void addParallel();
    void addPerpendicular();
    void addEqual();
    void addEqualAngle();
    void addMidpoint();
    void addWhereDragged();
    
    void prmValueChanged();
    void prmConstantChanged();
    
    void viewEntitiesToggled(bool);
    void viewConstraintsToggled(bool);
    void viewWorkToggled(bool);
    void selectEntitiesToggled(bool);
    void selectConstraintsToggled(bool);
    void selectWorkToggled(bool);
  };
}

#endif // CMV_SKETCH_H
