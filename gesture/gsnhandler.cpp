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
#include <assert.h>

#include <QImage>
#include <QGLWidget>

#include <osgViewer/View>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgUtil/LineSegmentIntersector>
#include <osg/ValueObject>
#include <osg/Depth>
// #include <osgDB/WriteFile>

#include "application/appapplication.h"
#include "viewer/vwrmessage.h"
#include "gesture/gsnnode.h"
#include "modelviz/mdvnodemaskdefs.h"
#include "message/msgnode.h"
#include "message/msgsift.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "gesture/gsnanimations.h"
#include "viewer/vwrspaceballosgevent.h"
#include "gesture/gsnhandler.h"

using namespace gsn;

static const std::string attributeMask = "CommandAttributeTitle";
static const std::string attributeStatus = "CommandAttributeStatus";

// static void dumpMenuGraphViz(const osg::Node &root, const std::string &fileName)
// {
//   osg::ref_ptr<osgDB::Options> options(new osgDB::Options());
//   options->setOptionString("rankdir = TB;");
//   
//   osgDB::writeNodeFile(root, fileName, options);
// }

class GestureAllSwitchesOffVisitor : public osg::NodeVisitor
{
public:
  GestureAllSwitchesOffVisitor() : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {}
  virtual void apply(osg::Switch &aSwitch)
  {
    traverse(aSwitch);
    aSwitch.setAllChildrenOff();
  }
};

Handler::Handler(osg::Camera *cameraIn) : osgGA::GUIEventHandler(), rightButtonDown(false),
    dragStarted(false), currentNodeLeft(false), iconRadius(32.0), includedAngle(90.0)
{
  node = std::make_unique<msg::Node>();
  node->connect(msg::hub());
  sift = std::make_unique<msg::Sift>();
  sift->name = "gsn::Handler";
  node->setHandler(std::bind(&msg::Sift::receive, sift.get(), std::placeholders::_1));
  setupDispatcher();
  
    gestureCamera = cameraIn;
    if (!gestureCamera.valid())
        return;
    gestureSwitch = dynamic_cast<osg::Switch *>(gestureCamera->getChild(0));
    if (!gestureSwitch.valid())
        return;
    
    updateVariables();
    constructMenu();
//     dumpMenuGraphViz(*startNode, "SOMEPATH");
}

