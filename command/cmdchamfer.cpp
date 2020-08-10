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

#include <osg/Geometry> //yuck

#include "application/appmainwindow.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "parameter/prmparameter.h"
#include "dialogs/dlgparameter.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvchamfer.h"
#include "annex/annseershape.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrchamfer.h"
#include "tools/featuretools.h"
#include "command/cmdchamfer.h"

using boost::uuids::uuid;

using namespace cmd;

Chamfer::Chamfer()
: Base("cmd::Chamfer")
, leafManager()
{
  //this command is different. We can create multiple features.
  //so we don't build a feature by default.
//   auto nf = std::make_shared<ftr::Chamfer>();
//   project->addFeature(nf);
//   feature = nf.get();
//   node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  isEdit = false;
  isFirstRun = true;
}

Chamfer::Chamfer(ftr::Base *fIn)
: Base("cmd::Chamfer")
, leafManager(fIn)
{
  feature = dynamic_cast<ftr::Chamfer::Feature*>(fIn);
  assert(feature);
  viewBase = std::make_unique<cmv::Chamfer>(this);
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  isEdit = true;
  isFirstRun = false;
}

Chamfer::~Chamfer() = default;

std::string Chamfer::getStatusMessage()
{
  return QObject::tr("Select geometry for Chamfer feature").toStdString();
}

void Chamfer::activate()
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

void Chamfer::deactivate()
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

bool Chamfer::isValidSelection(const slc::Message &mIn)
{
  if (mIn.type != slc::Type::Edge)
    return false;
  const ftr::Base *lf = project->findFeature(mIn.featureId);
  if (!lf->hasAnnex(ann::Type::SeerShape) || lf->getAnnex<ann::SeerShape>().isNull())
    return false;
  return true;
}

void Chamfer::setMode(int modeIn)
{
  assert(isActive);
  project->clearAllInputs(feature->getId());
  ftr::Chamfer::Mode newMode = static_cast<ftr::Chamfer::Mode>(modeIn);
  feature->setMode(newMode); //this clears all entries.
}

void Chamfer::setSelectionData(const SelectionData &dataIn)
{
  assert(dataIn.size() == feature->getEntries().size());
  project->clearAllInputs(feature->getId());
  
  int index = -1;
  for (const auto &tup : dataIn)
  {
    ++index;
    ftr::Picks edgePicks;
    for (const auto &sm : std::get<0>(tup))
    {
      edgePicks.push_back(tls::convertToPick(sm, *project->findFeature(sm.featureId), project->getShapeHistory()));
      edgePicks.back().tag = ftr::InputType::createIndexedTag(ftr::Chamfer::edge, edgePicks.size() - 1);
      project->connect(sm.featureId, feature->getId(), {edgePicks.back().tag});
    }
    feature->setEdgePicks(index, edgePicks);
    if (feature->getEntries().at(index).style != ftr::Chamfer::Style::Symmetric)
    {
      ftr::Picks facePicks;
      for (const auto &sm : std::get<1>(tup))
      {
        facePicks.push_back(tls::convertToPick(sm, *project->findFeature(sm.featureId), project->getShapeHistory()));
        facePicks.back().tag = ftr::InputType::createIndexedTag(ftr::Chamfer::face, facePicks.size() - 1);
        project->connect(sm.featureId, feature->getId(), {facePicks.back().tag});
      }
      feature->setFacePicks(index, facePicks);
    }
  }
}

void Chamfer::localUpdate()
{
  assert(isActive);
  feature->updateModel(project->getPayload(feature->getId()));
  feature->updateVisual();
  feature->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
}

void Chamfer::go()
{
  bool created = false;
  const slc::Containers &containers = eventHandler->getSelections();
  std::vector<slc::Containers> splits = slc::split(containers); //grouped by featureId.
  for (const auto &cs : splits)
  {
    assert(!cs.empty());
    
    slc::Messages targets;
    for (const auto c : cs)
    {
      auto m = slc::EventHandler::containerToMessage(c);
      if (!isValidSelection(m))
        continue;
      targets.push_back(m);
    }
    if (!targets.empty())
    {
      auto nf = std::make_shared<ftr::Chamfer::Feature>();
      project->addFeature(nf);
      feature = nf.get();
      feature->setColor(project->findFeature(targets.front().featureId)->getColor());
      feature->setMode(ftr::Chamfer::Mode::Classic);
      feature->addSymmetric();
      setSelectionData({std::make_tuple(targets, slc::Messages())});
      created = true;
      node->sendBlocked(msg::buildHideThreeD(targets.front().featureId));
      node->sendBlocked(msg::buildHideOverlay(targets.front().featureId));
      node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
      dlg::Parameter *pDialog = new dlg::Parameter(feature->getParameters().front(), feature->getId());
      pDialog->show();
      pDialog->raise();
      pDialog->activateWindow();
    }
  }
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  if (!created)
  {
    auto nf = std::make_shared<ftr::Chamfer::Feature>();
    project->addFeature(nf);
    feature = nf.get();
    node->sendBlocked(msg::buildHideOverlay(feature->getId()));
    node->sendBlocked(msg::buildHideThreeD(feature->getId()));
    node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
    node->sendBlocked(msg::buildStatusMessage("invalid pre selection", 2.0));
    viewBase = std::make_unique<cmv::Chamfer>(this);
  }
}
