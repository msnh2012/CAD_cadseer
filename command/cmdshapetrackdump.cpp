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

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "command/cmdshapetrackdump.h"

using namespace cmd;

ShapeTrackDump::ShapeTrackDump() : Base() {}

ShapeTrackDump::~ShapeTrackDump() = default;

std::string ShapeTrackDump::getStatusMessage()
{
  return QObject::tr("Select geometry for shape track feature").toStdString();
}

void ShapeTrackDump::activate()
{
  isActive = true;
  go();
  sendDone();
}

void ShapeTrackDump::deactivate()
{
  isActive = false;
}

void ShapeTrackDump::go()
{
  const slc::Containers &containers = eventHandler->getSelections();
  assert(project);
  if (containers.empty())
    return;
  
  for (const auto &container : containers)
  {
    if (container.shapeId.is_nil())
      continue;
    project->shapeTrackDump(container.shapeId, app::instance()->getApplicationDirectory());
  }
  node->sendBlocked(msg::buildStatusMessage("ShapeTrackDump created", 2.0));
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
}
