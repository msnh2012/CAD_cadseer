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

#include "message/msgnode.h"
#include "project/prjproject.h"
#include "selection/slceventhandler.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrnest.h"
#include "command/cmdnest.h"

using namespace cmd;

using boost::uuids::uuid;

Nest::Nest() = default;
Nest::~Nest() = default;

std::string Nest::getStatusMessage()
{
  return QObject::tr("Select features for blank").toStdString();
}

void Nest::activate()
{
  isActive = true;
  go();
  sendDone();
}

void Nest::deactivate()
{
  isActive = false;
}

void Nest::go()
{
  //only works with preselection for now.
  const slc::Containers &containers = eventHandler->getSelections();
  if (containers.size() != 1)
  {
    node->send(msg::buildStatusMessage("Incorrect preselection for nest feature", 2.0));
    shouldUpdate = false;
    return;
  }
  uuid bId = containers.at(0).featureId;
  
  auto *nest = new ftr::Nest();
  project->addFeature(std::unique_ptr<ftr::Nest>(nest));
  project->connect(bId, nest->getId(), ftr::InputType{ftr::Nest::blank});
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
}
