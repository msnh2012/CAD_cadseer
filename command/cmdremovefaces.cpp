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

#include <TopoDS.hxx>

#include "tools/featuretools.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "annex/annseershape.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrremovefaces.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvremovefaces.h"
#include "command/cmdremovefaces.h"

using boost::uuids::uuid;

using namespace cmd;

RemoveFaces::RemoveFaces()
: Base()
, leafManager()
{
  auto nf = std::make_shared<ftr::RemoveFaces>();
  project->addFeature(nf);
  feature = nf.get();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  isEdit = false;
  isFirstRun = true;
}

RemoveFaces::RemoveFaces(ftr::Base *fIn)
: Base("cmd::RemoveFaces")
, leafManager(fIn)
{
  feature = dynamic_cast<ftr::RemoveFaces*>(fIn);
  assert(feature);
  viewBase = std::make_unique<cmv::RemoveFaces>(this);
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  isEdit = true;
  isFirstRun = false; //bypass go() in activate.
}

RemoveFaces::~RemoveFaces() = default;

std::string RemoveFaces::getStatusMessage()
{
  return QObject::tr("Select faces to remove").toStdString();
}

void RemoveFaces::activate()
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

void RemoveFaces::deactivate()
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

void RemoveFaces::setSelections(const std::vector<slc::Message> &targets)
{
  assert(isActive);
  project->clearAllInputs(feature->getId());
  feature->setPicks(ftr::Picks());
  
  if (targets.empty())
    return;
  
  auto validFeature = [&](const ftr::Base *fIn)
  {
    if (fIn->hasAnnex(ann::Type::SeerShape) && !fIn->getAnnex<ann::SeerShape>().isNull())
      return true;
    return false;
  };
  
  const ftr::Base *tf = project->findFeature(targets.front().featureId);
  if (!validFeature(tf))
    return;
  
  ftr::Picks freshPicks;
  for (const auto &m : targets)
  {
    if (m.featureId != tf->getId()) //ensure offset is working on only one input.
      continue;
    const ftr::Base *lf = project->findFeature(m.featureId);
    if (!validFeature(lf))
      continue;
    freshPicks.push_back(tls::convertToPick(m, *lf, project->getShapeHistory()));
    freshPicks.back().tag = ftr::InputType::createIndexedTag(ftr::InputType::target, freshPicks.size() - 1);
    project->connectInsert(tf->getId(), feature->getId(), {freshPicks.back().tag});
  }
  feature->setPicks(freshPicks);
  feature->setColor(tf->getColor());
}

void RemoveFaces::localUpdate()
{
  assert(isActive);
  feature->updateModel(project->getPayload(feature->getId()));
  feature->updateVisual();
  feature->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
}

void RemoveFaces::go()
{
  const slc::Containers &containers = eventHandler->getSelections();
  
  std::vector<slc::Message> tm; //target messages
  for (const auto &c : containers)
  {
    if (c.selectionType == slc::Type::Face)
      tm.push_back(slc::EventHandler::containerToMessage(c));
  }
  
  if (!tm.empty())
  {
    setSelections(tm);
    node->sendBlocked(msg::buildStatusMessage("Remove Faces Added", 2.0));
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    node->sendBlocked(msg::buildHideThreeD(tm.front().featureId));
    node->sendBlocked(msg::buildHideOverlay(tm.front().featureId));
    return;
  }
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  node->sendBlocked(msg::buildHideThreeD(feature->getId()));
  node->sendBlocked(msg::buildHideOverlay(feature->getId()));
  viewBase = std::make_unique<cmv::RemoveFaces>(this);
}
