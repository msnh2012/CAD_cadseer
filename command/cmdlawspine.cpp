/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2022 Thomas S. Anderson blobfish.at.gmx.com
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
#include "feature/ftrpick.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrlawspine.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvlawspine.h"
#include "command/cmdlawspine.h"

using namespace cmd;

LawSpine::LawSpine()
: Base("cmd::LawSpine")
, leafManager()
{
  feature = new ftr::LawSpine::Feature();
  feature->initVessel();
  project->addFeature(std::unique_ptr<ftr::LawSpine::Feature>(feature));
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  isEdit = false;
  isFirstRun = true;
}

LawSpine::LawSpine(ftr::Base *fIn)
: Base("cmd::LawSpine")
, leafManager(fIn)
{
  feature = dynamic_cast<ftr::LawSpine::Feature*>(fIn);
  assert(feature);
  viewBase = std::make_unique<cmv::LawSpine>(this);
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  isEdit = true;
  isFirstRun = false;
}

LawSpine::~LawSpine() = default;

std::string LawSpine::getStatusMessage()
{
  return QObject::tr("Select Law Point").toStdString();
}

void LawSpine::activate()
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
    if (!isEdit.get())
      node->sendBlocked(msg::buildSelectionFreeze(feature->getId()));
  }
  else
    sendDone();
}

void LawSpine::deactivate()
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
    node->sendBlocked(msg::buildSelectionThaw(feature->getId()));
  }
  isActive = false;
}

bool LawSpine::isValidSelection(const slc::Message &mIn)
{
  const ftr::Base *lf = project->findFeature(mIn.featureId);
  if (!lf->hasAnnex(ann::Type::SeerShape) || lf->getAnnex<ann::SeerShape>().isNull())
    return false;
  
  if (mIn.type != slc::Type::Edge) return false;
  return true;
}

void LawSpine::setSelections(const slc::Messages &targets)
{
  assert(isActive);
  
  project->clearAllInputs(feature->getId());
  auto *parameter = feature->getParameter(ftr::LawSpine::Tags::spinePick);
  parameter->setValue(ftr::Picks());
  
  if (targets.empty())
    return;
  if (!isValidSelection(targets.front())) return;

  ftr::Picks freshPicks;
  const ftr::Base *lf = project->findFeature(targets.front().featureId);
  freshPicks.push_back(tls::convertToPick(targets.front(), *lf, project->getShapeHistory()));
  freshPicks.back().tag = indexTag(ftr::LawSpine::Tags::spinePick, 0);
  project->connect(lf->getId(), feature->getId(), {freshPicks.back().tag});
    
  parameter->setValue(freshPicks);
}

void LawSpine::localUpdate()
{
  assert(isActive);
  feature->updateModel(project->getPayload(feature->getId()));
  feature->updateVisual();
  feature->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
}

void LawSpine::go()
{
  const slc::Containers &cs = eventHandler->getSelections();
  
  std::vector<slc::Message> tm; //target messages
  for (const auto &c : cs)
  {
    auto m = slc::EventHandler::containerToMessage(c);
    if (isValidSelection(m)) tm.push_back(m);
  }
  
  if (!tm.empty())
  {
    setSelections(tm);
    node->sendBlocked(msg::buildStatusMessage("LawSpine Added", 2.0));
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return;
  }
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  node->sendBlocked(msg::buildShowThreeD(feature->getId()));
  node->sendBlocked(msg::buildHideOverlay(feature->getId()));
  viewBase = std::make_unique<cmv::LawSpine>(this);
}
