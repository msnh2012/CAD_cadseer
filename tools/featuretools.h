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

#include "tools/shapevector.h"
#include "feature/ftrpick.h"

namespace ann{class SeerShape;}
namespace ftr{class Base; class ShapeHistory; struct Pick; class UpdatePayload;}
namespace slc{class Message; class Container; typedef std::vector<Message> Messages;}

namespace tls
{
  /*! @brief Convert a selection message to a feature pick.
    * 
    * @param messageIn message to convert.
    * @param featureIn seer shape that is the target of the selection
    * @param pHistory project history used to generate pick history.
    * @return ftr:Pick representing the selection message.
    * @note will call convertToPick with SeerShape if applicable.
   */
  ftr::Pick convertToPick
  (
    const slc::Message &messageIn
    , const ftr::Base &featureIn
    , const ftr::ShapeHistory &pHistory
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
  
  /*! @brief Convert a selection container to a feature pick.
    * 
    * @param containerIn selection container to convert.
    * @param feature a feature that may or may not contain a seer shape
    * @param pHistory project history used to generate pick history.
    * @return ftr:Pick representing the selection message.
    * @note resolvedIds and highlightIds are set to containerIn.shapeid if not nil.
    * startpoints and endpoints get converted to ids for vertices.
   */
  ftr::Pick convertToPick
  (
    const slc::Container &containerIn
    , const ftr::Base &feature
    , const ftr::ShapeHistory &pHistory
  );
  
  /*! @brief Convert a feature pick to a selection message.
    * 
    * @param pickIn resolved pick to convert.
    * @param featureIn feature target of the pick.
    * @return selection message vector representing resolved pick.
    * @note pickIn should have been resolved with featureIn
   */
  slc::Messages convertToMessage(const ftr::Pick &pickIn, const ftr::Base *featureIn);
  
  /*! @class Resolver
   * @brief Used to resolve picks to ids, shapes and messages.
   * @note In 'resolve', we copy the passed in pick and clear both
   * 'resolvedIds' and 'highlightIds' of our copy 'workPick'. Then
   * we then set workPicks 'resolvedIds' and 'highlightIds'. There
   * are 2 modes of operation. When we construct with UpdatePayload
   * resolve will be probing the entire project shape history
   * to find the appropriate shapes. When we construct with Base,
   * there is no real id resolution going on and the root id in
   * the pick shape history should be already in the feature seershape.
   */
  class Resolver
  {
  public:
    Resolver() = delete;
    Resolver(const ftr::UpdatePayload&);
    Resolver(const ftr::Base*);
    Resolver(const Resolver&) = delete;
    Resolver(Resolver&& other) = default;
    Resolver& operator=(Resolver&& other) = default;
    
    bool resolve(const ftr::Pick&);
    bool hasSucceeded();
    
    const ftr::Base* getFeature() const {return feature;} //!< maybe nullptr
    const ann::SeerShape* getSeerShape() const {return sShape;} //!< maybe nullptr
    
    //@{
    //! obtain results from the last run pick resolve
    const std::vector<boost::uuids::uuid>& getResolvedIds() const {return workPick.resolvedIds;}
    slc::Messages convertToMessages() const;
    occt::ShapeVector getShapes(bool = true) const;
    std::vector<osg::Vec3d> getPoints() const;
    //@}
    
    //@{
    //! convenience functions that call resolve before returning results. Nice for 'one offs'.
    const std::vector<boost::uuids::uuid>& getResolvedIds(const ftr::Pick&);
    slc::Messages convertToMessages(const ftr::Pick&);
    occt::ShapeVector getShapes(const ftr::Pick&, bool = true);
    std::vector<osg::Vec3d> getPoints(const ftr::Pick&);
    //@}
    
    const ftr::Pick& getPick() const {return workPick;}
    
  private:
    const ftr::UpdatePayload *payload = nullptr;
    const ftr::Base *feature = nullptr; //!< set from pick passed in and payload.
    const ann::SeerShape *sShape = nullptr; //!< set from pick passed in and payload.
    ftr::Pick workPick; //!< copied from pick passed in to 'resolve'
    
    void resolveViaPayload();
    void resolveViaFeature();
  };
  
  /*! @struct Connector
   * @brief Used to store feature ids and tags for later project graph connections
   */
  struct Connector
  {
    void add(const boost::uuids::uuid &idIn, const std::string &sIn)
    {
      pairs.emplace_back(idIn, sIn);
    }
    std::vector<std::pair<boost::uuids::uuid, std::string>> pairs;
  };
}

#endif // TLS_FEATURETOOLS_H
