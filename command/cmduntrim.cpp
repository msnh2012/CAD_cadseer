/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2020 Thomas S. Anderson blobfish.at.gmx.com
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
#include "project/prjproject.h"
#include "annex/annseershape.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "parameter/prmparameter.h"
#include "feature/ftrinputtype.h"
#include "feature/ftruntrim.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvuntrim.h"
#include "command/cmduntrim.h"

using namespace cmd;

Untrim::Untrim()
: Base("cmd::Untrim")
, leafManager()
{
  auto nf = std::make_shared<ftr::Untrim>();
  project->addFeature(nf);
  feature = nf.get();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  isEdit = false;
  isFirstRun = true;
}

Untrim::Untrim(ftr::Base *fIn)
: Base("cmd::Untrim")
, leafManager(fIn)
{
  feature = dynamic_cast<ftr::Untrim*>(fIn);
  assert(feature);
  viewBase = std::make_unique<cmv::Untrim>(this);
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  isEdit = true;
  isFirstRun = false;
}

Untrim::~Untrim() = default;

std::string Untrim::getStatusMessage()
{
  return QObject::tr("Select geometry for untrim feature").toStdString();
}

void Untrim::activate()
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

void Untrim::deactivate()
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

bool Untrim::isValidSelection(const slc::Message &mIn)
{
  const ftr::Base *lf = project->findFeature(mIn.featureId);
  if (!lf->hasAnnex(ann::Type::SeerShape) || lf->getAnnex<ann::SeerShape>().isNull())
    return false;
  
  auto t = mIn.type;
  if ((t != slc::Type::Object) && (t != slc::Type::Face))
    return false;
  return true;
}

void Untrim::setSelection(const slc::Message &target)
{
  assert(isActive);
  
  project->clearAllInputs(feature->getId());
  feature->setPick(ftr::Pick());
  
  if (!isValidSelection(target))
    return;

  const ftr::Base *lf = project->findFeature(target.featureId);
  ftr::Pick pick = tls::convertToPick(target, *lf, project->getShapeHistory());
  pick.tag = ftr::InputType::createIndexedTag(ftr::InputType::target, 0);
  feature->setPick(pick);
  project->connect(lf->getId(), feature->getId(), {pick.tag});
}

void Untrim::localUpdate()
{
  assert(isActive);
  feature->updateModel(project->getPayload(feature->getId()));
  feature->updateVisual();
  feature->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
}

void Untrim::go()
{
  const slc::Containers &cs = eventHandler->getSelections();
  
  std::vector<slc::Message> tm; //target messages
  for (const auto &c : cs)
  {
    auto m = slc::EventHandler::containerToMessage(c);
    if (isValidSelection(m))
      tm.push_back(m);
  }
  
  if (!tm.empty())
  {
    setSelection(tm.front());
    node->sendBlocked(msg::buildStatusMessage("Untrim Added", 2.0));
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return;
  }
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  node->sendBlocked(msg::buildShowThreeD(feature->getId()));
  node->sendBlocked(msg::buildShowOverlay(feature->getId()));
  viewBase = std::make_unique<cmv::Untrim>(this);
}
