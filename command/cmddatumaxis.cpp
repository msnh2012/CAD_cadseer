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
#include "project/prjproject.h"
#include "annex/annseershape.h"
#include "tools/featuretools.h"
#include "tools/occtools.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrdatumaxis.h"
#include "command/cmddatumaxis.h"

using boost::uuids::uuid;
using namespace cmd;

DatumAxis::DatumAxis() : Base() {}
DatumAxis::~DatumAxis() {}

std::string DatumAxis::getStatusMessage()
{
  return QObject::tr("Select geometry for datum axis").toStdString();
}

void DatumAxis::activate()
{
  isActive = true;
  go();
  sendDone();
}

void DatumAxis::deactivate()
{
  isActive = false;
}

void DatumAxis::go()
{
  assert(project);
  
  const slc::Containers &cs = eventHandler->getSelections();
  if (cs.size() == 2 && slc::isPointType(cs.front().selectionType) && slc::isPointType(cs.back().selectionType))
  {
    std::vector<ftr::Base*> parents;
    parents.push_back(project->findFeature(cs.front().featureId));
    parents.push_back(project->findFeature(cs.back().featureId));
    
    for (const auto &p : parents) //overkill?
    {
      assert(p->hasAnnex(ann::Type::SeerShape));
      if (!p->hasAnnex(ann::Type::SeerShape))
      {
        node->sendBlocked(msg::buildStatusMessage("Error: selection not valid", 2.0));
        shouldUpdate = false;
        return;
      }
    }
    
    ftr::Picks picks;
    picks.push_back(tls::convertToPick(cs.front(), parents.front()->getAnnex<ann::SeerShape>(), project->getShapeHistory()));
    picks.push_back(tls::convertToPick(cs.back(), parents.back()->getAnnex<ann::SeerShape>(), project->getShapeHistory()));
    
    std::shared_ptr<ftr::DatumAxis> daxis(new ftr::DatumAxis());
    daxis->setAxisType(ftr::DatumAxis::AxisType::Points);
    daxis->setPicks(picks);
    daxis->setAutoSize(true);
    project->addFeature(daxis);
    gu::uniquefy(parents); //so we don't connect twice.
    for (const auto &p : parents)
      project->connectInsert(p->getId(), daxis->getId(), ftr::InputType{ftr::InputType::create});
    
    node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return;
  }
  
  int planarCount = 0;
  for (const auto &c : cs)
  {
    if (c.featureType == ftr::Type::DatumPlane)
      planarCount++;
    if (c.selectionType == slc::Type::Face)
      planarCount++;
  }
  if (cs.size() == 2 && planarCount == 2)
  {
    std::vector<ftr::Base*> parents;
    ftr::Picks picks;
    for (const auto &c : cs)
    {
      parents.push_back(project->findFeature(c.featureId));
      if (c.selectionType == slc::Type::Face)
        picks.push_back(tls::convertToPick(c, parents.back()->getAnnex<ann::SeerShape>(), project->getShapeHistory()));
    }
    
    std::shared_ptr<ftr::DatumAxis> daxis(new ftr::DatumAxis());
    daxis->setAxisType(ftr::DatumAxis::AxisType::Intersection);
    daxis->setPicks(picks);
    daxis->setAutoSize(true);
    project->addFeature(daxis);
    gu::uniquefy(parents); //so we don't connect twice.
    for (const auto &p : parents)
      project->connectInsert(p->getId(), daxis->getId(), ftr::InputType{ftr::InputType::create});
    
    node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return;
  }
  if (cs.size() == 1 && (cs.front().selectionType == slc::Type::Face || cs.front().selectionType == slc::Type::Edge))
  {
    const ftr::Base *parent = project->findFeature(cs.front().featureId);
    if (!parent->hasAnnex(ann::Type::SeerShape))
    {
      node->sendBlocked(msg::buildStatusMessage("Error: no seer shape", 2.0));
      shouldUpdate = false;
      return;
    }
    const ann::SeerShape &ss = parent->getAnnex<ann::SeerShape>();
    const TopoDS_Shape &s = ss.getOCCTShape(cs.front().shapeId);
    auto axisPair = occt::gleanAxis(s);
    if (!axisPair.second)
    {
      node->sendBlocked(msg::buildStatusMessage("Error: couldn't glean axis", 2.0));
      shouldUpdate = false;
      return;
    }
    ftr::Pick pick = tls::convertToPick(cs.front(), ss, project->getShapeHistory());
    std::shared_ptr<ftr::DatumAxis> daxis(new ftr::DatumAxis());
    daxis->setAxisType(ftr::DatumAxis::AxisType::Geometry);
    daxis->setPicks(ftr::Picks({pick}));
    daxis->setAutoSize(true);
    project->addFeature(daxis);
    project->connectInsert(parent->getId(), daxis->getId(), ftr::InputType{ftr::InputType::create});
    
    node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return;
  }
  
  //here is where we might launch a dialog, but for now I will just create a constant type.
  std::shared_ptr<ftr::DatumAxis> daxis(new ftr::DatumAxis());
  daxis->setAxisType(ftr::DatumAxis::AxisType::Constant);
  daxis->setAutoSize(false);
  
  osg::Matrixd current = app::instance()->getMainWindow()->getViewer()->getCurrentSystem();
  daxis->setOrigin(current.getTrans());
  daxis->setDirection(gu::getZVector(current));
  
  project->addFeature(daxis);
  return;
}
