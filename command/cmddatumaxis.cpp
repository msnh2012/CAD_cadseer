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

#include "globalutilities.h"
#include "application/appapplication.h"
#include "application/appmainwindow.h"
#include "viewer/vwrwidget.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "selection/slcmessage.h"
#include "project/prjproject.h"
#include "tools/featuretools.h"
#include "feature/ftrinputtype.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "feature/ftrdatumaxis.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvdatumaxis.h"
#include "command/cmddatumaxis.h"

using boost::uuids::uuid;
using namespace cmd;

DatumAxis::DatumAxis()
: Base()
, leafManager()
{
  auto daxis = std::make_shared<ftr::DatumAxis::Feature>();
  project->addFeature(daxis);
  feature = daxis.get();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  isEdit = false;
  isFirstRun = true;
}

DatumAxis::DatumAxis(ftr::Base *fIn)
: Base()
, leafManager(fIn)
{
  feature = dynamic_cast<ftr::DatumAxis::Feature*>(fIn);
  assert(feature);
  viewBase = std::make_unique<cmv::DatumAxis>(this);
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  isEdit = true;
  isFirstRun = false;
}

DatumAxis::~DatumAxis() {}

std::string DatumAxis::getStatusMessage()
{
  return QObject::tr("Select geometry for datum axis").toStdString();
}

void DatumAxis::activate()
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

void DatumAxis::deactivate()
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

void DatumAxis::go()
{
  assert(project);
  
  const slc::Containers &cs = eventHandler->getSelections();
  if (cs.size() == 2 && slc::isPointType(cs.front().selectionType) && slc::isPointType(cs.back().selectionType))
  {
    std::vector<slc::Message> msgs;
    msgs.push_back(slc::EventHandler::containerToMessage(cs.front()));
    msgs.push_back(slc::EventHandler::containerToMessage(cs.back()));
    setToPoints(msgs);
    feature->getParameter(prm::Tags::AutoSize)->setValue(true);
    node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return;
  }

  if (cs.size() == 2)
  {
    std::vector<slc::Message> msgs;
    msgs.push_back(slc::EventHandler::containerToMessage(cs.front()));
    msgs.push_back(slc::EventHandler::containerToMessage(cs.back()));
    setToIntersection(msgs);
    feature->getParameter(prm::Tags::AutoSize)->setValue(true);
    node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return;
  }
  
  if (cs.size() == 1)
  {
    if (cs.front().selectionType == slc::Type::Face || cs.front().selectionType == slc::Type::Edge)
    {
      std::vector<slc::Message> msgs;
      msgs.push_back(slc::EventHandler::containerToMessage(cs.front()));
      setToGeometry(msgs);
      feature->getParameter(prm::Tags::AutoSize)->setValue(true);
      node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
      return;
    }
    if (slc::isObjectType(cs.front().selectionType))
    {
      std::vector<slc::Message> msgs;
      msgs.push_back(slc::EventHandler::containerToMessage(cs.front()));
      setToLinked(msgs);
      node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
      return;
    }
  }
  
  //create a constant type. and launch command view
  setToConstant();
  const auto &sys = viewer->getCurrentSystem();
  feature->getParameter(prm::Tags::Origin)->setValue(sys.getTrans());
  feature->getParameter(prm::Tags::Direction)->setValue(gu::getZVector(sys));
  feature->getParameter(prm::Tags::Size)->setValue(viewer->getDiagonalLength() / 4.0);
  
  localUpdate();
  node->sendBlocked(msg::buildStatusMessage("Constant datum added at current system z axis", 2.0));
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  viewBase = std::make_unique<cmv::DatumAxis>(this);
}

void DatumAxis::setToConstant()
{
  assert(isActive);
  
  project->clearAllInputs(feature->getId());
  feature->getParameter(ftr::DatumAxis::Tags::AxisType)->setValue(static_cast<int>(ftr::DatumAxis::Constant));
}

void DatumAxis::setToParameters()
{
  assert(isActive);
  
  project->clearAllInputs(feature->getId());
  feature->getParameter(ftr::DatumAxis::Tags::AxisType)->setValue(static_cast<int>(ftr::DatumAxis::Parameters));
}

