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
#include <iomanip>

#include <boost/filesystem.hpp>
#include <boost/current_function.hpp>

#include <QHBoxLayout>
#include <QApplication>
#include <QString>
#include <QFileDialog>
#include <QTimer>
#include <QTextStream>
#include <QScreen>
#include <QSvgRenderer>
#include <QPainter>

#include <osgGA/OrbitManipulator>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osg/PolygonMode>
#include <osg/ShadeModel>
#include <osg/Depth>
#include <osgUtil/Optimizer>
#include <osg/BlendFunc>
#include <osg/ValueObject>
#include <osg/DisplaySettings>
#include <osgText/Text>
#include <osgViewer/ViewerEventHandlers> //for subclass of stats handler.
#include <osgQOpenGL/OSGRenderer>

#include "globalutilities.h"
#include "tools/infotools.h"
#include "application/appapplication.h"
#include "application/appmainwindow.h"
#include "project/prjproject.h"
#include "project/prjmessage.h"
#include "modelviz/mdvnodemaskdefs.h"
#include "modelviz/mdvhiddenlineeffect.h"
#include "library/lbrcsysdragger.h"
#include "selection/slcdefinitions.h"
#include "selection/slceventhandler.h"
#include "selection/slcoverlayhandler.h"
#include "selection/slcvisitors.h"
#include "message/msgnode.h"
#include "message/msgsift.h"
#include "lod/lodmessage.h"
#include "feature/ftrbase.h"
#include "gesture/gsnhandler.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "project/serial/xsdcxxoutput/view.h"
#include "viewer/vwrspaceballmanipulator.h"
#include "viewer/vwrspaceballqevent.h"
#include "viewer/vwrspaceballosgevent.h"
#include "viewer/vwrmessage.h"
#include "viewer/vwrtextcamera.h"
#include "viewer/vwroverlay.h"
#include "viewer/vwrwidget.h"

namespace vwr
{
  class StatsHandler : public osgViewer::StatsHandler
  {
  public:
    void collectWhichCamerasToRenderStatsFor
      (osgViewer::ViewerBase *viewer, osgViewer::ViewerBase::Cameras &cameras) override
    {
      osgViewer::ViewerBase::Cameras tempCams;
      viewer->getCameras(tempCams);
      for (auto *cam : tempCams)
      {
        if (cam->getName() == "main")
          cameras.push_back(cam);
        if (cam->getName() == "overlay")
          cameras.push_back(cam);
      }
    }
  };

  class HiddenLineVisitor : public osg::NodeVisitor
  {
  public:
    HiddenLineVisitor(bool visIn) :
      osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), visibility(visIn){}
    virtual void apply(osg::Group &aGroup) override
    {
      mdv::HiddenLineEffect *effect = dynamic_cast<mdv::HiddenLineEffect*>(&aGroup);
      if (effect)
        effect->setHiddenLine(visibility);
      traverse(aGroup);
    }
  protected:
    bool visibility;
  };
  
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
  
  //restore states from serialize
  class SerialInViewVisitor : public osg::NodeVisitor
  {
  public:
    SerialInViewVisitor(const prj::srl::States &statesIn) :
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
              msg::hub().sendBlocked(msg::Message(msg::Mask(msg::Response | msg::View | msg::Show | msg::ThreeD), vm));
            }
            else
            {
              switchIn.setAllChildrenOff();
              msg::hub().sendBlocked(msg::Message(msg::Mask(msg::Response | msg::View | msg::Hide | msg::ThreeD), vm));
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
}

using namespace vwr;

struct Widget::Stow
{
  vwr::Widget *widget;
  msg::Node node;
  msg::Sift sift;
  osg::ref_ptr<osg::Group> root;
  osg::ref_ptr<osg::Switch> systemSwitch;
  osg::ref_ptr<lbr::CSysDragger> currentSystem;
  osg::ref_ptr<lbr::CSysCallBack> currentSystemCallBack;
  osg::ref_ptr<slc::EventHandler> selectionHandler;
  osg::ref_ptr<vwr::SpaceballManipulator> spaceballManipulator;
  osg::ref_ptr<osgViewer::ScreenCaptureHandler> screenCaptureHandler;
  
  Stow(vwr::Widget *wIn)
  : widget(wIn)
  {
    node.connect(msg::hub());
    sift.name = "vwr::Widget";
    node.setHandler(std::bind(&msg::Sift::receive, &sift, std::placeholders::_1));
    setupDispatcher();
  }
  
  osg::Camera* getMainCamera()
  {
    osgViewer::ViewerBase::Cameras cameras;
    widget->getOsgViewer()->getCameras(cameras);
    assert(!cameras.empty());
    return cameras.front();
  }
  
  osg::Viewport* getMainCameraViewport()
  {
    auto *mc = getMainCamera();
    osg::Viewport *mcvp = mc->getViewport();
    assert(mcvp);
    return mcvp;
  }
  
  std::pair<int, int> getMainCameraViewPortSizes()
  {
    auto *vp = getMainCameraViewport();
    return std::make_pair(vp->width(), vp->height());
  }
  
  osg::GraphicsContext* getContext()
  {
    osgViewer::ViewerBase::Contexts contexts;
    widget->getOsgViewer()->getContexts(contexts);
    assert(contexts.size() == 1);
    return contexts.front();
  }
  
