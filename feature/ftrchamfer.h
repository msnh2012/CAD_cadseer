/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef FTR_CHAMFER_H
#define FTR_CHAMFER_H

#include "feature/ftrbase.h"
#include "parameter/prmparameter.h"

namespace prj{namespace srl{namespace chms{class Chamfer; class Entry;}}}
namespace lbr{class PLabel;}

namespace ftr
{
  namespace Chamfer
  {
    namespace PrmTags
    {
      inline constexpr std::string_view mode = "mode";
      inline constexpr std::string_view style = "style";
      inline constexpr std::string_view edgePicks = "edgePicks";
      inline constexpr std::string_view facePicks = "facePicks";
      inline constexpr std::string_view dist2Angle = "dist2Angle";
    }
    
    namespace InputTags
    {
      inline constexpr std::string_view edge = "edge";
      inline constexpr std::string_view face = "face";
    }
    
    /*! @struct Entry
     *@brief describes chamfer options
     *
     *@details symmetric uses only 1 parameter and 1 label and edgePicks.
     * TwoDistances and DistanceAngle: edgePicks.size() == facePicks.size()
     */
    struct Entry
    {
      prm::Parameter style;
      prm::Parameter edgePicks;
      prm::Parameter facePicks;
      prm::Parameter distance;
      prm::Parameter dist2Angle;
      osg::ref_ptr<lbr::PLabel> styleLabel;
      osg::ref_ptr<lbr::PLabel> distanceLabel;
      osg::ref_ptr<lbr::PLabel> dist2AngleLabel;
      prm::Observer styleObserver;
      
      Entry();
      Entry(const prj::srl::chms::Entry&);
      Entry(const Entry&) = delete;
      Entry& operator=(const Entry&) = delete;
      Entry(Entry&&) noexcept = default;
      Entry& operator=(Entry&&) noexcept = default;
      ~Entry() noexcept;
      
      prj::srl::chms::Entry serialOut() const;
      void serialIn(const prj::srl::chms::Entry&);
      
      prm::Parameters getParameters();
      void prmActiveSync();
    };
    using Entries = std::list<Entry>;
    
    /*! @class Feature
     * @brief chamfer feature
     * 
     * @details
     * Mode::Classic can have: Style::Symmetric, Style::TwoDistances, Style::DistanceAngle
     * Mode::Throat can only have: Style::Symmetric
     * Mode::ThroatPenetration can only have: Style::TwoDistances 
     * 
     */
    class Feature : public Base
    {
    public:
      
      Feature();
      ~Feature() override;
      void updateModel(const UpdatePayload&) override;
      Type getType() const override {return Type::Chamfer;}
      const std::string& getTypeString() const override {return toString(Type::Chamfer);}
      const QIcon& getIcon() const override {return icon;}
      Descriptor getDescriptor() const override {return Descriptor::Alter;}
      void serialWrite(const boost::filesystem::path&) override;
      void serialRead(const prj::srl::chms::Chamfer&);
      
      Entry& addSymmetric(); //!< assert on mode conflict
      Entry& addTwoDistances(); //!< assert on mode conflict
      Entry& addDistanceAngle(); //!< assert on mode conflict
      void removeEntry(int); //!< assert on entry index
      Entry& getEntry(int);
      Entries& getEntries();
      
    private:
      static QIcon icon;
      struct Stow;
      std::unique_ptr<Stow> stow;
    };
  }
}

#endif // CHAMFER_H