void DatumAxis::setToLinked(const slc::Messages &msIn)
{
  assert(isActive);
  
  project->clearAllInputs(feature->getId());
  feature->getParameter(ftr::DatumAxis::Tags::AxisType)->setValue(static_cast<int>(ftr::DatumAxis::Linked));
  
  if (msIn.empty())
    return;
  const ftr::Base &parent = *project->findFeature(msIn.front().featureId);
  auto pick = tls::convertToPick(msIn.front(), parent, project->getShapeHistory());
  pick.tag = indexTag(ftr::InputType::linkCSys, 0);
  feature->getParameter(ftr::DatumAxis::Tags::Linked)->setValue(pick);
  project->connectInsert(parent.getId(), feature->getId(), {pick.tag});
}

void DatumAxis::setToPoints(const slc::Messages &msIn)
{
  assert(isActive);
  
  project->clearAllInputs(feature->getId());
  feature->getParameter(ftr::DatumAxis::Tags::AxisType)->setValue(static_cast<int>(ftr::DatumAxis::Points));
  
  if (msIn.size() != 2)
    return;
  
  std::vector<ftr::Base*> parents;
  parents.push_back(project->findFeature(msIn.front().featureId));
  parents.push_back(project->findFeature(msIn.back().featureId));
  
  ftr::Picks picks;
  picks.push_back(tls::convertToPick(msIn.front(), *parents.front(), project->getShapeHistory()));
  picks.back().tag = indexTag(ftr::DatumAxis::Tags::Points, 0);
  picks.push_back(tls::convertToPick(msIn.back(), *parents.back(), project->getShapeHistory()));
  picks.back().tag = indexTag(ftr::DatumAxis::Tags::Points, 1);
  auto *pickPrm = feature->getParameter(ftr::DatumAxis::Tags::Points);
  pickPrm->setValue(picks);
  
  project->connectInsert(parents.front()->getId(), feature->getId(), {picks.front().tag});
  project->connectInsert(parents.back()->getId(), feature->getId(), {picks.back().tag});
}

void DatumAxis::setToIntersection(const slc::Messages &msIn)
{
  assert(isActive);
  
  project->clearAllInputs(feature->getId());
  auto *param = feature->getParameter(ftr::DatumAxis::Tags::AxisType);
  param->setValue(static_cast<int>(ftr::DatumAxis::Intersection));
  
  int planarCount = 0;
  for (const auto &c : msIn)
  {
    if (c.featureType == ftr::Type::DatumPlane)
      planarCount++;
    if (c.type == slc::Type::Face)
      planarCount++;
  }
  if (planarCount != 2)
    return;
  
  std::vector<ftr::Base*> parents;
  ftr::Picks picks;
  parents.push_back(project->findFeature(msIn.front().featureId));
  picks.push_back(tls::convertToPick(msIn.front(), *parents.front(), project->getShapeHistory()));
  picks.back().tag = indexTag(ftr::DatumAxis::Tags::Intersection, 0);
  parents.push_back(project->findFeature(msIn.back().featureId));
  picks.push_back(tls::convertToPick(msIn.back(), *parents.back(), project->getShapeHistory()));
  picks.back().tag = indexTag(ftr::DatumAxis::Tags::Intersection, 1);
  feature->getParameter(ftr::DatumAxis::Tags::Intersection)->setValue(picks);

  project->connectInsert(parents.front()->getId(), feature->getId(), {picks.front().tag});
  project->connectInsert(parents.back()->getId(), feature->getId(), {picks.back().tag});
}

void DatumAxis::setToGeometry(const slc::Messages &msIn)
{
  assert(isActive);
  project->clearAllInputs(feature->getId());
  feature->getParameter(ftr::DatumAxis::Tags::AxisType)->setValue(static_cast<int>(ftr::DatumAxis::Geometry));
  
  if (msIn.empty())
    return;
  
  const ftr::Base *parent = project->findFeature(msIn.front().featureId);
  ftr::Pick pick = tls::convertToPick(msIn.front(), *parent, project->getShapeHistory());
  pick.tag = indexTag(ftr::DatumAxis::Tags::Geometry, 0);
  feature->getParameter(ftr::DatumAxis::Tags::Geometry)->setValue(pick);
  
  project->connectInsert(parent->getId(), feature->getId(), {pick.tag});
}

void DatumAxis::localUpdate()
{
  assert(isActive);
  feature->updateModel(project->getPayload(feature->getId()));
  feature->updateVisual();
  feature->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
}
