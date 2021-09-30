/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) %YEAR% Thomas S. Anderson blobfish.at.gmx.com
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

// #include "tools/featuretools.h"
// #include "application/appmainwindow.h"
// #include "application/appapplication.h"
// #include "viewer/vwrwidget.h"
#include "project/prjproject.h"
#include "annex/annseershape.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "parameter/prmparameter.h"
#include "feature/ftrinputtype.h"
#include "feature/ftr%CLASSNAMELOWERCASE%.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmv%CLASSNAMELOWERCASE%.h"
#include "command/cmd%CLASSNAMELOWERCASE%.h"

using namespace cmd;

%CLASSNAME%::%CLASSNAME%()
: Base("cmd::%CLASSNAME%")
, leafManager()
{
  feature = new ftr::%CLASSNAME%::Feature();
  project->addFeature(std::unique_ptr<ftr::%CLASSNAME%::Feature>(feature));
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  isEdit = false;
  isFirstRun = true;
}

%CLASSNAME%::%CLASSNAME%(ftr::Base *fIn)
: Base("cmd::%CLASSNAME%")
, leafManager(fIn)
{
  feature = dynamic_cast<ftr::%CLASSNAME%::Feature*>(fIn);
  assert(feature);
  viewBase = std::make_unique<cmv::%CLASSNAME%>(this);
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  isEdit = true;
  isFirstRun = false;
}

%CLASSNAME%::~%CLASSNAME%() = default;

std::string %CLASSNAME%::getStatusMessage()
{
  return QObject::tr("Select geometry for %CLASSNAMELOWERCASE% feature").toStdString();
}

void %CLASSNAME%::activate()
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

void %CLASSNAME%::deactivate()
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

bool %CLASSNAME%::isValidSelection(const slc::Message &mIn)
{
//   const ftr::Base *lf = project->findFeature(mIn.featureId);
//   if (!lf->hasAnnex(ann::Type::SeerShape) || lf->getAnnex<ann::SeerShape>().isNull())
//     return false;
//   
//   auto t = mIn.type;
//   if
//   (
//     (t != slc::Type::Object)
//     && (t != slc::Type::Shell)
//     && (t != slc::Type::Face)
//   )
//     return false;
  return true;
}

void %CLASSNAME%::setSelections(const slc::Messages &targets)
{
  assert(isActive);
  
//   project->clearAllInputs(feature->getId());
//   auto *parameter = feature->getParameter(prm::Tags::pick);
//   parameter->setValue(ftr::Picks());
//   
//   if (targets.empty())
//     return;
//   
//   ftr::Picks freshPicks;
//   for (const auto &m : targets)
//   {
//     if (!isValidSelection(m))
//       continue;
// 
//     const ftr::Base *lf = project->findFeature(m.featureId);
//     freshPicks.push_back(tls::convertToPick(m, *lf, project->getShapeHistory()));
//     freshPicks.back().tag = indexTag(prm::Tags::pick, freshPicks.size() - 1);
//     project->connect(lf->getId(), feature->getId(), {freshPicks.back().tag});
//   }
//   parameter->setValue(freshPicks);
}

void %CLASSNAME%::localUpdate()
{
  assert(isActive);
  feature->updateModel(project->getPayload(feature->getId()));
  feature->updateVisual();
  feature->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
}

void %CLASSNAME%::go()
{
//   const slc::Containers &cs = eventHandler->getSelections();
//   
//   std::vector<slc::Message> tm; //target messages
//   for (const auto &c : cs)
//   {
//     auto m = slc::EventHandler::containerToMessage(c);
//     if (isValidSelection(m))
//       tm.push_back(m);
//   }
//   
//   if (!tm.empty())
//   {
//     setSelections(tm);
//     node->sendBlocked(msg::buildStatusMessage("%CLASSNAME% Added", 2.0));
//     node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
//     return;
//   }
//   
//   node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
//   node->sendBlocked(msg::buildShowThreeD(feature->getId()));
//   node->sendBlocked(msg::buildShowOverlay(feature->getId()));
//   viewBase = std::make_unique<cmv::%CLASSNAME%>(this);
}
