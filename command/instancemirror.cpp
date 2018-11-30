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
#include <boost/variant.hpp>

#include <project/project.h>
#include <message/node.h>
#include <selection/eventhandler.h>
#include <viewer/widget.h>
#include <parameter/parameter.h>
#include <feature/instancemirror.h>
#include <command/instancemirror.h>

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
  const slc::Containers &containers = eventHandler->getSelections();
  if (containers.empty() || (containers.size() > 2))
  {
    node->sendBlocked(msg::buildStatusMessage("wrong selection for instance mirror", 2.0));
    shouldUpdate = false;
    return;
  }
  
  ftr::Base *bf = project->findFeature(containers.front().featureId);
  if (!bf->hasAnnex(ann::Type::SeerShape))
  {
    node->sendBlocked(msg::buildStatusMessage("first selection should have shape for instance mirror", 2.0));
    shouldUpdate = false;
    return;
  }
  std::shared_ptr<ftr::InstanceMirror> instance(new ftr::InstanceMirror());
  
  ftr::Pick shapePick;
  shapePick.id = containers.front().shapeId;
  if (!shapePick.id.is_nil())
    shapePick.shapeHistory = project->getShapeHistory().createDevolveHistory(shapePick.id);
  instance->setShapePick(shapePick);
  
  project->addFeature(instance);
  project->connect(bf->getId(), instance->getId(), ftr::InputType{ftr::InputType::target});
  
  node->sendBlocked(msg::buildHideThreeD(bf->getId()));
  node->sendBlocked(msg::buildHideOverlay(bf->getId()));
  
  if (containers.size() == 1)
  {
    instance->setCSys(viewer->getCurrentSystem());
  }
  else //size == 2
  {
    boost::uuids::uuid fId = containers.back().featureId;
    ftr::Pick planePick;
    planePick.id = containers.back().shapeId;
    if (!planePick.id.is_nil())
      planePick.shapeHistory = project->getShapeHistory().createDevolveHistory(planePick.id);
    instance->setPlanePick(planePick);
    project->connect(fId, instance->getId(), ftr::InputType{ftr::InstanceMirror::mirrorPlane});
    
    //should we hide these?
    node->sendBlocked(msg::buildHideThreeD(fId));
    node->sendBlocked(msg::buildHideOverlay(fId));
  }
  
//   node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
}
