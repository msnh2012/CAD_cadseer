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
  return QObject::tr("Select Geometry To Chamfer").toStdString();
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

void Chamfer::setSelectionData(const SelectionData &dataIn)
{
  assert(dataIn.size() == feature->getEntries().size());
  project->clearAllInputs(feature->getId());
  
  int index = -1;
  for (const auto &tup : dataIn)
  {
    ++index;
    auto &entry = feature->getEntry(index);
    
    ftr::Picks edgePicks;
    for (const auto &sm : std::get<0>(tup))
    {
      edgePicks.push_back(tls::convertToPick(sm, *project->findFeature(sm.featureId), project->getShapeHistory()));
      edgePicks.back().tag = indexTag(ftr::Chamfer::InputTags::edge, edgePicks.size() - 1);
      project->connect(sm.featureId, feature->getId(), {edgePicks.back().tag});
    }
    entry.edgePicks.setValue(edgePicks);
    if (entry.style.getInt() != 0) //not equal to symmetric
    {
      ftr::Picks facePicks;
      for (const auto &sm : std::get<1>(tup))
      {
        facePicks.push_back(tls::convertToPick(sm, *project->findFeature(sm.featureId), project->getShapeHistory()));
        facePicks.back().tag = indexTag(ftr::Chamfer::InputTags::face, facePicks.size() - 1);
        project->connect(sm.featureId, feature->getId(), {facePicks.back().tag});
      }
      entry.facePicks.setValue(facePicks);
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
    for (const auto &c : cs)
    {
      auto m = slc::EventHandler::containerToMessage(c);
      if (!isValidSelection(m))
        continue;
      targets.push_back(m);
    }
    if (!targets.empty())
    {
      feature = new ftr::Chamfer::Feature();
      project->addFeature(std::unique_ptr<ftr::Chamfer::Feature>(feature));
      feature->setColor(project->findFeature(targets.front().featureId)->getColor());
      auto &entry = feature->addSymmetric();
      setSelectionData({std::make_tuple(targets, slc::Messages())});
      created = true;
      node->sendBlocked(msg::buildHideThreeD(targets.front().featureId));
      node->sendBlocked(msg::buildHideOverlay(targets.front().featureId));
      node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
      dlg::Parameter *pDialog = new dlg::Parameter(entry.getParameters().at(3), feature->getId());
      pDialog->show();
      pDialog->raise();
      pDialog->activateWindow();
    }
  }
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  if (!created)
  {
    feature = new ftr::Chamfer::Feature();
    project->addFeature(std::unique_ptr<ftr::Chamfer::Feature>(feature));
    node->sendBlocked(msg::buildHideOverlay(feature->getId()));
    node->sendBlocked(msg::buildHideThreeD(feature->getId()));
    node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
    node->sendBlocked(msg::buildStatusMessage("invalid pre selection", 2.0));
    viewBase = std::make_unique<cmv::Chamfer>(this);
  }
}
