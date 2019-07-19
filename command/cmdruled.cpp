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

#include "tools/featuretools.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "annex/annseershape.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrruled.h"
#include "command/cmdruled.h"

using namespace cmd;

Ruled::Ruled() : Base() {}

Ruled::~Ruled() {}

std::string Ruled::getStatusMessage()
{
  return QObject::tr("Select geometry for Ruled feature").toStdString();
}

void Ruled::activate()
{
  isActive = true;
  go();
  sendDone();
}

void Ruled::deactivate()
{
  isActive = false;
}

void Ruled::go()
{
  const slc::Containers &cs = eventHandler->getSelections();
  if (cs.size() != 2)
  {
    node->sendBlocked(msg::buildStatusMessage("Incorrect selection for Ruled", 2.0));
    shouldUpdate = false;
    return;
  }
  
  auto checkType = [](const slc::Container &cIn) -> bool
  {
    auto t = cIn.selectionType;
    if
    (
      (t == slc::Type::Object)
      || (t == slc::Type::Wire)
      || (t == slc::Type::Edge)
    )
      return true;
    return false;
  };
  if ((!checkType(cs.front())) || (!checkType(cs.back())))
  {
    node->sendBlocked(msg::buildStatusMessage("Wrong selection types for Ruled", 2.0));
    shouldUpdate = false;
    return;
  }
  
  const ftr::Base *bf0 = project->findFeature(cs.front().featureId);
  if (!bf0 || !bf0->hasAnnex(ann::Type::SeerShape))
  {
    node->sendBlocked(msg::buildStatusMessage("Invalid first selection for Ruled", 2.0));
    shouldUpdate = false;
    return;
  }
  const ann::SeerShape &ss0 = bf0->getAnnex<ann::SeerShape>();
  
  const ftr::Base *bf1 = project->findFeature(cs.back().featureId);
  if (!bf1 || !bf1->hasAnnex(ann::Type::SeerShape))
  {
    node->sendBlocked(msg::buildStatusMessage("Invalid first selection for Ruled", 2.0));
    shouldUpdate = false;
    return;
  }
  const ann::SeerShape &ss1 = bf1->getAnnex<ann::SeerShape>();
  
  ftr::Picks picks;
  picks.push_back(tls::convertToPick(cs.front(), ss0, project->getShapeHistory()));
  picks.back().tag = ftr::Ruled::pickZero;
  picks.push_back(tls::convertToPick(cs.back(), ss1, project->getShapeHistory()));
  picks.back().tag = ftr::Ruled::pickOne;
  
  auto f = std::make_shared<ftr::Ruled>();
  f->setPicks(picks);
  project->addFeature(f);
  project->connectInsert(cs.front().featureId, f->getId(), {picks.front().tag});
  project->connectInsert(cs.back().featureId, f->getId(), {picks.back().tag});
  
  node->sendBlocked(msg::buildStatusMessage("Ruled created", 2.0));
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
}
