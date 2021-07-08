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

#include <TopoDS_Face.hxx>
#include <TopoDS.hxx>

#include <osg/Geometry> //yuck

#include "application/appmainwindow.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "annex/annseershape.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvextract.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrextract.h"
#include "tools/featuretools.h"
#include "command/cmdextract.h"

using namespace cmd;
using boost::uuids::uuid;

Extract::Extract()
: Base("cmd::Extract")
, leafManager()
{
  //this command is different. We can create multiple features.
  //so we don't build a feature by default.
//   auto nf = std::make_shared<ftr::Extract>();
//   project->addFeature(nf);
//   feature = nf.get();
//   node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  isEdit = false;
  isFirstRun = true;
}

Extract::Extract(ftr::Base *fIn)
: Base("cmd::Extract")
, leafManager(fIn)
{
  feature = dynamic_cast<ftr::Extract::Feature*>(fIn);
  assert(feature);
  viewBase = std::make_unique<cmv::Extract>(this);
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  isEdit = true;
  isFirstRun = false;
}

Extract::~Extract() = default;

std::string Extract::getStatusMessage()
{
  return QObject::tr("Select geometry to extract").toStdString();
}

void Extract::activate()
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

void Extract::deactivate()
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

bool Extract::isValidSelection(const slc::Message &mIn)
{
  const ftr::Base *lf = project->findFeature(mIn.featureId);
  if (!lf->hasAnnex(ann::Type::SeerShape) || lf->getAnnex<ann::SeerShape>().isNull())
    return false;

  return true;
}

void Extract::setSelections(const slc::Messages &targets)
{
  assert(isActive);
  assert(feature);
  project->clearAllInputs(feature->getId());
  prm::Parameter *prmPicks = feature->getParameter(prm::Tags::Picks);
  prmPicks->setValue(ftr::Picks());
  
  if (targets.empty())
    return;
  
  ftr::Picks freshPicks;
  for (const auto &m : targets)
  {
    if (!isValidSelection(m))
      continue;

    const ftr::Base *lf = project->findFeature(m.featureId);
    freshPicks.push_back(tls::convertToPick(m, *lf, project->getShapeHistory()));
    freshPicks.back().tag = indexTag(ftr::Extract::InputTags::pick, freshPicks.size() - 1);
    project->connect(lf->getId(), feature->getId(), {freshPicks.back().tag});
  }
  prmPicks->setValue(freshPicks);
}

void Extract::localUpdate()
{
  assert(isActive);
  feature->updateModel(project->getPayload(feature->getId()));
  feature->updateVisual();
  feature->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
}

void Extract::go()
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
      auto nf = std::make_shared<ftr::Extract::Feature>();
      project->addFeature(nf);
      feature = nf.get();
      setSelections(targets);
      feature->setColor(project->findFeature(targets.front().featureId)->getColor());
      created = true;
      node->sendBlocked(msg::buildHideThreeD(targets.front().featureId));
      node->sendBlocked(msg::buildHideOverlay(targets.front().featureId));
      node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
    }
  }
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  if (!created)
  {
    auto nf = std::make_shared<ftr::Extract::Feature>();
    project->addFeature(nf);
    feature = nf.get();
    node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
    node->sendBlocked(msg::buildStatusMessage("invalid pre selection", 2.0));
    viewBase = std::make_unique<cmv::Extract>(this);
  }
}
