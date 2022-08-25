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
#include "selection/slcmessage.h"
#include "annex/annseershape.h"
#include "message/msgnode.h"
#include "parameter/prmparameter.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrlawspine.h"
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
    if (!isEdit.get())
      node->sendBlocked(msg::buildSelectionFreeze(feature->getId()));
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

bool Blend::isValidSelection(const slc::Message &mIn)
{
  if (mIn.featureType == ftr::Type::LawSpine) return true;
  if (mIn.type != slc::Type::Edge) return false;
  
  ftr::Base *f = project->findFeature(mIn.featureId);
  assert(f);
  if (!f->hasAnnex(ann::Type::SeerShape) || f->getAnnex<ann::SeerShape>().isNull())
    return false;
  
  return true;
}

void Blend::setSelections(const std::vector<slc::Messages> &msgsIn)
{
  assert(isActive);
  project->clearAllInputs(feature->getId());
  
  auto makePick = [&](const slc::Message &mIn, std::string_view tag, int index) -> ftr::Pick
  {
    ftr::Pick pick = tls::convertToPick(mIn, *project->findFeature(mIn.featureId), project->getShapeHistory());
    pick.tag = indexTag(tag, index);
      project->connectInsert(mIn.featureId, feature->getId(), {pick.tag});
      project->connect(mIn.featureId, feature->getId(), {pick.tag});
    return pick;
  };
  
  int localBlendType = feature->getParameter(ftr::Blend::PrmTags::blendType)->getInt();
  switch(localBlendType)
  {
    case 0: //constant
    {
      assert(msgsIn.size() == feature->getConstants().size());
      int index = -1;
      for (const auto &msgs : msgsIn)
      {
        index++;
        ftr::Picks currentPicks;
        for (const auto &m : msgs)
        {
          if (!isValidSelection(m))
            continue;
          currentPicks.push_back(makePick(m, ftr::Blend::InputTags::constant, index));
          project->connectInsert(m.featureId, feature->getId(), {currentPicks.back().tag});
        }
        feature->getConstant(index).contourPicks.setValue(currentPicks);
      }
      break;
    }
    case 1: //variable
    {
      /* message front() is for the contour picks.
       * the rest are for entries.
       */
      assert(!msgsIn.empty());
      auto &variable = feature->getVariable();
      assert(variable.entries.size() + 1 == msgsIn.size());
      
      ftr::Picks contourPicks;
      auto it = msgsIn.begin();
      for (auto it2 = it->begin(); it2 != it->end(); ++it2)
      {
        if (!isValidSelection(*it2))
          continue;
        contourPicks.push_back(makePick(*it2, ftr::Blend::InputTags::contourPick, std::distance(it->begin(), it2)));
        project->connectInsert(it2->featureId, feature->getId(), {contourPicks.back().tag});
      }
      variable.contourPicks.setValue(contourPicks);
      
      //onto the entries
      auto entryIt = variable.entries.begin();
      for (++it; it != msgsIn.end(); ++it, ++entryIt)
      {
        if (it->empty()) //should only have 1 pick for each entry.
          continue;
        auto cp = makePick(it->front(), ftr::Blend::InputTags::entryPick, std::distance(variable.entries.begin(), entryIt));
        entryIt->entryPick.setValue({cp});
        project->connectInsert(it->front().featureId, feature->getId(), {cp.tag});
      }
      break;
    }
    case 2: //law spine
    {
      if (msgsIn.size() != 1 || msgsIn.front().size() != 1 || msgsIn.front().front().featureType != ftr::Type::LawSpine) return;
      auto pick = makePick(msgsIn.front().front(), ftr::Blend::PrmTags::lawSpinePick, 0);
      feature->getParameter(ftr::Blend::PrmTags::lawSpinePick)->setValue({pick});
      project->connect(msgsIn.front().front().featureId, feature->getId(), {pick.tag, ftr::InputType::sever});
      
      const ftr::LawSpine::Feature *lsf = dynamic_cast<ftr::LawSpine::Feature*>(project->findFeature(msgsIn.front().front().featureId));
      assert(lsf);
      const auto &lsp = lsf->getParameter(ftr::LawSpine::Tags::spinePick)->getPicks();
      auto pl = project->getPayload(lsf->getId()).getFeatures("");
      if (!lsp.empty() && !pl.empty())
      {
        auto pick = lsp.front();
        pick.tag = indexTag(ftr::Blend::PrmTags::lawEdgePick, 0);
        feature->getParameter(ftr::Blend::PrmTags::lawEdgePick)->setValue({pick});
        project->connectInsert(pl.front()->getId(), feature->getId(), {pick.tag});
      }
      break;
    }
    default: //error
    {
      assert(0); //unknown blend type
      break;
    }
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
      
      dlg::Parameter *pDialog = nullptr;
      if (targets.size() == 1 && targets.front().featureType == ftr::Type::LawSpine)
        feature->getParameter(ftr::Blend::PrmTags::blendType)->setValue(2);
      else
      {
        auto &constant = feature->addConstant();
        pDialog = new dlg::Parameter(&constant.radius, feature->getId());
      }
      
      setSelections({targets});
      created = true;
      node->sendBlocked(msg::buildHideThreeD(targets.front().featureId));
      node->sendBlocked(msg::buildHideOverlay(targets.front().featureId));
      node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
      
      if (pDialog)
      {
        pDialog->show();
        pDialog->raise();
        pDialog->activateWindow();
      }
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
