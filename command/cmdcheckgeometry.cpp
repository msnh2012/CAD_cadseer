/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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

#include "project/prjproject.h"
#include "project/prjmessage.h"
#include "application/appapplication.h"
#include "application/appmainwindow.h"
#include "feature/ftrbase.h"
#include "feature/ftrmessage.h"
#include "annex/annseershape.h"
#include "selection/slceventhandler.h"
#include "selection/slcmanager.h"
#include "message/msgnode.h"
#include "message/msgsift.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvcheckgeometry.h"
#include "command/cmdcheckgeometry.h"

using namespace cmd;

CheckGeometry::CheckGeometry() : Base("cmd::CheckGeometry")
{
  shouldUpdate = false;
  isEdit = false;
  isFirstRun = true;
  setupDispatcher();
}

CheckGeometry::~CheckGeometry() = default;

std::string CheckGeometry::getStatusMessage()
{
  return QObject::tr("Select object to check").toStdString();
}

void CheckGeometry::activate()
{
  isActive = true;
  
  if (isFirstRun.get())
  {
    isFirstRun = false;
    node->send(msg::buildSelectionMask(slc::ObjectsEnabled | slc::ObjectsSelectable));
    go();
  }
  if (viewBase)
  {
    cmv::Message vm(viewBase.get(), viewBase->getPaneWidth());
    msg::Message out(msg::Mask(msg::Request | msg::Command | msg::View | msg::Show), vm);
    node->sendBlocked(out);
  }
  
  forceClose();
}

void CheckGeometry::deactivate()
{
  if (viewBase)
  {
    msg::Message out(msg::Mask(msg::Request | msg::Command | msg::View | msg::Hide));
    node->sendBlocked(out);
  }
  
  isActive = false;
}

void CheckGeometry::go()
{
  //look for first valid pre selection
  boost::optional<slc::Message> msg;
  for (const auto &c : eventHandler->getSelections())
  {
    if (!slc::isObjectType(c.selectionType))
      continue;
    ftr::Base *lf = project->findFeature(c.featureId);
    assert(lf);
    if (lf->hasAnnex(ann::Type::SeerShape) && !lf->getAnnex<ann::SeerShape>().isNull())
    {
      msg = slc::EventHandler::containerToMessage(c);
      break;
    }
  }
  if (msg)
    goMessage(msg.get());
  else
  {
    node->sendBlocked(msg::buildStatusMessage(QObject::tr("Invalid pre selection").toStdString(), 2));
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  }
}

void CheckGeometry::goMessage(const slc::Message &mIn)
{
  ftr::Base *lf = project->findFeature(mIn.featureId);
  assert(lf);
  if (lf->hasAnnex(ann::Type::SeerShape) && !lf->getAnnex<ann::SeerShape>().isNull())
  {
    assert(!viewBase);
    feature = lf;
    viewBase = std::make_unique<cmv::CheckGeometry>(*feature);
    node->send(msg::buildSelectionMask(~slc::All));
    node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return;
  }
  node->sendBlocked(msg::buildStatusMessage(QObject::tr("Invalid Selection").toStdString(), 2));
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

void CheckGeometry::setupDispatcher()
{
  sift->insert
  (
    {
      std::make_pair
      (
        msg::Response | msg::Post | msg::Selection | msg::Add
        , std::bind(&CheckGeometry::selectionAdditionDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Pre | msg::Remove | msg::Feature
        , std::bind(&CheckGeometry::featureRemovedDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Feature | msg::Status
        , std::bind(&CheckGeometry::featureStateChangedDispatched, this, std::placeholders::_1)
      )
    }
  );
}

void CheckGeometry::selectionAdditionDispatched(const msg::Message &mIn)
{
  if (!isActive || !mIn.isSLC() || viewBase)
    return;
  goMessage(mIn.getSLC());
}

void CheckGeometry::featureRemovedDispatched(const msg::Message &messageIn)
{
  assert(messageIn.isPRJ());
  prj::Message message = messageIn.getPRJ();
  if
  (
    (message.featureIds.size() == 1)
    && (message.featureIds.front() == feature->getId())
  )
  {
    isCompromised = true;
    forceClose();
  }
}

void CheckGeometry::featureStateChangedDispatched(const msg::Message &messageIn)
{
  assert(messageIn.isFTR());
  ftr::Message fMessage = messageIn.getFTR();
  if (fMessage.featureId == feature->getId())
  {
    isCompromised = true;
    forceClose();
  }
}

void CheckGeometry::forceClose()
{
  if (isCompromised && isActive)
    node->send(msg::Message(msg::Request | msg::Command | msg::Done));
}
  
