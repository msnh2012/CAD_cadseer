/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#include "application/appmainwindow.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrstrip.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvstrip.h"
#include "command/cmdstrip.h"

using namespace cmd;
using boost::uuids::uuid;

Strip::Strip()
: Base("cmd::Strip")
, leafManager()
{
  auto nf = std::make_shared<ftr::Strip>();
  project->addFeature(nf);
  feature = nf.get();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  isEdit = false;
  isFirstRun = true;
}

Strip::Strip(ftr::Base *fIn)
: Base("cmd::Strip")
, leafManager(fIn)
{
  feature = dynamic_cast<ftr::Strip*>(fIn);
  assert(feature);
  viewBase = std::make_unique<cmv::Strip>(this);
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  isEdit = true;
  isFirstRun = false;
}

Strip::~Strip() = default;

std::string Strip::getStatusMessage()
{
  return QObject::tr("Select features for part and blank").toStdString();
}

void Strip::activate()
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

void Strip::deactivate()
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

void Strip::setSelections(const slc::Messages &parts, const slc::Messages &blanks, const slc::Messages &nests)
{
  project->clearAllInputs(feature->getId());
  if (!parts.empty())
    project->connect(parts.front().featureId, feature->getId(), {ftr::Strip::part});
  if (!blanks.empty())
    project->connect(blanks.front().featureId, feature->getId(), {ftr::Strip::blank});
  if (!nests.empty())
    project->connect(nests.front().featureId, feature->getId(), {ftr::Strip::nest});
}

void Strip::localUpdate()
{
  assert(isActive);
  feature->updateModel(project->getPayload(feature->getId()));
  feature->updateVisual();
  feature->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
}

void Strip::go()
{
  uuid partId = gu::createNilId();
  uuid blankId = gu::createNilId();
  uuid nestId = gu::createNilId();
  
  const slc::Containers &containers = eventHandler->getSelections();
  for (const auto &c : containers)
  {
    if (c.featureType == ftr::Type::Squash)
      blankId = c.featureId;
    else if (c.featureType == ftr::Type::Nest)
      nestId = c.featureId;
    else
      partId = c.featureId;
  }
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  //this scans the entire project to find a blank feature and a nest feature.
  if (blankId.is_nil() || nestId.is_nil())
  {
    auto ids = project->getAllFeatureIds();
    for (const auto &id : ids)
    {
      ftr::Base *bf = project->findFeature(id);
      if ((bf->getType() == ftr::Type::Squash) && (blankId.is_nil()))
        blankId = id;
      if ((bf->getType() == ftr::Type::Nest) && (nestId.is_nil()))
        nestId = id;
    }
  }
  
  //we will make connections to parents and let the view initialize.
  if (!partId.is_nil())
    project->connect(partId, feature->getId(), {ftr::Strip::part});
  if (!blankId.is_nil())
    project->connect(blankId, feature->getId(), {ftr::Strip::blank});
  if (!nestId.is_nil())
    project->connect(nestId, feature->getId(), {ftr::Strip::nest});
  
  //try to update with default parameters.
  localUpdate();
  
  viewBase = std::make_unique<cmv::Strip>(this);
}