bool Handler::handle(const osgGA::GUIEventAdapter& eventAdapter,
                            osgGA::GUIActionAdapter&, osg::Object *,
                            osg::NodeVisitor *)
{
  if (eventAdapter.getEventType() == osgGA::GUIEventAdapter::USER)
  {
    const vwr::SpaceballOSGEvent *event = 
      dynamic_cast<const vwr::SpaceballOSGEvent *>(eventAdapter.getUserData());
    assert (event);
    
    if (event->theType == vwr::SpaceballOSGEvent::Button)
    {
      int currentButton = event->buttonNumber;
      vwr::SpaceballOSGEvent::ButtonState currentState = event->theButtonState;
      if (currentState == vwr::SpaceballOSGEvent::Pressed)
      {
        spaceballButton = currentButton;
        if (!rightButtonDown)
        {
          std::string maskString = prf::manager().getSpaceballButton(spaceballButton);
          if (!maskString.empty())
          {
            msg::Mask msgMask(maskString);
            msg::Message messageOut;
            messageOut.mask = msgMask;
            app::instance()->queuedMessage(messageOut);
          }
        }
        else
        {
          std::ostringstream stream;
          stream << QObject::tr("Link to spaceball button ").toStdString() << spaceballButton;
          node->send(msg::buildStatusMessage(stream.str()));
        }
      }
      else
      {
        assert(currentState == vwr::SpaceballOSGEvent::Released);
        spaceballButton = -1;
      }
      return true;
    }
  }
  
  /* hot keys are a little different than the spaceball buttons. The keyboard focus can be taken
   * away from the gesture handler by a command launching a dialog. This results in never receiving
   * the KEYUP message and the hot key value not reaching -1 as it should. This results in the hotkey
   * being assigned to the command launched by the menu. To avoid this we set the hotkey to -1 right after
   * it is used.
   */
  if (eventAdapter.getEventType() == osgGA::GUIEventAdapter::KEYDOWN)
  {
    if (!rightButtonDown)
      return false;
    hotKey = eventAdapter.getKey();
    if (dragStarted)
    {
      std::ostringstream stream;
      stream << QObject::tr("Link to key ").toStdString() << hotKey;
      node->send(msg::buildStatusMessage(stream.str()));
    }
    else
    {
      std::string maskString = prf::manager().getHotKey(hotKey);
      if (!maskString.empty())
      {
        msg::Mask msgMask(maskString);
        msg::Message messageOut;
        messageOut.mask = msgMask;
        app::instance()->queuedMessage(messageOut);
        rightButtonDown = false; //don't do menu if launching a command.
      }
      hotKey = -1;
    }
    return true;
  }
  if (eventAdapter.getEventType() == osgGA::GUIEventAdapter::KEYUP)
  {
    hotKey = -1;
    if (!rightButtonDown)
      return false;
    node->send(msg::buildStatusMessage(""));
    return true;
  }
  
    //lambda to clear status.
    auto clearStatus = [&]()
    {
      //clear any status message
      node->send(msg::buildStatusMessage(""));
    };
    
    if (eventAdapter.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_LEFT_CTRL)
    {
        if (dragStarted)
        {
          clearStatus();
          GestureAllSwitchesOffVisitor visitor;
          gestureCamera->accept(visitor);
          dragStarted = false;
          gestureSwitch->setAllChildrenOff();
        }
        return false;
    }
  
    if (!gestureSwitch.valid())
    {
      std::cout << "gestureSwitch is invalid in Handler::handle" << std::endl;
      return false;
    }

    if (eventAdapter.getButton() == osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON)
    {
        if (eventAdapter.getEventType() == osgGA::GUIEventAdapter::PUSH)
        {
            GestureAllSwitchesOffVisitor visitor;
            gestureCamera->accept(visitor);

            rightButtonDown = true;
        }

        if (eventAdapter.getEventType() == osgGA::GUIEventAdapter::RELEASE)
        {
            clearStatus();
            rightButtonDown = false;
            if (!dragStarted)
                return false;
            dragStarted = false;
            gestureSwitch->setAllChildrenOff();
            if (currentNode && (currentNode->getNodeMask() & mdv::gestureCommand))
            {
                std::string msgMaskString;
                if (currentNode->getUserValue<std::string>(attributeMask, msgMaskString))
                {
                    if (spaceballButton != -1)
                    {
                      prf::manager().setSpaceballButton(spaceballButton, msgMaskString);
                      prf::manager().saveConfig();
                    }
                    if (hotKey != -1)
                    {
                      prf::manager().setHotKey(hotKey, msgMaskString);
                      prf::manager().saveConfig();
                      hotKey = -1;
                    }
                    
                    msg::Mask msgMask(msgMaskString);
                    msg::Message messageOut;
                    messageOut.mask = msgMask;
                    app::instance()->queuedMessage(messageOut);
                }
                else
                    assert(0); //gesture node doesn't have msgMask attribute;
            }
        }
    }

    if (eventAdapter.getEventType() == osgGA::GUIEventAdapter::DRAG)
    {
      if (!rightButtonDown) //only right button drag
          return false;
      if (!dragStarted)
      {
          dragStarted = true;
          startDrag(eventAdapter);
      }

      osg::Matrixd transformation = osg::Matrixd::inverse
        (gestureCamera->getProjectionMatrix() * gestureCamera->getViewport()->computeWindowMatrix());
      osg::Vec3d temp(eventAdapter.getX(), eventAdapter.getY(), 0.0);
      temp = transformation * temp;
      temp.z() = 0.0;

      osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector = new osgUtil::LineSegmentIntersector
              (osgUtil::Intersector::WINDOW, temp.x(), temp.y());
      osgUtil::IntersectionVisitor iv(intersector);
      iv.setTraversalMask(~mdv::noIntersect);
      gestureSwitch->accept(iv);
      
      //look for icon intersection, but send message when intersecting lines for user feedback.
      osg::ref_ptr<osg::Drawable> drawable;
      osg::ref_ptr<osg::MatrixTransform> tnode;
      osg::Vec3 hPoint;
      for (auto &intersection : intersector->getIntersections())
      {
        osg::ref_ptr<osg::Drawable> tempDrawable = intersection.drawable;
        assert(tempDrawable.valid());
        osg::ref_ptr<osg::MatrixTransform> tempNode = dynamic_cast<osg::MatrixTransform*>
          (tempDrawable->getParent(0)->getParent(0));
        assert(temp.valid());
        
        std::string statusString;
        if (tempNode->getUserValue<std::string>(attributeStatus, statusString))
          node->send(msg::buildStatusMessage(statusString));
        
        if (tempDrawable->getName() != "Line")
        {
          drawable = tempDrawable;
          tnode = tempNode;
          hPoint = intersection.getLocalIntersectPoint();
          break;
        }
      }
      
      if (!drawable.valid()) //no icon intersection found.
      {
        if (currentNodeLeft == false)
        {
          currentNodeLeft = true;
          if (currentNode && (currentNode->getNodeMask() & mdv::gestureMenu))
            spraySubNodes(temp);
        }
        
        return false;
      }
      
      if (tnode == currentNode)
      {
        if (currentNodeLeft == true)
        {
          currentNodeLeft = false;
          if (currentNode->getNodeMask() & mdv::gestureMenu)
              contractSubNodes();
        }
      }
      else
      {
        //here we are entering an already sprayed node.
        osg::MatrixTransform *parentNode = currentNode;
        currentNode = tnode;

        osg::Switch *geometrySwitch = dynamic_cast<osg::Switch*>(parentNode->getChild(parentNode->getNumChildren() - 1));
        assert(geometrySwitch);
        geometrySwitch->setAllChildrenOff();

        unsigned int childIndex = parentNode->getChildIndex(currentNode);
        for (unsigned int index = 0; index < parentNode->getNumChildren() - 1; ++index)
        {
          osg::MatrixTransform *childNode = dynamic_cast<osg::MatrixTransform*>(parentNode->getChild(index));
          assert(childNode);
          
          osg::Switch *lineNodeSwitch = childNode->getChild(childNode->getNumChildren() - 1)->asSwitch();
          assert(lineNodeSwitch);
          lineNodeSwitch->setAllChildrenOff();

          if (index == childIndex)
            lineNodeSwitch->setValue(1, true);
        }

        currentNodeLeft = false;
        aggregateMatrix = currentNode->getMatrix() * aggregateMatrix;
      }
    }
    return false;
}

