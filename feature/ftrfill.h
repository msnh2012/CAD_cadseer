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

#ifndef FTR_FILL_H
#define FTR_FILL_H

#include <osg/ref_ptr>

#include "feature/ftrpick.h"
#include "feature/ftrbase.h"

namespace prj{namespace srl{namespace fls{class Fill;}}}
namespace lbr{class PLabel;}

namespace ftr
{
  namespace Fill
  {
    namespace InputTags
    {
      inline constexpr std::string_view initialPick = "initialPick";
      inline constexpr std::string_view edgePick = "edgePick";
      inline constexpr std::string_view facePick = "facePick";
      inline constexpr std::string_view internalPicks = "internalPicks"; //can be empty
    }
    namespace PrmTags
    {
      inline constexpr std::string_view initialPick = "initialPick"; //input face to deform, can be null
      inline constexpr std::string_view edgePick = "edgePick";
      inline constexpr std::string_view facePick = "facePick";
      inline constexpr std::string_view internalPicks = "internalPicks"; //can be empty
      inline constexpr std::string_view continuity = "continuity";
    }
    
    struct Boundary
    {
      Boundary();
      ~Boundary() noexcept;
      Boundary(const Boundary&) = delete;
      Boundary& operator=(const Boundary&) = delete;
      Boundary(Boundary&&) noexcept = default;
      Boundary& operator=(Boundary&&) noexcept = default;
      prm::Parameter edgePick;
      prm::Parameter facePick;
      prm::Parameter continuity;
      osg::ref_ptr<lbr::PLabel> continuityLabel;
    };
    using Boundaries = std::list<Boundary>;
    
    class Feature : public Base
    {
    public:
      
      Feature();
      ~Feature() override;
      
      void updateModel(const UpdatePayload&) override;
      Type getType() const override {return Type::Fill;}
      const std::string& getTypeString() const override {return toString(Type::Fill);}
      const QIcon& getIcon() const override {return icon;}
      Descriptor getDescriptor() const override {return Descriptor::Create;}
      
      void serialWrite(const boost::filesystem::path&) override;
      void serialRead(const prj::srl::fls::Fill&);
      
      Boundary& addBoundary();
      void removeBoundary(int);
      Boundaries& getBoundaries() const;
      
    private:
      static QIcon icon;
      struct Stow;
      std::unique_ptr<Stow> stow;
    };
  }
}

#endif //FTR_FILL_H
