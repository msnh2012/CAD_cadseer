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
#include "commandview/cmvmessage.h"
#include "commandview/cmvboolean.h"
#include "tools/featuretools.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrintersect.h"
#include "feature/ftrsubtract.h"
#include "feature/ftrunion.h"
#include "command/cmdboolean.h"

namespace
{
  struct SetPickVisitor
  {
    const ftr::Picks &targets;
    const ftr::Picks &tools;
    SetPickVisitor(const ftr::Picks &targetsIn, ftr::Picks &toolsIn):targets(targetsIn),tools(toolsIn){}

    void operator()(ftr::Intersect *iIn)
    {
      iIn->setTargetPicks(targets);
      iIn->setToolPicks(tools);
    }
    
    void operator()(ftr::Subtract *sIn)
    {
      sIn->setTargetPicks(targets);
      sIn->setToolPicks(tools);
    }
    
    void operator()(ftr::Union *uIn)
    {
      uIn->setTargetPicks(targets);
      uIn->setToolPicks(tools);
    }
  };
  
  struct GetPickVisitor
  {
    ftr::Picks &targets;
    ftr::Picks &tools;
    GetPickVisitor(ftr::Picks &targetsIn, ftr::Picks &toolsIn):targets(targetsIn),tools(toolsIn){}

    void operator()(ftr::Intersect *iIn)
    {
      targets = iIn->getTargetPicks();
      tools = iIn->getToolPicks();
    }
    
    void operator()(ftr::Subtract *sIn)
    {
      targets = sIn->getTargetPicks();
      tools = sIn->getToolPicks();
    }
    
    void operator()(ftr::Union *uIn)
    {
      targets = uIn->getTargetPicks();
      tools = uIn->getToolPicks();
    }
  };
}

using namespace cmd;

Boolean::Boolean(ftr::Type tIn) //new feature
: Base("cmd::Boolean")
, leafManager()
{
  if (tIn == ftr::Type::Intersect)
  {
    auto nf = std::make_shared<ftr::Intersect>();
    project->addFeature(nf);
    feature = nf.get();
    basePtr = nf.get();
  }
  else if (tIn == ftr::Type::Subtract)
  {
    auto nf = std::make_shared<ftr::Subtract>();
    project->addFeature(nf);
    feature = nf.get();
    basePtr = nf.get();
  }
  else if (tIn == ftr::Type::Union)
  {
    auto nf = std::make_shared<ftr::Union>();
    project->addFeature(nf);
    feature = nf.get();
    basePtr = nf.get();
  }
  else
  {
    assert(0); //wrong type to cmd boolean constructor.
  }
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  isEdit = false;
  isFirstRun = true;
}

Boolean::Boolean(ftr::Base *fIn) //edit feature
: Base("cmd::Boolean")
, basePtr(fIn)
, leafManager(fIn)
{
  ftr::Type tIn = fIn->getType();
  if (tIn == ftr::Type::Intersect)
    feature = static_cast<ftr::Intersect*>(fIn);
  else if (tIn == ftr::Type::Subtract)
    feature = static_cast<ftr::Subtract*>(fIn);
  else if (tIn == ftr::Type::Union)
    feature = static_cast<ftr::Union*>(fIn);
  else
    assert(0); //wrong type to cmd boolean constructor.
  
  viewBase = std::make_unique<cmv::Boolean>(this);
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  isEdit = true;
  isFirstRun = false;
}

Boolean::~Boolean() = default;

std::string Boolean::getCommandName()
{
  std::string out = QObject::tr("Create ").toStdString();
  if (isEdit)
    out = QObject::tr("Edit ").toStdString();
  
  return out + basePtr->getTypeString();
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
    basePtr->setEditing();
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
    basePtr->setNotEditing();
    msg::Message out(msg::Mask(msg::Request | msg::Command | msg::View | msg::Hide));
    node->sendBlocked(out);
  }
  leafManager.fastForward();
  if (!isEdit.get())
  {
    node->sendBlocked(msg::buildShowThreeD(basePtr->getId()));
    node->sendBlocked(msg::buildShowOverlay(basePtr->getId()));
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

void Boolean::setSelections(const slc::Messages &targets, const slc::Messages &tools)
{
  assert(isActive);
  
  project->clearAllInputs(basePtr->getId());
  
  auto connect = [&](const slc::Messages &msgs, const std::string &tag) -> ftr::Picks
  {
    ftr::Picks picks;
    for (const auto &m : msgs)
    {
      const ftr::Base *lf = project->findFeature(m.featureId);
      picks.push_back(tls::convertToPick(m, *lf, project->getShapeHistory()));
      picks.back().tag = ftr::InputType::createIndexedTag(tag, picks.size() - 1);
      project->connect(lf->getId(), basePtr->getId(), {picks.back().tag});
    }
    return picks;
  };
  
  ftr::Picks targetPicks = connect(targets, ftr::InputType::target);
  ftr::Picks toolPicks = connect(tools, ftr::InputType::tool);
  std::visit(SetPickVisitor(targetPicks, toolPicks), feature);
}

std::tuple<slc::Messages, slc::Messages> Boolean::getSelections()
{
  ftr::Picks targetPicks;
  ftr::Picks toolPicks;
  std::visit(GetPickVisitor(targetPicks, toolPicks), feature);
  
  auto payload = project->getPayload(basePtr->getId());
  tls::Resolver resolver(payload);
  slc::Messages targetMessages;
  for (const auto &p : targetPicks)
  {
    if (!resolver.resolve(p))
      continue;
    auto msgs = resolver.convertToMessages();
    targetMessages.insert(targetMessages.end(), msgs.begin(), msgs.end());
  }
  
  slc::Messages toolMessages;
  for (const auto &p : toolPicks)
  {
    if (!resolver.resolve(p))
      continue;
    auto msgs = resolver.convertToMessages();
    toolMessages.insert(toolMessages.end(), msgs.begin(), msgs.end());
  }
  
  return std::make_tuple(targetMessages, toolMessages);
}

void Boolean::localUpdate()
{
  assert(isActive);
  basePtr->updateModel(project->getPayload(basePtr->getId()));
  basePtr->updateVisual();
  basePtr->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
}

void Boolean::go()
{
  const slc::Containers &containers = eventHandler->getSelections();
  if (containers.size() > 1)
  {
    slc::Messages targets; //pick first only supports 1 target
    slc::Messages tools;
    for (const auto &c : containers)
    {
      auto m = slc::EventHandler::containerToMessage(c);
      if (!isValidSelection(m))
        continue;
      if (targets.empty())
      {
        targets.push_back(m);
        continue;
      }
      tools.push_back(m);
    }
    if (!targets.empty() && !tools.empty())
    {
      setSelections(targets, tools);
      node->sendBlocked(msg::buildStatusMessage(basePtr->getTypeString() + " Added", 2.0));
      node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
      return;
    }
  }
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  node->sendBlocked(msg::buildShowThreeD(basePtr->getId()));
  node->sendBlocked(msg::buildShowOverlay(basePtr->getId()));
  node->sendBlocked(msg::buildStatusMessage("invalid pre selection", 2.0));
  viewBase = std::make_unique<cmv::Boolean>(this);
}
