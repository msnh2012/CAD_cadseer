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

#include <osg/Geometry>

#include "project/prjproject.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "annex/annseershape.h"
#include "parameter/prmparameter.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrtrim.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvtrim.h"
#include "command/cmdtrim.h"

using boost::uuids::uuid;

using namespace cmd;

Trim::Trim()
: Base()
, leafManager()
{
  feature = new ftr::Trim();
  project->addFeature(std::unique_ptr<ftr::Trim>(feature));
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  isEdit = false;
  isFirstRun = true;
}

Trim::Trim(ftr::Base *fIn)
: Base()
, leafManager(fIn)
{
  feature = dynamic_cast<ftr::Trim*>(fIn);
  assert(feature);
  viewBase = std::make_unique<cmv::Trim>(this);
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  isEdit = true;
  isFirstRun = false;
}

Trim::~Trim() = default;

std::string Trim::getStatusMessage()
{
  return QObject::tr("Select object for trim feature").toStdString();
}

void Trim::activate()
{
  isActive = true;
  leafManager.rewind();
  if (isFirstRun.get())
  {
    isFirstRun = false;
    go();
  }
  if (viewBase)
  {
    feature->setEditing();
    cmv::Message vm(viewBase.get(), viewBase->getPaneWidth());
    msg::Message out(msg::Mask(msg::Request | msg::Command | msg::View | msg::Show), vm);
    node->sendBlocked(out);
  }
  else
    sendDone();
}

void Trim::deactivate()
{
  if (viewBase)
  {
    feature->setNotEditing();
    msg::Message out(msg::Mask(msg::Request | msg::Command | msg::View | msg::Hide));
    node->sendBlocked(out);
  }
  leafManager.fastForward();
  if (!isEdit.get())
  {
    project->hideAlterParents(feature->getId());
    node->sendBlocked(msg::buildShowThreeD(feature->getId()));
    node->sendBlocked(msg::buildShowOverlay(feature->getId()));
  }
  isActive = false;
}

void Trim::setSelections(const std::vector<slc::Message> &target, const std::vector<slc::Message> &tool)
{
  assert(isActive);
  project->clearAllInputs(feature->getId());
  
  if (target.empty() || tool.empty())
    return;
  
  project->connectInsert(target.front().featureId, feature->getId(), {ftr::InputType::target});
  project->connect(tool.front().featureId, feature->getId(), {ftr::InputType::tool});
  project->connect(tool.front().featureId, feature->getId(), {ftr::InputType::sever}); //break alter chain
  
  node->sendBlocked(msg::buildHideThreeD(target.front().featureId));
  node->sendBlocked(msg::buildHideOverlay(target.front().featureId));
  ftr::Base *t = project->findFeature(target.front().featureId);
  feature->setColor(t->getColor());
}

void Trim::localUpdate()
{
  assert(isActive);
  feature->updateModel(project->getPayload(feature->getId()));
  feature->updateVisual();
  feature->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
}

void Trim::go()
{
  slc::Containers filtered;
  const slc::Containers &containers = eventHandler->getSelections();
  for (const auto &c : containers)
  {
    if (c.selectionType == slc::Type::Object)
      filtered.push_back(c);
  }
  if (filtered.size() == 2)
  {
    std::vector<slc::Message> targets(1, slc::EventHandler::containerToMessage(filtered.front()));
    std::vector<slc::Message> tools(1, slc::EventHandler::containerToMessage(filtered.back()));
    setSelections(targets, tools);
    node->sendBlocked(msg::buildStatusMessage("Trim added", 2.0));
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return;
  }
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  viewBase = std::make_unique<cmv::Trim>(this);
}
