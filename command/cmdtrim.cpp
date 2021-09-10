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
#include "feature/ftrpick.h"
#include "tools/featuretools.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvtrim.h"
#include "command/cmdtrim.h"

using boost::uuids::uuid;

using namespace cmd;

Trim::Trim()
: Base()
, leafManager()
{
  feature = new ftr::Trim::Feature();
  project->addFeature(std::unique_ptr<ftr::Trim::Feature>(feature));
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  isEdit = false;
  isFirstRun = true;
}

Trim::Trim(ftr::Base *fIn)
: Base()
, leafManager(fIn)
{
  feature = dynamic_cast<ftr::Trim::Feature*>(fIn);
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

void Trim::setSelections(const slc::Messages &target, const slc::Messages &tool)
{
  assert(isActive);
  project->clearAllInputs(feature->getId());
  auto *targetPrm = feature->getParameter(ftr::Trim::PrmTags::targetPicks);
  targetPrm->setValue(ftr::Picks());
  auto *toolPrm = feature->getParameter(ftr::Trim::PrmTags::toolPicks);
  toolPrm->setValue(ftr::Picks());
  
  if (!target.empty())
  {
    auto targetPick = tls::convertToPick(target.front(), *project->findFeature(target.front().featureId), project->getShapeHistory());
    targetPick.tag = indexTag(ftr::Trim::PrmTags::targetPicks, 0);
    targetPrm->setValue(targetPick);
    project->connectInsert(target.front().featureId, feature->getId(), {targetPick.tag});
    feature->setColor(project->findFeature(target.front().featureId)->getColor());
  }
  
  if (!tool.empty())
  {
    auto toolPick = tls::convertToPick(tool.front(), *project->findFeature(tool.front().featureId), project->getShapeHistory());
    toolPick.tag = indexTag(ftr::Trim::PrmTags::toolPicks, 0);
    toolPrm->setValue(toolPick);
    //adding 'sever' to tool to break the 'alter' chain
    project->connect(tool.front().featureId, feature->getId(), {toolPick.tag, ftr::InputType::sever});
  }
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
  const slc::Containers &containers = eventHandler->getSelections();
  if (containers.size() == 2)
  {
    slc::Messages targets;
    if
    (
      containers.front().selectionType == slc::Type::Object
      || containers.front().selectionType == slc::Type::Solid
      || containers.front().selectionType == slc::Type::Shell
      || containers.front().selectionType == slc::Type::Face
    )
      targets.push_back(slc::EventHandler::containerToMessage(containers.front()));
      
    slc::Messages tools;
    if
    (
      containers.back().selectionType == slc::Type::Object
      || containers.back().selectionType == slc::Type::Shell
      || containers.back().selectionType == slc::Type::Face
    )
      tools.push_back(slc::EventHandler::containerToMessage(containers.back()));
    
    if (!targets.empty() && !tools.empty())
    {
      setSelections(targets, tools);
      node->sendBlocked(msg::buildHideThreeD(targets.front().featureId));
      node->sendBlocked(msg::buildHideOverlay(targets.front().featureId));
      node->sendBlocked(msg::buildStatusMessage("Trim added", 2.0));
      node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
      return;
    }
  }
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  viewBase = std::make_unique<cmv::Trim>(this);
}
