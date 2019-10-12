/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2019 Thomas S. Anderson blobfish.at.gmx.com
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

// #include "tools/featuretools.h"
// #include "application/appmainwindow.h"
// #include "application/appapplication.h"
#include "project/prjproject.h"
#include "viewer/vwrwidget.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
// #include "parameter/prmparameter.h"
// #include "dialogs/dlgparameter.h"
// #include "dialogs/dlginfo.h"
// #include "annex/annseershape.h"
// #include "feature/ftrinputtype.h"
#include "application/appmessage.h"
#include "annex/annseershape.h"
#include "feature/ftrbase.h"
#include "command/cmdinfo.h"

using namespace cmd;

Info::Info() : Base() {}

Info::~Info() = default;

std::string Info::getStatusMessage()
{
  return QObject::tr("Select geometry for Info feature").toStdString();
}

void Info::activate()
{
  isActive = true;
  go();
  sendDone();
}

void Info::deactivate()
{
  isActive = false;
}

void Info::go()
{
  QString infoMessage;
  QTextStream stream(&infoMessage);
  stream.setRealNumberNotation(QTextStream::FixedNotation);
  forcepoint(stream) << qSetRealNumberPrecision(12);
  stream << endl;
  
  osg::Matrixd ics = osg::Matrixd::inverse(viewer->getCurrentSystem()); //inverted current system.
  
  auto streamPoint = [&](const osg::Vec3d &p)
  {
    stream
      << "Absolute Point location: "
      << "["
      << p.x() << ", "
      << p.y() << ", "
      << p.z() << "]"
      << endl;
      
    osg::Vec3d tp = p * ics;
    stream
      << "Current System Point location: "
      << "["
      << tp.x() << ", "
      << tp.y() << ", "
      << tp.z() << "]"
      << endl;
  };
  
  //maybe if selection is empty we will dump out application information.
  //or better yet project information or even better yet both. careful
  //we might want to turn the window on and off keeping information as a 
  //reference and we don't to fill the window with shit. we will be alright, 
  //we will have a different facility for show and hiding the info window
  //other than this command.
  assert(project);
  const slc::Containers &containers = eventHandler->getSelections();
  if (containers.empty())
  {
    //nothing selected so get app and project information.
    //TODO get git hash for application version.
    project->getInfo(stream);
    viewer->getInfo(stream);
  }
  else
  {
    for (const auto &container : containers)
    {
      ftr::Base *feature = project->findFeature(container.featureId);
      stream <<  endl;
      if
      (
        (container.selectionType == slc::Type::Object) ||
        (container.selectionType == slc::Type::Feature)
      )
      {
        feature->getInfo(stream);
      }
      else if
      (
        (container.selectionType == slc::Type::Solid) ||
        (container.selectionType == slc::Type::Shell) ||
        (container.selectionType == slc::Type::Face) ||
        (container.selectionType == slc::Type::Wire) ||
        (container.selectionType == slc::Type::Edge)
      )
      {
        feature->getShapeInfo(stream, container.shapeId);
      }
      else if (container.selectionType == slc::Type::StartPoint)
      {
        const ann::SeerShape &s = feature->getAnnex<ann::SeerShape>();
        feature->getShapeInfo(stream, s.useGetStartVertex(container.shapeId));
        streamPoint(container.pointLocation);
      }
      else if (container.selectionType == slc::Type::EndPoint)
      {
        const ann::SeerShape &s = feature->getAnnex<ann::SeerShape>();
        feature->getShapeInfo(stream, s.useGetEndVertex(container.shapeId));
        streamPoint(container.pointLocation);
      }
      else //all other points.
      {
        streamPoint(container.pointLocation);
      }
    }
  }
  
  app::Message appMessage;
  appMessage.infoMessage = infoMessage;
  msg::Message viewInfoMessage(msg::Request | msg::Info | msg::Text, appMessage);
  node->send(viewInfoMessage);
}
