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

#include <osg/Geometry>

#include "application/appmainwindow.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "project/prjmessage.h"
#include "selection/slceventhandler.h"
#include "commandview/cmvmessage.h"
#include "command/cmdremove.h"

using namespace cmd;

Remove::Remove() : Base() {isFirstRun = true;}

Remove::~Remove() = default;

std::string Remove::getStatusMessage()
{
  return QObject::tr("Select features for removal").toStdString();
}

void Remove::activate()
{
  isActive = true;
  go();
  sendDone();
}

void Remove::deactivate()
{
  isActive = false;
}

void Remove::go()
{
  const slc::Containers &cs = eventHandler->getSelections();
  slc::Containers rs;
  for (const auto &s : cs)
  {
    if (slc::isObjectType(s.selectionType))
      rs.push_back(s);
  }
  if (rs.empty())
  {
    node->sendBlocked(msg::buildStatusMessage("invalid pre selection", 2.0));
    shouldUpdate = false;
    return;
  }
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  for (const auto &s : rs)
  {
    if (!project->hasFeature(s.featureId))
      continue;
    prj::Message payload;
    payload.featureIds.push_back(s.featureId);
    msg::Message removeMessage(msg::Request | msg::Remove | msg::Feature, payload);
    node->sendBlocked(removeMessage);
  }
  
  node->sendBlocked(msg::buildStatusMessage("Features Removed", 2.0));
}
