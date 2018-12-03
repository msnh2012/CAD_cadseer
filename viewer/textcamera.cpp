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

#include <boost/current_function.hpp>

#include <QApplication>

#include <osg/Switch>
#include <osg/PolygonMode>
#include <osgViewer/GraphicsWindow>
#include <osgText/Text>
#include <osgQt/QFontImplementation>
#include <osgAnimation/EaseMotion>

#include <tools/idtools.h>
#include <selection/definitions.h>
#include <selection/message.h>
#include <message/node.h>
#include <message/sift.h>
#include <viewer/message.h>
#include <viewer/textcamera.h>


namespace vwr
{
  class TextFadeCallback : public osg::Drawable::UpdateCallback
  {
  public:
    TextFadeCallback(const float &timeSpanIn) :
    timeSpan(timeSpanIn)
    {
      motion = new osgAnimation::InOutCubicMotion(0.0, timeSpanIn, 1.0, osgAnimation::Motion::CLAMP);
    }
    bool isFinished(){return finished;}
    virtual void update(osg::NodeVisitor *visitor, osg::Drawable *drawable)
    {
      if (!finished)
      {
        osgText::Text *text = dynamic_cast<osgText::Text*>(drawable);
        assert(text);
        
        double currentTime = visitor->getFrameStamp()->getSimulationTime();
        if(lastTime < 0.0f)
          lastTime = currentTime;
        motion->update(currentTime - lastTime);
        lastTime = currentTime;
        
        float motionValue = motion->getValue();
        if (std::fabs(1.0 - motionValue) < 0.001)
          finished = true;
        motionValue = 1.0 - motionValue; //fading out.
        osg::Vec4 color = text->getColor();
        color.w() = motionValue;
        text->setColor(color);
      }
      //no need to call traverse. 
    }
    
  protected:
    float timeSpan = 1.0;
    bool finished = false;
    float lastTime = -1.0;
    osg::ref_ptr<osgAnimation::InOutCubicMotion> motion;
  };
  
  class TextFinishedCallback : public osg::NodeCallback
  {
    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
      TextCamera *tc = dynamic_cast<TextCamera*>(node);
      assert(tc); //attached to wrong node.
      if (tc->cleanFade())
        tc->layoutStatus();
      
      traverse(node,nv);
    }
  };
}


using namespace vwr;

ResizeEventHandler::ResizeEventHandler(const osg::observer_ptr<TextCamera> slaveCameraIn) :
                                       osgGA::GUIEventHandler(), slaveCamera(slaveCameraIn)
{

}

bool ResizeEventHandler::handle(const osgGA::GUIEventAdapter& eventAdapter, osgGA::GUIActionAdapter&,
                                osg::Object*, osg::NodeVisitor*)
{
  if (eventAdapter.getEventType() != osgGA::GUIEventAdapter::RESIZE)
    return false;
  
  if (!slaveCamera.valid())
    return false;
  
  osg::Viewport *viewPort = slaveCamera->getViewport();
  slaveCamera->setProjectionMatrix(osg::Matrixd::ortho2D(viewPort->x(), viewPort->width(), viewPort->y(), viewPort->height()));
  
  slaveCamera->layoutStatus();
  slaveCamera->layoutSelection();
  slaveCamera->layoutCommand();
  
  //other cameras, views etc.. might want to respond to this event.
  return false;
}

