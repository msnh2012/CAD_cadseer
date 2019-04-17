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

#include <iostream>
#include <sstream>
#include <assert.h>

#include <boost/filesystem.hpp>
#include <boost/current_function.hpp>

#include <osgViewer/GraphicsWindow>

#include "globalutilities.h"
#include "tools/idtools.h"
#include "application/appapplication.h"
#include "project/prjproject.h"
#include "project/prjmessage.h"
#include "modelviz/mdvnodemaskdefs.h"
#include "feature/ftrbase.h"
#include "message/msgnode.h"
#include "message/msgsift.h"
#include "viewer/vwrmessage.h"
#include "selection/slcvisitors.h"
#include "project/serial/xsdcxxoutput/view.h"
#include "viewer/vwroverlay.h"

using namespace vwr;

Overlay::Overlay(osgViewer::GraphicsWindow *windowIn) : osg::Camera()
{
  node = std::make_unique<msg::Node>();
  node->connect(msg::hub());
  sift = std::make_unique<msg::Sift>();
  sift->name = "vwr::Overlay";
  node->setHandler(std::bind(&msg::Sift::receive, sift.get(), std::placeholders::_1));
    
  fleetingGeometry = new osg::Switch();
  this->addChild(fleetingGeometry);
  
  setNodeMask(mdv::overlay);
  setName("overlay");
  setupDispatcher();
  setGraphicsContext(windowIn);
  setClearMask(GL_DEPTH_BUFFER_BIT);
  setRenderOrder(osg::Camera::NESTED_RENDER, 2);
  
  osg::Camera *mainCamera = nullptr;
  const osg::GraphicsContext::Cameras &cameras = windowIn->getCameras();
  for (const auto it : cameras)
  {
    if (it->getName() == "main")
    {
      mainCamera = it;
      break;
    }
  }
  assert(mainCamera); //couldn't find main camera
  setViewport(new osg::Viewport(*mainCamera->getViewport()));
  
  this->getOrCreateStateSet()->setMode(GL_MULTISAMPLE_ARB, osg::StateAttribute::ON);
}

