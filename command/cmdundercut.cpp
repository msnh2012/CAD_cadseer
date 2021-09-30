/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2021 Thomas S. Anderson blobfish.at.gmx.com
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
// #include "application/appmainwindow.h"
// #include "application/appapplication.h"
// #include "viewer/vwrwidget.h"
#include "project/prjproject.h"
#include "annex/annseershape.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "parameter/prmparameter.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrundercut.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvundercut.h"
#include "command/cmdundercut.h"

using namespace cmd;

UnderCut::UnderCut()
: Base("cmd::UnderCut")
, leafManager()
{
  feature = new ftr::UnderCut::Feature();
  project->addFeature(std::unique_ptr<ftr::UnderCut::Feature>(feature));
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  isEdit = false;
  isFirstRun = true;
}

UnderCut::UnderCut(ftr::Base *fIn)
: Base("cmd::UnderCut")
, leafManager(fIn)
{
  feature = dynamic_cast<ftr::UnderCut::Feature*>(fIn);
  assert(feature);
  viewBase = std::make_unique<cmv::UnderCut>(this);
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  isEdit = true;
  isFirstRun = false;
}

UnderCut::~UnderCut() = default;

std::string UnderCut::getStatusMessage()
{
  return QObject::tr("Select geometry for undercut feature").toStdString();
}

void UnderCut::activate()
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

void UnderCut::deactivate()
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


void UnderCut::setSelections(const slc::Messages &sources, const slc::Messages &directions)
{
  assert(isActive);
  
  auto *sourcePicks = feature->getParameter(ftr::UnderCut::PrmTags::sourcePick);
  auto *directionPicks = feature->getParameter(ftr::UnderCut::PrmTags::directionPicks);
  
  project->clearAllInputs(feature->getId());
  sourcePicks->setValue(ftr::Picks());
  directionPicks->setValue(ftr::Picks());
  
  for (const auto &m : sources)
  {
    const ftr::Base *lf = project->findFeature(m.featureId);
    auto sp = tls::convertToPick(m, *lf, project->getShapeHistory());
    sp.tag = indexTag(ftr::UnderCut::PrmTags::sourcePick, 0);
    project->connect(lf->getId(), feature->getId(), {sp.tag});
    break; //should only be 1
  }

  ftr::Picks freshPicks;
  for (const auto &m : directions)
  {
    const ftr::Base *lf = project->findFeature(m.featureId);
    freshPicks.push_back(tls::convertToPick(m, *lf, project->getShapeHistory()));
    freshPicks.back().tag = indexTag(ftr::UnderCut::PrmTags::directionPicks, freshPicks.size() - 1);
    project->connect(lf->getId(), feature->getId(), {freshPicks.back().tag});
  }
  directionPicks->setValue(freshPicks);
}

void UnderCut::localUpdate()
{
  assert(isActive);
  feature->updateModel(project->getPayload(feature->getId()));
  feature->updateVisual();
  feature->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
}

void UnderCut::go()
{
  slc::Messages sources;
  slc::Messages directions;
  
  const slc::Containers &cs = eventHandler->getSelections();
  slc::Messages tm; //target messages
  for (const auto &c : cs)
  {
    auto m = slc::EventHandler::containerToMessage(c);
    if (slc::isObjectType(m.type) && sources.empty())
      sources.push_back(m);
    else
      directions.push_back(m);
  }
  
  if (!sources.empty() && !directions.empty())
  {
    setSelections(sources, directions);
    node->sendBlocked(msg::buildStatusMessage("UnderCut Added", 2.0));
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return;
  }
  
  node->sendBlocked(msg::buildStatusMessage("Invalid pre-selection", 2.0));
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  node->sendBlocked(msg::buildShowThreeD(feature->getId()));
  node->sendBlocked(msg::buildShowOverlay(feature->getId()));
  viewBase = std::make_unique<cmv::UnderCut>(this);
}
