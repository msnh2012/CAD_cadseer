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

#include "viewer/vwrwidget.h"
#include "project/prjproject.h"
#include "annex/annseershape.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrbase.h"
#include "feature/ftrprimitive.h"
#include "tools/featuretools.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvprimitive.h"
#include "command/cmdprimitive.h"

using namespace cmd;

Primitive::Primitive(ftr::Base *fIn, bool eIn)
: Base("cmd::Primitive")
, leafManager(fIn)
{
  feature = fIn;
  assert(feature);
  
  isEdit = eIn;
  if (isEdit.get())
  {
    viewBase = std::make_unique<cmv::Primitive>(this);
    isFirstRun = false;
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  }
  else
  {
    isFirstRun = true;
    node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  }
}

Primitive::~Primitive() = default;

std::string Primitive::getStatusMessage()
{
  return QObject::tr("Select geometry for primitive feature").toStdString();
}

void Primitive::activate()
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

void Primitive::deactivate()
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

void Primitive::setToLinked(const slc::Messages &msIn)
{
  assert(isActive);
  project->clearAllInputs(feature->getId());
  if (msIn.empty())
    return;
  
  const ftr::Base &parent = *project->findFeature(msIn.front().featureId);
  auto pick = tls::convertToPick(msIn.front(), parent, project->getShapeHistory());
  pick.tag = indexTag(prm::Tags::CSysLinked, 0);
  feature->getParameter(prm::Tags::CSysLinked)->setValue({pick});
  project->connectInsert(parent.getId(), feature->getId(), {pick.tag});
}

void Primitive::setToConstant()
{
  assert(isActive);
  project->clearAllInputs(feature->getId());
}

void Primitive::go()
{
  auto systemParameter = feature->getParameter(prm::Tags::CSys);
  systemParameter->setValue(viewer->getCurrentSystem());
  for (const auto &c : eventHandler->getSelections())
  {
    ftr::Base *tf = project->findFeature(c.featureId);
    if (!tf || tf->getParameters(prm::Tags::CSys).empty())
      continue;
    feature->getParameter(prm::Tags::CSysType)->setValue(static_cast<int>(ftr::Primitive::Linked));
    auto msg = slc::EventHandler::containerToMessage(c);
    setToLinked({msg});
    break;
  }
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  node->sendBlocked(msg::buildStatusMessage("Primitive created", 2.0));
  //not going to launch view
}
