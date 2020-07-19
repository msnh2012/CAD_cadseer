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

#include <osg/Geometry>

// #include "tools/featuretools.h"
#include "application/appmainwindow.h"
// #include "application/appapplication.h"
#include "project/prjproject.h"
// #include "viewer/vwrwidget.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "parameter/prmparameter.h"
#include "dialogs/dlgparameter.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvhollow.h"
#include "annex/annseershape.h"
#include "tools/featuretools.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrhollow.h"
#include "command/cmdhollow.h"

using boost::uuids::uuid;

using namespace cmd;

Hollow::Hollow()
: Base("cmd::Hollow")
, leafManager()
{
  auto nf = std::make_shared<ftr::Hollow>();
  project->addFeature(nf);
  feature = nf.get();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  isEdit = false;
  isFirstRun = true;
}

Hollow::Hollow(ftr::Base *fIn)
: Base("cmd::Hollow")
, leafManager(fIn)
{
  feature = dynamic_cast<ftr::Hollow*>(fIn);
  assert(feature);
  viewBase = std::make_unique<cmv::Hollow>(this);
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  isEdit = true;
  isFirstRun = false;
}

Hollow::~Hollow() = default;

std::string Hollow::getStatusMessage()
{
  return QObject::tr("Select faces for Hollow feature").toStdString();
}

void Hollow::activate()
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

void Hollow::deactivate()
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

bool Hollow::isValidSelection(const slc::Message &mIn)
{
  const ftr::Base *lf = project->findFeature(mIn.featureId);
  if (!lf->hasAnnex(ann::Type::SeerShape) || lf->getAnnex<ann::SeerShape>().isNull())
    return false;
  
  if (mIn.type != slc::Type::Face)
    return false;
  return true;
}

void Hollow::setSelections(const slc::Messages &targets)
{
  assert(isActive);
  project->clearAllInputs(feature->getId());
  feature->setHollowPicks(ftr::Picks());
  
  if (targets.empty())
    return;
  
  uuid fId = gu::createNilId(); //ensure all belong to same feature.
  ftr::Picks freshPicks;
  for (const auto &m : targets)
  {
    if (fId.is_nil())
      fId = m.featureId;
    if (fId != m.featureId)
      continue;
    if (!isValidSelection(m))
      continue;

    const ftr::Base *lf = project->findFeature(m.featureId);
    freshPicks.push_back(tls::convertToPick(m, *lf, project->getShapeHistory()));
    freshPicks.back().tag = ftr::InputType::createIndexedTag(ftr::InputType::target, freshPicks.size() - 1);
    project->connect(lf->getId(), feature->getId(), {freshPicks.back().tag});
  }
  feature->setHollowPicks(freshPicks);
}

void Hollow::localUpdate()
{
  assert(isActive);
  feature->updateModel(project->getPayload(feature->getId()));
  feature->updateVisual();
  feature->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
}

void Hollow::go()
{
  const slc::Containers &cs = eventHandler->getSelections();
  if (!cs.empty())
  {
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
      setSelections(targets);
      node->sendBlocked(msg::buildHideThreeD(targets.front().featureId));
      node->sendBlocked(msg::buildHideOverlay(targets.front().featureId));
      dlg::Parameter *pDialog = new dlg::Parameter(&feature->getOffset(), feature->getId());
      pDialog->show();
      pDialog->raise();
      pDialog->activateWindow();
      return;
    }
  }
  
  node->sendBlocked(msg::buildStatusMessage("invalid pre selection", 2.0));
  viewBase = std::make_unique<cmv::Hollow>(this);
}
