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

#include "tools/featuretools.h"
#include "tools/occtools.h"
#include "message/msgnode.h"
#include "project/prjproject.h"
#include "selection/slceventhandler.h"
#include "annex/annseershape.h"
#include "feature/ftrinputtype.h"
#include "parameter/prmparameter.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvextrude.h"
#include "feature/ftrextrude.h"
#include "command/cmdextrude.h"

using boost::uuids::uuid;

using namespace cmd;

Extrude::Extrude()
: Base()
, leafManager()
{
  feature = new ftr::Extrude::Feature();
  project->addFeature(std::unique_ptr<ftr::Extrude::Feature>(feature));
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  isEdit = false;
  isFirstRun = true;
}

Extrude::Extrude(ftr::Base *fIn)
: Base()
, leafManager(fIn)
{
  feature = dynamic_cast<ftr::Extrude::Feature*>(fIn);
  assert(feature);
  viewBase = std::make_unique<cmv::Extrude>(this);
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  isEdit = true;
  isFirstRun = false;
}

Extrude::~Extrude() {}

std::string Extrude::getStatusMessage()
{
  return QObject::tr("Select feature or geometry for extrude feature").toStdString();
}

void Extrude::activate()
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

void Extrude::deactivate()
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

void Extrude::localUpdate()
{
  assert(isActive);
  feature->updateModel(project->getPayload(feature->getId()));
  feature->updateVisual();
  feature->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
}

void Extrude::go()
{
  /* We can have more than 1 selection for profile geometry and 
   * we can use geometry for extrude direction. So there is no good way
   * to glean the purpose of the pre-selections. So all selections
   * are going to be for profiles and we will set the direction to
   * infer.
   */
  
  slc::Messages profiles, axes;
  const slc::Containers &containers = eventHandler->getSelections();
  for (const auto &c : containers)
  {
    if (c.featureType == ftr::Type::DatumAxis)
    {
      axes.push_back(slc::EventHandler::containerToMessage(c));
      continue;
    }
    const ftr::Base *pfIn = project->findFeature(containers.front().featureId);
    if (pfIn->hasAnnex(ann::Type::SeerShape))
      profiles.push_back(slc::EventHandler::containerToMessage(c));
  }
  if (!profiles.empty())
  {
    if (!axes.empty())
    {
      setToAxisPicks(profiles, axes);
      node->sendBlocked(msg::buildStatusMessage("Extrude added with datum axis", 2.0));
      node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
      return;
    }
    setToAxisInfer(profiles);
    node->sendBlocked(msg::buildStatusMessage("Extrude added with inferred axis", 2.0));
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return;
  }
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  viewBase = std::make_unique<cmv::Extrude>(this);
}

ftr::Picks Extrude::connect(const std::vector<slc::Message> &msIn, std::string_view prefix)
{
  ftr::Picks out;
  for (const auto &mIn : msIn)
  {
    const ftr::Base *f = project->findFeature(mIn.featureId);
    ftr::Pick pick = tls::convertToPick(mIn, *f, project->getShapeHistory());
    pick.tag = indexTag(prefix, out.size());
    out.push_back(pick);
    project->connectInsert(f->getId(), feature->getId(), {pick.tag});
  };
  return out;
}

void Extrude::setToAxisInfer(const std::vector<slc::Message> &profileMessages)
{
  assert(isActive);
  project->clearAllInputs(feature->getId());
  
  auto getPrm = [&](std::string_view tag) -> prm::Parameter*
  {
    auto prms = feature->getParameters(tag);
    assert(!prms.empty());
    return prms.front();
  };
  getPrm(ftr::Extrude::PrmTags::extrusionType)->setValue(static_cast<int>(ftr::Extrude::ExtrusionType::Infer));
  getPrm(ftr::Extrude::PrmTags::profilePicks)->setValue(connect(profileMessages, ftr::Extrude::InputTags::profile));
}

void Extrude::setToAxisPicks(const std::vector<slc::Message> &profileMessages, const std::vector<slc::Message> &axisMessages)
{
  assert(isActive);
  project->clearAllInputs(feature->getId());
  
  auto getPrm = [&](std::string_view tag) -> prm::Parameter*
  {
    auto prms = feature->getParameters(tag);
    assert(!prms.empty());
    return prms.front();
  };
  getPrm(ftr::Extrude::PrmTags::extrusionType)->setValue(static_cast<int>(ftr::Extrude::ExtrusionType::Picks));
  getPrm(ftr::Extrude::PrmTags::profilePicks)->setValue(connect(profileMessages, ftr::Extrude::InputTags::profile));
  getPrm(ftr::Extrude::PrmTags::axisPicks)->setValue(connect(axisMessages, ftr::Extrude::InputTags::axis));
}

void Extrude::setToAxisParameter(const std::vector<slc::Message> &profileMessages)
{
  assert(isActive);
  project->clearAllInputs(feature->getId());
  
  auto getPrm = [&](std::string_view tag) -> prm::Parameter*
  {
    auto prms = feature->getParameters(tag);
    assert(!prms.empty());
    return prms.front();
  };
  
  getPrm(ftr::Extrude::PrmTags::extrusionType)->setValue(static_cast<int>(ftr::Extrude::ExtrusionType::Constant));
  getPrm(ftr::Extrude::PrmTags::profilePicks)->setValue(connect(profileMessages, ftr::Extrude::InputTags::profile));
}
