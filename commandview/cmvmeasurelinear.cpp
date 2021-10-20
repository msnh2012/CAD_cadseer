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
#include <QPlainTextEdit>
#include <QTextStream>

#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>

#include <osg/Vec3d>
#include <osg/Matrixd>
#include <osg/AutoTransform>

#include "globalutilities.h"
#include "tools/tlsstring.h"
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
#include "library/lbrlineardimension.h"
#include "command/cmdmeasurelinear.h"
#include "commandview/cmvmeasurelinear.h"

using namespace cmv;

struct MeasureLinear::Stow
{
  cmd::MeasureLinear *command;
  cmv::MeasureLinear *view;
  slc::Messages messages;
  QPlainTextEdit *textEdit;
  
  Stow(cmd::MeasureLinear *cIn, cmv::MeasureLinear *vIn)
  : command(cIn)
  , view(vIn)
  {
    buildGui();
    setupDispatcher();
  }
  
  void buildGui()
  {
    QVBoxLayout *mainLayout = new QVBoxLayout();
    view->setLayout(mainLayout);
    Base::clearContentMargins(view);
    view->setSizePolicy(view->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);
    
    textEdit = new QPlainTextEdit(view);
    textEdit->setReadOnly(true);
    textEdit->setWordWrapMode(QTextOption::NoWrap);
    textEdit->setPlainText(tr("Select 2 Objects to Measure"));
    mainLayout->addWidget(textEdit);
  }
  
  void slcAdded(const msg::Message &mIn)
  {
    if (view->isHidden())
      return;
    slc::add(messages, mIn.getSLC()); //add checks for duplicates
    
    if (messages.size() == 2)
      go();
    // go calls clear which should clear messages.
  }
  
  void slcRemoved(const msg::Message &mIn)
  {
    if (view->isHidden())
      return;
    slc::remove(messages, mIn.getSLC());
  }
  
  void setupDispatcher()
  {
    view->sift->insert
    (
      {
        std::make_pair
        (
          msg::Response | msg::Post | msg::Selection | msg::Add
          , std::bind(&Stow::slcAdded, this, std::placeholders::_1)
        )
        , std::make_pair
        (
          msg::Response | msg::Pre | msg::Selection | msg::Remove
          , std::bind(&Stow::slcRemoved, this, std::placeholders::_1)
        )
      }
    );
  }
  
  void go()
  {
    auto getShape = [&](const slc::Message &cIn) -> TopoDS_Shape
    {
      if (slc::isPointType(cIn.type))
      {
        TopoDS_Vertex v = BRepBuilderAPI_MakeVertex(gp_Pnt(gu::toOcc(cIn.pointLocation).XYZ()));
        return v;
      }
      
      ftr::Base *fb = view->project->findFeature(cIn.featureId);
      if (!fb->hasAnnex(ann::Type::SeerShape) || fb->getAnnex<ann::SeerShape>().isNull())
        return TopoDS_Shape();
      if (cIn.shapeId.is_nil())
        return fb->getAnnex<ann::SeerShape>().getRootOCCTShape();
      return fb->getAnnex<ann::SeerShape>().getOCCTShape(cIn.shapeId);
    };
    
    assert(messages.size() == 2);
    if (messages.size() != 2)
      return;
    TopoDS_Shape s0 = getShape(messages.front());
    TopoDS_Shape s1 = getShape(messages.back());
    if(s0.IsNull() || s1.IsNull())
      return;
    
    BRepExtrema_DistShapeShape ext(s0, s1, Extrema_ExtFlag_MIN);
    if (!ext.IsDone() || ext.NbSolution() < 1)
    {
      view->node->sendBlocked(msg::buildStatusMessage(tr("Extrema Failed").toStdString(), 2.0));
      return;
    }
    view->node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
    
    osg::Vec3d point0(gu::toOsg(ext.PointOnShape1(1)));
    osg::Vec3d point1(gu::toOsg(ext.PointOnShape2(1)));
    build(point0, point1);
  }
  
  void build(const osg::Vec3d &point1, const osg::Vec3d &point2)
  {
    double distance = (point2 - point1).length();
    vwr::Widget *viewer = app::instance()->getMainWindow()->getViewer();
    osg::Matrixd ics = osg::Matrixd::inverse(viewer->getCurrentSystem()); //inverted current system.
    osg::Vec3d csPoint1 = point1 * ics; //current system point1
    osg::Vec3d csPoint2 = point2 * ics; //current system point2
    
    QString infoMessage;
    QTextStream stream(&infoMessage);
    stream
    << Qt::endl << "Measure linear:"
    << Qt::endl << "    3d distance: "<< QString::fromStdString(tls::prettyDouble(distance))
    << Qt::endl << "    Absolute System:"
    << Qt::endl << "        Point1 location: " << QString::fromStdString(tls::valueString(point1))
    << Qt::endl << "        Point2 location: " << QString::fromStdString(tls::valueString(point2))
    << Qt::endl << "        delta X: " << QString::fromStdString(tls::prettyDouble(point2.x() - point1.x()))
    << Qt::endl << "        delta Y: " << QString::fromStdString(tls::prettyDouble(point2.y() - point1.y()))
    << Qt::endl << "        delta Z: " << QString::fromStdString(tls::prettyDouble(point2.z() - point1.z()))
    << Qt::endl << "    Current system:"
    << Qt::endl << "        Point1 location: " << QString::fromStdString(tls::valueString(csPoint1))
    << Qt::endl << "        Point2 location: " << QString::fromStdString(tls::valueString(csPoint2))
    << Qt::endl << "        delta X: " << QString::fromStdString(tls::prettyDouble(csPoint2.x() - csPoint1.x()))
    << Qt::endl << "        delta Y: " << QString::fromStdString(tls::prettyDouble(csPoint2.y() - csPoint1.y()))
    << Qt::endl << "        delta Z: " << QString::fromStdString(tls::prettyDouble(csPoint2.z() - csPoint1.z()))
    << Qt::endl;
    
    textEdit->setPlainText(infoMessage);
    
//     app::Message appMessage;
//     appMessage.infoMessage = infoMessage;
//     msg::Message viewInfoMessage(msg::Request | msg::Info | msg::Text, appMessage);
//     view->node->sendBlocked(viewInfoMessage);
    
    if (distance < std::numeric_limits<float>::epsilon())
      return;
    
    //get the view matrix for orientation.
    osg::Matrixd viewMatrix = viewer->getViewSystem();
    osg::Vec3d yVector = point2 - point1; yVector.normalize();
    osg::Vec3d zVectorView = gu::getZVector(viewMatrix); zVectorView.normalize();
    osg::Vec3d xVector = zVectorView ^ yVector;
    if (xVector.isNaN())
    {
      view->node->sendBlocked(msg::buildStatusMessage(
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
    view->node->sendBlocked(message);
  }
};

MeasureLinear::MeasureLinear(cmd::MeasureLinear *cIn)
: Base("cmv::MeasureLinear")
, stow(new Stow(cIn, this))
{
  maskDefault = slc::AllEnabled | slc::AllPointsEnabled | slc::EndPointsSelectable | slc::CenterPointsSelectable;
  goMaskDefault();
  goSelectionToolbar();
}

MeasureLinear::~MeasureLinear() = default;
