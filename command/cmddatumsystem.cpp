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
  auto nf = std::make_shared<ftr::DatumSystem::Feature>();
  project->addFeature(nf);
  feature = nf.get();
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

bool DatumSystem::set3Points(const slc::Messages &msIn)
{
  assert(isActive);
  project->clearAllInputs(feature->getId());
  feature->setCue(ftr::DatumSystem::Cue()); //default to a constant system with no picks
  
  if
  (
    msIn.size() != 3
    || !slc::isPointType(msIn.at(0).type)
    || !slc::isPointType(msIn.at(1).type)
    || !slc::isPointType(msIn.at(2).type)
  )
  {
    node->sendBlocked(msg::buildStatusMessage("invalid points", 2.0));
    return false;
  }
  
  Cue cue;
  cue.systemType = Through3Points;
  auto process = [&](const slc::Message& mIn, const char *tag)
  {
    auto *featureIn = project->findFeature(mIn.featureId);
    assert(featureIn); //should already be verified.

    cue.picks.push_back(tls::convertToPick(mIn, featureIn->getAnnex<ann::SeerShape>(), project->getShapeHistory()));
    cue.picks.back().tag = tag;
    project->connect(mIn.featureId, feature->getId(), {cue.picks.back().tag});
  };
  
  for (const auto &m : msIn)
  {
    if (!isValidSelection(m))
    {
      node->sendBlocked(msg::buildStatusMessage("invalid selection", 2.0));
      return false;
    }
  }
  process(msIn.at(0), point0);
  process(msIn.at(1), point1);
  process(msIn.at(2), point2);
  feature->setCue(cue);
  
  return true;
}

void DatumSystem::setLinked(const uuid &idIn)
{
  assert(isActive);
  project->clearAllInputs(feature->getId());
  
  ftr::DatumSystem::Cue cue;
  if (idIn.is_nil())
  {
    cue.systemType = ftr::DatumSystem::SystemType::Constant;
  }
  else
  {
    cue.systemType = ftr::DatumSystem::SystemType::Linked;
    project->connect(idIn, feature->getId(), {ftr::InputType::linkCSys});
  }
  
  feature->setCue(cue);
}

bool DatumSystem::isValidSelection(const slc::Message &mIn)
{
  const ftr::Base *lf = project->findFeature(mIn.featureId);
  if (!lf->hasAnnex(ann::Type::SeerShape) || lf->getAnnex<ann::SeerShape>().isNull())
    return false;
  return true;
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
  {
    auto m = slc::EventHandler::containerToMessage(c);
    if (!isValidSelection(m))
      continue;
    targets.push_back(m);
  }
  
  node->sendBlocked(msg::buildShowThreeD(feature->getId()));
  node->sendBlocked(msg::buildShowOverlay(feature->getId()));
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  auto goConstantSystem = [&]()
  {
    //build constant system.
    feature->setCSys(viewer->getCurrentSystem());
    feature->setSize(viewer->getDiagonalLength() / 4.0);
    localUpdate();
    node->sendBlocked(msg::buildStatusMessage("invalid pre selection", 2.0));
    viewBase = std::make_unique<cmv::DatumSystem>(this);
  };
  
  if (targets.size() == 1)
  {
    if (slc::isObjectType(targets.front().type))
    {
      setLinked(targets.front().featureId);
    }
  }
  else if (targets.size() == 3)
  {
    if (set3Points(targets))
    {
      node->sendBlocked(msg::buildStatusMessage("System Through 3 Points Constructed", 2.0));
      feature->setAutoSize(true);
    }
    else
      goConstantSystem();
  }
  else
  {
    goConstantSystem();
  }
}