  void setupMainCamera()
  {
    auto *mc = getMainCamera(); //main camera
    mc->setName("main");
    mc->setRenderOrder(osg::Camera::NESTED_RENDER, 1);
    mc->setClearMask(GL_DEPTH_BUFFER_BIT);
    mc->setCullingMode(mc->getCullingMode() & ~osg::CullSettings::SMALL_FEATURE_CULLING); //always show points
    spaceballManipulator = new vwr::SpaceballManipulator(mc);
    widget->getOsgViewer()->setCameraManipulator(spaceballManipulator);
    mc->setProjectionMatrixAsOrtho
    (
      0.0, static_cast<double>(widget->width()),
      0.0, static_cast<double>(widget->height()),
      1.0, 10000.0
    );
  }
  
  void setupBackgroundCamera()
  {
    const int imageSize = 1024;
    QImage qImage(imageSize, imageSize, QImage::Format_RGBA8888);
    qImage.fill(Qt::transparent);
    QSvgRenderer svgRenderer(QString(":/resources/images/background.svg"));
    if (svgRenderer.isValid())
    {
      QPainter painter(&qImage);
      painter.setRenderHint(QPainter::Antialiasing);
      svgRenderer.render(&painter, qImage.rect());
    }
    osg::ref_ptr<osg::Image> image = new osg::Image(); //default to blank image
    image->allocateImage(imageSize, imageSize, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    assert(qImage.sizeInBytes() == image->getTotalDataSize());
    if(qImage.sizeInBytes() == image->getTotalDataSize())
    {
      std::memcpy(image->data(), qImage.bits(), qImage.sizeInBytes());
    }
    else
    {
      std::cout << "Error: QImage size doesn't match osg::Image size" << std::endl;
      std::cout << "QImage size: " << qImage.sizeInBytes() << std::endl;
      std::cout << "osg::Image size: " << image->getTotalDataSize() << std::endl;
    }
    image->flipVertical(); //don't know why images are flipped and I need this?
        
    osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
    texture->setImage(image.get());
    osg::ref_ptr<osg::Geometry> quad = osg::createTexturedQuadGeometry
            (osg::Vec3(), osg::Vec3(1.0f, 0.0f, 0.0f), osg::Vec3(0.0f, 1.0f, 0.0f));
    texture->setDataVariance(osg::Object::STATIC);
    texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    quad->getOrCreateStateSet()->setTextureAttributeAndModes(0, texture.get());
    
    osg::Camera *bgCamera = new osg::Camera();
    bgCamera->setName("backgrd");
    bgCamera->setGraphicsContext(getContext());
    bgCamera->setCullingActive(false);
    bgCamera->setAllowEventFocus(false);
    bgCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    bgCamera->setRenderOrder(osg::Camera::NESTED_RENDER, 0);
    bgCamera->setProjectionMatrix(osg::Matrix::ortho2D(0.0, 1.0, 0.0, 1.0));
    bgCamera->addChild(quad.get());
    bgCamera->setNodeMask(mdv::backGroundCamera);

    osg::StateSet* ss = bgCamera->getOrCreateStateSet();
    ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    ss->setAttributeAndModes(new osg::Depth(osg::Depth::LEQUAL, 1.0, 1.0));
    
    auto p = getMainCameraViewPortSizes();
    bgCamera->setViewport(new osg::Viewport(0, 0, p.first, p.second));
    bgCamera->setProjectionResizePolicy(osg::Camera::ProjectionResizePolicy::FIXED);
    widget->getOsgViewer()->addSlave(bgCamera, false);
  }
  
  void setupGestureCamera()
  {
    double quadSize = 10000;

    osg::ref_ptr<osg::Geometry> quad = osg::createTexturedQuadGeometry
      (osg::Vec3(), osg::Vec3(quadSize, 0.0, 0.0), osg::Vec3(0.0, quadSize, 0.0));
    osg::Vec4Array *colorArray = new osg::Vec4Array();
    colorArray->push_back(osg::Vec4(0.0, 0.0, 0.0, 0.5));
    quad->setColorArray(colorArray);
    quad->setColorBinding(osg::Geometry::BIND_OVERALL);
    quad->setNodeMask(mdv::noIntersect);
    osg::Depth *depth = new osg::Depth;
    depth->setRange(0.005, 1.005);
    quad->getOrCreateStateSet()->setAttribute(depth);

    auto p = getMainCameraViewPortSizes();
    osg::Camera *fadeCamera = new osg::Camera();
    fadeCamera->setName("gesture");
    fadeCamera->setCullingActive(false);
    fadeCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
    fadeCamera->setAllowEventFocus(false);
    fadeCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    fadeCamera->setRenderOrder(osg::Camera::NESTED_RENDER, 4);
    fadeCamera->setProjectionMatrixAsOrtho2D
    (
      0.0, static_cast<double>(p.first),
      0.0, static_cast<double>(p.second)
    );
    fadeCamera->setViewMatrix(osg::Matrixd::identity());
    fadeCamera->setGraphicsContext(getContext());
    fadeCamera->setNodeMask(mdv::gestureCamera);
    fadeCamera->getOrCreateStateSet()->setMode(GL_MULTISAMPLE_ARB, osg::StateAttribute::ON);
    fadeCamera->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    fadeCamera->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    fadeCamera->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
    osg::ref_ptr<osg::BlendFunc> blendFunc = new osg::BlendFunc;
    blendFunc->setFunction(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    fadeCamera->getOrCreateStateSet()->setAttributeAndModes(blendFunc);

    osg::Switch *aSwitch = new osg::Switch();
    aSwitch->addChild(quad.get());
    aSwitch->setAllChildrenOff();
    fadeCamera->addChild(aSwitch);
    
    fadeCamera->setViewport(new osg::Viewport(0, 0, p.first, p.second));
    fadeCamera->setProjectionResizePolicy(osg::Camera::ProjectionResizePolicy::FIXED);
    widget->getOsgViewer()->addSlave(fadeCamera, false);
    widget->getOsgViewer()->addEventHandler(new gsn::Handler(fadeCamera));
  }
  
  void setupTextCamera()
  {
    TextCamera *infoCamera = new TextCamera();
    auto p = getMainCameraViewPortSizes();
    infoCamera->setViewport(0, 0, p.first, p.second);
    infoCamera->setProjectionResizePolicy(osg::Camera::ProjectionResizePolicy::FIXED);
    infoCamera->setGraphicsContext(getContext());
    infoCamera->setProjectionMatrix(osg::Matrix::ortho2D(0, p.first, 0, p.second));
    widget->getOsgViewer()->addSlave(infoCamera, false);
    widget->getOsgViewer()->addEventHandler(new ResizeEventHandler(infoCamera));
  }
  
  void setupOverlayCamera()
  {
    Overlay *oCamera = new Overlay();
    oCamera->setGraphicsContext(getContext());
    oCamera->setViewport(new osg::Viewport(*getMainCameraViewport()));
    widget->getOsgViewer()->addSlave(oCamera, false);
    systemSwitch = new osg::Switch();
    oCamera->addChild(systemSwitch);
    widget->getOsgViewer()->addEventHandler(new slc::OverlayHandler(oCamera));
  }
  
  void setupSystem()
  {
    currentSystem = new lbr::CSysDragger();
    currentSystem->setScreenScale(100.0f);
    currentSystem->setRotationIncrement(15.0);
    currentSystem->setTranslationIncrement(0.25);
    currentSystem->setHandleEvents(false);
    currentSystem->setupDefaultGeometry();
    currentSystem->setUnlink();
  //   currentSystem->hide(lbr::CSysDragger::SwitchIndexes::XRotate);
  //   currentSystem->hide(lbr::CSysDragger::SwitchIndexes::YRotate);
  //   currentSystem->hide(lbr::CSysDragger::SwitchIndexes::ZRotate);
    currentSystem->hide(lbr::CSysDragger::SwitchIndexes::LinkIcon);
    currentSystemCallBack = new lbr::CSysCallBack(currentSystem.get());
    currentSystem->addDraggerCallback(currentSystemCallBack.get());
    systemSwitch->addChild(currentSystem);
    systemSwitch->setValue(0, prf::manager().rootPtr->visual().display().showCurrentSystem());
  }
  
  void loadCursor()
  {
    //from
    //https://falsinsoft.blogspot.com/2016/04/qt-snippet-render-svg-to-qpixmap-for.html
    
    const qreal PixelRatio = qApp->primaryScreen()->devicePixelRatio();
    QSvgRenderer SvgRenderer(QString(":/resources/images/cursor.svg"));
    int size = static_cast<int>(32.0 * PixelRatio);
    QPixmap image (size, size);
    QPainter painter;

    image.fill(Qt::transparent);

    painter.begin(&image );
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    SvgRenderer.render(&painter );
    painter.end();

    image.setDevicePixelRatio(PixelRatio);
    widget->setCursor( image );
  }
  
  void myUpdate()
  {
    //I am not sure how useful calling optimizer on my generated data is?
    //TODO build large file and run with and without the optimizer to test.
    
    //commented out for osg upgrade to 3.5.6. screwed up structure
    //    to the point of selection not working.
    //remove redundant nodes was screwing up hidden line effect.
  //   osgUtil::Optimizer opt;
  //   opt.optimize
  //   (
  //     root,
  //     osgUtil::Optimizer::DEFAULT_OPTIMIZATIONS
  //     & ~osgUtil::Optimizer::REMOVE_REDUNDANT_NODES
  //   );
    
    //set the hidden line state.
    HiddenLineVisitor v(prf::Manager().rootPtr->visual().display().showHiddenLines());
    root->accept(v);
    //why not setting render style?
  }
  
  void viewTopDispatched(const msg::Message&)
  {
    spaceballManipulator->setView(osg::Vec3d(0.0, 0.0, -1.0), osg::Vec3d(0.0, 1.0, 0.0));
    spaceballManipulator->viewFit();
  }

  void viewTopCurrentDispatched(const msg::Message&)
  {
    osg::Vec3d z = -gu::getZVector(currentSystem->getMatrix());
    osg::Vec3d y = gu::getYVector(currentSystem->getMatrix());
    spaceballManipulator->setView(z, y);
    spaceballManipulator->viewFit();
  }

  void viewFrontDispatched(const msg::Message&)
  {
    spaceballManipulator->setView(osg::Vec3d(0.0, 1.0, 0.0), osg::Vec3d(0.0, 0.0, 1.0));
    spaceballManipulator->viewFit();
  }

  void viewFrontCurrentDispatched(const msg::Message&)
  {
    osg::Vec3d z = gu::getZVector(currentSystem->getMatrix());
    osg::Vec3d y = gu::getYVector(currentSystem->getMatrix());
    spaceballManipulator->setView(y, z);
    spaceballManipulator->viewFit();
  }

  void viewRightDispatched(const msg::Message&)
  {
    spaceballManipulator->setView(osg::Vec3d(-1.0, 0.0, 0.0), osg::Vec3d(0.0, 0.0, 1.0));
    spaceballManipulator->viewFit();
  }

  void viewRightCurrentDispatched(const msg::Message&)
  {
    osg::Vec3d x = -gu::getXVector(currentSystem->getMatrix());
    osg::Vec3d z = gu::getZVector(currentSystem->getMatrix());
    spaceballManipulator->setView(x, z);
    spaceballManipulator->viewFit();
  }

  void viewIsoDispatched(const msg::Message &)
  {
    osg::Vec3d upVector(-1.0, 1.0, 1.0); upVector.normalize();
    osg::Vec3d lookVector(-1.0, 1.0, -1.0); lookVector.normalize();
    spaceballManipulator->setView(lookVector, upVector);
    spaceballManipulator->viewFit();
  }

  void viewFitDispatched(const msg::Message&)
  {
    spaceballManipulator->home(1.0);
    spaceballManipulator->viewFit();
  }

  void viewFillDispatched(const msg::Message&)
  {
    osg::PolygonMode *pMode = dynamic_cast<osg::PolygonMode*>
      (root->getOrCreateStateSet()->getAttribute(osg::StateAttribute::POLYGONMODE));
    assert(pMode);
    pMode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::FILL);
    
    //update preferences.
    prf::manager().rootPtr->visual().display().renderStyle() = prf::RenderStyle::fill;
    prf::manager().saveConfig();
  }
  
  void viewTriangulationDispatched(const msg::Message&)
  {
    osg::PolygonMode *pMode = dynamic_cast<osg::PolygonMode*>
      (root->getOrCreateStateSet()->getAttribute(osg::StateAttribute::POLYGONMODE));
    assert(pMode);
    pMode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
    
    //update preferences.
    prf::manager().rootPtr->visual().display().renderStyle() = prf::RenderStyle::triangulation;
    prf::manager().saveConfig();
  }

  void renderStyleToggleDispatched(const msg::Message &)
  {
    prf::RenderStyle::Value cStyle = prf::Manager().rootPtr->visual().display().renderStyle().get();
    if (cStyle == prf::RenderStyle::fill)
      viewTriangulationDispatched(msg::Message());
    else if (cStyle == prf::RenderStyle::triangulation)
      viewFillDispatched(msg::Message());
    //wireframe some day.
  }

  void exportOSGDispatched(const msg::Message&)
  {
    //ive doesn't appear to be working?
    
    QString fileName = QFileDialog::getSaveFileName
    (
      app::instance()->getMainWindow(),
      tr("Save File"),
      QString::fromStdString(prf::manager().rootPtr->project().lastDirectory().get()),
      tr("Scene (*.osgt *.osgx *.osgb *.osg *.ive)")
    );
    if (fileName.isEmpty())
      return;
    
    boost::filesystem::path p = fileName.toStdString();
    prf::manager().rootPtr->project().lastDirectory() = p.remove_filename().string();
    prf::manager().saveConfig();
    
    if
    (
      (!fileName.endsWith(QObject::tr(".osgt"))) &&
      (!fileName.endsWith(QObject::tr(".osgx"))) &&
      (!fileName.endsWith(QObject::tr(".osgb"))) &&
      (!fileName.endsWith(QObject::tr(".osg"))) &&
      (!fileName.endsWith(QObject::tr(".ive")))
    )
      fileName += QObject::tr(".osgt");
      
    node.sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    osgDB::writeNodeFile(*root, fileName.toStdString());
  }
  
  void serialRead()
  {
    boost::filesystem::path file = app::instance()->getProject()->getSaveDirectory();
    file /= "view.xml";
    if (!boost::filesystem::exists(file))
      return;
    
    auto sView = prj::srl::view(file.string(), ::xml_schema::Flags::dont_validate);
    SerialInViewVisitor v(sView->states());
    root->accept(v);
    
    if (sView->csys().present())
    {
      const auto &mIn = sView->csys().get();
      osg::Matrixd m
      (
        mIn.i0j0(), mIn.i0j1(), mIn.i0j2(), mIn.i0j3(),
        mIn.i1j0(), mIn.i1j1(), mIn.i1j2(), mIn.i1j3(),
        mIn.i2j0(), mIn.i2j1(), mIn.i2j2(), mIn.i2j3(),
        mIn.i3j0(), mIn.i3j1(), mIn.i3j2(), mIn.i3j3()
      );
      spaceballManipulator->setByMatrix(m);
    }
    
    if (sView->ortho().present())
    {
      const auto &p = sView->ortho().get();
      getMainCamera()->setProjectionMatrixAsOrtho(p.left(), p.right(), p.bottom(), p.top(), p.near(), p.far());
    }
  }

  void serialWrite()
  {
    prj::srl::States states;
    SerialOutVisitor v(states);
    root->accept(v);
    prj::srl::View svOut(states);
    
    osg::Matrixd m = getMainCamera()->getViewMatrix();
    prj::srl::Matrixd mOut
    (
      m(0,0), m(0,1), m(0,2), m(0,3),
      m(1,0), m(1,1), m(1,2), m(1,3),
      m(2,0), m(2,1), m(2,2), m(2,3),
      m(3,0), m(3,1), m(3,2), m(3,3)
    );
    svOut.csys() = mOut;
    
    double left, right, bottom, top, near, far;
    if (getMainCamera()->getProjectionMatrixAsOrtho(left, right, bottom, top, near, far))
      svOut.ortho() = prj::srl::Ortho(left, right, bottom, top, near, far);
    
    boost::filesystem::path file = app::instance()->getProject()->getSaveDirectory();
    file /= "view.xml";
    xml_schema::NamespaceInfomap infoMap;
    std::ofstream stream(file.string());
    prj::srl::view(stream, svOut, infoMap);
  }
  
  void featureAddedDispatched(const msg::Message &messageIn)
  {
    prj::Message message = messageIn.getPRJ();
    root->addChild(message.feature->getMainSwitch());
    
    slc::PLODPathVisitor visitor(app::instance()->getProject()->getSaveDirectory().string());
    message.feature->getMainSwitch()->accept(visitor);
  }

  void featureRemovedDispatched(const msg::Message &messageIn)
  {
    prj::Message message = messageIn.getPRJ();
    root->removeChild(message.feature->getMainSwitch());
  }

  void visualUpdatedDispatched(const msg::Message &)
  {
    this->myUpdate();
  }

  void closeProjectDispatched(const msg::Message&)
  {
    //don't need to keep any children of the viewer.
    root->removeChildren(0, root->getNumChildren());
  }

  void systemResetDispatched(const msg::Message&)
  {
    currentSystem->setMatrix(osg::Matrixd::identity());
  }

  void systemToggleDispatched(const msg::Message&)
  {
    if (systemSwitch->getValue(0))
      systemSwitch->setAllChildrenOff();
    else
      systemSwitch->setAllChildrenOn();
    
    prf::Manager &manager = prf::manager();
    manager.rootPtr->visual().display().showCurrentSystem() = systemSwitch->getValue(0);
    manager.saveConfig();
  }

  void showHiddenLinesDispatched(const msg::Message&)
  {
    HiddenLineVisitor v(true);
    root->accept(v);
    
    //make sure prefs are in sync.
    prf::manager().rootPtr->visual().display().showHiddenLines() = true;
    prf::manager().saveConfig();
  }

  void hideHiddenLinesDispatched(const msg::Message&)
  {
    HiddenLineVisitor v(false);
    root->accept(v);
    
    //make sure prefs are in sync.
    prf::manager().rootPtr->visual().display().showHiddenLines() = false;
    prf::manager().saveConfig();
  }

  void viewToggleHiddenLinesDispatched(const msg::Message&)
  {
    prf::Manager &manager = prf::manager();
    bool oldValue = manager.rootPtr->visual().display().showHiddenLines();
    manager.rootPtr->visual().display().showHiddenLines() = !oldValue;
    manager.saveConfig();
    
    //set the hidden line state.
    HiddenLineVisitor v(!oldValue);
    root->accept(v);
  }

  void showThreeDDispatched(const msg::Message &msgIn)
  {
    const vwr::Message &vm = msgIn.getVWR();
    slc::MainSwitchVisitor v(vm.featureId);
    root->accept(v);
    assert(v.out);
    if (!v.out)
      return;
    
    if (v.out->getNewChildDefaultValue()) //already shown.
      return;
    
    v.out->setAllChildrenOn();
    
    msg::Message mOut(msg::Response | msg::View | msg::Show | msg::ThreeD, vm);
    node.sendBlocked(mOut);
    
    //see message in toggled for why we do this after the message.
    HiddenLineVisitor hlv(prf::manager().rootPtr->visual().display().showHiddenLines());
    v.out->accept(hlv);
  }

  void hideThreeDDispatched(const msg::Message &msgIn)
  {
    const vwr::Message &vm = msgIn.getVWR();
    slc::MainSwitchVisitor v(vm.featureId);
    root->accept(v);
    assert(v.out);
    if (!v.out)
      return;
    
    if (!v.out->getNewChildDefaultValue()) //already hidden.
      return;
    
    v.out->setAllChildrenOff();
    
    msg::Message mOut(msg::Response | msg::View | msg::Hide | msg::ThreeD, vm);
    node.sendBlocked(mOut);
  }

  void threeDToggleDispatched(const msg::Message &msgIn)
  {
    const vwr::Message &vm = msgIn.getVWR();
    slc::MainSwitchVisitor v(vm.featureId);
    root->accept(v);
    assert(v.out);
    if (!v.out)
      return;
    msg::Mask maskOut; //notice we are not responding with Msg::Toggle that got us here.
    if (v.out->getNewChildDefaultValue())
    {
      v.out->setAllChildrenOff();
      maskOut = msg::Response | msg::View | msg::Hide | msg::ThreeD;
    }
    else
    {
      v.out->setAllChildrenOn();
      maskOut = msg::Response | msg::View | msg::Show | msg::ThreeD;
    }
    
    msg::Message mOut(maskOut, vm);
    node.sendBlocked(mOut);
    
    //we don't generate visuals until needed. so hidden and inactive features
    //have none or bogus osg data. This is the reason for the response message above.
    //It triggers the project to update the feature's visual data. Now we can run
    //the hidden line visitor over the generated osg data.
    HiddenLineVisitor hlv(prf::manager().rootPtr->visual().display().showHiddenLines());
    v.out->accept(hlv);
  }

  void projectOpenedDispatched(const msg::Message &)
  {
    serialRead();
    spaceballManipulator->computeHome();
  }

  void projectUpdatedDispatched(const msg::Message &)
  {
    serialWrite();
  }

  void lodGeneratedDispatched(const msg::Message &mIn)
  {
    const lod::Message &m = mIn.getLOD();
    slc::PLODIdVisitor vis(m.featureId);
    root->accept(vis);
    assert(vis.out != nullptr); //temp for development. feature might have been deleted.
    if (vis.out == nullptr)
    {
      std::cout << "WARNING: can't find paged lod node in lodGeneratedDispatched" << std::endl;
      return;
    }
    if (!boost::filesystem::exists(m.filePathOSG))
    {
      std::cout
      << "Warning: file doesn't exist"
      << m.filePathOSG.string()
      << " in lodGeneratedDispatched"
      << std::endl;
      return;
    }
    osg::Node *fileNode = osgDB::readNodeFile(m.filePathOSG.string());
    if (!fileNode)
    {
      std::cout
      << "Warning: failed to load file "
      << m.filePathOSG.string()
      << " in lodGeneratedDispatched"
      << std::endl;
      return;
    }
    
    HiddenLineVisitor v(prf::manager().rootPtr->visual().display().showHiddenLines());
    fileNode->accept(v);
    
    //find child to remove.
    bool foundChild = false;
    for (unsigned int i = 0; i < vis.out->getNumChildren(); ++i)
    {
      float tempMin = vis.out->getMinRange(i);
      float tempMax = vis.out->getMaxRange(i);
      if
      (
        (tempMin == static_cast<float>(m.rangeMin))
        && (tempMax == static_cast<float>(m.rangeMax))
      )
      {
        vis.out->removeChild(i);
        foundChild = true;
        break;
      }
    }
    assert(foundChild);
    if (!foundChild)
    {
      std::cout
      << "Warning: failed to find child for "
      << m.filePathOSG.string()
      << " in lodGeneratedDispatched"
      << std::endl;
      return;
    }
    
    vis.out->addChild(fileNode, static_cast<float>(m.rangeMin), static_cast<float>(m.rangeMax), m.filePathOSG.string());
    ftr::Base *f = app::instance()->getProject()->findFeature(m.featureId);
    assert(f);
    f->applyColor();
  }
    
  void setupDispatcher()
  {
    sift.insert
    (
      {
        std::make_pair
        (
          msg::Response | msg::Post | msg::Add | msg::Feature
          , std::bind(&Stow::featureAddedDispatched, this, std::placeholders::_1)
        )
        , std::make_pair
        (
          msg::Response | msg::Pre | msg::Remove | msg::Feature
          , std::bind(&Stow::featureRemovedDispatched, this, std::placeholders::_1)
        )
        , std::make_pair
        (
          msg::Response | msg::Post | msg::Project | msg::Update | msg::Visual
          , std::bind(&Stow::visualUpdatedDispatched, this, std::placeholders::_1)
        )
        , std::make_pair
        (
          msg::Request | msg::View | msg::Top
          , std::bind(&Stow::viewTopDispatched, this, std::placeholders::_1)
        )
        , std::make_pair
        (
          msg::Request | msg::View | msg::Top | msg::Current
          , std::bind(&Stow::viewTopCurrentDispatched, this, std::placeholders::_1)
        )
        , std::make_pair
        (
          msg::Request | msg::View | msg::Front
          , std::bind(&Stow::viewFrontDispatched, this, std::placeholders::_1)
        )
        , std::make_pair
        (
          msg::Request | msg::View | msg::Front | msg::Current
          , std::bind(&Stow::viewFrontCurrentDispatched, this, std::placeholders::_1)
        )
        , std::make_pair
        (
          msg::Request | msg::View | msg::Right
          , std::bind(&Stow::viewRightDispatched, this, std::placeholders::_1)
        )
        , std::make_pair
        (
          msg::Request | msg::View | msg::Right | msg::Current
          , std::bind(&Stow::viewRightCurrentDispatched, this, std::placeholders::_1)
        )
        , std::make_pair
        (
          msg::Request | msg::View | msg::Iso
          , std::bind(&Stow::viewIsoDispatched, this, std::placeholders::_1)
        )
        , std::make_pair
        (
          msg::Request | msg::View | msg::Fit
          , std::bind(&Stow::viewFitDispatched, this, std::placeholders::_1)
        )
        , std::make_pair
        (
          msg::Request | msg::View | msg::Fill
          , std::bind(&Stow::viewFillDispatched, this, std::placeholders::_1)
        )
        , std::make_pair
        (
          msg::Request | msg::View | msg::Triangulation
          , std::bind(&Stow::viewTriangulationDispatched, this, std::placeholders::_1)
        )
        , std::make_pair
        (
          msg::Request | msg::View | msg::RenderStyle | msg::Toggle
          , std::bind(&Stow::renderStyleToggleDispatched, this, std::placeholders::_1)
        )
        , std::make_pair
        (
          msg::Request | msg::Export | msg::OSG
          , std::bind(&Stow::exportOSGDispatched, this, std::placeholders::_1)
        )
        , std::make_pair
        (
          msg::Response | msg::Pre | msg::Close | msg::Project
          , std::bind(&Stow::closeProjectDispatched, this, std::placeholders::_1)
        )
        , std::make_pair
        (
          msg::Request | msg::SystemReset
          , std::bind(&Stow::systemResetDispatched, this, std::placeholders::_1)
        )
        , std::make_pair
        (
          msg::Request | msg::SystemToggle
          , std::bind(&Stow::systemToggleDispatched, this, std::placeholders::_1)
        )
        , std::make_pair
        (
          msg::Request | msg::View | msg::Show | msg::HiddenLine
          , std::bind(&Stow::showHiddenLinesDispatched, this, std::placeholders::_1)
        )
        , std::make_pair
        (
          msg::Request | msg::View | msg::Hide | msg::HiddenLine
          , std::bind(&Stow::hideHiddenLinesDispatched, this, std::placeholders::_1)
        )
        , std::make_pair
        (
          msg::Request | msg::View | msg::Toggle | msg::HiddenLine
          , std::bind(&Stow::viewToggleHiddenLinesDispatched, this, std::placeholders::_1)
        )
        , std::make_pair
        (
          msg::Request | msg::View | msg::Show | msg::ThreeD
          , std::bind(&Stow::showThreeDDispatched, this, std::placeholders::_1)
        )
        , std::make_pair
        (
          msg::Request | msg::View | msg::Hide | msg::ThreeD
          , std::bind(&Stow::hideThreeDDispatched, this, std::placeholders::_1)
        )
        , std::make_pair
        (
          msg::Request | msg::View | msg::Toggle | msg::ThreeD
          , std::bind(&Stow::threeDToggleDispatched, this, std::placeholders::_1)
        )
        , std::make_pair
        (
          msg::Response | msg::Post | msg::Open | msg::Project
          , std::bind(&Stow::projectOpenedDispatched, this, std::placeholders::_1)
        )
        , std::make_pair
        (
          msg::Response | msg::Post | msg::Project | msg::Update | msg::Model
          , std::bind(&Stow::projectUpdatedDispatched, this, std::placeholders::_1)
        )
        , std::make_pair
        (
          msg::Response | msg::Construct | msg::LOD
          , std::bind(&Stow::lodGeneratedDispatched, this, std::placeholders::_1)
        )
      }
    );
  }
  
  void go()
  {
    auto *v = widget->getOsgViewer();
    v->setKeyEventSetsDone(0);
//     v->setUseConfigureAffinity(false);
    
    root = new osg::Group;
    root->setName("root");
    auto *rss = root->getOrCreateStateSet(); //root state set.
    rss->setAttribute(new osg::ShadeModel(osg::ShadeModel::SMOOTH));
    rss->setMode(GL_MULTISAMPLE_ARB, osg::StateAttribute::ON);
    rss->setMode(GL_LIGHTING, osg::StateAttribute::ON);
    rss->setAttribute(new osg::PolygonMode());
    prf::RenderStyle::Value renderStyle = prf::Manager().rootPtr->visual().display().renderStyle().get();
    if (renderStyle == prf::RenderStyle::fill)
      app::instance()->queuedMessage(msg::Message(msg::Request | msg::View | msg::Fill));
    else if (renderStyle == prf::RenderStyle::triangulation)
      app::instance()->queuedMessage(msg::Message(msg::Request | msg::View | msg::Triangulation));
    //nothing for wireframe yet.
    v->setSceneData(root.get()); //need this set before manipulator is added in 'setupMainCamera'
    
    setupMainCamera();
    setupBackgroundCamera();
    setupGestureCamera();
    setupTextCamera();
    setupOverlayCamera();
    setupSystem();
    loadCursor();
    
    v->addEventHandler(new StatsHandler());
    selectionHandler = new slc::EventHandler(root);
    v->addEventHandler(selectionHandler.get());
    screenCaptureHandler = new osgViewer::ScreenCaptureHandler();
    screenCaptureHandler->setKeyEventTakeScreenShot(-1); //no keyboard execution
    screenCaptureHandler->setKeyEventToggleContinuousCapture(-1); //no keyboard execution
    
    //get screen resolution. do I need this?
    osg::GraphicsContext::WindowingSystemInterface *wsi = getContext()->getWindowingSystemInterface();
    assert(wsi->getNumScreens() > 0);
    osg::GraphicsContext::ScreenSettings settings;
    wsi->getScreenSettings(osg::GraphicsContext::ScreenIdentifier(0), settings);
    //wsi->getDisplaySettings is null here and also in myUpdate.
    osg::DisplaySettings::instance()->setScreenHeight(static_cast<float>(settings.height));
    osg::DisplaySettings::instance()->setScreenWidth(static_cast<float>(settings.width));
  }
};

Widget::Widget(QWidget *parent)
: osgQOpenGLWidget(parent)
, stow(std::make_unique<Stow>(this))
{
  setMouseTracking(true);
  setFocusPolicy(Qt::ClickFocus);
  connect(this, &Widget::initialized, std::bind(&Stow::go, stow.get()));
}

Widget::~Widget() = default;

bool Widget::event(QEvent *event)
{
  auto buildMotion = [&]() -> vwr::SpaceballOSGEvent*
  {
    vwr::SpaceballOSGEvent *osgEvent = new vwr::SpaceballOSGEvent();
    vwr::MotionEvent *spaceQEvent = dynamic_cast<vwr::MotionEvent *>(event);
    osgEvent->theType = vwr::SpaceballOSGEvent::Motion;
    osgEvent->translationX = spaceQEvent->translationX();
    osgEvent->translationY = spaceQEvent->translationY();
    osgEvent->translationZ = spaceQEvent->translationZ();
    osgEvent->rotationX = spaceQEvent->rotationX();
    osgEvent->rotationY = spaceQEvent->rotationY();
    osgEvent->rotationZ = spaceQEvent->rotationZ();
    return osgEvent;
  };
  
  auto buildButton = [&]() -> vwr::SpaceballOSGEvent*
  {
    vwr::SpaceballOSGEvent *osgEvent = new vwr::SpaceballOSGEvent();
    vwr::ButtonEvent *buttonQEvent = dynamic_cast<vwr::ButtonEvent*>(event);
    assert(buttonQEvent);
    osgEvent->theType = vwr::SpaceballOSGEvent::Button;
    osgEvent->buttonNumber = buttonQEvent->buttonNumber();
    if (buttonQEvent->buttonStatus() == vwr::BUTTON_PRESSED)
      osgEvent->theButtonState = vwr::SpaceballOSGEvent::Pressed;
    else
      osgEvent->theButtonState = vwr::SpaceballOSGEvent::Released;
    return osgEvent;
  };
  
  auto sendEvent = [&](vwr::SpaceballOSGEvent *e)
  {
    static_cast<vwr::EventBase*>(event)->setHandled(true);
    osg::ref_ptr<osgGA::GUIEventAdapter> osgEvent = getOsgViewer()->getEventQueue()->createEvent();
    osgEvent->setEventType(osgGA::GUIEventAdapter::USER);
    osgEvent->setUserData(e);
    getOsgViewer()->getEventQueue()->addEvent(osgEvent);
  };
  
  if (event->type() == vwr::MotionEvent::Type)
  {
    sendEvent(buildMotion());
    return true;
  }
  else if (event->type() == vwr::ButtonEvent::Type)
  {
    sendEvent(buildButton());
    return true;
  }
  return osgQOpenGLWidget::event(event);
}

void Widget::keyPressEvent(QKeyEvent* event)
{
  Q_ASSERT(m_renderer);
  // forward event to renderer
  m_renderer->keyPressEvent(event);
}

slc::EventHandler* Widget::getSelectionEventHandler()
{
  return stow->selectionHandler;
}

const osg::Matrixd& Widget::getCurrentSystem() const
{
  return stow->currentSystem->getMatrix();
}

void Widget::setCurrentSystem(const osg::Matrixd &mIn)
{
  stow->currentSystem->setMatrix(mIn);
}

const osg::Matrixd& Widget::getViewSystem() const
{
  return stow->getMainCamera()->getViewMatrix();
}

QTextStream& Widget::getInfo(QTextStream &stream) const
{
  stream << endl << QObject::tr("Current System: ");
  gu::osgMatrixOut(stream, getCurrentSystem());
  
  return stream;
}

/*! @brief Diagonal length of screen in world units.
 * 
 * used for scaling world objects to current view.
 */
double Widget::getDiagonalLength() const
{
  auto *mc = stow->getMainCamera();
  osg::Matrixd m = mc->getViewMatrix();
  m.postMult(mc->getProjectionMatrix());
  m.postMult(mc->getViewport()->computeWindowMatrix());
  osg::Matrixd i = osg::Matrixd::inverse(m);
  
  auto p = stow->getMainCameraViewPortSizes();
  osg::Vec3d origin(osg::Vec3d(0.0, 0.0, 0.0) * i);
  osg::Vec3d corner(osg::Vec3d(static_cast<double>(p.first), static_cast<double>(p.second), 0.0) * i);
  double length = (corner - origin).length(); 
  return length;
}

/*! @brief Create a screen grab of graphics.
 * 
 * @param fp file path without dot and extension.
 * @param e extension without the dot
 * 
 */
void Widget::screenCapture(const std::string &fp, const std::string &e)
{
  osg::ref_ptr<osgViewer::ScreenCaptureHandler::WriteToFile> wtf = new osgViewer::ScreenCaptureHandler::WriteToFile
  (
    fp, e, osgViewer::ScreenCaptureHandler::WriteToFile::OVERWRITE
  );
  stow->screenCaptureHandler->setFramesToCapture(1);
  stow->screenCaptureHandler->setCaptureOperation(wtf.get());
  stow->screenCaptureHandler->captureNextFrame(*getOsgViewer());
}