TextCamera::TextCamera(osgViewer::GraphicsWindow *windowIn) : osg::Camera()
{
  node = std::make_unique<msg::Node>();
  node->connect(msg::hub());
  sift = std::make_unique<msg::Sift>();
  sift->name = "vwr::TextCamera";
  node->setHandler(std::bind(&msg::Sift::receive, sift.get(), std::placeholders::_1));
  setupDispatcher();
  
  setGraphicsContext(windowIn);
  setProjectionMatrix(osg::Matrix::ortho2D(0, windowIn->getTraits()->width, 0, windowIn->getTraits()->width));
  setReferenceFrame(osg::Transform::ABSOLUTE_RF);
  setRenderOrder(osg::Camera::NESTED_RENDER, 3);
  setAllowEventFocus(false);
  setViewMatrix(osg::Matrix::identity());
  setClearMask(0);
  setName("text");
  this->addUpdateCallback(new TextFinishedCallback());
  
  infoSwitch = new osg::Switch();
  osg::StateSet* stateset = infoSwitch->getOrCreateStateSet();
  stateset->setMode(GL_LIGHTING,osg::StateAttribute::OFF);
  stateset->setMode(GL_BLEND,osg::StateAttribute::ON);
  stateset->setMode(GL_DEPTH_TEST,osg::StateAttribute::OFF);
  stateset->setAttribute(new osg::PolygonMode(), osg::StateAttribute::PROTECTED);
  addChild(infoSwitch);
  
  //set up on screen text.
  osg::Vec3 pos(0.0, 0.0f, 0.0f); //position gets updated in resize handler.
  osg::Vec4 color(0.0f,0.0f,0.0f,1.0f);
  
  selectionLabel = new osgText::Text();
  selectionLabel->setName("selection");
  osg::ref_ptr<osgQt::QFontImplementation> fontImplement(new osgQt::QFontImplementation(qApp->font()));
  osg::ref_ptr<osgText::Font> textFont(new osgText::Font(fontImplement.get()));
  selectionLabel->setFont(textFont);
  selectionLabel->setFontResolution(qApp->font().pointSize(), qApp->font().pointSize());
  selectionLabel->setColor(color);
  selectionLabel->setBackdropType(osgText::Text::OUTLINE);
  selectionLabel->setBackdropColor(osg::Vec4(1.0, 1.0, 1.0, 1.0));
  selectionLabel->setCharacterSize(qApp->font().pointSizeF()); //this is 9.0 here.
  selectionLabel->setPosition(pos);
  selectionLabel->setAlignment(osgText::TextBase::RIGHT_TOP);
  selectionLabel->setText("Selection: ");
  infoSwitch->addChild(selectionLabel.get());
  
  statusGroup = new osg::Group();
  statusGroup->setName("statusGroup");
  infoSwitch->addChild(statusGroup.get());
  
  statusLabel = new osgText::Text();
  statusLabel->setName("status");
  statusLabel->setFont(textFont);
  statusLabel->setFontResolution(qApp->font().pointSize(), qApp->font().pointSize());
  statusLabel->setColor(color);
  statusLabel->setBackdropType(osgText::Text::OUTLINE);
  statusLabel->setBackdropColor(osg::Vec4(1.0, 1.0, 1.0, 1.0));
  statusLabel->setCharacterSize(qApp->font().pointSizeF()); //this is 9.0 here.
  statusLabel->setPosition(pos);
  statusLabel->setAlignment(osgText::TextBase::LEFT_TOP);
  statusLabel->setText("Status: ");
  statusGroup->addChild(statusLabel.get());
  
  commandLabel = new osgText::Text();
  commandLabel->setName("command");
  commandLabel->setFont(textFont);
  commandLabel->setFontResolution(qApp->font().pointSize(), qApp->font().pointSize());
  commandLabel->setColor(color);
  commandLabel->setBackdropType(osgText::Text::OUTLINE);
  commandLabel->setBackdropColor(osg::Vec4(1.0, 1.0, 1.0, 1.0));
  commandLabel->setCharacterSize(qApp->font().pointSizeF()); //this is 9.0 here.
  commandLabel->setPosition(pos);
  commandLabel->setAlignment(osgText::TextBase::LEFT_BOTTOM);
  commandLabel->setText("Active command count: 0");
  infoSwitch->addChild(commandLabel.get());
  
  infoSwitch->setAllChildrenOn();
  
  this->getOrCreateStateSet()->setMode(GL_MULTISAMPLE_ARB, osg::StateAttribute::ON);
}

TextCamera::~TextCamera() //for ref_ptr and forward declare.
{

}

bool TextCamera::cleanFade()
{
  bool isDirty = false;
  for (auto it = statusFades.begin(); it != statusFades.end();)
  {
    TextFadeCallback *cb = dynamic_cast<TextFadeCallback *>((*it)->getUpdateCallback());
    assert(cb);
    if (cb->isFinished())
    {
      statusGroup->removeChild(*it);
      it = statusFades.erase(it);
      isDirty = true;
    }
    else
      it++;
  }
  
  return isDirty;
}

//keep in mind the alignment of the text that was set in constructor.
void TextCamera::layoutStatus()
{
  osg::Vec3 corner; //top left.
  osg::BoundingBox::value_type padding = selectionLabel->getCharacterHeight() / 2.0;
  corner.x() = padding;
  corner.y() = getViewport()->height() - padding;
  statusLabel->setPosition(corner);
  
  auto offsetBelow = [&](const osgText::Text *t) -> osg::Vec3
  {
    osg::Vec3 out;
    out.x() = padding;
    out.y() = t->getBoundingBox().yMin() - padding;
    return out;
  };
  
  corner = offsetBelow(statusLabel.get());
  for (const auto &t : statusFades)
  {
    t->setPosition(corner);
    corner = offsetBelow(t.get());
  }
}

