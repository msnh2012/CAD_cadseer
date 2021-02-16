/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2020 Thomas S. Anderson blobfish.at.gmx.com
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

#include <QVBoxLayout>
#include <QTextStream>

#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>

#include <osg/Vec3d>
#include <osg/Matrixd>
#include <osg/AutoTransform>

#include "globalutilities.h"
#include "application/appapplication.h"
#include "application/appmainwindow.h"
#include "application/appmessage.h"
#include "viewer/vwrwidget.h"
#include "viewer/vwrmessage.h"
#include "project/prjproject.h"
#include "feature/ftrbase.h"
#include "annex/annseershape.h"
#include "message/msgnode.h"
#include "message/msgsift.h"
#include "selection/slcmessage.h"
#include "dialogs/dlgselectionwidget.h"
#include "library/lbrlineardimension.h"
#include "commandview/cmvmeasurelinear.h"

using namespace cmv;

MeasureLinear::MeasureLinear()
: Base("cmv::MeasureLinear")
{
  sift->name = name.toStdString();
  buildGui();
}

MeasureLinear::~MeasureLinear() = default;

void MeasureLinear::buildGui()
{
  QVBoxLayout *mainLayout = new QVBoxLayout();
//   mainLayout->setContentsMargins(0, 0, 0, 0);
  
  std::vector<dlg::SelectionWidgetCue> cues;
  dlg::SelectionWidgetCue cue;
  cue.name = tr("Item 1");
  cue.singleSelection = true;
  cue.mask = slc::AllEnabled | slc::AllPointsEnabled | slc::PointsSelectable | slc::EndPointsSelectable | slc::CenterPointsSelectable;
  cue.statusPrompt = tr("Select First Item");
  cue.showAccrueColumn = false;
  cues.push_back(cue);
  cue.name = tr("Item 2");
  cue.statusPrompt = tr("Select Second Item");
  cues.push_back(cue);
  selectionWidget = new dlg::SelectionWidget(this, cues);
  connect(selectionWidget, &dlg::SelectionWidget::finishedSignal, this, &MeasureLinear::selectionDone);
  selectionWidget->activate(0);
  mainLayout->addWidget(selectionWidget);
  
  mainLayout->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
  this->setLayout(mainLayout);
}

void MeasureLinear::selectionDone()
{
  //build linear dimension.
  go();
  selectionWidget->reset();
}

void MeasureLinear::go()
{
  auto getShape = [&](const slc::Message &cIn) -> TopoDS_Shape
  {
    if (slc::isPointType(cIn.type))
    {
      TopoDS_Vertex v = BRepBuilderAPI_MakeVertex(gp_Pnt(gu::toOcc(cIn.pointLocation).XYZ()));
      return v;
    }
    
    ftr::Base *fb = project->findFeature(cIn.featureId);
    if (!fb->hasAnnex(ann::Type::SeerShape) || fb->getAnnex<ann::SeerShape>().isNull())
      return TopoDS_Shape();
    if (cIn.shapeId.is_nil())
      return fb->getAnnex<ann::SeerShape>().getRootOCCTShape();
    return fb->getAnnex<ann::SeerShape>().getOCCTShape(cIn.shapeId);
  };
  
  const slc::Messages &m0 = selectionWidget->getMessages(0);
  assert(m0.size() == 1);
  if (m0.size() != 1)
    return;
  const slc::Messages &m1 = selectionWidget->getMessages(1);
  assert(m1.size() == 1);
  if (m1.size() != 1)
    return;
  TopoDS_Shape s0 = getShape(m0.front());
  TopoDS_Shape s1 = getShape(m1.front());
  if(s0.IsNull() || s1.IsNull())
    return;
  
  BRepExtrema_DistShapeShape ext(s0, s1, Extrema_ExtFlag_MIN);
  if (!ext.IsDone() || ext.NbSolution() < 1)
  {
    node->sendBlocked(msg::buildStatusMessage(tr("Extrema Failed").toStdString()));
    return;
  }
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  osg::Vec3d point0(gu::toOsg(ext.PointOnShape1(1)));
  osg::Vec3d point1(gu::toOsg(ext.PointOnShape2(1)));
  build(point0, point1);
}

void MeasureLinear::build(const osg::Vec3d &point1, const osg::Vec3d &point2)
{
  double distance = (point2 - point1).length();
    
  QString infoMessage;
  QTextStream stream(&infoMessage);
  //if I don't set the realNumberNotation I get 10 decimal places. 
  //with it I get the specified 12. Why qt?
  stream.setRealNumberNotation(QTextStream::FixedNotation);
  Qt::forcepoint(stream) << qSetRealNumberPrecision(12);
  auto streamPoint = [&](const osg::Vec3d &p)
  {
    stream
      << "["
      << p.x() << ", "
      << p.y() << ", "
      << p.z() << "]";
  };
  
  stream << Qt::endl << "Measure linear: "<< Qt::endl
    << "    3d distance: "<< distance
    << Qt::endl << "    Absolute System:" << Qt::endl << "        Point1 location: ";
  streamPoint(point1);
  stream << Qt::endl << "        Point2 location: ";
  streamPoint(point2);
  
  stream << Qt::endl << "        delta X: " << point2.x() - point1.x()
    << Qt::endl << "        delta Y: " << point2.y() - point1.y()
    << Qt::endl << "        delta Z: " << point2.z() - point1.z();
  
  vwr::Widget *viewer = app::instance()->getMainWindow()->getViewer();
  //report points in current system.
  osg::Matrixd ics = osg::Matrixd::inverse(viewer->getCurrentSystem()); //inverted current system.
  osg::Vec3d csPoint1 = point1 * ics; //current system point1
  osg::Vec3d csPoint2 = point2 * ics; //current system point2
  stream << Qt::endl << "    Current system:" << Qt::endl << "        Point1 location: ";
  streamPoint(csPoint1);
  stream << Qt::endl << "        Point2 location: ";
  streamPoint(csPoint2);
  stream << Qt::endl << "        delta X: " << csPoint2.x() - csPoint1.x()
    << Qt::endl << "        delta Y: " << csPoint2.y() - csPoint1.y()
    << Qt::endl << "        delta Z: " << csPoint2.z() - csPoint1.z();
  stream << Qt::endl;
  
  app::Message appMessage;
  appMessage.infoMessage = infoMessage;
  msg::Message viewInfoMessage(msg::Request | msg::Info | msg::Text, appMessage);
  node->sendBlocked(viewInfoMessage);
  
  if (distance < std::numeric_limits<float>::epsilon())
    return;
  
  //get the view matrix for orientation.
  osg::Matrixd viewMatrix = viewer->getViewSystem();
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
