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
#include "application/appmainwindow.h"
#include "project/prjproject.h"
#include "viewer/vwrwidget.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvdatumsystem.h"
#include "annex/annseershape.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrdatumsystem.h"
#include "command/cmddatumsystem.h"

using namespace cmd;
using namespace ftr::DatumSystem;
using boost::uuids::uuid;

DatumSystem::DatumSystem()
: Base("cmd::DatumSystem")
, leafManager()
{
  feature = new ftr::DatumSystem::Feature();
  auto nf = std::make_shared<ftr::DatumSystem::Feature>();
  project->addFeature(std::unique_ptr<ftr::DatumSystem::Feature>(feature));
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  isEdit = false;
  isFirstRun = true;
}

DatumSystem::DatumSystem(ftr::Base *fIn)
: Base("cmd::DatumSystem")
, leafManager(fIn)
{
  feature = dynamic_cast<ftr::DatumSystem::Feature*>(fIn);
  assert(feature);
  viewBase = std::make_unique<cmv::DatumSystem>(this);
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  isEdit = true;
  isFirstRun = false;
}

DatumSystem::~DatumSystem() = default;

std::string DatumSystem::getStatusMessage()
{
  return QObject::tr("Select geometry for DatumSystem feature").toStdString();
}

void DatumSystem::activate()
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

void DatumSystem::deactivate()
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

void DatumSystem::setConstant()
{
  assert(isActive);
  project->clearAllInputs(feature->getId());
  
  auto *param = feature->getParameter(ftr::DatumSystem::Tags::SystemType);
  param->setValue(static_cast<int>(ftr::DatumSystem::Constant));
}

void DatumSystem::setLinked(const slc::Messages &msIn)
{
  assert(isActive);
  project->clearAllInputs(feature->getId());
  
  auto *param = feature->getParameter(ftr::DatumSystem::Tags::SystemType);
  param->setValue(static_cast<int>(ftr::DatumSystem::Linked));
  
  if (msIn.empty())
    return;
  const ftr::Base &parent = *project->findFeature(msIn.front().featureId);
  auto pick = tls::convertToPick(msIn.front(), parent, project->getShapeHistory());
  pick.tag = indexTag(ftr::InputType::linkCSys, 0);
  feature->getParameter(ftr::DatumSystem::Tags::Linked)->setValue(pick);
  project->connectInsert(parent.getId(), feature->getId(), {pick.tag});
}

void DatumSystem::set3Points(const slc::Messages &msIn)
{
  assert(isActive);
  project->clearAllInputs(feature->getId());
  
  auto *param = feature->getParameter(ftr::DatumSystem::Tags::SystemType);
  param->setValue(static_cast<int>(ftr::DatumSystem::Through3Points));
  
  auto notPoint = [](const slc::Message &m){return !slc::isPointType(m.type);};
  if (msIn.size() != 3 || notPoint(msIn.at(0)) || notPoint(msIn.at(1)) || notPoint(msIn.at(2)))
    return;
  
  ftr::Picks picks;
  auto process = [&](const slc::Message& mIn, std::string_view sv)
  {
    const auto *featureIn = project->findFeature(mIn.featureId);
    picks.push_back(tls::convertToPick(mIn, *featureIn, project->getShapeHistory()));
    picks.back().tag = indexTag(sv, picks.size());
    project->connect(mIn.featureId, feature->getId(), {picks.back().tag});
  };

  process(msIn.at(0), ftr::DatumSystem::Tags::Points);
  process(msIn.at(1), ftr::DatumSystem::Tags::Points);
  process(msIn.at(2), ftr::DatumSystem::Tags::Points);
  auto *pp = feature->getParameter(ftr::DatumSystem::Tags::Points);
  pp->setValue(picks);
  
  return;
}

void DatumSystem::localUpdate()
{
  assert(isActive);
  feature->updateModel(project->getPayload(feature->getId()));
  feature->updateVisual();
  feature->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
}

void DatumSystem::go()
{
  assert(project);
  
  slc::Messages targets;
  for (const auto &c : eventHandler->getSelections())
    targets.push_back(eventHandler->containerToMessage(c));
  
  node->sendBlocked(msg::buildShowThreeD(feature->getId()));
  node->sendBlocked(msg::buildShowOverlay(feature->getId()));
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  auto goConstantSystem = [&]()
  {
    setConstant();
    feature->getParameter(prm::Tags::CSys)->setValue(viewer->getCurrentSystem());
    feature->getParameter(prm::Tags::Size)->setValue(viewer->getDiagonalLength() / 4.0);
    localUpdate();
    node->sendBlocked(msg::buildStatusMessage("invalid pre selection", 2.0));
    viewBase = std::make_unique<cmv::DatumSystem>(this);
  };
  
  if (targets.size() == 1)
  {
    if (slc::isObjectType(targets.front().type))
      setLinked(targets);
  }
  else if (targets.size() == 3)
  {
    bool allPoints = true;
    for (const auto &m : targets)
    {
      if (!slc::isPointType(m.type))
      {
        allPoints = false;
        break;
      }
    }
    if (allPoints)
    {
      set3Points(targets);
      node->sendBlocked(msg::buildStatusMessage("System Through 3 Points Constructed", 2.0));
      feature->getParameter(prm::Tags::AutoSize)->setValue(true);
    }
    else
      goConstantSystem();
  }
  else
  {
    goConstantSystem();
  }
}