void TextCamera::layoutSelection()
{
  osg::Vec3 pos;
  osg::BoundingBox::value_type padding = selectionLabel->getCharacterHeight() / 2.0;
  pos.x() = getViewport()->width() - padding;
  pos.y() = getViewport()->height() - padding;
  selectionLabel->setPosition(pos);
}

void TextCamera::layoutCommand()
{
  osg::Vec3 pos;
  osg::BoundingBox::value_type padding = commandLabel->getCharacterHeight() / 2.0;
  padding = commandLabel->getCharacterHeight() / 2.0;
  pos.x() = padding;
  pos.y() = padding;
  commandLabel->setPosition(pos);
}

void TextCamera::setupDispatcher()
{
  sift->insert
  (
    {
      std::make_pair
      (
        msg::Response | msg::Post | msg::Preselection | msg::Add
        , std::bind(&TextCamera::preselectionAdditionDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Pre | msg::Preselection | msg::Remove
        , std::bind(&TextCamera::preselectionSubtractionDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Post | msg::Selection | msg::Add
        , std::bind(&TextCamera::selectionAdditionDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Pre | msg::Selection | msg::Remove
        , std::bind(&TextCamera::selectionSubtractionDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Status | msg::Text
        , std::bind(&TextCamera::statusTextDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Command | msg::Text
        , std::bind(&TextCamera::commandTextDispatched, this, std::placeholders::_1)
      )
    }
  );
}

void TextCamera::preselectionAdditionDispatched(const msg::Message &messageIn)
{
  slc::Message sMessage = messageIn.getSLC();
  
  preselectionText.clear();
  std::ostringstream preselectStream;
  preselectStream << "Pre-Selection:" << std::endl;
  preselectStream << "object type: " << slc::getNameOfType(sMessage.type) << std::endl <<
    "Feature Type: " << ftr::toString(sMessage.featureType) << std::endl <<
    "Feature id: " << gu::idToShortString(sMessage.featureId) << std::endl <<
    "Shape id: " << gu::idToShortString(sMessage.shapeId) << std::endl;
  preselectionText = preselectStream.str();
  updateSelectionLabel();
}

void TextCamera::preselectionSubtractionDispatched(const msg::Message &)
{
  preselectionText.clear();
  updateSelectionLabel();
}

void TextCamera::selectionAdditionDispatched(const msg::Message &messageIn)
{
  slc::Message sMessage = messageIn.getSLC();
  assert(std::find(selections.begin(), selections.end(), sMessage) == selections.end());
  selections.push_back(sMessage);
  updateSelectionLabel();
}

void TextCamera::selectionSubtractionDispatched(const msg::Message &messageIn)
{
  slc::Message sMessage = messageIn.getSLC();
  auto it = std::find(selections.begin(), selections.end(), sMessage); 
  assert(it != selections.end());
  selections.erase(it);
  updateSelectionLabel();
}

void TextCamera::statusTextDispatched(const msg::Message &messageIn)
{
  vwr::Message sMessage = messageIn.getVWR();
  if (sMessage.time == 0.0)
  {
    std::string display = "What is thy bidding, my master?";
    if (!sMessage.text.empty())
      display = sMessage.text;
    statusLabel->setText(display);
  }
  else if (!sMessage.text.empty())
  {
    //these will be fading messages.
    osg::ref_ptr<osgText::Text> t = dynamic_cast<osgText::Text*>(statusLabel->clone(osg::CopyOp::SHALLOW_COPY));
    t->setText(sMessage.text);
    t->setColor(osg::Vec4(1.0f,0.0f,0.0f,1.0f));
    t->addUpdateCallback(new TextFadeCallback(sMessage.time));
    t->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    statusFades.push_back(t);
    statusGroup->addChild(t.get());
  }
  
  layoutStatus();
}

void TextCamera::updateSelectionLabel()
{
  std::ostringstream labelStream;
  labelStream << preselectionText << std::endl <<
    "Selection:";
  for (auto it = selections.begin(); it != selections.end(); ++it)
  {
    labelStream << std::endl <<
      "object type: " << slc::getNameOfType(it->type) << std::endl <<
      "Feature Type: " << ftr::toString(it->featureType) << std::endl <<
      "Feature id: " << gu::idToShortString(it->featureId) << std::endl <<
      "Shape id: " << gu::idToShortString(it->shapeId) << std::endl;
  }
  
  selectionLabel->setText(labelStream.str());
}

void TextCamera::commandTextDispatched(const msg::Message &messageIn)
{
  vwr::Message vMessage = messageIn.getVWR();
  commandLabel->setText(vMessage.text);
}
