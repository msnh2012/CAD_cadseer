/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2021 Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef FTR_PRIMITIVE_H
#define FTR_PRIMITIVE_H

// namespace prj{namespace srl{namespace FIXME{class Primitive;}}}
#include "feature/ftrbase.h"
#include "feature/ftrpick.h"
#include "annex/annseershape.h"
#include "annex/anncsysdragger.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "library/lbrplabel.h"
#include "library/lbripgroup.h"

namespace ftr
{
  /*! @struct Primitive
   * @brief Struct to relieve the burden of making primitives
   * 
   * @details Not an actual feature. Use in stow of primitive features.
   * parameters and annexes are protected members of ftr::Base.
   * so we can do everything here. Users of this object will
   * have to load those protected members.
   */
  struct Primitive
  {
    enum CSysType
    {
      Constant
      , Linked
    };
    
    /*! @struct Input
     * @brief inaccessible, protected members of feature base
     * 
     * @details We want access to the protected members
     * of feature base without inheritance or friends. So we will
     * load up the protected members from within feature stow and
     * pass them on here.
     */
    struct Input
    {
      Base &feature;
      prm::Parameters &prms;
      ann::Annexes &annexes;
    };
    Input input;
    
    //every primitive has these
    prm::Parameter csysType{prm::Names::CSysType, 0, prm::Tags::CSysType};
    prm::Parameter csys{prm::Names::CSys, osg::Matrixd::identity(), prm::Tags::CSys};
    prm::Parameter csysLinked{prm::Names::CSysLinked, Picks(), prm::Tags::CSysLinked};
    prm::Observer csysObserver; //so we can block dirtying of feature in update.
    prm::Observer syncObserver; //so we can activate and deactivate csys and csyslinked
    ann::CSysDragger csysDragger;
    ann::SeerShape sShape;
    
    //optionals
    std::optional<prm::Parameter> length;
    osg::ref_ptr<lbr::IPGroup> lengthIP;
    std::optional<prm::Parameter> width;
    osg::ref_ptr<lbr::IPGroup> widthIP;
    std::optional<prm::Parameter> height;
    osg::ref_ptr<lbr::IPGroup> heightIP;
    std::optional<prm::Parameter> radius; //cylinder
    osg::ref_ptr<lbr::IPGroup> radiusIP;
    std::optional<prm::Parameter> radius1; //cone
    osg::ref_ptr<lbr::IPGroup> radius1IP;
    std::optional<prm::Parameter> radius2; //cone
    osg::ref_ptr<lbr::IPGroup> radius2IP;
    
    Primitive() = delete;
    Primitive(const Input &input);
    lbr::IPGroup* common(prm::Parameter*); //dynamically allocated return. sets non zero positive constraint.
    prm::Parameter* addLength(double);
    prm::Parameter* addWidth(double);
    prm::Parameter* addHeight(double);
    prm::Parameter* addRadius(double);
    prm::Parameter* addRadius1(double);
    prm::Parameter* addRadius2(double);
    void IPsToCsys();
  private:
    void prmActiveSync();
  };
}

#endif //FTR_PRIMITIVE_H
