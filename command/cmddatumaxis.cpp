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
#include "annex/annseershape.h"
#include "tools/featuretools.h"
#include "tools/occtools.h"
#include "feature/ftrinputtype.h"
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
  auto daxis = std::make_shared<ftr::DatumAxis>();
  project->addFeature(daxis);
  feature = daxis.get();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
}

DatumAxis::DatumAxis(ftr::Base *fIn)
: Base()
, leafManager(fIn)
{
  feature = dynamic_cast<ftr::DatumAxis*>(fIn);
  assert(feature);
  firstRun = false; //bypass go() in activate.
  viewBase = std::make_unique<cmv::DatumAxis>(this);
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
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
  if (firstRun)
  {
    firstRun = false;
    go();
  }
  if (viewBase)
  {
    cmv::Message vm(viewBase.get(), viewBase->getPaneWidth());
    msg::Message out(msg::Mask(msg::Request | msg::Command | msg::View | msg::Show), vm);
    node->sendBlocked(out);
  }
  else
    sendDone();
}

void DatumAxis::deactivate()
{
  isActive = false;
  if (viewBase)
  {
    msg::Message out(msg::Mask(msg::Request | msg::Command | msg::View | msg::Hide));
    node->sendBlocked(out);
  }
  leafManager.fastForward();
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
    feature->setAutoSize(true);
    node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return;
  }

  if (cs.size() == 2)
  {
    std::vector<slc::Message> msgs;
    msgs.push_back(slc::EventHandler::containerToMessage(cs.front()));
    msgs.push_back(slc::EventHandler::containerToMessage(cs.back()));
    setToIntersection(msgs);
    feature->setAutoSize(true);
    node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return;
  }
  
  if (cs.size() == 1 && (cs.front().selectionType == slc::Type::Face || cs.front().selectionType == slc::Type::Edge))
  {
    std::vector<slc::Message> msgs;
    msgs.push_back(slc::EventHandler::containerToMessage(cs.front()));
    setToGeometry(msgs);
    feature->setAutoSize(true);
    node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return;
  }
  
  //create a constant type. and launch command view
  feature->setAxisType(ftr::DatumAxis::AxisType::Constant);
  osg::Matrixd current = viewer->getCurrentSystem();
  feature->setOrigin(current.getTrans());
  feature->setDirection(gu::getZVector(current));
  feature->setSize(viewer->getDiagonalLength() / 4.0);
  localUpdate();
  node->sendBlocked(msg::buildStatusMessage("Constant datum added at current system z axis", 2.0));
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  viewBase = std::make_unique<cmv::DatumAxis>(this);
}

void DatumAxis::setToConstant()
{
  project->clearAllInputs(feature->getId());
  feature->setAxisType(ftr::DatumAxis::AxisType::Constant);
}

void DatumAxis::setToPoints(const std::vector<slc::Message> &msIn)
{
  assert(msIn.size() == 2); //caller to verify
  assert(slc::isPointType(msIn.front().type));
  assert(slc::isPointType(msIn.back().type));
  std::vector<ftr::Base*> parents;
  parents.push_back(project->findFeature(msIn.front().featureId));
  parents.push_back(project->findFeature(msIn.back().featureId));
  
  for (const auto &p : parents) //overkill?
  {
    assert(p->hasAnnex(ann::Type::SeerShape));
    if (!p->hasAnnex(ann::Type::SeerShape))
      return;
  }
  
  ftr::Picks picks;
  picks.push_back(tls::convertToPick(msIn.front(), parents.front()->getAnnex<ann::SeerShape>(), project->getShapeHistory()));
  picks.back().tag = ftr::InputType::createIndexedTag(ftr::InputType::create, 0);
  picks.push_back(tls::convertToPick(msIn.back(), parents.back()->getAnnex<ann::SeerShape>(), project->getShapeHistory()));
  picks.back().tag = ftr::InputType::createIndexedTag(ftr::InputType::create, 1);
  
  feature->setAxisType(ftr::DatumAxis::AxisType::Points);
  feature->setPicks(picks);
  project->clearAllInputs(feature->getId());
  project->connectInsert(parents.front()->getId(), feature->getId(), {picks.front().tag});
  project->connectInsert(parents.back()->getId(), feature->getId(), {picks.back().tag});
}

void DatumAxis::setToIntersection(const std::vector<slc::Message> &msIn)
{
  assert(msIn.size() == 2); //caller to verify
  
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
  picks.back().tag = ftr::InputType::createIndexedTag(ftr::InputType::create, 0);
  parents.push_back(project->findFeature(msIn.back().featureId));
  picks.push_back(tls::convertToPick(msIn.back(), *parents.back(), project->getShapeHistory()));
  picks.back().tag = ftr::InputType::createIndexedTag(ftr::InputType::create, 1);

  feature->setAxisType(ftr::DatumAxis::AxisType::Intersection);
  feature->setPicks(picks);
  project->clearAllInputs(feature->getId());
  project->connectInsert(parents.front()->getId(), feature->getId(), {picks.front().tag});
  project->connectInsert(parents.back()->getId(), feature->getId(), {picks.back().tag});
}

void DatumAxis::setToGeometry(const std::vector<slc::Message> &msIn)
{
  const ftr::Base *parent = project->findFeature(msIn.front().featureId);
  if (!parent->hasAnnex(ann::Type::SeerShape))
    return;
  const ann::SeerShape &ss = parent->getAnnex<ann::SeerShape>();
  const TopoDS_Shape &s = ss.getOCCTShape(msIn.front().shapeId);
  auto axisPair = occt::gleanAxis(s);
  if (!axisPair.second)
    return;
  ftr::Pick pick = tls::convertToPick(msIn.front(), ss, project->getShapeHistory());
  pick.tag = ftr::InputType::createIndexedTag(ftr::InputType::create, 0);
  feature->setAxisType(ftr::DatumAxis::AxisType::Geometry);
  feature->setPicks(ftr::Picks({pick}));
  project->clearAllInputs(feature->getId());
  project->connectInsert(parent->getId(), feature->getId(), {pick.tag});
}

void DatumAxis::localUpdate()
{
  feature->updateModel(project->getPayload(feature->getId()));
  feature->updateVisual();
  feature->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
}
