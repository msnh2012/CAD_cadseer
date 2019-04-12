/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2019 Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef FTR_RULED_H
#define FTR_RULED_H

#include <set>

#include "feature/pick.h"
#include "feature/base.h"

namespace ann{class SeerShape;}
namespace prj{namespace srl{class FeatureRuled;}}

namespace ftr
{
  /*! @class Ruled
   *  @brief Surface created from connecting 2 edges or 2 wires
   *  with lines.
   *  @note Wires need to contain the same number of edges.
   *  cannot connect edges with wires. 
   * 
   */
  class Ruled : public Base
  {
  public:
    constexpr static const char *pickZero = "pickZero";
    constexpr static const char *pickOne = "pickOne";
    
    Ruled();
    ~Ruled() override;
    
    void updateModel(const UpdatePayload&) override;
    Type getType() const override {return Type::Ruled;}
    const std::string& getTypeString() const override {return toString(Type::Ruled);}
    const QIcon& getIcon() const override {return icon;}
    Descriptor getDescriptor() const override {return Descriptor::Create;}
    
    void serialWrite(const boost::filesystem::path&) override;
    void serialRead(const prj::srl::FeatureRuled&);
    
    void setPicks(const Picks&);
    const Picks& getPicks() const {return picks;}
    
  private:
    std::unique_ptr<ann::SeerShape> sShape;
    Picks picks;
    boost::uuids::uuid parentId; //!< edge to edge = face id. wire to wire = shell id.
    typedef std::set<boost::uuids::uuid> IdSet;
    typedef std::map<IdSet, boost::uuids::uuid> SetToIdMap;
    SetToIdMap efMap; //!< map 2 edges to one generated face. only used on wires
    SetToIdMap veMap; //!< map 2 verts to one projected edge.
    std::map<boost::uuids::uuid, boost::uuids::uuid> outerWireMap; //!< map face id to wire id
    void goRuledMapping();
    static QIcon icon;
  };
}

#endif //FTR_RULED_H
