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

#ifndef FTR_DATUMAXIS_H
#define FTR_DATUMAXIS_H

#include <osg/Vec3d>

#include "feature/ftrpick.h"
#include "feature/ftrbase.h"

/* types
 * 
 * constant. absolute x, y, z. no picks.
 * between 2 points. 2 picks.
 * intersection of planes both datum and faces. 2 picks.
 * from geometry. use occt::gleanAxis. 1 pick
 * 
 */

namespace mdv{class DatumAxis;}
namespace ann{class CSysDragger;}
namespace lbr{class PLabel;}
namespace prj{namespace srl{namespace dtas{class DatumAxis;}}}

namespace ftr
{
  class UpdatePayload;
  
  /**
  * @brief Datum feature representing an axis/line in 3d space.
  */
  class DatumAxis : public Base
  {
  public:
    enum class AxisType
    {
      Constant, //!< doesn't reference anything
      Points, //!< 2 points
      Intersection, //!< intersection of 2 planes.
      Geometry //!< 1 pick of geometry. ie. cylindrical face.
    };
    DatumAxis();
    virtual ~DatumAxis() override;
    
    virtual void updateModel(const UpdatePayload&) override;
    virtual void updateVisual() override;
    virtual Type getType() const override {return Type::DatumAxis;}
    virtual const std::string& getTypeString() const override {return toString(Type::DatumAxis);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
    virtual void serialWrite(const boost::filesystem::path&) override;
    void serialRead(const prj::srl::dtas::DatumAxis &);
    
    void setAxisType(AxisType);
    AxisType getAxisType() const {return axisType;}
    void setPicks(const Picks &pIn);
    const Picks& getPicks(){return picks;}
    void setAutoSize(bool);
    
    void setOrigin(const osg::Vec3d&); //!< makes type constant
    osg::Vec3d getOrigin() const {return origin;}
    void setDirection(const osg::Vec3d&); //!< makes type constant
    osg::Vec3d getDirection() const {return direction;}
    
    double getSize() const;
    
    prm::Parameter* getCSysParameter();
    prm::Parameter* getAutoSizeParameter();
    prm::Parameter* getSizeParameter();
    
  protected:
    void goUpdateConstant();
    void goUpdatePoints(const UpdatePayload&);
    void goUpdateIntersection(const UpdatePayload&);
    void goUpdateGeometry(const UpdatePayload&);
    void updateDraggerViz();
    void updateLabelViz();
    void updateVisualInternal();
    
    AxisType axisType;
    Picks picks; //understood that 1st is head and 2nd is tail of vector.
    
    osg::Vec3d origin;
    osg::Vec3d direction;
    
    std::unique_ptr<prm::Parameter> csys; //!< for constant type
    std::unique_ptr<prm::Parameter> autoSize; //!< for constant type
    std::unique_ptr<prm::Parameter> size; //!< for constant type
    std::unique_ptr<ann::CSysDragger> csysDragger; //!< for constant type
    osg::ref_ptr<lbr::PLabel> autoSizeLabel;
    osg::ref_ptr<lbr::PLabel> sizeLabel;
    
    osg::ref_ptr<mdv::DatumAxis> display;
    
    double cachedSize; //!< always set by update when possible.
    
  private:
    static QIcon icon;
  };
}

#endif // FTR_DATUMAXIS_H
