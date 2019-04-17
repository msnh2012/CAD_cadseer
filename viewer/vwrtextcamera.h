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

#ifndef VWR_TEXTCAMERA_H
#define VWR_TEXTCAMERA_H

#include <osg/Camera>
#include <osg/observer_ptr>
#include <osg/NodeVisitor>
#include <osgGA/GUIEventHandler>

#include "selection/slcmessage.h"

namespace osgViewer{class GraphicsWindow;}
namespace osg{class Switch; class Group;}
namespace osgText{class osgText;}

namespace msg{class Message; struct Node; struct Sift;}
namespace vwr
{
  class TextCamera;
  
class ResizeEventHandler : public osgGA::GUIEventHandler
{
public:
  ResizeEventHandler(const osg::observer_ptr<TextCamera> slaveCameraIn);
  
protected:
  virtual bool handle(const osgGA::GUIEventAdapter& eventAdapter,
                      osgGA::GUIActionAdapter& actionAdapter, osg::Object *object,
                      osg::NodeVisitor *nodeVistor);
  
  osg::observer_ptr<TextCamera> slaveCamera;
};


class TextCamera : public osg::Camera
{
public:
    TextCamera(osgViewer::GraphicsWindow *);
    virtual ~TextCamera() override;
    bool cleanFade(); //return true if something was changed.
    void layoutStatus(); //position the text on screen
    void layoutSelection(); //position the text on screen
    void layoutCommand(); //position the text on screen
private:
  std::unique_ptr<msg::Node> node;
  std::unique_ptr<msg::Sift> sift;
  void setupDispatcher();
  void preselectionAdditionDispatched(const msg::Message &);
  void preselectionSubtractionDispatched(const msg::Message &);
  void selectionAdditionDispatched(const msg::Message &);
  void selectionSubtractionDispatched(const msg::Message &);
  void statusTextDispatched(const msg::Message &);
  void commandTextDispatched(const msg::Message &);
    
  osg::ref_ptr<osg::Switch> infoSwitch;
  
  osg::ref_ptr<osgText::Text> selectionLabel;
  void updateSelectionLabel();
  std::string preselectionText;
  
  std::vector<slc::Message> selections;
  
  osg::ref_ptr<osg::Group> statusGroup;
  std::vector<osg::ref_ptr<osgText::Text>> statusFades;
  osg::ref_ptr<osgText::Text> statusLabel;
  
  osg::ref_ptr<osgText::Text> commandLabel;
};
}

#endif // VWR_TEXTCAMERA_H
