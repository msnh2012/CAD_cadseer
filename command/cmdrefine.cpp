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

#include "project/prjproject.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrrefine.h"
#include "command/cmdrefine.h"

using namespace cmd;

Refine::Refine() : Base() {}
Refine::~Refine(){}

std::string Refine::getStatusMessage()
{
  return QObject::tr("Select feature for refine").toStdString();
}

void Refine::activate()
{
  isActive = true;
  go();
  sendDone();
}

void Refine::deactivate()
{
  isActive = false;
}

void Refine::go()
{
  bool created = false;
  const slc::Containers &containers = eventHandler->getSelections();
  for (const auto &c : containers)
  {
    if (c.selectionType != slc::Type::Object)
      continue;
    
    ftr::Base *bf = project->findFeature(c.featureId);
    if (!bf->hasAnnex(ann::Type::SeerShape))
      continue;
    
    auto *refine = new ftr::Refine();
    project->addFeature(std::unique_ptr<ftr::Refine>(refine));
    project->connectInsert(c.featureId, refine->getId(), ftr::InputType{ftr::InputType::target});
    created = true;
    
    node->sendBlocked(msg::buildHideThreeD(c.featureId));
    node->sendBlocked(msg::buildHideOverlay(c.featureId));
    
    node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
    
    break;
  }
  
  if (!created)
    shouldUpdate = false;
  else
    node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
}