void Handler::spraySubNodes(osg::Vec3 cursorLocation)
{
    cursorLocation = cursorLocation * osg::Matrixd::inverse(aggregateMatrix);
    osg::Vec3 direction = cursorLocation;

    int childCount = currentNode->getNumChildren();
    assert(childCount > 1);//line, icon and sub items.
    if (childCount < 2)
      return;
    std::vector<osg::Vec3> locations = buildNodeLocations(direction, childCount - 1);
    for (unsigned int index = 0; index < locations.size(); ++index)
    {
        osg::MatrixTransform *tempLocation = dynamic_cast<osg::MatrixTransform *>
                (currentNode->getChild(index));
        assert(tempLocation);
        
        osg::Vec3d startLocation = osg::Vec3d(0.0, 0.0, -0.001); //cheat in z to be under parent.
        tempLocation->setMatrix(osg::Matrixd::translate(startLocation));
        
        gsn::NodeExpand *childAnimation = new gsn::NodeExpand(startLocation, locations.at(index), time());
        tempLocation->setUpdateCallback(childAnimation);

        osg::Switch *tempSwitch = dynamic_cast<osg::Switch *>
                (tempLocation->getChild(tempLocation->getNumChildren() - 1));
        assert(tempSwitch);
        tempSwitch->setAllChildrenOn();
        
        osg::Geometry *geometry = tempSwitch->getChild(0)->asGeometry();
        assert(geometry);
        osg::Vec3Array *pointArray = dynamic_cast<osg::Vec3Array *>(geometry->getVertexArray());
        assert(pointArray);
        (*pointArray)[1] = locations.at(index) * -1.0;
        geometry->dirtyDisplayList();
        geometry->dirtyBound();
        
        gsn::GeometryExpand *lineAnimate = new gsn::GeometryExpand
          (osg::Vec3d(0.0, 0.0, 0.0), -locations.at(index), time());
        geometry->setUpdateCallback(lineAnimate);
    }
}

void Handler::contractSubNodes()
{
    int childCount = currentNode->getNumChildren();
    assert(childCount > 1);//line, icon and sub items.
    for (int index = 0; index < childCount - 1; ++index)
    {
        osg::MatrixTransform *tempLocation = dynamic_cast<osg::MatrixTransform *>
                (currentNode->getChild(index));
        assert(tempLocation);
        
        gsn::NodeCollapse *childAnimation = new gsn::NodeCollapse
          (tempLocation->getMatrix().getTrans(), osg::Vec3d(0.0, 0.0, -0.001), time());
        tempLocation->setUpdateCallback(childAnimation);
        
        osg::Geometry *geometry = tempLocation->getChild(tempLocation->getNumChildren() - 1)->
          asSwitch()->getChild(0)->asGeometry();
        assert(geometry);
        
        gsn::GeometryCollapse *lineAnimate = new gsn::GeometryCollapse
          (osg::Vec3d(0.0, 0.0, 0.0), -(tempLocation->getMatrix().getTrans()), time());
          
        geometry->setUpdateCallback(lineAnimate);
    }
}

float Handler::time()
{
  return static_cast<float>(prf::manager().rootPtr->gesture().animationSeconds());
}

