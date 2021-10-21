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

#ifndef FTR_SWEEP_H
#define FTR_SWEEP_H

#include "feature/ftrbase.h"

namespace ann{class SeerShape; class LawFunction;}
namespace lwf{struct Cue;}
namespace prj
{
  namespace srl
  {
    namespace swps
    {
      class SweepProfile;
      class SweepProfiles;
      class SweepAuxiliary;
      class SweepBinormal;
      class Sweep;
    }
  }
}

namespace ftr
{
  namespace Sweep
  {
    namespace InputTags
    {
      inline constexpr std::string_view profile = "profile";
      inline constexpr std::string_view binormal = "binormal";
      inline constexpr std::string_view support = "support";
      inline constexpr std::string_view auxiliary = "auxiliary";
    }
    /* Variable parameters like profiles won't be added to the parameter array.
     * So they will only be accessible through typed feature.
     */
    namespace PrmTags
    {
      inline constexpr std::string_view none = "none";
      inline constexpr std::string_view spine = "spine";
      inline constexpr std::string_view profile = "profile";
      inline constexpr std::string_view support = "support";
      inline constexpr std::string_view trihedron = "trihedron";
      inline constexpr std::string_view transition = "transition";
      inline constexpr std::string_view forcec1 = "forcec1";
      inline constexpr std::string_view solid = "solid";
      inline constexpr std::string_view useLaw = "useLaw";
      inline constexpr std::string_view auxPick = "auxPick";
      inline constexpr std::string_view auxCurvilinear = "auxCurvilinear";
      inline constexpr std::string_view auxContact = "auxContact";
      inline constexpr std::string_view biPicks = "biPicks";
      inline constexpr std::string_view biVector = "biVector";
    }
    
    /*! @struct Profile
    * @brief value class to bundle a profile pick with options
    */
    struct Profile
    {
      Profile();
      Profile(const prj::srl::swps::SweepProfile&);
      Profile(const Profile&) = delete;
      Profile& operator=(const Profile&) = delete;
      Profile(Profile&&) noexcept = default;
      Profile& operator=(Profile&&) noexcept = default;
      ~Profile() noexcept;
      prj::srl::swps::SweepProfile serialOut() const;
      prm::Parameter pick;
      prm::Parameter contact;
      prm::Parameter correction;
      osg::ref_ptr<lbr::PLabel> contactLabel;
      osg::ref_ptr<lbr::PLabel> correctionLabel;
      
      prm::Parameters getParameters();
    };
    typedef std::list<Profile> Profiles;
    
    /*! @struct Auxilary
    * @brief value class to exchange auxiliary data to and from dialog
    */
    struct Auxiliary
    {
      Auxiliary();
      Auxiliary(const prj::srl::swps::SweepAuxiliary&);
      Auxiliary(const Auxiliary&) = delete;
      Auxiliary& operator=(const Auxiliary&) = delete;
      Auxiliary(Auxiliary&&) noexcept = default;
      Auxiliary& operator=(Auxiliary&&) noexcept = default;
      ~Auxiliary() noexcept;
      prj::srl::swps::SweepAuxiliary serialOut() const;
      void serialIn(const prj::srl::swps::SweepAuxiliary&);
      void setActive(bool);
      prm::Parameter pick;
      prm::Parameter curvilinearEquivalence;
      prm::Parameter contactType;
      osg::ref_ptr<lbr::PLabel> curvilinearEquivalenceLabel;
      osg::ref_ptr<lbr::PLabel> contactTypeLabel;
    };
    
    struct Binormal
    {
      Binormal();
      Binormal(const Binormal&) = delete;
      Binormal& operator=(const Binormal&) = delete;
      Binormal(Binormal&&) noexcept = default;
      Binormal& operator=(Binormal&&) noexcept = default;
      ~Binormal() noexcept;
      prj::srl::swps::SweepBinormal serialOut() const;
      void serialIn(const prj::srl::swps::SweepBinormal&);
      void setActive(bool);
      prm::Parameter picks;
      prm::Parameter vector;
      osg::ref_ptr<lbr::PLabel> vectorLabel;
    };
    
    /*! @class Feature
    * @brief ref class for sweep feature
    * @note only works with edges and wires. No sweeping of faces.
    */
    class Feature : public Base
    {
    public:
      Feature();
      ~Feature() override;
      
      void updateModel(const UpdatePayload&) override;
      Type getType() const override {return Type::Sweep;}
      const std::string& getTypeString() const override {return toString(Type::Sweep);}
      const QIcon& getIcon() const override {return icon;}
      Descriptor getDescriptor() const override {return Descriptor::Create;}
      
      void serialWrite(const boost::filesystem::path&) override;
      void serialRead(const prj::srl::swps::Sweep&);
      
      Profile& addProfile();
      void removeProfile(int);
      Profiles& getProfiles() const;
      
      Auxiliary& getAuxiliary() const;
      
      Binormal& getBinormal() const;

      void setLaw(const lwf::Cue&); //doesn't change useLaw
    private:
      static QIcon icon;
      struct Stow;
      std::unique_ptr<Stow> stow;
    };
  }
}

#endif //FTR_SWEEP_H
