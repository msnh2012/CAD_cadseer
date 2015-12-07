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

#ifndef CSYSBASE_H
#define CSYSBASE_H

#include <gp_Ax2.hxx>

#include <osg/csysdragger.h>
#include <feature/base.h>
#include <message/message.h>

namespace ftr
{
  class DCallBack;
  
  /*! Base class for features dependent on a coordinate system
   *
   * Child classes must call this classes updateModel and updateVisual
   * in overrides.
   */
  class CSysBase : public Base
  {
  public:
    CSysBase();
    virtual Type getType() const override {return Type::CSys;}
    virtual const std::string& getTypeString() const override {return toString(Type::CSys);}
    virtual const QIcon& getIcon() const override {static QIcon junk; return junk;}
    virtual Descriptor getDescriptor() const override {return Descriptor::None;}
    virtual void updateModel(const UpdateMap&) override;
    void setSystem(const gp_Ax2 &systemIn);
    void setSystem(const osg::Matrixd &systemIn);
    void updateDragger(); //!< dragger to match feature system.
    const gp_Ax2& getSystem() const {return system;}
    CSysDragger& getDragger() {return *dragger;}

  protected:
    gp_Ax2 system;
    osg::ref_ptr<CSysDragger> dragger;
    osg::ref_ptr<DCallBack> callBack;
  };
  
    /*! @brief class to mark the feature dirty when the dragger is used.
   * 
   * not really in love with this, but don't see a better way.
   */
  class DCallBack : public osgManipulator::DraggerTransformCallback
  {
  public:
    DCallBack(osg::MatrixTransform *t, CSysBase *csysBaseIn);
    virtual bool receive(const osgManipulator::MotionCommand &) override;
    
        
    typedef boost::signals2::signal<void (const msg::Message &)> MessageOutSignal;
    boost::signals2::connection connectMessageOut(const MessageOutSignal::slot_type &subscriber)
    {
      return messageOutSignal.connect(subscriber);
    }
    
  private:
    CSysBase *csysBase = nullptr;
    osg::Vec3d originStart;
    MessageOutSignal messageOutSignal;
  };
}

#endif // CSYSBASE_H
