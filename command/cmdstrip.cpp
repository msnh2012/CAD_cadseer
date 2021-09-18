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
#include "parameter/prmparameter.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrstrip.h"
#include "feature/ftrpick.h"
#include "tools/featuretools.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvstrip.h"
#include "command/cmdstrip.h"

using namespace cmd;
using boost::uuids::uuid;

Strip::Strip()
: Base("cmd::Strip")
, leafManager()
{
  feature = new ftr::Strip::Feature();
  project->addFeature(std::unique_ptr<ftr::Strip::Feature>(feature));
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  isEdit = false;
  isFirstRun = true;
  
  ftr::Strip::Stations basic
  {
    ftr::Strip::Station::Blank
    , ftr::Strip::Station::Blank
    , ftr::Strip::Station::Blank
    , ftr::Strip::Station::Form
  };
  feature->setStations(basic);
}

Strip::Strip(ftr::Base *fIn)
: Base("cmd::Strip")
, leafManager(fIn)
{
  feature = dynamic_cast<ftr::Strip::Feature*>(fIn);
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
  auto connect = [&](const slc::Message &mIn, std::string_view tag)
  {
    const ftr::Base *lf = project->findFeature(mIn.featureId);
    ftr::Pick pick = tls::convertToPick(mIn, *lf, project->getShapeHistory());
    pick.tag = indexTag(tag, 0);
    feature->getParameter(tag)->setValue(pick);
    project->connect(lf->getId(), feature->getId(), {pick.tag});
  };
  project->clearAllInputs(feature->getId());
  if (!parts.empty())
    connect(parts.front(), ftr::Strip::PrmTags::part);
  if (!blanks.empty())
    connect(blanks.front(), ftr::Strip::PrmTags::blank);
  if (!nests.empty())
    connect(nests.front(), ftr::Strip::PrmTags::nest);
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
  slc::Messages parts;
  slc::Messages blanks;
  slc::Messages nests;
  
  const slc::Containers &containers = eventHandler->getSelections();
  for (const auto &c : containers)
  {
    if (c.featureType == ftr::Type::Squash)
      blanks.push_back(slc::EventHandler::containerToMessage(c));
    else if (c.featureType == ftr::Type::Nest)
      nests.push_back(slc::EventHandler::containerToMessage(c));
    else
      parts.push_back(slc::EventHandler::containerToMessage(c));
  }
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  //this scans the entire project to find a blank feature and a nest feature.
  auto ids = project->getAllFeatureIds();
  for (const auto &id : ids)
  {
    ftr::Base *bf = project->findFeature(id);
    slc::Message m;
    m.featureId = id;
    m.type = slc::Type::Object;
    m.featureType = bf->getType();
    if ((bf->getType() == ftr::Type::Squash) && (blanks.empty()))
      blanks.push_back(m);
    if ((bf->getType() == ftr::Type::Nest) && (nests.empty()))
      nests.push_back(m);
  }
  
  setSelections(parts, blanks, nests);
  localUpdate();
  
  viewBase = std::make_unique<cmv::Strip>(this);
}
