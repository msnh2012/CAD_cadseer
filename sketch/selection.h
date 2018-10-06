#ifndef SKT_SELECTION_H
#define SKT_SELECTION_H

#include <boost/timer/timer.hpp>

#include <osgGA/GUIEventHandler>

#include "nodemasks.h"

namespace osg
{
  class Group;
}

namespace skt
{
  class Visual;
  
  /*! @class Selection
   * @brief openscenegraph event handler for selection
   */
  class Selection : public osgGA::GUIEventHandler
  {
  public:
    Selection(Visual*);
    
    /** @anchor Masking 
     * @name Masking of selection
     * 
     * Functions for controlling what is currently selectable.
     * By default everything is selectable.
     */
    ///@{
    void setMaskAll();
    void setMaskNone();
    NodeMask getMask();
    void setMask(NodeMask);
    void selectionPlaneOn();
    void selectionPlaneOff();
    void workPlaneOn();
    void workPlaneOff();
    void workPlaneAxesOn();
    void workPlaneAxesOff();
    void workPlaneOriginOn();
    void workPlaneOriginOff();
    void entitiesOn();
    void entitiesOff();
    void constraintsOn();
    void constraintsOff();
    ///@}
    
  protected:
    virtual bool handle(const osgGA::GUIEventAdapter&, osgGA::GUIActionAdapter&, osg::Object*, osg::NodeVisitor*) override;
    
  private:
    Visual *visual; //!< Target for events.
    NodeMask mask = 0xffffffff; //!< Node mask for intersection visitors.
    bool leftButtonDown = false; //!< Left mouse button state. Used for drag operation.
    
    boost::timer::cpu_timer timer;
  };
}

#endif // SKT_SELECTION_H
