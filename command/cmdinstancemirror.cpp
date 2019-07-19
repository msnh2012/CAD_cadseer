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
#include "parameter/prmparameter.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrinstancemirror.h"
#include "tools/featuretools.h"
#include "command/cmdinstancemirror.h"

using namespace cmd;

InstanceMirror::InstanceMirror() : Base() {}
InstanceMirror::~InstanceMirror() {}

std::string InstanceMirror::getStatusMessage()
{
  return QObject::tr("Select feature or geometry for mirrored instance").toStdString();
}

void InstanceMirror::activate()
{
  isActive = true;
  go();
  sendDone();
}

void InstanceMirror::deactivate()
{
  isActive = false;
}

void InstanceMirror::go()
{
  shouldUpdate = false;
  const slc::Containers &containers = eventHandler->getSelections();
  if (containers.empty() || (containers.size() > 2))
  {
    node->sendBlocked(msg::buildStatusMessage("wrong selection for instance mirror", 2.0));
    return;
  }
  
  ftr::Base *bf = project->findFeature(containers.front().featureId);
  if (!bf->hasAnnex(ann::Type::SeerShape))
  {
    node->sendBlocked(msg::buildStatusMessage("first selection should have shape for instance mirror", 2.0));
    return;
  }
  std::shared_ptr<ftr::InstanceMirror> instance(new ftr::InstanceMirror());
  
  ftr::Pick shapePick = tls::convertToPick(containers.front(), *bf, project->getShapeHistory());
  shapePick.tag = ftr::InputType::target;
  instance->setShapePick(shapePick);
  
  project->addFeature(instance);
  project->connect(bf->getId(), instance->getId(), ftr::InputType{shapePick.tag});
  
  node->sendBlocked(msg::buildHideThreeD(bf->getId()));
  node->sendBlocked(msg::buildHideOverlay(bf->getId()));
  
  if (containers.size() == 1)
  {
    instance->setCSys(viewer->getCurrentSystem());
  }
  else //size == 2
  {
    boost::uuids::uuid fId = containers.back().featureId;
    assert(project->hasFeature(fId));
    ftr::Pick planePick = tls::convertToPick(containers.back(), *project->findFeature(fId), project->getShapeHistory());
    planePick.tag = ftr::InstanceMirror::mirrorPlane;
    instance->setPlanePick(planePick);
    project->connect(fId, instance->getId(), ftr::InputType{planePick.tag});
    
    //should we hide these?
    node->sendBlocked(msg::buildHideThreeD(fId));
    node->sendBlocked(msg::buildHideOverlay(fId));
  }
  shouldUpdate = true;
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
}
