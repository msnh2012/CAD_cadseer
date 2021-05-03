/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2018  Thomas S. Anderson blobfish.at.gmx.com
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

#include <memory>

#include "application/appapplication.h"
#include "application/appmainwindow.h"
#include "viewer/vwrwidget.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrinstancelinear.h"
#include "tools/featuretools.h"
#include "command/cmdinstancelinear.h"

using namespace cmd;

InstanceLinear::InstanceLinear() : Base() {}
InstanceLinear::~InstanceLinear() {}

std::string InstanceLinear::getStatusMessage()
{
  return QObject::tr("Select feature or geometry for linear instance").toStdString();
}

void InstanceLinear::activate()
{
  isActive = true;
  go();
  sendDone();
}

void InstanceLinear::deactivate()
{
  isActive = false;
}

void InstanceLinear::go()
{
  shouldUpdate = false;
  
  const slc::Containers &containers = eventHandler->getSelections();
  if (containers.size() != 1)
  {
    node->sendBlocked(msg::buildStatusMessage("Wrong Preselection", 2.0));
    return;
  }
  ftr::Base *bf = project->findFeature(containers.front().featureId);
  if (!bf->hasAnnex(ann::Type::SeerShape))
  {
    node->sendBlocked(msg::buildStatusMessage("Wrong Preselection", 2.0));
    return;
  }
  
  auto instance = std::make_shared<ftr::InstanceLinear::Feature>();
  
  ftr::Pick pick = tls::convertToPick(containers.front(), *bf, project->getShapeHistory());
  pick.tag = indexTag(ftr::InstanceLinear::Tags::Source, 0);
  instance->getParameter(ftr::InstanceLinear::Tags::Source)->setValue(pick);
  instance->getParameter(prm::Tags::CSys)->setValue(viewer->getCurrentSystem());
  
  project->addFeature(instance);
  project->connectInsert(containers.front().featureId, instance->getId(), {pick.tag});
  
  node->sendBlocked(msg::buildHideThreeD(containers.front().featureId));
  node->sendBlocked(msg::buildHideOverlay(containers.front().featureId));
  
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  
  shouldUpdate = true;
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
}
