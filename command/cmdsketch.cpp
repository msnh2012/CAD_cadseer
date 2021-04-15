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

#include "application/appapplication.h"
#include "application/appmainwindow.h"
#include "selection/slceventhandler.h"
#include "viewer/vwrwidget.h"
#include "message/msgnode.h"
#include "project/prjproject.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrsketch.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvsketch.h"
#include "command/cmdsketch.h"

using boost::uuids::uuid;

using namespace cmd;

Sketch::Sketch()
: Base("cmd::Sketch")
, leafManager()
{
  auto nf = std::make_shared<ftr::Sketch>();
  project->addFeature(nf);
  feature = nf.get();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  isEdit = false;
  isFirstRun = true;
}

Sketch::Sketch(ftr::Base *fIn)
: Base("cmd::Sketch")
, leafManager(fIn)
{
  feature = dynamic_cast<ftr::Sketch*>(fIn);
  assert(feature);
  viewBase = std::make_unique<cmv::Sketch>(this);
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  isEdit = true;
  isFirstRun = false;
}

Sketch::~Sketch() = default;

std::string Sketch::getStatusMessage()
{
  return QObject::tr("Select feature or geometry for sketch feature").toStdString();
}

void Sketch::activate()
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
    static_cast<cmv::Sketch*>(viewBase.get())->activate();
  }
  else
    sendDone();
}

void Sketch::deactivate()
{
  //we do a shitty job of keeping track of changes during sketch editing.
  //so we just hack in update here.
  localUpdate(); //assume dirty
    
  if (viewBase)
  {
    static_cast<cmv::Sketch*>(viewBase.get())->deactivate();
    msg::Message out(msg::Mask(msg::Request | msg::Command | msg::View | msg::Hide));
    node->sendBlocked(out);
    feature->setNotEditing();
  }
  leafManager.fastForward();
  if (!isEdit.get())
  {
    node->sendBlocked(msg::buildShowThreeD(feature->getId()));
    node->sendBlocked(msg::buildShowOverlay(feature->getId()));
  }
  isActive = false;
}

void Sketch::localUpdate()
{
  assert(isActive);
  feature->updateModel(project->getPayload(feature->getId()));
  feature->draggerHide(); //update will turn it on.
  feature->updateVisual();
  feature->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
}

void Sketch::go()
{
  feature->buildDefault(viewer->getCurrentSystem(), viewer->getDiagonalLength() / 5.0);
  for (const auto &c : eventHandler->getSelections())
  {
    ftr::Base *tf = project->findFeature(c.featureId);
    if (!tf || tf->getParameters(prm::Tags::CSys).empty())
      continue;
    project->connectInsert(tf->getId(), feature->getId(), ftr::InputType{ftr::InputType::linkCSys});
    node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
    break;
  }
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  node->sendBlocked(msg::buildStatusMessage("Sketch created", 2.0));
  viewBase = std::make_unique<cmv::Sketch>(this);
}
