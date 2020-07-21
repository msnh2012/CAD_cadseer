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
  //build multiples in go. so don't build a feature by default.
//   auto nf = std::make_shared<ftr::Draft>();
//   project->addFeature(nf);
//   feature = nf.get();
//   node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  isEdit = false;
  isFirstRun = true;
}

Draft::Draft(ftr::Base *fIn)
: Base("cmd::Draft")
, leafManager(fIn)
{
  feature = dynamic_cast<ftr::Draft*>(fIn);
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
    node->sendBlocked(msg::buildShowThreeD(feature->getId()));
    node->sendBlocked(msg::buildShowOverlay(feature->getId()));
  }
  isActive = false;
}

bool Draft::isValidSelection(const slc::Message &mIn)
{
  const ftr::Base *lf = project->findFeature(mIn.featureId);
  if (!lf->hasAnnex(ann::Type::SeerShape) || lf->getAnnex<ann::SeerShape>().isNull())
    return false;
  
  if (mIn.type != slc::Type::Face)
    return false;
  return true;
}

void Draft::setSelections(const slc::Messages &neutral, const slc::Messages &targets)
{
  assert(isActive);
  assert(feature);
  project->clearAllInputs(feature->getId());
  feature->setNeutralPick(ftr::Pick());
  feature->setTargetPicks(ftr::Picks());
  
  if (neutral.empty() || targets.empty())
    return;
  
  //currently draft feature only supports the neutral pick to be a planar face.
  //this will change as we will want datum planes and maybe more.
  //so don't validate consistent feature ids to neutral.
  
  const ftr::Base *nf = project->findFeature(neutral.front().featureId);
  ftr::Pick neutralPick = tls::convertToPick(neutral.front(), *nf, project->getShapeHistory());
  neutralPick.tag = ftr::Draft::neutral;
  feature->setNeutralPick(neutralPick);
  project->connect(nf->getId(), feature->getId(), {neutralPick.tag});
  
  ftr::Picks targetPicks;
  const ftr::Base *lf = project->findFeature(targets.front().featureId);
  for (const auto &m : targets)
  {
    if (!isValidSelection(m))
      continue;
    if (lf->getId() != m.featureId)
      continue;
    targetPicks.push_back(tls::convertToPick(m, *lf, project->getShapeHistory()));
    targetPicks.back().tag = ftr::InputType::createIndexedTag(ftr::InputType::target, targetPicks.size() - 1);
    project->connect(lf->getId(), feature->getId(), {targetPicks.back().tag});
  }
  feature->setTargetPicks(targetPicks);
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
  std::vector<slc::Containers> splits = slc::split(containers); //grouped by featureId.
  for (const auto &cs : splits)
  {
    assert(!cs.empty());
    
    boost::optional<slc::Message> neutral;
    slc::Messages targets;
    
    for (const auto c : cs)
    {
      auto m = slc::EventHandler::containerToMessage(c);
      if (!isValidSelection(m))
        continue;
      if (neutral)
        targets.push_back(m);
      else
        neutral = m;
    }
    if (neutral && !targets.empty())
    {
      auto nf = std::make_shared<ftr::Draft>();
      project->addFeature(nf);
      feature = nf.get();
      setSelections(slc::Messages(1, neutral.get()), targets);
      feature->setColor(project->findFeature(targets.front().featureId)->getColor());
      created = true;
      node->sendBlocked(msg::buildHideThreeD(targets.front().featureId));
      node->sendBlocked(msg::buildHideOverlay(targets.front().featureId));
      node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
      dlg::Parameter *pDialog = new dlg::Parameter(feature->getAngleParameter().get(), feature->getId());
      pDialog->show();
      pDialog->raise();
      pDialog->activateWindow();
    }
  }
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  if (!created)
  {
    auto nf = std::make_shared<ftr::Draft>();
    project->addFeature(nf);
    feature = nf.get();
    node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
    node->sendBlocked(msg::buildStatusMessage("invalid pre selection", 2.0));
    viewBase = std::make_unique<cmv::Draft>(this);
  }
}
