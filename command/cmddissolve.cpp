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

#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "project/prjmessage.h"
#include "annex/annseershape.h"
#include "command/cmddissolve.h"

using boost::uuids::uuid;
using namespace cmd;

Dissolve::Dissolve() : Base() {}
Dissolve::~Dissolve() {}

std::string Dissolve::getStatusMessage()
{
  return QObject::tr("Select feature to dissolve").toStdString();
}

void Dissolve::activate()
{
  isActive = true;
  go();
  sendDone();
}

void Dissolve::deactivate()
{
  isActive = false;
}

void Dissolve::go()
{
  assert(project);
  
  //only 1 object at a time. that way we don't have to worry about multiple objects
  //that belong to the same subgraph.
  const slc::Containers &cs = eventHandler->getSelections();
  if (cs.size() != 1 || cs.front().selectionType != slc::Type::Object)
  {
    shouldUpdate = false;
    node->send(msg::buildStatusMessage(QObject::tr("Dissolve needs 1 object selected").toStdString(), 2.0));
    return;
  }
  
  prj::Message pm;
  pm.featureIds.push_back(cs.front().featureId);
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  node->send(msg::Message(msg::Request | msg::Project | msg::Feature | msg::Dissolve, pm));
}
