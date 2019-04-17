/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#include <QTextStream>

#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>

#include <osg/AutoTransform>

#include "globalutilities.h"
#include "application/appmainwindow.h"
#include "application/appmessage.h"
#include "viewer/vwrwidget.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "message/msgsift.h"
#include "selection/slceventhandler.h"
#include "selection/slcmessage.h"
#include "viewer/vwrmessage.h"
#include "feature/ftrbase.h"
#include "annex/annseershape.h"
#include "library/lbrlineardimension.h"
#include "command/cmdmeasurelinear.h"

using namespace cmd;

MeasureLinear::MeasureLinear() : Base()
{
  sift = std::make_unique<msg::Sift>();
  sift->name = "cmd::MeasureLinear";
  node->setHandler(std::bind(&msg::Sift::receive, sift.get(), std::placeholders::_1));
  setupDispatcher();
  
  selectionMask = slc::AllEnabled;
  
  shouldUpdate = false;
}

MeasureLinear::~MeasureLinear(){}

std::string MeasureLinear::getStatusMessage()
{
  return QObject::tr("Select 2 objects for measure linear").toStdString();
}

void MeasureLinear::activate()
{
  isActive = true;
  node->sendBlocked(msg::buildSelectionMask(selectionMask));
  go();
}

void MeasureLinear::deactivate()
{
  isActive = false;
}

void MeasureLinear::go()
{
  auto getShape = [&](const slc::Container &cIn) -> TopoDS_Shape
  {
    if (slc::isPointType(cIn.selectionType))
    {
      TopoDS_Vertex v = BRepBuilderAPI_MakeVertex(gp_Pnt(gu::toOcc(cIn.pointLocation).XYZ()));
      return v;
    }
    
    ftr::Base *fb = project->findFeature(cIn.featureId);
    if (!fb->hasAnnex(ann::Type::SeerShape) || fb->getAnnex<ann::SeerShape>(ann::Type::SeerShape).isNull())
      return TopoDS_Shape();
    if (cIn.shapeId.is_nil())
      return fb->getAnnex<ann::SeerShape>(ann::Type::SeerShape).getRootOCCTShape();
    return fb->getAnnex<ann::SeerShape>(ann::Type::SeerShape).getOCCTShape(cIn.shapeId);
  };
  
  const slc::Containers &containers = eventHandler->getSelections();
  if (containers.size() != 2)
    return;
  TopoDS_Shape s1 = getShape(containers.front());
  TopoDS_Shape s2 = getShape(containers.back());
  if(s1.IsNull() || s2.IsNull())
    return;
  
  BRepExtrema_DistShapeShape ext(s1, s2, Extrema_ExtFlag_MIN);
  if (!ext.IsDone() || ext.NbSolution() < 1)
  {
    std::cout << "extrema failed in MeasureLinear::go" << std::endl;
    return;
  }
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  osg::Vec3d point1(gu::toOsg(ext.PointOnShape1(1)));
  osg::Vec3d point2(gu::toOsg(ext.PointOnShape2(1)));
  build(point1, point2);
  //don't send done. this causes a loop and continues to measure until user hits esc.
//   sendDone();
}

