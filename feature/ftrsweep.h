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

#include "feature/ftrpick.h"
#include "feature/ftrbase.h"

namespace ann{class SeerShape; class LawFunction;}
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
namespace lbr{class PLabel;}
namespace lwf{struct Cue;}

namespace ftr
{
  /*! @struct SweepProfile
   * @brief value class to bundle a profile pick with options
   */
  struct SweepProfile
  {
    SweepProfile();
    SweepProfile(const Pick&);
    SweepProfile(const Pick&, bool, bool);
    SweepProfile(const prj::srl::swps::SweepProfile&);
    ~SweepProfile();
    prj::srl::swps::SweepProfile serialOut() const;
    Pick pick;
    std::shared_ptr<prm::Parameter> contact;
    std::shared_ptr<prm::Parameter> correction;
    osg::ref_ptr<lbr::PLabel> contactLabel;
    osg::ref_ptr<lbr::PLabel> correctionLabel;
  };
  typedef std::vector<SweepProfile> SweepProfiles;
  
  /*! @struct SweepAuxilary
   * @brief value class to exchange auxiliary data to and from dialog
   */
  struct SweepAuxiliary
  {
    SweepAuxiliary();
    SweepAuxiliary(const Pick&);
    SweepAuxiliary(const Pick&, bool, int);
    SweepAuxiliary(const prj::srl::swps::SweepAuxiliary&);
    ~SweepAuxiliary();
    prj::srl::swps::SweepAuxiliary serialOut() const;
    void serialIn(const prj::srl::swps::SweepAuxiliary&);
    Pick pick;
    std::shared_ptr<prm::Parameter> curvilinearEquivalence;
    std::shared_ptr<prm::Parameter> contactType;
    osg::ref_ptr<lbr::PLabel> curvilinearEquivalenceLabel;
    osg::ref_ptr<lbr::PLabel> contactTypeLabel;
  };
  
  struct SweepBinormal
  {
    SweepBinormal();
    SweepBinormal(const Pick&);
    SweepBinormal(const Picks&);
    SweepBinormal(const osg::Vec3d&);
    SweepBinormal(const Picks&, const osg::Vec3d&); //!< don't use. just initializer for constructor delegation
    ~SweepBinormal();
    prj::srl::swps::SweepBinormal serialOut() const;
    void serialIn(const prj::srl::swps::SweepBinormal&);
    Picks picks;
    std::shared_ptr<prm::Parameter> binormal;
    osg::ref_ptr<lbr::PLabel> binormalLabel;
  };
  
  /*! @struct SweepData
   * @brief value class to exchange sweep data to and from dialog
   */
  struct SweepData
  {
    Pick spine;
    SweepProfiles profiles;
    SweepAuxiliary auxiliary;
    Pick support;
    SweepBinormal binormal;
    int trihedron;
    int transition;
    bool forceC1 = false;
    bool solid = true;
    bool useLaw = false;
  };
  
  /*! @class Sweep
   * @brief ref class for sweep feature
   * @note only works with edges and wires. No sweeping of faces.
   */
  class Sweep : public Base
  {
  public:
    constexpr static const char *spineTag = "spine";
    constexpr static const char *profileTag = "profile";
    constexpr static const char *auxiliaryTag = "auxiliary";
    constexpr static const char *supportTag = "support";
    constexpr static const char *binormalTag = "binormal";
    
    Sweep();
    ~Sweep() override;
    
    void updateModel(const UpdatePayload&) override;
    Type getType() const override {return Type::Sweep;}
    const std::string& getTypeString() const override {return toString(Type::Sweep);}
    const QIcon& getIcon() const override {return icon;}
    Descriptor getDescriptor() const override {return Descriptor::Create;}
    
    void serialWrite(const boost::filesystem::path&) override;
    void serialRead(const prj::srl::swps::Sweep&);
    
    void setSweepData(const SweepData&);
    SweepData getSweepData() const;
    
    
    
    
    void setSpine(const Pick&);
    void setProfiles(const SweepProfiles&);
    void setAuxiliary(const SweepAuxiliary&); //sets trihedron value
    void setSupport(const Pick&); //sets trihedron value
    void setBinormal(const SweepBinormal&); //sets trihedron value
    void setTrihedron(int);
    void setTransition(int);
    void setForceC1(bool);
    void setSolid(bool);
    void setUseLaw(bool);
    void setLaw(const lwf::Cue&); //doesn't change useLaw
    
    const Pick& getSpine(){return spine;}
    const SweepProfiles& getProfiles(){return profiles;}
    const SweepAuxiliary& getAuxiliary(){return auxiliary;}
    const Pick& getSupport(){return support;}
    const SweepBinormal& getBinormal(){return binormal;}
    
    int getTrihedron();
    int getTransition();
    bool getForceC1();
    bool getSolid();
    bool getUseLaw();
    
    
    //! remove associations between law and feature
    void severLaw();
    //! add associations between law and feature
    void attachLaw();
    //! when law structure changes we need to regenerate viz.
    void regenerateLawViz();
    //! just redraw law viz using scale.
    void updateLawViz(double);
    
  private:
    std::unique_ptr<ann::SeerShape> sShape;
    std::unique_ptr<ann::LawFunction> lawFunction;
    std::unique_ptr<prm::Parameter> trihedron;
    std::unique_ptr<prm::Parameter> transition;
    std::unique_ptr<prm::Parameter> forceC1;
    std::unique_ptr<prm::Parameter> solid;
    std::unique_ptr<prm::Parameter> useLaw;
    Pick spine;
    SweepProfiles profiles;
    SweepAuxiliary auxiliary;
    Pick support;
    SweepBinormal binormal;
    
    osg::ref_ptr<lbr::PLabel> trihedronLabel;
    osg::ref_ptr<lbr::PLabel> transitionLabel;
    osg::ref_ptr<lbr::PLabel> forceC1Label;
    osg::ref_ptr<lbr::PLabel> solidLabel;
    osg::ref_ptr<lbr::PLabel> useLawLabel;
    osg::ref_ptr<osg::Switch> auxiliarySwitch;
    osg::ref_ptr<osg::Switch> binormalSwitch;
    osg::ref_ptr<osg::Switch> lawSwitch;
    
    boost::uuids::uuid solidId;
    boost::uuids::uuid shellId;
    boost::uuids::uuid firstFaceId;
    boost::uuids::uuid lastFaceId;
    std::map<boost::uuids::uuid, boost::uuids::uuid> outerWireMap; //!< map face id to wire id
    typedef std::map<boost::uuids::uuid, std::vector<boost::uuids::uuid>> InstanceMap;
    InstanceMap instanceMap; //for profile vertices and edges -> edges and face.
    std::map<boost::uuids::uuid, boost::uuids::uuid> firstShapeMap; //faceId to edgeId for first shapes.
    std::map<boost::uuids::uuid, boost::uuids::uuid> lastShapeMap; //faceId to edgeId for last shapes.
    
    static QIcon icon;
    
    void cleanTrihedron();
  };
}

#endif //FTR_SWEEP_H
