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

#include <boost/filesystem.hpp>

#include "application/appapplication.h"
#include "application/appmainwindow.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrquote.h"
#include "parameter/prmparameter.h"
#include "tools/featuretools.h"
#include "viewer/vwrwidget.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvquote.h"
#include "command/cmdquote.h"

using namespace cmd;

using boost::uuids::uuid;

Quote::Quote()
: Base("cmd::Quote")
, leafManager()
{
  auto nf = std::make_shared<ftr::Quote::Feature>();
  project->addFeature(nf);
  feature = nf.get();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  isEdit = false;
  isFirstRun = true;
}

Quote::Quote(ftr::Base *fIn)
: Base("cmd::Quote")
, leafManager(fIn)
{
  feature = dynamic_cast<ftr::Quote::Feature*>(fIn);
  assert(feature);
  viewBase = std::make_unique<cmv::Quote>(this);
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  isEdit = true;
  isFirstRun = false;
}

Quote::~Quote() = default;

std::string Quote::getStatusMessage()
{
  return QObject::tr("Select features for strip and dieset").toStdString();
}

void Quote::activate()
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

void Quote::deactivate()
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

void Quote::setSelections(const slc::Messages &strip, const slc::Messages &dieset)
{
  assert(isActive);
  
  project->clearAllInputs(feature->getId());
  auto *stripPick = feature->getParameter(ftr::Quote::PrmTags::stripPick);
  stripPick->setValue(ftr::Picks());
  auto *diesetPick = feature->getParameter(ftr::Quote::PrmTags::diesetPick);
  diesetPick->setValue(ftr::Picks());
  
  if (strip.empty() || dieset.empty())
    return;
  {
    const ftr::Base *inputFeature = project->findFeature(strip.front().featureId);
    auto pick = tls::convertToPick(strip.front(), *inputFeature, project->getShapeHistory());
    pick.tag = indexTag(ftr::Quote::InputTags::stripPick, 0);
    stripPick->setValue(pick);
    project->connect(inputFeature->getId(), feature->getId(), {pick.tag});
  }
  {
    const ftr::Base *inputFeature = project->findFeature(dieset.front().featureId);
    auto pick = tls::convertToPick(dieset.front(), *inputFeature, project->getShapeHistory());
    pick.tag = indexTag(ftr::Quote::InputTags::diesetPick, 0);
    diesetPick->setValue(pick);
    project->connect(inputFeature->getId(), feature->getId(), {pick.tag});
  }
}

void Quote::localUpdate()
{
  assert(isActive);
  feature->updateModel(project->getPayload(feature->getId()));
  feature->updateVisual();
  feature->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
}

void Quote::go()
{
  uuid stripId = gu::createNilId();
  uuid diesetId = gu::createNilId();
  
  const slc::Containers &containers = eventHandler->getSelections();
  for (const auto &c : containers)
  {
    if (c.featureType == ftr::Type::Strip)
      stripId = c.featureId;
    else if (c.featureType == ftr::Type::DieSet)
      diesetId = c.featureId;
  }
  
  if (stripId.is_nil() || diesetId.is_nil())
  {
    auto ids = project->getAllFeatureIds();
    for (const auto &id : ids)
    {
      ftr::Base *bf = project->findFeature(id);
      if ((bf->getType() == ftr::Type::Strip) && (stripId.is_nil()))
        stripId = id;
      if ((bf->getType() == ftr::Type::DieSet) && (diesetId.is_nil()))
        diesetId = id;
    }
  }
  
  slc::Messages strip;
  if (!stripId.is_nil())
  {
    slc::Message m;
    m.type = slc::Type::Object;
    m.featureType = ftr::Type::Strip;
    m.featureId = stripId;
    strip.push_back(m);
  }
  slc::Messages dieset;
  if (!diesetId.is_nil())
  {
    slc::Message m;
    m.type = slc::Type::Object;
    m.featureType = ftr::Type::DieSet;
    m.featureId = diesetId;
    dieset.push_back(m);
  }

  setSelections(strip, dieset);
//   goScreenCapture(); //screen capture is broke see viewer widget.
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  viewBase = std::make_unique<cmv::Quote>(this);
}

void Quote::goScreenCapture()
{
  namespace bfs = boost::filesystem;
  
  bfs::path pp = (app::instance()->getProject()->getSaveDirectory());
  assert(bfs::exists(pp));
  bfs::path fp = pp /= gu::idToString(feature->getId());
  std::string ext = "png";
  bfs::path fullPath = fp.string() + "_0." + ext;
  
  auto *picturePrm = feature->getParameter(ftr::Quote::PrmTags::pFile);
  picturePrm->setValue(fullPath);
  
  app::instance()->getMainWindow()->getViewer()->screenCapture(fp.string(), ext);
}
