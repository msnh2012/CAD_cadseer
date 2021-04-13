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

#ifndef FTR_DATUMPLANE_H
#define FTR_DATUMPLANE_H

#include <osg/ref_ptr>

#include "tools/idtools.h"
#include "feature/ftrpick.h"
#include "feature/ftrbase.h"

namespace osg{class MatrixTransform; class Switch;}
namespace mdv{class DatumPlane;}
namespace lbr{class IPGroup; class PLabel;}
namespace ann{class CSysDragger;}
namespace prj{namespace srl{namespace dtps{class DatumPlane;}}}
namespace prm{struct Observer;}

namespace ftr
{
  class UpdatePayload;
  class ShapeHistory;
  
  class DatumPlane : public Base
  {
  public:
    enum class DPType
    {
      Constant = 0 //!< no links
      , POffset //!< offset from 1 planar face. parameter, ipgroup
      , PCenter //!< 2 planar inputs. center, bisect
      , AAngleP //!< through axis angle to planar
      , Average3P //!< average of three planes.
      , Through3P //!< through 3 points
//       , Tangent //!< 1 face pick(use normal at uv), 1 face pick and 1 point pick
//       , PofC //!< plane of curve. xy plane of conics
    };
    
    constexpr static const char *axis = "axis";
    constexpr static const char *center = "center";
    constexpr static const char *plane = "plane";
    constexpr static const char *point = "point";
    
    DatumPlane();
    ~DatumPlane();
    
    virtual void updateModel(const UpdatePayload&) override;
    virtual void updateVisual() override;
    virtual Type getType() const override {return Type::DatumPlane;}
    virtual const std::string& getTypeString() const override {return toString(Type::DatumPlane);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
    virtual void serialWrite(const boost::filesystem::path&) override;
    void serialRead(const prj::srl::dtps::DatumPlane&);
    virtual QTextStream& getInfo(QTextStream &) const override;
    
    void setSystem(const osg::Matrixd &); //!< makes type constant.
    osg::Matrixd getSystem() const;
    
    void setSize(double);
    double getSize() const;
    
    void setPicks(const Picks&);
    const Picks& getPicks(){return picks;}
    
    void setDPType(DPType);
    DPType getDPType(){return dpType;}
    
    void setAutoSize(bool);
    
    prm::Parameter* getAutoSizeParameter();
    prm::Parameter* getSizeParameter();
    prm::Parameter* getOffsetParameter();
    prm::Parameter* getAngleParameter();
  private:
    typedef Base Inherited;
    static QIcon icon;
    DPType dpType;
    double cachedSize; //!< for auto calc.
    Picks picks;
    
    std::unique_ptr<prm::Parameter> csys;
    std::unique_ptr<prm::Parameter> flip; //!< double. reverse normal
    std::unique_ptr<prm::Parameter> autoSize; //!< bool. auto calculate radius.
    std::unique_ptr<prm::Parameter> size; //!< double. distance to edges.
    std::unique_ptr<prm::Parameter> offset; //!< double. distance for POffset
    std::unique_ptr<prm::Parameter> angle; //!< double. angle for rotationn
    std::unique_ptr<prm::Observer> prmObserver;
    std::unique_ptr<ann::CSysDragger> csysDragger; //!< for constant type
    osg::ref_ptr<lbr::PLabel> flipLabel;
    osg::ref_ptr<lbr::PLabel> autoSizeLabel;
    osg::ref_ptr<lbr::IPGroup> sizeIP; //!< for POffset
    osg::ref_ptr<lbr::IPGroup> offsetIP; //!< for POffset
    osg::ref_ptr<lbr::PLabel> angleLabel;
    osg::ref_ptr<mdv::DatumPlane> display;
    
    void updateGeometry();
    void prmActiveSync();
    void updateLabelPositions();
    
    void goUpdateConstant();
    void goUpdatePOffset(const UpdatePayload&);
    void goUpdatePCenter(const UpdatePayload&);
    void goUpdateAAngleP(const UpdatePayload&);
    void goUpdateAverage3P(const UpdatePayload&);
    void goUpdateThrough3P(const UpdatePayload&);
  };
}

#endif // FTR_DATUMPLANE_H