void Overlay::setupDispatcher()
{
  sift->insert
  (
    {
      std::make_pair
      (
        msg::Response | msg::Post | msg::Add | msg::Feature
        , std::bind(&Overlay::featureAddedDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Pre | msg::Remove | msg::Feature
        , std::bind(&Overlay::featureRemovedDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Pre | msg::Close | msg::Project
        , std::bind(&Overlay::closeProjectDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Add | msg::Overlay
        , std::bind(&Overlay::addOverlayGeometryDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Clear | msg::Overlay
        , std::bind(&Overlay::clearOverlayGeometryDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::View | msg::Show | msg::Overlay
        , std::bind(&Overlay::showOverlayDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::View | msg::Hide | msg::Overlay
        , std::bind(&Overlay::hideOverlayDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::View | msg::Toggle | msg::Overlay
        , std::bind(&Overlay::overlayToggleDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Post | msg::Open | msg::Project
        , std::bind(&Overlay::projectOpenedDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Post | msg::Project | msg::Update | msg::Model
        , std::bind(&Overlay::projectUpdatedDispatched, this, std::placeholders::_1)
      )
    }
  );
}

void Overlay::featureAddedDispatched(const msg::Message &messageIn)
{
  const prj::Message &message = messageIn.getPRJ();
  addChild(message.feature->getOverlaySwitch());
}

void Overlay::featureRemovedDispatched(const msg::Message &messageIn)
{
  const prj::Message &message = messageIn.getPRJ();
  removeChild(message.feature->getOverlaySwitch());
}

void Overlay::closeProjectDispatched(const msg::Message&)
{
    //this code assumes that the first child is the absolute csys switch.
    //also the fleeting switch.
    removeChildren(2, getNumChildren() - 2);
}

void Overlay::addOverlayGeometryDispatched(const msg::Message &message)
{
    const vwr::Message &vMessage = message.getVWR();
    fleetingGeometry->addChild(vMessage.node.get());
}

void Overlay::clearOverlayGeometryDispatched(const msg::Message&)
{
    fleetingGeometry->removeChildren(0, fleetingGeometry->getNumChildren());
}

void Overlay::showOverlayDispatched(const msg::Message &msgIn)
{
  const vwr::Message &vm = msgIn.getVWR();
  slc::MainSwitchVisitor v(vm.featureId);
  this->accept(v);
  assert(v.out);
  if (!v.out)
    return;
  
  if (v.out->getNewChildDefaultValue()) //already shown
    return;
  
  v.out->setAllChildrenOn();
  
  msg::Message mOut(msg::Response | msg::View | msg::Show | msg::Overlay, vm);
  node->sendBlocked(mOut);
}

void Overlay::hideOverlayDispatched(const msg::Message &msgIn)
{
  const vwr::Message &vm = msgIn.getVWR();
  slc::MainSwitchVisitor v(vm.featureId);
  this->accept(v);
  assert(v.out);
  if (!v.out)
    return;
  
  if (!v.out->getNewChildDefaultValue()) //already hidden
    return;
  
  v.out->setAllChildrenOff();
  
  msg::Message mOut(msg::Response | msg::View | msg::Hide | msg::Overlay, vm);
  node->sendBlocked(mOut);
}

void Overlay::overlayToggleDispatched(const msg::Message &msgIn)
{
  const vwr::Message &vm = msgIn.getVWR();
  slc::MainSwitchVisitor v(vm.featureId);
  this->accept(v);
  assert(v.out); //some features won't have overlay, but we want to filter those out before the message call.
  if (!v.out)
    return;
  
  msg::Mask maskOut; //notice we are not responding with Msg::Toggle that got us here.
  if (v.out->getNewChildDefaultValue())
  {
    v.out->setAllChildrenOff();
    maskOut = msg::Response | msg::View | msg::Hide | msg::Overlay;
  }
  else
  {
    v.out->setAllChildrenOn();
    maskOut = msg::Response | msg::View | msg::Show | msg::Overlay;
  }
  
  msg::Message mOut(maskOut, vm);
  node->sendBlocked(mOut);
}

void Overlay::projectOpenedDispatched(const msg::Message &)
{
  serialRead();
}

void Overlay::projectUpdatedDispatched(const msg::Message &)
{
  serialWrite();
}

//restore states from serialize
class SerialInOverlayVisitor : public osg::NodeVisitor
{
public:
  SerialInOverlayVisitor(const prj::srl::States &statesIn) :
  NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
  states(statesIn)
  {
  }
  virtual void apply(osg::Switch &switchIn) override
  {
    std::string userValue;
    if (switchIn.getUserValue<std::string>(gu::idAttributeTitle, userValue))
    {
      for (const auto &s : states.array())
      {
        if (userValue == s.id())
        {
          vwr::Message vm(gu::stringToId(userValue));
          if (s.visible())
          {
            switchIn.setAllChildrenOn();
            msg::hub().sendBlocked(msg::Message(msg::Mask(msg::Response | msg::View | msg::Show | msg::Overlay), vm));
          }
          else
          {
            switchIn.setAllChildrenOff();
            msg::hub().sendBlocked(msg::Message(msg::Mask(msg::Response | msg::View | msg::Hide | msg::Overlay), vm));
          }
          break;
        }
      }
    }
    
    //only interested in top level children, so don't need to call traverse here.
  }
protected:
  const prj::srl::States &states;
};

void Overlay::serialRead()
{
  boost::filesystem::path file = app::instance()->getProject()->getSaveDirectory();
  file /= "overlay.xml";
  if (!boost::filesystem::exists(file))
    return;
  
  auto sView = prj::srl::view(file.string(), ::xml_schema::Flags::dont_validate);
  SerialInOverlayVisitor v(sView->states());
  this->accept(v);
}

//get all states to serialize
class SerialOutVisitor : public osg::NodeVisitor
{
public:
  SerialOutVisitor(prj::srl::States &statesIn) : NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), states(statesIn){}
  virtual void apply(osg::Switch &switchIn) override
  {
    std::string userValue;
    if (switchIn.getUserValue<std::string>(gu::idAttributeTitle, userValue))
      states.array().push_back(prj::srl::State(userValue, switchIn.getNewChildDefaultValue()));
    
    //only interested in top level children, so don't need to call traverse here.
  }
protected:
  prj::srl::States &states;
};

void Overlay::serialWrite()
{
  prj::srl::States states;
  SerialOutVisitor v(states);
  this->accept(v);
  prj::srl::View svOut(states);
  
  boost::filesystem::path file = app::instance()->getProject()->getSaveDirectory();
  file /= "overlay.xml";
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(file.string());
  prj::srl::view(stream, svOut, infoMap);
}