void MeasureLinear::build(const osg::Vec3d &point1, const osg::Vec3d &point2)
{
  double distance = (point2 - point1).length();
    
  QString infoMessage;
  QTextStream stream(&infoMessage);
  //if I don't set the realNumberNotation I get 10 decimal places. 
  //with it I get the specified 12. Why qt?
  stream.setRealNumberNotation(QTextStream::FixedNotation);
  forcepoint(stream) << qSetRealNumberPrecision(12);
  auto streamPoint = [&](const osg::Vec3d &p)
  {
    stream
      << "["
      << p.x() << ", "
      << p.y() << ", "
      << p.z() << "]";
  };
  
  stream << endl << "Measure linear: "<< endl
    << "    3d distance: "<< distance
    << endl << "    Absolute System:" << endl << "        Point1 location: ";
  streamPoint(point1);
  stream << endl << "        Point2 location: ";
  streamPoint(point2);
  
  stream << endl << "        delta X: " << point2.x() - point1.x()
    << endl << "        delta Y: " << point2.y() - point1.y()
    << endl << "        delta Z: " << point2.z() - point1.z();
  
  //report points in current system.
  osg::Matrixd ics = osg::Matrixd::inverse(viewer->getCurrentSystem()); //inverted current system.
  osg::Vec3d csPoint1 = point1 * ics; //current system point1
  osg::Vec3d csPoint2 = point2 * ics; //current system point2
  stream << endl << "    Current system:" << endl << "        Point1 location: ";
  streamPoint(csPoint1);
  stream << endl << "        Point2 location: ";
  streamPoint(csPoint2);
  stream << endl << "        delta X: " << csPoint2.x() - csPoint1.x()
    << endl << "        delta Y: " << csPoint2.y() - csPoint1.y()
    << endl << "        delta Z: " << csPoint2.z() - csPoint1.z();
  stream << endl;
  
  app::Message appMessage;
  appMessage.infoMessage = infoMessage;
  msg::Message viewInfoMessage(msg::Request | msg::Info | msg::Text, appMessage);
  node->sendBlocked(viewInfoMessage);
  
  if (distance < std::numeric_limits<float>::epsilon())
    return;
  
  //get the view matrix for orientation.
  osg::Matrixd viewMatrix = mainWindow->getViewer()->getViewSystem();
  osg::Vec3d yVector = point2 - point1; yVector.normalize();
  osg::Vec3d zVectorView = gu::getZVector(viewMatrix); zVectorView.normalize();
  osg::Vec3d xVector = zVectorView ^ yVector;
  if (xVector.isNaN())
  {
    node->sendBlocked(msg::buildStatusMessage(
      QObject::tr("Can't make dimension with current view direction").toStdString(), 2.0));
    return;
  }
  xVector.normalize();
  //got to be an easier way!
  osg::Vec3d zVector  = xVector ^ yVector;
  zVector.normalize();
  osg::Matrixd transform
  (
      xVector.x(), xVector.y(), xVector.z(), 0.0,
      yVector.x(), yVector.y(), yVector.z(), 0.0,
      zVector.x(), zVector.y(), zVector.z(), 0.0,
      0.0, 0.0, 0.0, 1.0
  );
  
  //probably should be somewhere else.
  osg::ref_ptr<osg::AutoTransform> autoTransform = new osg::AutoTransform();
  autoTransform->setPosition(point1);
  autoTransform->setAutoRotateMode(osg::AutoTransform::ROTATE_TO_AXIS);
  autoTransform->setAxis(yVector);
  autoTransform->setNormal(-zVector);
  
  osg::ref_ptr<lbr::LinearDimension> dim = new lbr::LinearDimension();
  dim->setMatrix(transform);
  dim->setColor(osg::Vec4d(0.8, 0.0, 0.0, 1.0));
  dim->setSpread((point2 - point1).length());
  autoTransform->addChild(dim.get());
  
  vwr::Message vwrMessage;
  vwrMessage.node = autoTransform;
  msg::Message message(msg::Request | msg::Add | msg::Overlay, vwrMessage);
  node->sendBlocked(message);
}

void MeasureLinear::setupDispatcher()
{
  sift->insert
  (
    msg::Response | msg::Post | msg::Selection | msg::Add
    , std::bind(&MeasureLinear::selectionAdditionDispatched, this, std::placeholders::_1)
  );
}

void MeasureLinear::selectionAdditionDispatched(const msg::Message&)
{
  if (!isActive)
    return;
  
  go();
}

void MeasureLinear::selectionMaskDispatched(const msg::Message &messageIn)
{
  if (!isActive)
    return;
  
  slc::Message sMsg = messageIn.getSLC();
  selectionMask = sMsg.selectionMask;
}
