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

#include <TopoDS.hxx>

#include "tools/featuretools.h"
#include "application/appapplication.h"
#include "application/appmainwindow.h"
#include "viewer/vwrwidget.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "annex/annseershape.h"
#include "parameter/prmparameter.h"
#include "feature/ftrinputtype.h"
#include "feature/ftroffset.h"
#include "command/cmdoffset.h"

using boost::uuids::uuid;

using namespace cmd;

Offset::Offset() : Base() {}
Offset::~Offset() {}

std::string Offset::getStatusMessage()
{
  return QObject::tr("Select feature or geometry for offset feature").toStdString();
}

void Offset::activate()
{
  isActive = true;
  go();
  sendDone();
}

void Offset::deactivate()
{
  isActive = false;
}

void Offset::go()
{
  shouldUpdate = false;
  const slc::Containers &containers = eventHandler->getSelections();
  if (containers.empty())
  {
    node->sendBlocked(msg::buildStatusMessage("Empty pre selection for offset", 2.0));
    return;
  }
  
  ftr::Base *bf = project->findFeature(containers.front().featureId);
  if (!bf->hasAnnex(ann::Type::SeerShape) || bf->getAnnex<ann::SeerShape>().isNull())
  {
    node->sendBlocked(msg::buildStatusMessage("No seer shape, wrong pre selection for offset", 2.0));
    return;
  }
  const ann::SeerShape &ss = bf->getAnnex<ann::SeerShape>();
  
  ftr::Picks picks;
  tls::Connector connector;
  if (containers.size() == 1 && containers.front().isObjectType())
  {
    ftr::Base *bf = project->findFeature(containers.front().featureId);
    if (!bf->hasAnnex(ann::Type::SeerShape) || bf->getAnnex<ann::SeerShape>().isNull())
    {
      node->sendBlocked(msg::buildStatusMessage("Offset only works on shape features", 2.0));
      return;
    }
    
    ftr::Pick p = tls::convertToPick(containers.front(), bf->getAnnex<ann::SeerShape>(), project->getShapeHistory());
    p.tag = ftr::InputType::createIndexedTag(ftr::InputType::target, picks.size());
    picks.push_back(p);
    connector.add(bf->getId(), p.tag);
  }
  else
  {
    for (const auto &c : containers)
    {
      if (c.selectionType != slc::Type::Face)
        continue;
      //make sure all selections belong to the same feature.
      if (bf->getId() != c.featureId)
        continue;
      ftr::Pick p = tls::convertToPick(c, ss, project->getShapeHistory());
      p.tag = ftr::InputType::createIndexedTag(ftr::InputType::target, picks.size());
      picks.push_back(p);
      connector.add(bf->getId(), p.tag);
    }
  }
  
  if (picks.empty() || connector.pairs.empty())
  {
    node->sendBlocked(msg::buildStatusMessage("Incorrect preselection for offset", 2.0));
    return;
  }
    
  std::shared_ptr<ftr::Offset> offset(new ftr::Offset());
  offset->setPicks(picks);
  offset->setColor(bf->getColor());
  project->addFeature(offset);
  for (const auto &p : connector.pairs)
    project->connectInsert(p.first, offset->getId(), {p.second});
  shouldUpdate = true;
  
  node->sendBlocked(msg::buildHideThreeD(bf->getId()));
  node->sendBlocked(msg::buildHideOverlay(bf->getId()));
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
}
