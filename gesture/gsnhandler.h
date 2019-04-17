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

#ifndef GSN_HANDLER_H
#define GSN_HANDLER_H

#include <memory>

#include <osgGA/GUIEventHandler>

namespace msg{class Message; struct Node; struct Sift;}

namespace gsn
{
  class Handler : public osgGA::GUIEventHandler
  {
  public:
    Handler(osg::Camera *cameraIn);
      
  protected:
    virtual bool handle(const osgGA::GUIEventAdapter& eventAdapter,
                        osgGA::GUIActionAdapter& actionAdapter, osg::Object *object,
                        osg::NodeVisitor *nodeVistor) override;
    double calculateSprayRadius(int nodeCount);
    void startDrag(const osgGA::GUIEventAdapter& eventAdapter);
    void constructMenu();
    std::vector<osg::Vec3> buildNodeLocations(osg::Vec3 direction, int nodeCount);
    void spraySubNodes(osg::Vec3 cursorLocation);
    void contractSubNodes();
    bool rightButtonDown;
    bool dragStarted;
    int spaceballButton = -1; //button pressed or -1 for no button pressed.
    int hotKey = -1; //-1 = no key or invalid key pressed.
    osg::ref_ptr<osg::Switch> gestureSwitch;
    osg::ref_ptr<osg::Camera> gestureCamera;
    osg::ref_ptr<osg::MatrixTransform> startNode;
    osg::ref_ptr<osg::MatrixTransform> currentNode;
    osg::Matrixd aggregateMatrix;
    bool currentNodeLeft;
    
    float time();

    double iconRadius;
    double includedAngle; //in degrees
    double minimumSprayRadius;
    double nodeSpread;
    void updateVariables();
    
    std::unique_ptr<msg::Node> node;
    std::unique_ptr<msg::Sift> sift;
    void setupDispatcher();
    void preferencesChangedDispatched(const msg::Message&);
  };
}

#endif // GSN_HANDLER_H