void Handler::constructMenu()
{
  //should only be 2. the start object and the transparent quad
  gestureSwitch->removeChildren(0, gestureSwitch->getNumChildren() - 1);
  
  gsn::NodeBuilder builder(iconRadius);
  startNode = builder.buildNode(":/resources/images/start.svg", mdv::gestureMenu);
  startNode->setUserValue<std::string>(attributeStatus, QObject::tr("Start Menu").toStdString());
  gestureSwitch->insertChild(0, startNode);

  auto constructNode = [&]
  (
    unsigned int nodeMask,
    const char *resource,
    const std::string &statusText,
    const std::string &mask,
    osg::MatrixTransform *parent
  ) -> osg::MatrixTransform*
  {
    assert(nodeMask == mdv::gestureMenu || nodeMask == mdv::gestureCommand);
    assert(!statusText.empty());
    assert(parent);
    if (nodeMask == mdv::gestureCommand)
      assert(!mask.empty());
    
    osg::MatrixTransform *out = builder.buildNode(resource, nodeMask);
    out->setUserValue<std::string>(attributeStatus, statusText);
    if (nodeMask == mdv::gestureCommand)
      out->setUserValue<std::string>(attributeMask, mask);
    parent->insertChild(parent->getNumChildren() - 1, out);
    
    return out;
  };
  
  //using braces and indents to show hierarchy of menu

  osg::MatrixTransform *viewBase = constructNode
  (
    mdv::gestureMenu,
    ":/resources/images/viewBase.svg",
    QObject::tr("View Menu").toStdString(),
    std::string(),
    startNode.get()
  );
  {
    osg::MatrixTransform *viewStandard = constructNode
    (
      mdv::gestureMenu,
      ":/resources/images/viewStandard.svg",
      QObject::tr("Standard Views Menu").toStdString(),
      std::string(),
      viewBase
    );
    {
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/viewTop.svg",
        QObject::tr("Top View Command").toStdString(),
        (msg::Request | msg::View | msg::Top).to_string(),
        viewStandard
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/viewFront.svg",
        QObject::tr("View Front Command").toStdString(),
        (msg::Request | msg::View | msg::Front).to_string(),
        viewStandard
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/viewRight.svg",
        QObject::tr("View Right Command").toStdString(),
        (msg::Request | msg::View | msg::Right).to_string(),
        viewStandard
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/viewIso.svg",
        QObject::tr("View Iso Command").toStdString(),
        (msg::Request | msg::View | msg::Iso).to_string(),
        viewStandard
      );
    }
    osg::MatrixTransform *viewStandardCurrent = constructNode
    (
      mdv::gestureMenu,
      ":/resources/images/viewStandardCurrent.svg",
      QObject::tr("Standard Views Menu Relative To Current System").toStdString(),
      std::string(),
      viewBase
    );
    {
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/viewTopCurrent.svg",
        QObject::tr("Top View Current Command").toStdString(),
        (msg::Request | msg::View | msg::Top | msg::Current).to_string(),
        viewStandardCurrent
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/viewFrontCurrent.svg",
        QObject::tr("View Front Current Command").toStdString(),
        (msg::Request | msg::View | msg::Front | msg::Current).to_string(),
        viewStandardCurrent
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/viewRightCurrent.svg",
        QObject::tr("View Right Current Command").toStdString(),
        (msg::Request | msg::View | msg::Right | msg::Current).to_string(),
        viewStandardCurrent
      );
    }
    //back to viewbase menu
    constructNode
    (
      mdv::gestureCommand,
      ":/resources/images/viewFit.svg",
      QObject::tr("View Fit Command").toStdString(),
      (msg::Request | msg::View | msg::Fit).to_string(),
      viewBase
    );
    constructNode
    (
      mdv::gestureCommand,
      ":/resources/images/viewFill.svg",
      QObject::tr("View Fill Command").toStdString(),
      (msg::Request | msg::View | msg::Fill).to_string(),
      viewBase
    );
    constructNode
    (
      mdv::gestureCommand,
      ":/resources/images/viewTriangulation.svg",
      QObject::tr("View Triangulation Command").toStdString(),
      (msg::Request | msg::View | msg::Triangulation).to_string(),
      viewBase
    );
    constructNode
    (
      mdv::gestureCommand,
      ":/resources/images/renderStyleToggle.svg",
      QObject::tr("Toggle Render Style Command").toStdString(),
      (msg::Request | msg::View | msg::RenderStyle | msg::Toggle).to_string(),
      viewBase
    );
    constructNode
    (
      mdv::gestureCommand,
      ":/resources/images/viewHiddenLines.svg",
      QObject::tr("Toggle hidden lines Command").toStdString(),
      (msg::Request | msg::View | msg::Toggle | msg::HiddenLine).to_string(),
      viewBase
    );
    constructNode
    (
      mdv::gestureCommand,
      ":/resources/images/viewIsolate.svg",
      QObject::tr("View Only Selected").toStdString(),
      (msg::Request | msg::View | msg::ThreeD | msg::Overlay | msg::Isolate).to_string(),
      viewBase
    );
  }
  osg::MatrixTransform *constructionBase = constructNode
  (
    mdv::gestureMenu,
    ":/resources/images/constructionBase.svg",
    QObject::tr("Construction Menu").toStdString(),
    std::string(),
    startNode.get()
  );
  {
    osg::MatrixTransform *constructionPrimitives = constructNode
    (
      mdv::gestureMenu,
      ":/resources/images/constructionPrimitives.svg",
      QObject::tr("Primitives Menu").toStdString(),
      std::string(),
      constructionBase
    );
    {
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/constructionBox.svg",
        QObject::tr("Box Command").toStdString(),
        (msg::Request | msg::Construct | msg::Box).to_string(),
        constructionPrimitives
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/constructionSphere.svg",
        QObject::tr("Sphere Command").toStdString(),
        (msg::Request | msg::Construct | msg::Sphere).to_string(),
        constructionPrimitives
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/constructionCone.svg",
        QObject::tr("Cone Command").toStdString(),
        (msg::Request | msg::Construct | msg::Cone).to_string(),
        constructionPrimitives
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/constructionCylinder.svg",
        QObject::tr("Cylinder Command").toStdString(),
        (msg::Request | msg::Construct | msg::Cylinder).to_string(),
        constructionPrimitives
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/constructionOblong.svg",
        QObject::tr("Oblong Command").toStdString(),
        (msg::Request | msg::Construct | msg::Oblong).to_string(),
        constructionPrimitives
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/constructionTorus.svg",
        QObject::tr("Torus Command").toStdString(),
        (msg::Request | msg::Construct | msg::Torus).to_string(),
        constructionPrimitives
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/constructionThread.svg",
        QObject::tr("Thread Command").toStdString(),
        (msg::Request | msg::Construct | msg::Thread).to_string(),
        constructionPrimitives
      );
    }
    osg::MatrixTransform *constructionFinishing = constructNode
    (
      mdv::gestureMenu,
      ":/resources/images/constructionFinishing.svg",
      QObject::tr("Finishing Menu").toStdString(),
      std::string(),
      constructionBase
    );
    {
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/constructionBlend.svg",
        QObject::tr("Blend Command").toStdString(),
        (msg::Request | msg::Construct | msg::Blend).to_string(),
        constructionFinishing
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/constructionChamfer.svg",
        QObject::tr("Chamfer Command").toStdString(),
        (msg::Request | msg::Construct | msg::Chamfer).to_string(),
        constructionFinishing
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/constructionDraft.svg",
        QObject::tr("Draft Command").toStdString(),
        (msg::Request | msg::Construct | msg::Draft).to_string(),
        constructionFinishing
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/constructionHollow.svg",
        QObject::tr("Hollow Command").toStdString(),
        (msg::Request | msg::Construct | msg::Hollow).to_string(),
        constructionFinishing
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/constructionExtract.svg",
        QObject::tr("Extract Command").toStdString(),
        (msg::Request | msg::Construct | msg::Extract).to_string(),
        constructionFinishing
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/constructionRefine.svg",
        QObject::tr("Refine Command").toStdString(),
        (msg::Request | msg::Construct | msg::Refine).to_string(),
        constructionFinishing
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/constructionOffset.svg",
        QObject::tr("Offset Command").toStdString(),
        (msg::Request | msg::Construct | msg::Offset).to_string(),
        constructionFinishing
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/constructionThicken.svg",
        QObject::tr("Thicken Command").toStdString(),
        (msg::Request | msg::Construct | msg::Thicken).to_string(),
        constructionFinishing
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/constructionSew.svg",
        QObject::tr("Sew Command").toStdString(),
        (msg::Request | msg::Construct | msg::Sew).to_string(),
        constructionFinishing
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/constructionTrim.svg",
        QObject::tr("Trim Command").toStdString(),
        (msg::Request | msg::Construct | msg::Trim).to_string(),
        constructionFinishing
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/constructionRemoveFaces.svg",
        QObject::tr("Remove faces from body").toStdString(),
        (msg::Request | msg::Construct | msg::RemoveFaces).to_string(),
        constructionFinishing
      );
    }
    osg::MatrixTransform *constructionBoolean = constructNode
    (
      mdv::gestureMenu,
      ":/resources/images/constructionBoolean.svg",
      QObject::tr("Boolean Menu").toStdString(),
      std::string(),
      constructionBase
    );
    {
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/constructionUnion.svg",
        QObject::tr("Union Command").toStdString(),
        (msg::Request | msg::Construct | msg::Union).to_string(),
        constructionBoolean
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/constructionSubtract.svg",
        QObject::tr("Subtract Command").toStdString(),
        (msg::Request | msg::Construct | msg::Subtract).to_string(),
        constructionBoolean
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/constructionIntersect.svg",
        QObject::tr("Intersection Command").toStdString(),
        (msg::Request | msg::Construct | msg::Intersect).to_string(),
        constructionBoolean
      );
    }
    osg::MatrixTransform *constructionDie = constructNode
    (
      mdv::gestureMenu,
      ":/resources/images/constructionDie.svg",
      QObject::tr("Die Menu").toStdString(),
      std::string(),
      constructionBase
    );
    {
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/constructionSquash.svg",
        QObject::tr("Squash Command").toStdString(),
        (msg::Request | msg::Construct | msg::Squash).to_string(),
        constructionDie
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/constructionStrip.svg",
        QObject::tr("Strip Command").toStdString(),
        (msg::Request | msg::Construct | msg::Strip).to_string(),
        constructionDie
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/constructionNest.svg",
        QObject::tr("Nest Command").toStdString(),
        (msg::Request | msg::Construct | msg::Nest).to_string(),
        constructionDie
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/constructionDieSet.svg",
        QObject::tr("DieSet Command").toStdString(),
        (msg::Request | msg::Construct | msg::DieSet).to_string(),
        constructionDie
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/constructionQuote.svg",
        QObject::tr("Quote Command").toStdString(),
        (msg::Request | msg::Construct | msg::Quote).to_string(),
        constructionDie
      );
    }
    constructNode //TODO build datum menu node and move datum plane to it.
    (
      mdv::gestureCommand,
      ":/resources/images/constructionDatumPlane.svg",
      QObject::tr("Datum Plane Command").toStdString(),
      (msg::Request | msg::Construct | msg::DatumPlane).to_string(),
      constructionBase
    );
    constructNode //TODO build datum menu node and move datum axis to it.
    (
      mdv::gestureCommand,
      ":/resources/images/constructionDatumAxis.svg",
      QObject::tr("Datum Axis Command").toStdString(),
      (msg::Request | msg::Construct | msg::DatumAxis).to_string(),
      constructionBase
    );
    constructNode
    (
      mdv::gestureCommand,
      ":/resources/images/constructionSketch.svg",
      QObject::tr("Sketch Command").toStdString(),
      (msg::Request | msg::Construct | msg::Sketch).to_string(),
      constructionBase
    );
    constructNode
    (
      mdv::gestureCommand,
      ":/resources/images/constructionExtrude.svg",
      QObject::tr("Extrude Command").toStdString(),
      (msg::Request | msg::Construct | msg::Extrude).to_string(),
      constructionBase
    );
    constructNode
    (
      mdv::gestureCommand,
      ":/resources/images/constructionRevolve.svg",
      QObject::tr("Revolve Command").toStdString(),
      (msg::Request | msg::Construct | msg::Revolve).to_string(),
      constructionBase
    );
    osg::MatrixTransform *constructionInstance = constructNode
    (
      mdv::gestureMenu,
      ":/resources/images/constructionInstance.svg",
      QObject::tr("Instance Menu").toStdString(),
      std::string(),
      constructionBase
    );
    {
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/constructionInstanceLinear.svg",
        QObject::tr("Instance Linear Command").toStdString(),
        (msg::Request | msg::Construct | msg::InstanceLinear).to_string(),
        constructionInstance
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/constructionInstancePolar.svg",
        QObject::tr("Instance Polar Command").toStdString(),
        (msg::Request | msg::Construct | msg::InstancePolar).to_string(),
        constructionInstance
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/constructionInstanceMirror.svg",
        QObject::tr("Instance Mirror Command").toStdString(),
        (msg::Request | msg::Construct | msg::InstanceMirror).to_string(),
        constructionInstance
      );
    }
    osg::MatrixTransform *constructionMesh = constructNode
    (
      mdv::gestureMenu,
      ":/resources/images/constructionMesh.svg",
      QObject::tr("Mesh Menu").toStdString(),
      std::string(),
      constructionBase
    );
    {
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/constructionSurfaceMesh.svg",
        QObject::tr("Surface Mesh Command").toStdString(),
        (msg::Request | msg::Construct | msg::SurfaceMesh).to_string(),
        constructionMesh
      );
    }
    osg::MatrixTransform *constructionCurves = constructNode
    (
      mdv::gestureMenu,
      ":/resources/images/constructionCurves.svg",
      QObject::tr("Curves Menu").toStdString(),
      std::string(),
      constructionBase
    );
    {
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/sketchLine.svg",
        QObject::tr("Line Command").toStdString(),
        (msg::Request | msg::Construct | msg::Line).to_string(),
        constructionCurves
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/sketchBezeir.svg",
        QObject::tr("Transition Curve Command").toStdString(),
        (msg::Request | msg::Construct | msg::TransitionCurve).to_string(),
        constructionCurves
      );
    }
    osg::MatrixTransform *constructionSurface = constructNode
    (
      mdv::gestureMenu,
      ":/resources/images/constructionSurface.svg",
      QObject::tr("Surface Menu").toStdString(),
      std::string(),
      constructionBase
    );
    {
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/constructionRuled.svg",
        QObject::tr("Ruled Surface Command").toStdString(),
        (msg::Request | msg::Construct | msg::Ruled).to_string(),
        constructionSurface
      );
    }
    constructNode
    (
      mdv::gestureCommand,
      ":/resources/images/constructionImagePlane.svg",
      QObject::tr("Image Plane Command").toStdString(),
      (msg::Request | msg::Construct | msg::ImagePlane).to_string(),
      constructionBase
    );
  }
  osg::MatrixTransform *editBase = constructNode
  (
    mdv::gestureMenu,
    ":/resources/images/editBase.svg",
    QObject::tr("Edit Menu").toStdString(),
    std::string(),
    startNode.get()
  );
  {
    constructNode
    (
      mdv::gestureCommand,
      ":/resources/images/editCommandCancel.svg",
      QObject::tr("Cancel Command").toStdString(),
      (msg::Request | msg::Command | msg::Cancel).to_string(),
      editBase
    );
    constructNode
    (
      mdv::gestureCommand,
      ":/resources/images/editFeature.svg",
      QObject::tr("Edit Feature Command").toStdString(),
      (msg::Request | msg::Edit | msg::Feature).to_string(),
      editBase
    );
    constructNode
    (
      mdv::gestureCommand,
      ":/resources/images/editRemove.svg",
      QObject::tr("Remove Command").toStdString(),
      (msg::Request | msg::Remove).to_string(),
      editBase
    );
    constructNode
    (
      mdv::gestureCommand,
      ":/resources/images/editColor.svg",
      QObject::tr("Edit Color Command").toStdString(),
      (msg::Request | msg::Edit | msg::Feature | msg::Color).to_string(),
      editBase
    );
    constructNode
    (
      mdv::gestureCommand,
      ":/resources/images/editRename.svg",
      QObject::tr("Edit Name Command").toStdString(),
      (msg::Request | msg::Edit | msg::Feature | msg::Name).to_string(),
      editBase
    );
    constructNode
    (
      mdv::gestureCommand,
      ":/resources/images/editUpdate.svg",
      QObject::tr("Update Command").toStdString(),
      (msg::Request | msg::Project | msg::Update).to_string(),
      editBase
    );
    constructNode
    (
      mdv::gestureCommand,
      ":/resources/images/editForceUpdate.svg",
      QObject::tr("Force Update Command").toStdString(),
      (msg::Request | msg::Force | msg::Update).to_string(),
      editBase
    );
    constructNode
    (
      mdv::gestureCommand,
      ":/resources/images/editFeatureDissolve.svg",
      QObject::tr("Dissolve Feature Command").toStdString(),
      (msg::Request | msg::Feature | msg::Dissolve).to_string(),
      editBase
    );
    osg::MatrixTransform *editSystemBase = constructNode
    (
      mdv::gestureMenu,
      ":/resources/images/systemBase.svg",
      QObject::tr("Coordinate System Menu").toStdString(),
      std::string(),
      editBase
    );
    {
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/editDraggerToFeature.svg",
        QObject::tr("Dragger To Feature Command").toStdString(),
        (msg::Request | msg::DraggerToFeature).to_string(),
        editSystemBase
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/editFeatureToDragger.svg",
        QObject::tr("Feature To Dragger Command").toStdString(),
        (msg::Request | msg::FeatureToDragger).to_string(),
        editSystemBase
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/featureToSystem.svg",
        QObject::tr("Feature To Current System Command").toStdString(),
        (msg::Request | msg::FeatureToSystem).to_string(),
        editSystemBase
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/featureReposition.svg",
        QObject::tr("Dragger To Current System Command").toStdString(),
        (msg::Request | msg::FeatureReposition).to_string(),
        editSystemBase
      );
    }
    constructNode
    (
      mdv::gestureCommand,
      ":/resources/images/preferences.svg",
      QObject::tr("Preferences Command").toStdString(),
      (msg::Request | msg::Preferences).to_string(),
      editBase
    );
  }
  osg::MatrixTransform *fileBase = constructNode
  (
    mdv::gestureMenu,
    ":/resources/images/fileBase.svg",
    QObject::tr("File Menu").toStdString(),
    std::string(),
    startNode.get()
  );
  {
    osg::MatrixTransform *fileImport = constructNode
    (
      mdv::gestureMenu,
      ":/resources/images/fileImport.svg",
      QObject::tr("Import Menu").toStdString(),
      std::string(),
      fileBase
    );
    {
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/fileOCC.svg",
        QObject::tr("Import OCC Brep Command").toStdString(),
        (msg::Request | msg::Import | msg::OCC).to_string(),
        fileImport
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/fileStep.svg",
        QObject::tr("Import Step Command").toStdString(),
        (msg::Request | msg::Import | msg::Step).to_string(),
        fileImport
      );
    }
    osg::MatrixTransform *fileExport = constructNode
    (
      mdv::gestureMenu,
      ":/resources/images/fileExport.svg",
      QObject::tr("Export Menu").toStdString(),
      std::string(),
      fileBase
    );
    {
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/fileOSG.svg",
        QObject::tr("Export Open Scene Graph Command").toStdString(),
        (msg::Request | msg::Export | msg::OSG).to_string(),
        fileExport
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/fileOCC.svg",
        QObject::tr("Export OCC Brep Command").toStdString(),
        (msg::Request | msg::Export | msg::OCC).to_string(),
        fileExport
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/fileStep.svg",
        QObject::tr("Export Step Command").toStdString(),
        (msg::Request | msg::Export | msg::Step).to_string(),
        fileExport
      );
    }
    constructNode
    (
      mdv::gestureCommand,
      ":/resources/images/fileOpen.svg",
      QObject::tr("Open Project Command").toStdString(),
      (msg::Request | msg::Project | msg::Dialog).to_string(),
      fileBase
    );
    constructNode
    (
      mdv::gestureCommand,
      ":/resources/images/fileSave.svg",
      QObject::tr("Save Project Command").toStdString(),
      (msg::Request | msg::Save | msg::Project).to_string(),
      fileBase
    );
    constructNode
    (
      mdv::gestureCommand,
      ":/resources/images/fileRevision.svg",
      QObject::tr("Manage Revisions Command").toStdString(),
      (msg::Request | msg::Project | msg::Revision | msg::Dialog).to_string(),
      fileBase
    );
  }
  osg::MatrixTransform *systemBase = constructNode
  (
    mdv::gestureMenu,
    ":/resources/images/systemBase.svg",
    QObject::tr("Coordinate System Menu").toStdString(),
    std::string(),
    startNode.get()
  );
  {
    constructNode
    (
      mdv::gestureCommand,
      ":/resources/images/systemReset.svg",
      QObject::tr("Coordinate System Reset Command").toStdString(),
      (msg::Request | msg::SystemReset).to_string(),
      systemBase
    );
    constructNode
    (
      mdv::gestureCommand,
      ":/resources/images/systemToggle.svg",
      QObject::tr("Toggle Coordinate System Visibility Command").toStdString(),
      (msg::Request | msg::SystemToggle).to_string(),
      systemBase
    );
    constructNode
    (
      mdv::gestureCommand,
      ":/resources/images/systemToFeature.svg",
      QObject::tr("Coordinate System To Feature Command").toStdString(),
      (msg::Request | msg::SystemToFeature).to_string(),
      systemBase
    );
    constructNode
    (
      mdv::gestureCommand,
      ":/resources/images/systemToSelection.svg",
      QObject::tr("Coordinate System To Selection").toStdString(),
      (msg::Request | msg::SystemToSelection).to_string(),
      systemBase
    );
  }
  osg::MatrixTransform *inspectBase = constructNode
  (
    mdv::gestureMenu,
    ":/resources/images/inspectBase.svg",
    QObject::tr("Inspect Menu").toStdString(),
    std::string(),
    startNode.get()
  );
  {
    constructNode
    (
      mdv::gestureCommand,
      ":/resources/images/inspectAbout.svg",
      QObject::tr("About CadSeer").toStdString(),
      (msg::Request | msg::About | msg::Dialog).to_string(),
      inspectBase
    );
    constructNode
    (
      mdv::gestureCommand,
      ":/resources/images/inspectInfo.svg",
      QObject::tr("View Info Command").toStdString(),
      (msg::Request | msg::Info).to_string(),
      inspectBase
    );
    constructNode
    (
      mdv::gestureCommand,
      ":/resources/images/inspectCheckGeometry.svg",
      QObject::tr("Check Geometry For Errors").toStdString(),
      (msg::Request | msg::CheckGeometry).to_string(),
      inspectBase
    );
    osg::MatrixTransform *inspectMeasureBase = constructNode
    (
      mdv::gestureMenu,
      ":/resources/images/inspectMeasureBase.svg",
      QObject::tr("Measure Menu").toStdString(),
      std::string(),
      inspectBase
    );
    {
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/inspectMeasureClear.svg",
        QObject::tr("Clear Measure Command").toStdString(),
        (msg::Request | msg::Clear | msg::Overlay).to_string(),
        inspectMeasureBase
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/inspectLinearMeasure.svg",
        QObject::tr("LinearMeasure Command").toStdString(),
        (msg::Request | msg::LinearMeasure).to_string(),
        inspectMeasureBase
      );
    }
    osg::MatrixTransform *debugBase = constructNode
    (
      mdv::gestureMenu,
      ":/resources/images/debugBase.svg",
      QObject::tr("Debug Menu").toStdString(),
      std::string(),
      inspectBase
    );
    {
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/debugCheckShapeIds.svg",
        QObject::tr("Check Shaped Ids").toStdString(),
        (msg::Request | msg::CheckShapeIds).to_string(),
        debugBase
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/debugDump.svg",
        QObject::tr("Debug Feature Dump").toStdString(),
        (msg::Request | msg::DebugDump).to_string(),
        debugBase
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/debugShapeTrackUp.svg",
        QObject::tr("Track Shape Up").toStdString(),
        (msg::Request | msg::DebugShapeTrackUp).to_string(),
        debugBase
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/debugShapeTrackDown.svg",
        QObject::tr("Track Shape Down").toStdString(),
        (msg::Request | msg::DebugShapeTrackDown).to_string(),
        debugBase
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/debugShapeGraph.svg",
        QObject::tr("Write shape graph to application directory").toStdString(),
        (msg::Request | msg::DebugShapeGraph).to_string(),
        debugBase
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/debugDumpProjectGraph.svg",
        QObject::tr("Write project graph to application directory").toStdString(),
        (msg::Request | msg::DebugDumpProjectGraph).to_string(),
        debugBase
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/debugDumpDAGViewGraph.svg",
        QObject::tr("Write DAGView graph to application directory").toStdString(),
        (msg::Request | msg::DebugDumpDAGViewGraph).to_string(),
        debugBase
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/debugInquiry.svg",
        QObject::tr("Inquiry. A testing facility").toStdString(),
        (msg::Request | msg::DebugInquiry).to_string(),
        debugBase
      );
      constructNode
      (
        mdv::gestureCommand,
        ":/resources/images/dagViewPending.svg",
        QObject::tr("Dirty selected features").toStdString(),
        (msg::Request | msg::Feature | msg::Model | msg::Dirty).to_string(),
        debugBase
      );
    }
  }
}

