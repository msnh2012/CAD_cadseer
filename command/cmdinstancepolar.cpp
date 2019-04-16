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

#include <project/project.h>
#include <message/node.h>
#include <selection/eventhandler.h>
#include <viewer/vwrwidget.h>
#include <parameter/parameter.h>
#include <feature/inputtype.h>
#include <feature/instancepolar.h>
#include <command/instancepolar.h>

using namespace cmd;

InstancePolar::InstancePolar() : Base() {}
InstancePolar::~InstancePolar() {}

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
  const slc::Containers &containers = eventHandler->getSelections();
  if (containers.empty() || (containers.size() > 2))
  {
    node->sendBlocked(msg::buildStatusMessage("wrong selection for instance polar", 2.0));
    shouldUpdate = false;
    return;
  }
  
  ftr::Base *bf = project->findFeature(containers.front().featureId);
  if (!bf->hasAnnex(ann::Type::SeerShape))
  {
    node->sendBlocked(msg::buildStatusMessage("first selection should have shape for instance polar", 2.0));
    shouldUpdate = false;
    return;
  }
  
  std::shared_ptr<ftr::InstancePolar> instance(new ftr::InstancePolar());
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
    ftr::Pick axisPick;
    axisPick.id = containers.back().shapeId;
    if (!axisPick.id.is_nil())
      axisPick.shapeHistory = project->getShapeHistory().createDevolveHistory(axisPick.id);
    instance->setAxisPick(axisPick);
    project->connect(fId, instance->getId(), ftr::InputType{ftr::InstancePolar::rotationAxis});
    
    node->sendBlocked(msg::buildHideThreeD(fId));
    node->sendBlocked(msg::buildHideOverlay(fId));
  }
  
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
}
