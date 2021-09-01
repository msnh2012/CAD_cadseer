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

#include <optional>

#include "application/appmainwindow.h"
#include "project/prjproject.h"
#include "viewer/vwrwidget.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "annex/annseershape.h"
#include "parameter/prmparameter.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvboolean.h"
#include "tools/featuretools.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrboolean.h"
#include "command/cmdboolean.h"

using namespace cmd;

Boolean::Boolean(int tIn) //new feature
: Base("cmd::Boolean")
, leafManager()
{
  feature = new ftr::Boolean::Feature();
  project->addFeature(std::unique_ptr<ftr::Boolean::Feature>(feature));
  assert(tIn >= 0 && tIn < 3);
  auto *typePrm = feature->getParameter(ftr::Boolean::PrmTags::booleanType);
  typePrm->setValue(tIn);

  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  isEdit = false;
  isFirstRun = true;
}

Boolean::Boolean(ftr::Base *fIn) //edit feature
: Base("cmd::Boolean")
, feature(static_cast<ftr::Boolean::Feature*>(fIn))
, leafManager(fIn)
{
  viewBase = std::make_unique<cmv::Boolean>(this);
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  isEdit = true;
  isFirstRun = false;
}

Boolean::~Boolean() = default;

std::string Boolean::getCommandName()
{
  std::string out = QObject::tr("Create Boolean").toStdString();
  if (isEdit)
    out = QObject::tr("Edit Boolean").toStdString();
  return out;
}

std::string Boolean::getStatusMessage()
{
  std::string message = std::string("Select geometry for ") + getCommandName() + std::string(" feature");
  return message;
}

void Boolean::activate()
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

void Boolean::deactivate()
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
    node->sendBlocked(msg::buildShowThreeD(feature->getId()));
    node->sendBlocked(msg::buildShowOverlay(feature->getId()));
  }
  isActive = false;
}

bool Boolean::isValidSelection(const slc::Message &mIn)
{
  if (mIn.type == slc::Type::Solid)
    return true;
  if (slc::isObjectType(mIn.type))
  {
    const ftr::Base *lf = project->findFeature(mIn.featureId);
    if (lf->hasAnnex(ann::Type::SeerShape) && !lf->getAnnex<ann::SeerShape>().isNull())
      return true;
  }
  
  return false;
}

void Boolean::setSelections(const slc::Messages &targets)
{
  assert(isActive);
  project->clearAllInputs(feature->getId());
  
  auto *picksPrm = feature->getParameter(ftr::Boolean::PrmTags::picks);
  ftr::Picks picks;
  for (const auto &m : targets)
  {
    const ftr::Base *lf = project->findFeature(m.featureId);
    picks.push_back(tls::convertToPick(m, *lf, project->getShapeHistory()));
    picks.back().tag = indexTag(ftr::Boolean::InputTags::pick, picks.size() - 1);
    project->connect(lf->getId(), feature->getId(), {picks.back().tag});
  }
  picksPrm->setValue(picks);
}

slc::Messages Boolean::getSelections()
{
  const ftr::Picks &targetPicks = feature->getParameter(ftr::Boolean::PrmTags::picks)->getPicks();
  
  auto payload = project->getPayload(feature->getId());
  tls::Resolver resolver(payload);
  slc::Messages targetMessages;
  for (const auto &p : targetPicks)
  {
    if (!resolver.resolve(p))
      continue;
    auto msgs = resolver.convertToMessages();
    targetMessages.insert(targetMessages.end(), msgs.begin(), msgs.end());
  }
  return targetMessages;
}

void Boolean::setType(int tIn)
{
  assert(tIn >= 0 && tIn < 3);
  auto *typePrm = feature->getParameter(ftr::Boolean::PrmTags::booleanType);
  typePrm->setValue(tIn);
}

int Boolean::getType()
{
  return feature->getParameter(ftr::Boolean::PrmTags::booleanType)->getInt();
}

void Boolean::localUpdate()
{
  assert(isActive);
  feature->updateModel(project->getPayload(feature->getId()));
  feature->updateVisual();
  feature->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
}

void Boolean::go()
{
  const slc::Containers &containers = eventHandler->getSelections();
  if (!containers.empty())
  {
    slc::Messages targets;
    for (const auto &c : containers)
    {
      auto m = slc::EventHandler::containerToMessage(c);
      if (!isValidSelection(m))
        continue;
      targets.push_back(m);
      node->sendBlocked(msg::buildHideThreeD(m.featureId));
      node->sendBlocked(msg::buildHideOverlay(m.featureId));
    }
    if (!targets.empty())
    {
      setSelections(targets);
      node->sendBlocked(msg::buildStatusMessage(feature->getTypeString() + " Added", 2.0));
      node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
      return;
    }
  }
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  node->sendBlocked(msg::buildShowThreeD(feature->getId()));
  node->sendBlocked(msg::buildShowOverlay(feature->getId()));
  node->sendBlocked(msg::buildStatusMessage("invalid pre selection", 2.0));
  viewBase = std::make_unique<cmv::Boolean>(this);
}
