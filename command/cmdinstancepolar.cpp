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

#include "project/prjproject.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "viewer/vwrwidget.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrinstancepolar.h"
#include "tools/featuretools.h"
#include "command/cmdinstancepolar.h"

using namespace cmd;

InstancePolar::InstancePolar() : Base() {}
InstancePolar::~InstancePolar() = default;

std::string InstancePolar::getStatusMessage()
{
  return QObject::tr("Select feature or geometry for polar instance").toStdString();
}

void InstancePolar::activate()
{
  isActive = true;
  go();
  sendDone();
}

void InstancePolar::deactivate()
{
  isActive = false;
}

void InstancePolar::go()
{
  shouldUpdate = false;
  const slc::Containers &containers = eventHandler->getSelections();
  if (containers.empty() || (containers.size() > 2))
  {
    node->sendBlocked(msg::buildStatusMessage("wrong selection for instance polar", 2.0));
    return;
  }
  
  ftr::Base *bf = project->findFeature(containers.front().featureId);
  if (!bf->hasAnnex(ann::Type::SeerShape))
  {
    node->sendBlocked(msg::buildStatusMessage("first selection should have shape for instance polar", 2.0));
    return;
  }
  
  auto *instance = new ftr::InstancePolar::Feature();
  ftr::Pick shapePick = tls::convertToPick(containers.front(), *bf, project->getShapeHistory());
  shapePick.tag = indexTag(ftr::InstancePolar::Tags::Source, 0);
  auto *sourcePrm = instance->getParameter(ftr::InstancePolar::Tags::Source);
  sourcePrm->setValue(shapePick);
  
  project->addFeature(std::unique_ptr<ftr::InstancePolar::Feature>(instance));
  project->connect(bf->getId(), instance->getId(), {shapePick.tag});
  
  node->sendBlocked(msg::buildHideThreeD(bf->getId()));
  node->sendBlocked(msg::buildHideOverlay(bf->getId()));
  
  if (containers.size() == 1)
  {
    instance->getParameter(ftr::InstancePolar::Tags::AxisType)->setValue(static_cast<int>(ftr::InstancePolar::Constant));
    instance->getParameter(prm::Tags::CSys)->setValue(viewer->getCurrentSystem());
  }
  else //size == 2
  {
    instance->getParameter(ftr::InstancePolar::Tags::AxisType)->setValue(static_cast<int>(ftr::InstancePolar::Pick));
    
    boost::uuids::uuid fId = containers.back().featureId;
    ftr::Pick axisPick = tls::convertToPick(containers.back(), *project->findFeature(fId), project->getShapeHistory());
    axisPick.tag = indexTag(ftr::InstancePolar::Tags::Axis, 0);
    instance->getParameter(ftr::InstancePolar::Tags::Axis)->setValue(axisPick);
    project->connect(fId, instance->getId(), {axisPick.tag});
  }
  shouldUpdate = true;
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
}
