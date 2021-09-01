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

#include <boost/optional/optional.hpp>

#include <osg/Geometry> //yuck

#include "tools/featuretools.h"
#include "application/appmainwindow.h"
#include "application/appapplication.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "dialogs/dlgparameter.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvdraft.h"
#include "annex/annseershape.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrdraft.h"
#include "command/cmddraft.h"

using namespace cmd;

Draft::Draft()
: Base("cmd::Draft")
, leafManager()
{
  feature = new ftr::Draft::Feature();
  project->addFeature(std::unique_ptr<ftr::Draft::Feature>(feature));
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  isEdit = false;
  isFirstRun = true;
}

Draft::Draft(ftr::Base *fIn)
: Base("cmd::Draft")
, leafManager(fIn)
{
  feature = dynamic_cast<ftr::Draft::Feature*>(fIn);
  assert(feature);
  viewBase = std::make_unique<cmv::Draft>(this);
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  isEdit = true;
  isFirstRun = false;
}

Draft::~Draft() = default;

std::string Draft::getStatusMessage()
{
  return QObject::tr("Select geometry for Draft feature").toStdString();
}

void Draft::activate()
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

void Draft::deactivate()
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

bool Draft::isValidTarget(const slc::Message &mIn)
{
  const ftr::Base *lf = project->findFeature(mIn.featureId);
  if (!lf->hasAnnex(ann::Type::SeerShape) || lf->getAnnex<ann::SeerShape>().isNull())
    return false;
  
  if (mIn.type != slc::Type::Face) //could expand on drafts subset of face types.
    return false;
  return true;
}

bool Draft::isValidNeutral(const slc::Message &mIn)
{
  const ftr::Base *lf = project->findFeature(mIn.featureId);
  auto prms = lf->getParameters(prm::Tags::CSys);
  if (!prms.empty())
    return true;
  
  if (!lf->hasAnnex(ann::Type::SeerShape) || lf->getAnnex<ann::SeerShape>().isNull())
    return false;
  if (mIn.type != slc::Type::Face) //could expand on drafts subset of face types.
    return false;
  return true;
}

void Draft::setSelections(const slc::Messages &neutral, const slc::Messages &targets)
{
  assert(isActive);
  assert(feature);
  project->clearAllInputs(feature->getId());
  auto *targetsPrm = feature->getParameter(ftr::Draft::PrmTags::targetPicks);
  auto *neutralPrm = feature->getParameter(ftr::Draft::PrmTags::neutralPick);
  targetsPrm->setValue(ftr::Picks());
  neutralPrm->setValue(ftr::Picks());
  
  if (!neutral.empty())
  {
    if (isValidNeutral(neutral.front()))
    {
      //neutral plane can be outside of target body so don't validate consistent feature ids to neutral.
      const ftr::Base *nf = project->findFeature(neutral.front().featureId);
      ftr::Pick neutralPick = tls::convertToPick(neutral.front(), *nf, project->getShapeHistory());
      neutralPick.tag = indexTag(ftr::Draft::InputTags::neutralPick, 0);
      neutralPrm->setValue({neutralPick});
      if (targets.empty() || neutral.front().featureId != targets.front().featureId)
        project->connect(nf->getId(), feature->getId(), {neutralPick.tag, ftr::InputType::sever});
      else
        project->connect(nf->getId(), feature->getId(), {neutralPick.tag});
    }
    else
      node->sendBlocked(msg::buildStatusMessage("Invalid Neutral Plane", 2.0));
  }
  
  if (!targets.empty())
  {
    ftr::Picks targetPicks;
    const ftr::Base *lf = project->findFeature(targets.front().featureId);
    for (const auto &m : targets)
    {
      if (!isValidTarget(m))
      {
        node->sendBlocked(msg::buildStatusMessage("Skipping Invalid Target", 2.0));
        continue;
      }
      if (lf->getId() != m.featureId)
      {
        node->sendBlocked(msg::buildStatusMessage("Target Face Should Belong To Same Feature", 2.0));
        continue;
      }
      targetPicks.push_back(tls::convertToPick(m, *lf, project->getShapeHistory()));
      targetPicks.back().tag = indexTag(ftr::Draft::InputTags::targetPick, targetPicks.size() - 1);
      project->connect(lf->getId(), feature->getId(), {targetPicks.back().tag});
    }
    targetsPrm->setValue(targetPicks);
  }
}

void Draft::localUpdate()
{
  assert(isActive);
  feature->updateModel(project->getPayload(feature->getId()));
  feature->updateVisual();
  feature->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
}

void Draft::go()
{
  bool created = false;
  const slc::Containers &containers = eventHandler->getSelections();
  std::optional<slc::Message> neutral;
  slc::Messages targets;
  for (const auto &cs : containers)
  {
    auto m = slc::EventHandler::containerToMessage(cs);
    if (!neutral)
    {
      if (isValidNeutral(m))
        neutral = m;
    }
    else
    {
      if (isValidTarget(m))
        targets.push_back(m);
    }
  }
  if (neutral && !targets.empty())
  {
    setSelections(slc::Messages(1, *neutral), targets);
    feature->setColor(project->findFeature(targets.front().featureId)->getColor());
    created = true;
    node->sendBlocked(msg::buildHideThreeD(targets.front().featureId));
    node->sendBlocked(msg::buildHideOverlay(targets.front().featureId));
    node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
    dlg::Parameter *pDialog = new dlg::Parameter(feature->getParameter(prm::Tags::Angle), feature->getId());
    pDialog->show();
    pDialog->raise();
    pDialog->activateWindow();
  }
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  if (!created)
  {
    node->sendBlocked(msg::buildStatusMessage("invalid pre selection", 2.0));
    viewBase = std::make_unique<cmv::Draft>(this);
  }
}
