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

#ifndef LWF_VESSEL_H
#define LWF_VESSEL_H

#include <memory>

#include<Law_Function.hxx>

#include "law/lwflaw.h"

namespace prj{namespace srl{namespace lwsp{class Vessel;}}}

namespace lwf
{
  /*! @struct Vessel
   * @brief Container for law curves.
   * @details Container for law curves. When editing this
   * structure, the following sequence is recommended.
   * setPeriodic
   * setRange
   * getPickPairs externally convert pick to spine parameter
   * setLaws & add remove points.
   */
  struct Vessel
  {
    Vessel();
    ~Vessel();
    
    bool setPeriodic(bool); //do ASAP.
    bool setRange(double, double); //do before resolving picks.
    void setLaws(const Laws&);
    
    bool addPoint(double);
    bool addPoint(const ftr::Pick&, double);
    bool removePoint(int);
    bool togglePoint(int, const ftr::Pick&);
    
    void resetConstant(double, double, double);
    void forceReprocess();
    
    Law getLawType() const;
    int getPointCount() const;
    prm::Parameters getPointParameters(int) const;
    std::vector<prm::Parameters> getPickPairs() const;
    std::vector<std::pair<prm::Parameter*, osg::Transform*>> getGridPairs() const;
    const Laws& getLaws() const;
    opencascade::handle<Law_Function> buildLawFunction() const;
    std::vector<osg::Vec3d> mesh(int) const;
    osg::Group* getOsgGroup() const;
    bool isValid() const;
    int getInternalState() const; //debugging.
    void outputGraphviz(std::string_view) const; //debugging
    const QString& getValidationText() const;
    
    prm::Parameters getNewParameters() const;
    void resetNew();
    const prm::Parameters& getRemovedParameters() const; //watch out, pointers to deleted memory
    void resetRemoved();
    
    prj::srl::lwsp::Vessel serialOut();
    void serialIn(const prj::srl::lwsp::Vessel&);
    
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
  };
}

#endif //LWF_VESSEL_H