std::vector<osg::Vec3> Handler::buildNodeLocations(osg::Vec3 direction, int nodeCount)
{
    double sprayRadius = calculateSprayRadius(nodeCount);
    
    osg::Vec3 point = direction;
    point.normalize();
    point *= sprayRadius;

    double localIncludedAngle = includedAngle;
    if (sprayRadius == minimumSprayRadius)
    {
        //now we limit the angle to get node separation.
        double singleAngle = osg::RadiansToDegrees(2 * asin(nodeSpread / (sprayRadius * 2)));
        localIncludedAngle = singleAngle * (nodeCount -1);
    }

    double incrementAngle = localIncludedAngle / (nodeCount - 1);
    double startAngle = localIncludedAngle / 2.0;
    if (nodeCount < 2)
        startAngle = 0;
    //I am missing something why are the following 2 vectors are opposite of what I would expect?
    osg::Matrixd startRotation = osg::Matrixd::rotate(osg::DegreesToRadians(startAngle), osg::Vec3d(0.0, 0.0, -1.0));
    osg::Matrixd incrementRotation = osg::Matrixd::rotate(osg::DegreesToRadians(incrementAngle), osg::Vec3d(0.0, 0.0, 1.0));
    point = (startRotation * point);
    std::vector<osg::Vec3> pointArray;
    pointArray.push_back(point);
    for (int index = 0; index < nodeCount - 1; index++)
    {
        point = (incrementRotation * point);
        pointArray.push_back(point);
    }
    return pointArray;
}

