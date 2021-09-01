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

#include <osg/Geometry> //yuck

#include "application/appapplication.h"
#include "application/appmainwindow.h"
#include "project/prjproject.h"
#include "selection/slceventhandler.h"
#include "annex/annseershape.h"
#include "message/msgnode.h"
#include "parameter/prmparameter.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrblend.h"
#include "tools/featuretools.h"
#include "dialogs/dlgparameter.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvblend.h"
#include "command/cmdblend.h"

using namespace cmd;

using boost::uuids::uuid;

Blend::Blend()
: Base("cmd::Blend")
, leafManager()
{
  //this command is different. We can create multiple features.
  //so we don't build a feature by default.
//   auto nf = std::make_shared<ftr::Blend>();
//   project->addFeature(nf);
//   feature = nf.get();
//   node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  isEdit = false;
  isFirstRun = true;
}

Blend::Blend(ftr::Base *fIn)
: Base("cmd::Blend")
, leafManager(fIn)
{
  feature = dynamic_cast<ftr::Blend::Feature*>(fIn);
  assert(feature);
  viewBase = std::make_unique<cmv::Blend>(this);
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  isEdit = true;
  isFirstRun = false;
}

Blend::~Blend() = default;

std::string Blend::getStatusMessage()
{
  return QObject::tr("Select edge to blend").toStdString();
}

void Blend::activate()
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

void Blend::deactivate()
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

void Blend::setType(int type)
{
  assert(type == 0 || type == 1);
  auto t = static_cast<ftr::Blend::BlendType>(type);
  feature->setBlendType(t);
}

bool Blend::isValidSelection(const slc::Message &mIn)
{
  if (mIn.type != slc::Type::Edge)
    return false;
  
  ftr::Base *f = project->findFeature(mIn.featureId);
  assert(f);
  if (!f->hasAnnex(ann::Type::SeerShape) || f->getAnnex<ann::SeerShape>().isNull())
    return false;
  
  return true;
}

void Blend::setConstantSelections(const std::vector<slc::Messages> &msgsIn)
{
  assert(isActive);
  assert(msgsIn.size() == feature->getConstantBlends().size());
  project->clearAllInputs(feature->getId());
  
  int index = -1;
  for (const auto &msgs : msgsIn)
  {
    index++;
    ftr::Picks currentPicks;
    for (const auto &m : msgs)
    {
      if (!isValidSelection(m))
        continue;
      ftr::Pick pick = tls::convertToPick(m, *project->findFeature(m.featureId), project->getShapeHistory());
      pick.tag = ftr::InputType::createIndexedTag(ftr::InputType::target, index);
      currentPicks.push_back(pick);
      project->connectInsert(m.featureId, feature->getId(), {currentPicks.back().tag});
    }
    feature->setConstantPicks(index, currentPicks);
  }
}

/*! Setting of project connections and feature picks.
 * 
 * @param msgsIn Tuple consisting of a vector of messages for edges
 * and a vector of ftr::VariableEntry. All should belong to same body.
 */
void Blend::setVariableSelections(const SelectionData &msgsIn)
{
  assert(isActive);
  project->clearAllInputs(feature->getId());
  feature->clearBlends();
  
  if (std::get<0>(msgsIn).empty() || std::get<1>(msgsIn).empty())
    return;
  
  //we know the edges passed in is not empty so get first edge and
  //assign input feature and test for validity.
  assert(project->hasFeature(std::get<0>(msgsIn).front().featureId));
  const auto &inputFeature = *project->findFeature(std::get<0>(msgsIn).front().featureId);
  assert(inputFeature.hasAnnex(ann::Type::SeerShape) && !inputFeature.getAnnex<ann::SeerShape>().isNull());
  const auto &ssIn = inputFeature.getAnnex<ann::SeerShape>();
  
  int index = -1;
  const auto &edges = std::get<0>(msgsIn);
  for (const auto &edge : edges)
  {
    if (inputFeature.getId() != edge.featureId)
    {
      node->sendBlocked(msg::buildStatusMessage(QObject::tr("Filtering selected object on separate feature").toStdString(), 2.0));
      continue;
    }
    index++;
    ftr::Pick pick = tls::convertToPick(edge, ssIn, project->getShapeHistory());
    pick.tag = ftr::InputType::createIndexedTag(ftr::InputType::target, index);
    feature->addOrCreateVariableBlend(pick);
    project->connect(inputFeature.getId(), feature->getId(), {pick.tag});
  }
  
  
  for (auto entry : std::get<1>(msgsIn))
  {
    index++;
    entry.pick.tag = ftr::InputType::createIndexedTag(ftr::InputType::target, index);
    feature->addVariableEntry(entry);
    project->connect(inputFeature.getId(), feature->getId(), {entry.pick.tag});
  }
}

void Blend::localUpdate()
{
  assert(isActive);
  feature->updateModel(project->getPayload(feature->getId()));
  feature->updateVisual();
  feature->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
}

void Blend::go()
{
  assert(project);
  
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
      feature = new ftr::Blend::Feature();
      project->addFeature(std::unique_ptr<ftr::Blend::Feature>(feature));
      feature->setColor(project->findFeature(targets.front().featureId)->getColor());
      ftr::Blend::Constant simpleBlend;
      simpleBlend.radius = ftr::Blend::buildRadiusParameter();
      feature->addConstantBlend(simpleBlend);
      setConstantSelections({targets});
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
    feature = new ftr::Blend::Feature();
    project->addFeature(std::unique_ptr<ftr::Blend::Feature>(feature));
    node->sendBlocked(msg::buildHideOverlay(feature->getId()));
    node->sendBlocked(msg::buildHideThreeD(feature->getId()));
    node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
    node->sendBlocked(msg::buildStatusMessage("invalid pre selection", 2.0));
    viewBase = std::make_unique<cmv::Blend>(this);
  }
}
