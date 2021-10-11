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

#include <boost/optional.hpp>

#include "tools/featuretools.h"
#include "application/appapplication.h"
#include "application/appmainwindow.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "library/lbrplabel.h"
#include "annex/annseershape.h"
#include "parameter/prmparameter.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrsweep.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvsweep.h"
#include "command/cmdsweep.h"

using boost::uuids::uuid;

using namespace cmd;

Sweep::Sweep()
: Base("cmd::Sweep")
, leafManager()
{
  feature = new ftr::Sweep::Feature();
  project->addFeature(std::unique_ptr<ftr::Sweep::Feature>(feature));
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  isEdit = false;
  isFirstRun = true;
}

Sweep::Sweep(ftr::Base *fIn)
: Base("cmd::Sweep")
, leafManager(fIn)
{
  feature = dynamic_cast<ftr::Sweep::Feature*>(fIn);
  assert(feature);
  viewBase = std::make_unique<cmv::Sweep>(this);
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  isEdit = true;
  isFirstRun = false;
}

Sweep::~Sweep() = default;

std::string Sweep::getStatusMessage()
{
  return QObject::tr("Select geometry for Sweep feature").toStdString();
}

void Sweep::activate()
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

void Sweep::deactivate()
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

void Sweep::connectCommon(const slc::Messages &spine, const Profiles& pMessages)
{
  project->clearAllInputs(feature->getId());
  
  if (!spine.empty())
  {
    const ftr::Base *f = project->findFeature(spine.front().featureId);
    ftr::Pick sp = tls::convertToPick(spine.front(), *f, project->getShapeHistory());
    sp.tag = indexTag(ftr::Sweep::PrmTags::spine, 0);
    feature->getParameter(ftr::Sweep::PrmTags::spine)->setValue(sp);
    project->connect(f->getId(), feature->getId(), ftr::InputType{sp.tag});
  }
  auto &profiles = feature->getProfiles();
  if (!pMessages.empty() && pMessages.size() == profiles.size())
  {
    int index = 0;
    auto apply = [&](const slc::Message &message, ftr::Sweep::Profile &profile)
    {
      const ftr::Base *f = project->findFeature(message.featureId);
      ftr::Pick sp = tls::convertToPick(message, *f, project->getShapeHistory());
      sp.tag = indexTag(ftr::Sweep::InputTags::profile, index);
      profile.pick.setValue(sp);
      project->connect(f->getId(), feature->getId(), {sp.tag});
    };
    
    auto mIt = pMessages.begin();
    auto pIt = profiles.begin();
    for (; mIt != pMessages.end(); ++mIt, ++pIt)
    {
      if (mIt->empty())
        pIt->pick.setValue(ftr::Picks());
      else
        apply(mIt->front(), *pIt);
      index++;
    }
  }
}

// Corrected Frenet, Frenet, Discrete, Fixed
void Sweep::setCommon(const slc::Messages &spine, const Profiles &profiles)
{
  connectCommon(spine, profiles);
}

void Sweep::setBinormal(const slc::Messages &spine, const Profiles &profiles, const slc::Messages &bms)
{
  connectCommon(spine, profiles);
  
  ftr::Picks bps;
  for (const auto &m : bms)
  {
    const ftr::Base *aFeature = project->findFeature(m.featureId);
    assert(aFeature);
    
    ftr::Pick ap = tls::convertToPick(m, *project->findFeature(m.featureId), project->getShapeHistory());
    ap.tag = indexTag(ftr::Sweep::InputTags::binormal, bps.size());
    project->connect(aFeature->getId(), feature->getId(), ftr::InputType{ap.tag});
    bps.push_back(ap);
  }
  feature->getParameter(ftr::Sweep::PrmTags::biPicks)->setValue(bps);
}

void Sweep::setSupport(const slc::Messages &spine, const Profiles &profiles, const slc::Messages &support)
{
  connectCommon(spine, profiles);
  
  if (!support.empty())
  {
    ftr::Pick sp = tls::convertToPick(support.front(), *project->findFeature(support.front().featureId), project->getShapeHistory());
    sp.tag = indexTag(ftr::Sweep::InputTags::support, 0);
    project->connect(support.front().featureId, feature->getId(), {sp.tag});
    feature->getParameter(ftr::Sweep::PrmTags::support)->setValue(sp);
  }
}

void Sweep::setAuxiliary(const slc::Messages &spine, const Profiles &profiles, const slc::Messages &auxiliary)
{
  connectCommon(spine, profiles);
  
  if (!auxiliary.empty())
  {
    ftr::Pick ap = tls::convertToPick(auxiliary.front(), *project->findFeature(auxiliary.front().featureId), project->getShapeHistory());
    ap.tag = indexTag(ftr::Sweep::InputTags::auxiliary, 0);
    project->connect(auxiliary.front().featureId, feature->getId(), {ap.tag});
    feature->getParameter(ftr::Sweep::PrmTags::auxPick)->setValue(ap);
  }
}

void Sweep::localUpdate()
{
  assert(isActive);
  feature->updateModel(project->getPayload(feature->getId()));
  feature->updateVisual();
  feature->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
}

bool Sweep::isValidSelection(const slc::Message &mIn)
{
  const ftr::Base *lf = project->findFeature(mIn.featureId);
  if (!lf->hasAnnex(ann::Type::SeerShape) || lf->getAnnex<ann::SeerShape>().isNull())
    return false;
  
  auto t = mIn.type;
  if
  (
    (t != slc::Type::Object)
    && (t != slc::Type::Wire)
    && (t != slc::Type::Edge)
  )
    return false;
  return true;
}

void Sweep::go()
{
  const slc::Containers &cs = eventHandler->getSelections();
  boost::optional<slc::Message> spine;
  
  std::vector<slc::Messages> profiles;
  for (const auto &c : cs)
  {
    auto m = slc::EventHandler::containerToMessage(c);
    if (!isValidSelection(m))
      continue;
    if (!spine)
    {
      spine = m;
      continue;
    }
    feature->addProfile();
    profiles.push_back({m});
  }
  if (spine && !profiles.empty())
  {
    setCommon({spine.get()}, profiles);
    node->sendBlocked(msg::buildStatusMessage("Sweep Added", 2.0));
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return;
  }
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  node->sendBlocked(msg::buildShowThreeD(feature->getId()));
  node->sendBlocked(msg::buildShowOverlay(feature->getId()));
  viewBase = std::make_unique<cmv::Sweep>(this);
}