double Handler::calculateSprayRadius(int nodeCount)
{
    double segmentCount = nodeCount - 1;
    if (segmentCount < 1)
        return minimumSprayRadius;
    
    double angle = 0.0;
    double includedAngleRadians = osg::DegreesToRadians(includedAngle);
    
    //try to use minimum spray radius and check against include angle.
    angle = std::asin(nodeSpread / 2.0 / minimumSprayRadius) * 2.0;
    if ((angle * segmentCount) < includedAngleRadians)
      return minimumSprayRadius;
    
    //that didn't work so calculate angle and use to determin spray radius for node spread.
    double halfAngle = includedAngleRadians / segmentCount / 2.0;
    double hypt = nodeSpread / 2.0 / std::sin(halfAngle);
    return hypt;
}

void Handler::startDrag(const osgGA::GUIEventAdapter& eventAdapter)
{
    //send status
    node->send(msg::buildStatusMessage(QObject::tr("Start Menu").toStdString()));
  
    gestureSwitch->setAllChildrenOn();
    osg::Switch *startSwitch = dynamic_cast<osg::Switch *>(startNode->getChild(startNode->getNumChildren() - 1));
    assert(startSwitch);
    startSwitch->setValue(1, true);
    currentNode = startNode;
    currentNodeLeft = false;

    osg::Matrixd t = gestureCamera->getProjectionMatrix() * gestureCamera->getViewport()->computeWindowMatrix();
    t = osg::Matrixd::inverse(t);
    osg::Vec3d position(osg::Vec3d(eventAdapter.getX(), eventAdapter.getY(), 0.0) * t);
    position.z() = 0.0;
    
    osg::Matrixd temp = osg::Matrixd::scale(t.getScale()) * osg::Matrixd::translate(position);
    startNode->setMatrix(temp);
    aggregateMatrix = startNode->getMatrix();
}

void Handler::setupDispatcher()
{
  sift->insert
  (
    msg::Response | msg::Preferences
    , std::bind(&Handler::preferencesChangedDispatched, this, std::placeholders::_1)
  );
}

void Handler::updateVariables()
{
  iconRadius = prf::manager().rootPtr->gesture().iconRadius();
  iconRadius = std::max(iconRadius, 16.0);
  iconRadius = std::min(iconRadius, 128.0);
  
  includedAngle = static_cast<double>(prf::manager().rootPtr->gesture().includeAngle());
  includedAngle = std::max(includedAngle, 15.0);
  includedAngle = std::min(includedAngle, 360.0);
  
  double iconDiameter = iconRadius * 2.0;
  nodeSpread = iconDiameter + iconDiameter * prf::manager().rootPtr->gesture().spreadFactor();
  minimumSprayRadius = iconDiameter + iconDiameter * prf::manager().rootPtr->gesture().sprayFactor();
}

void Handler::preferencesChangedDispatched(const msg::Message&)
{
  
  if (iconRadius != prf::manager().rootPtr->gesture().iconRadius())
  {
    updateVariables();
    constructMenu();
  }
  else
    updateVariables();
}
