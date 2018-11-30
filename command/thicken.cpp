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
#include <boost/variant.hpp>

#include <project/project.h>
#include <message/node.h>
#include <selection/eventhandler.h>
#include <annex/seershape.h>
#include <parameter/parameter.h>
#include <feature/thicken.h>
#include <command/thicken.h>


using boost::uuids::uuid;

using namespace cmd;

Thicken::Thicken() : Base() {}
Thicken::~Thicken() {}

std::string Thicken::getStatusMessage()
{
  return QObject::tr("Select feature or geometry for thicken feature").toStdString();
}

void Thicken::activate()
{
  isActive = true;
  go();
  sendDone();
}

void Thicken::deactivate()
{
  isActive = false;
}

void Thicken::go()
{
  const slc::Containers &containers = eventHandler->getSelections();
  if (containers.empty())
  {
    node->sendBlocked(msg::buildStatusMessage("Wrong pre selection for thicken", 2.0));
    shouldUpdate = false;
    return;
  }

  ftr::Base *bf = project->findFeature(containers.front().featureId);
  if (!bf->hasAnnex(ann::Type::SeerShape))
  {
    node->sendBlocked(msg::buildStatusMessage("Wrong pre selection for thicken", 2.0));
    shouldUpdate = false;
    return;
  }
  
  std::shared_ptr<ftr::Thicken> thicken(new ftr::Thicken());
  project->addFeature(thicken);
  project->connectInsert(bf->getId(), thicken->getId(), ftr::InputType{ftr::InputType::target});
  
  node->sendBlocked(msg::buildHideThreeD(bf->getId()));
  node->sendBlocked(msg::buildHideOverlay(bf->getId()));
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
}
