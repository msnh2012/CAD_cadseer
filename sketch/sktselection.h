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

#ifndef SKT_SELECTION_H
#define SKT_SELECTION_H

#include <boost/timer/timer.hpp>

#include <osgGA/GUIEventHandler>

#include "sketch/sktnodemasks.h"

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
