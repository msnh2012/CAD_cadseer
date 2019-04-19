/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2018  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef TLS_FEATURETOOLS_H
#define TLS_FEATURETOOLS_H

#include <vector>

#include <boost/uuid/uuid.hpp>
#include <boost/optional/optional_fwd.hpp>

#include "feature/ftrpick.h"

namespace ann{class SeerShape;}
namespace ftr{class Base; class ShapeHistory; class Pick;}
namespace slc{class Message; class Container;}

namespace tls
{
  /*! @brief Output of pick resolution
   * 
   * 
   */
  struct Resolved
  {
    Resolved(const ftr::Base *, const boost::uuids::uuid&, const ftr::Pick&);  
    const ftr::Base *feature; //!< feature containing the result
    boost::uuids::uuid resultId; //!< result id of pick in feature.
    ftr::Pick pick; //!< pick to resolve.
    bool operator<(const Resolved&) const;
    bool operator==(const Resolved&) const;
    boost::optional<osg::Vec3d> getPoint(const ann::SeerShape&) const;
  };
  
  /*! @brief Get feature and shape id pairs.
   * 
   * @param features input features that contain the result of the id.
   * @param picks picks to resolve.
   * @param pHistory project history.
   * @return vector of resolved
   * @note pilot test for this is the booleans.
   * @note if 1 pick results into 3 ids there will be 3 entries in the return.
   */
  std::vector<Resolved>
  resolvePicks
  (
    const std::vector<const ftr::Base*> &features,
    std::vector<ftr::Pick> picks, //make a work copy of the picks.
    const ftr::ShapeHistory &pHistory
  );
  
  /*! @brief Same as overloaded. Just convenience.

   */
  std::vector<Resolved>
  resolvePicks
  (
    const ftr::Base *feature,
    const ftr::Pick &pick,
    const ftr::ShapeHistory &pHistory
  );
  
  /*! @brief Convert a selection message to a feature pick.
    * 
    * @param messageIn message to convert.
    * @param sShapeIn seer shape that is the target of the selection
    * @param pHistory project history used to generate pick history.
    * @return ftr:Pick representing the selection message.
    * @note resolvedIds and highlightIds are set to id.
    * startpoints and endpoints get converted to ids for vertices.
   */
  ftr::Pick convertToPick
  (
    const slc::Message &messageIn
    , const ann::SeerShape &sShapeIn
    , const ftr::ShapeHistory &pHistory
  );
  
  /*! @brief Convert a selection container to a feature pick.
    * 
    * @param containerIn selection container to convert.
    * @param sShapeIn seer shape that is the target of the selection
    * @param pHistory project history used to generate pick history.
    * @return ftr:Pick representing the selection message.
    * @note resolvedIds and highlightIds are set to containerIn.shapeid if not nil.
    * startpoints and endpoints get converted to ids for vertices.
   */
  ftr::Pick convertToPick
  (
    const slc::Container &containerIn
    , const ann::SeerShape &sShapeIn
    , const ftr::ShapeHistory &pHistory
  );
  
  /*! @brief Convert a feature pick to a selection message.
    * 
    * @param pickIn resolved pick to convert.
    * @param sShapeIn seer shape that is the target of the pick.
    * @return selection message representing the pick.
    * @note assumes pickIn has been resolved with featureIn
   */
  slc::Message convertToMessage(const ftr::Pick &pickIn, const ftr::Base *featureIn);
}

#endif // TLS_FEATURETOOLS_H
