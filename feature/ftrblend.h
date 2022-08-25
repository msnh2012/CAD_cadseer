/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2015  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef FTR_BLEND_H
#define FTR_BLEND_H

#include <memory>

#include <osg/ref_ptr>

#include "library/lbrplabel.h"
#include "parameter/prmparameter.h"
#include "feature/ftrbase.h"

namespace prj{namespace srl{namespace blns
{
  class Blend;
  class Constant;
  class Variable;
  class Entry;
}}}
namespace ann{class SeerShape;}
namespace ftr
{
  namespace Blend
  {
    namespace InputTags
    {
      inline constexpr std::string_view constant = "constant";
      inline constexpr std::string_view contourPick = "contourPick";
      inline constexpr std::string_view entryPick = "entryPick";
    }
    namespace PrmTags
    {
      inline constexpr std::string_view filletShape = "filletShape";
      inline constexpr std::string_view blendType = "blendType";
      inline constexpr std::string_view contourPicks = "contourPicks";
      inline constexpr std::string_view entryPick = "entryPick";
      inline constexpr std::string_view lawSpinePick = "lawSpinePick";
      inline constexpr std::string_view lawEdgePick = "lawEdgePick";
    }
    
    struct Constant
    {
      prm::Parameter contourPicks;
      prm::Parameter radius;
      osg::ref_ptr<lbr::PLabel> radiusLabel;
      
      Constant();
      Constant(const prj::srl::blns::Constant&);
      Constant(const Constant&) = delete;
      Constant& operator=(const Constant&) = delete;
      Constant(Constant&&) = delete;
      Constant& operator=(Constant&&) = delete;
      
      prj::srl::blns::Constant serialOut() const;
      
      prm::Parameters getParameters();
    };
    using Constants = std::list<Constant>;

    struct Entry
    {
      prm::Parameter entryPick; //!< edge or vertex.
      prm::Parameter radius; //!< value of blend.
      prm::Parameter position; //!< parameter along edge 0 to 1. ignored if vertex. maybe invalid
      osg::ref_ptr<lbr::PLabel> radiusLabel; //!< graphic icon
      osg::ref_ptr<lbr::PLabel> positionLabel; //!< graphic icon
      
      Entry();
      Entry(const prj::srl::blns::Entry&);
      Entry(const Entry&) = delete;
      Entry& operator=(const Entry&) = delete;
      Entry(const Entry&&) = delete;
      Entry& operator=(Entry&&) = delete;
      
      prm::Parameters getParameters();
      void prmActiveSync();
      
      prj::srl::blns::Entry serialOut() const;
    };
    using Entries = std::list<Entry>;

    /*! @struct Variable
     * @brief Data for a variable blend
     * 
     * @details We have to combine the picks/edges/contours
     * and the parameters. Think about a box where 3 adjacent
     * edges are picked and we share a vertex on all 3 edges.
     */
    struct Variable
    {
      prm::Parameter contourPicks;
      Entries entries;
      
      Variable();
      Variable(const Variable&) = delete;
      Variable& operator=(const Variable&) = delete;
      Variable(const Variable&&) = delete;
      Variable& operator=(Variable&&) = delete;
      
      prj::srl::blns::Variable serialOut() const;
      void serialIn(const prj::srl::blns::Variable&);
    };
    
    class Feature : public Base
    {
      public:
        
        Feature();
        ~Feature() override;
        
        Constant& addConstant();
        void removeConstant(int);
        Constants& getConstants() const;
        Constant& getConstant(int) const;
        
        void setVariablePicks(const Picks&);
        Entry& addVariableEntry();
        void removeVariableEntry(int);
        Variable& getVariable() const; //!< assert on blend type
        
        void updateModel(const UpdatePayload&) override;
        Type getType() const override {return Type::Blend;}
        const std::string& getTypeString() const override {return toString(Type::Blend);}
        const QIcon& getIcon() const override {return icon;}
        Descriptor getDescriptor() const override {return Descriptor::Alter;}
        void serialWrite(const boost::filesystem::path&) override; //!< write xml file. not const, might reset a modified flag.
        void serialRead(const prj::srl::blns::Blend&); //!<initializes this from serial. not virtual, type already known.
      
      private:
        static QIcon icon;
        struct Stow;
        std::unique_ptr<Stow> stow;
    };
  }
}

#endif // FTR_BLEND_H
