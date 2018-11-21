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

#ifndef FTR_SKETCH_H
#define FTR_SKETCH_H

#include "feature/base.h"

namespace prj{namespace srl{class FeatureSketch;}}
namespace skt{struct Solver; class Visual;}
namespace ann{class SeerShape; class CSysDragger;}
namespace prj{namespace srl{class FeatureSketch;}}

namespace ftr
{
  /**
  * @brief 2d sketch feature.
  */
  class Sketch : public Base
  {
  public:
    Sketch();
    virtual ~Sketch() override;
    
    virtual void updateModel(const UpdatePayload&) override;
    virtual Type getType() const override {return Type::Sketch;}
    virtual const std::string& getTypeString() const override {return toString(Type::Sketch);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
    
    virtual void serialWrite(const boost::filesystem::path&) override;
    void serialRead(const prj::srl::FeatureSketch&);
    
    skt::Solver* getSolver(){return solver.get();}
    skt::Visual* getVisual(){return visual.get();}
    void draggerShow();
    void draggerHide();
    void buildDefault();
    
    bool hasHPPair(uint32_t);
    bool hasHPPair(const ftr::prm::Parameter*);
    void addHPPair(uint32_t, const std::shared_ptr<ftr::prm::Parameter>&);
    void removeHPPair(uint32_t);
    ftr::prm::Parameter* getHPParameter(uint32_t);
    uint32_t getHPHandle(const ftr::prm::Parameter*);
    
  protected:
    std::unique_ptr<ann::SeerShape> sShape;
    std::unique_ptr<skt::Solver> solver;
    std::unique_ptr<skt::Visual> visual;
    std::unique_ptr<prm::Parameter> csys;
    std::unique_ptr<ann::CSysDragger> csysDragger;
    osg::ref_ptr<osg::Switch> draggerSwitch;
    std::vector<boost::uuids::uuid> wireIds;
    std::vector<std::pair<uint32_t, std::shared_ptr<ftr::prm::Parameter>>> hpPairs;
    
    void updateSeerShape();
    
    
  private:
    static QIcon icon;
  };
}

#endif // FTR_SKETCH_H
