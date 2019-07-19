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

#include <boost/optional.hpp>

#include <TopoDS.hxx>
#include <BRepAdaptor_Surface.hxx>

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
#include "feature/ftrdatumplane.h"
#include "command/cmddatumplane.h"

using namespace cmd;
using boost::uuids::uuid;

DatumPlane::DatumPlane() : Base() {}
DatumPlane::~DatumPlane() {}

std::string DatumPlane::getStatusMessage()
{
  return QObject::tr("Select geometry for datum plane").toStdString();
}

void DatumPlane::activate()
{
  isActive = true;
  go();
  sendDone();
}

void DatumPlane::deactivate()
{
  isActive = false;
}

bool DatumPlane::attemptOffset(const slc::Container &cIn)
{
  const ftr::Base *f = project->findFeature(cIn.featureId);
  ftr::Pick pick = tls::convertToPick(cIn, *f, project->getShapeHistory());
  pick.tag = ftr::InputType::create;
  
  std::shared_ptr<ftr::DatumPlane> dp(new ftr::DatumPlane());
  dp->setDPType(ftr::DatumPlane::DPType::POffset);
  dp->setPicks(ftr::Picks({pick}));
  dp->setAutoSize(true);
  
  auto build = [&]()
  {
    project->addFeature(dp);
    project->connectInsert(f->getId(), dp->getId(), {pick.tag});
    node->sendBlocked(msg::buildStatusMessage("Offset datum plane added", 2.0));
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    shouldUpdate = true;
  };
  
  if (cIn.isObjectType() && cIn.featureType == ftr::Type::DatumPlane)
  {
    build();
    return true;
  }
  if (cIn.selectionType == slc::Type::Face)
  {
    if (!f->hasAnnex(ann::Type::SeerShape))
      return false;
    const ann::SeerShape &ss = f->getAnnex<ann::SeerShape>();
    assert(ss.hasId(cIn.shapeId));
    const TopoDS_Shape &fs = ss.getOCCTShape(cIn.shapeId);
    assert(fs.ShapeType() == TopAbs_FACE);
    BRepAdaptor_Surface sa(TopoDS::Face(fs));
    if (sa.GetType() == GeomAbs_Plane)
    {
      build();
      return true;
    }
  }
  return false;
}

bool DatumPlane::attemptCenter(const std::vector<slc::Container> &csIn)
{
  tls::Connector connector;
  ftr::Picks picks;
  std::vector<const ftr::Base*> parents;
  for (const auto &s : csIn)
  {
    const ftr::Base *f = project->findFeature(s.featureId);
    assert(f);
    ftr::Pick tempPick = tls::convertToPick(s, *f, project->getShapeHistory());
    tempPick.tag = ftr::InputType::createIndexedTag(ftr::DatumPlane::center, picks.size());
    if (slc::isObjectType(s.selectionType))
    {
      if (f->getType() != ftr::Type::DatumPlane)
        continue;
    }
    else
    {
      if (!f->hasAnnex(ann::Type::SeerShape))
        continue;
      const ann::SeerShape &ss = f->getAnnex<ann::SeerShape>();
      if (ss.isNull())
        continue;
      assert(ss.hasId(s.shapeId));
      const TopoDS_Shape &fs = ss.getOCCTShape(s.shapeId);
      if (fs.ShapeType() != TopAbs_FACE)
        continue;
      BRepAdaptor_Surface sa(TopoDS::Face(fs));
      if (sa.GetType() != GeomAbs_Plane)
        continue;
    }
    connector.add(f->getId(), tempPick.tag);
    picks.push_back(tempPick);
  }
  if (picks.size() == 2)
  {
    std::shared_ptr<ftr::DatumPlane> dp(new ftr::DatumPlane());
    dp->setDPType(ftr::DatumPlane::DPType::PCenter);
    dp->setPicks(picks);
    dp->setAutoSize(true);
    project->addFeature(dp);
    for (const auto &p : connector.pairs)
      project->connectInsert(p.first, dp->getId(), {p.second});
    node->sendBlocked(msg::buildStatusMessage("Center datum plane added", 2.0));
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    shouldUpdate = true;
    return true;
  }
  return false;
}

bool DatumPlane::attemptAxisAngle(const std::vector<slc::Container> &csIn)
{
  boost::optional<ftr::Pick> planePick;
  boost::optional<ftr::Pick> axisPick;
  tls::Connector connector;
  
  //datum planes and planar face will be for planePick and tried first
  //everything else will be for axis pick.
  auto assignPick = [&](const slc::Container &cIn)
  {
    const ftr::Base *f = project->findFeature(cIn.featureId);
    if (cIn.featureType == ftr::Type::DatumPlane)
    {
      if (planePick) //already assigned the planePick, but we have a datum plane? get out.
        return;
      planePick = tls::convertToPick(cIn, *f, project->getShapeHistory());
      planePick.get().tag = ftr::DatumPlane::plane;
      connector.add(f->getId(), planePick.get().tag);
    }
    else if (cIn.featureType == ftr::Type::DatumAxis)
    {
      if (axisPick) //already assigned the axisPick, but we have a datum axis? get out.
        return;
      axisPick = tls::convertToPick(cIn, *f, project->getShapeHistory());
      axisPick.get().tag = ftr::DatumPlane::axis;
      connector.add(f->getId(), axisPick.get().tag);
    }
    else if (cIn.isShapeType() && !cIn.isPointType())
    {
      if (!f->hasAnnex(ann::Type::SeerShape))
        return;
      const ann::SeerShape &ss = f->getAnnex<ann::SeerShape>();
      if (ss.isNull())
        return;
      assert(ss.hasId(cIn.shapeId));
      const TopoDS_Shape &s = ss.getOCCTShape(cIn.shapeId);
      auto localGleanAxis = [&]()
      {
        if (!axisPick)
        {
          auto glean = occt::gleanAxis(s);
          if (glean.second)
          {
            axisPick = tls::convertToPick(cIn, ss, project->getShapeHistory());
            axisPick.get().tag = ftr::DatumPlane::axis;
            connector.add(f->getId(), axisPick.get().tag);
          }
        }
      };
      if (s.ShapeType() == TopAbs_FACE)
      {
        BRepAdaptor_Surface sa(TopoDS::Face(s));
        if (sa.GetType() == GeomAbs_Plane)
        {
          if (!planePick)
          {
            planePick = tls::convertToPick(cIn, ss, project->getShapeHistory());
            planePick.get().tag = ftr::DatumPlane::plane;
            connector.add(f->getId(), planePick.get().tag);
          }
        }
        else 
          localGleanAxis();
      }
      else
        localGleanAxis();
    }
  };
  
  assignPick(csIn.front());
  assignPick(csIn.back());
  if (!planePick || !axisPick)
    return false;
  
  std::shared_ptr<ftr::DatumPlane> dp(new ftr::DatumPlane());
  dp->setDPType(ftr::DatumPlane::DPType::AAngleP);
  dp->setPicks({planePick.get(), axisPick.get()});
  dp->setAutoSize(true);
  project->addFeature(dp);
  for (const auto &p : connector.pairs)
    project->connectInsert(p.first, dp->getId(), {p.second});
  node->sendBlocked(msg::buildStatusMessage("Axis angle datum plane added", 2.0));
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  shouldUpdate = true;
  return true;
}

bool DatumPlane::attemptAverage3P(const std::vector<slc::Container> &csIn)
{
  ftr::Picks picks;
  tls::Connector connector;
  for (const auto &c : csIn)
  {
    const ftr::Base *f = project->findFeature(c.featureId);
    if (c.isObjectType() && c.featureType == ftr::Type::DatumPlane)
    {
      ftr::Pick p = tls::convertToPick(c, *f, project->getShapeHistory());
      p.tag = ftr::InputType::createIndexedTag(ftr::DatumPlane::plane, picks.size());
      connector.add(f->getId(), p.tag);
      picks.push_back(p);
    }
    else if (!c.isPointType())
    {
      if (!f->hasAnnex(ann::Type::SeerShape))
        continue;;
      const ann::SeerShape &ss = f->getAnnex<ann::SeerShape>();
      if (ss.isNull())
        continue;
      const TopoDS_Shape &shape = ss.getOCCTShape(c.shapeId);
      if (shape.ShapeType() != TopAbs_FACE)
        continue;
      BRepAdaptor_Surface adaptor(TopoDS::Face(shape));
      if (adaptor.GetType() != GeomAbs_Plane)
        continue;
      ftr::Pick p = tls::convertToPick(c, ss, project->getShapeHistory());
      p.tag = ftr::InputType::createIndexedTag(ftr::DatumPlane::plane, picks.size());
      connector.add(f->getId(), p.tag);
      picks.push_back(p);
    }
  }
  
  if (picks.size() != 3)
    return false;
  
  std::shared_ptr<ftr::DatumPlane> dp(new ftr::DatumPlane());
  dp->setDPType(ftr::DatumPlane::DPType::Average3P);
  dp->setPicks(picks);
  dp->setAutoSize(true);
  project->addFeature(dp);
  for (const auto &p : connector.pairs)
    project->connectInsert(p.first, dp->getId(), {p.second});
  node->sendBlocked(msg::buildStatusMessage("Average datum plane added", 2.0));
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  shouldUpdate = true;
  return true;
}

bool DatumPlane::attemptThrough3P(const std::vector<slc::Container> &csIn)
{
  ftr::Picks picks;
  tls::Connector connector;
  for (const auto &c : csIn)
  {
    if (!c.isPointType())
      return false;
    ftr::Pick temp = tls::convertToPick(c, *project->findFeature(c.featureId), project->getShapeHistory());
    temp.tag = ftr::InputType::createIndexedTag(ftr::DatumPlane::point, picks.size());
    connector.add(c.featureId, temp.tag);
    picks.push_back(temp);
  }
  
  if (picks.size() != 3)
    return false;

  std::shared_ptr<ftr::DatumPlane> dp(new ftr::DatumPlane());
  dp->setDPType(ftr::DatumPlane::DPType::Through3P);
  dp->setPicks(picks);
  dp->setAutoSize(true);
  project->addFeature(dp);
  for (const auto &p : connector.pairs)
    project->connectInsert(p.first, dp->getId(), {p.second});
  node->sendBlocked(msg::buildStatusMessage("Average datum plane added", 2.0));
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  shouldUpdate = true;
  return true;
}

void DatumPlane::go()
{
  shouldUpdate = false;
  
  assert(project);
  const slc::Containers &cs = eventHandler->getSelections();
  
  if (cs.empty())
  {
    //build constant at current system.
    //maybe launch dialog in future.
    std::shared_ptr<ftr::DatumPlane> dp(new ftr::DatumPlane());
    dp->setDPType(ftr::DatumPlane::DPType::Constant);
    dp->setAutoSize(false);
    dp->setSystem(app::instance()->getMainWindow()->getViewer()->getCurrentSystem());
    project->addFeature(dp);
    node->sendBlocked(msg::buildStatusMessage("Constant datum plane added at current system", 2.0));
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    shouldUpdate = true;
    return;
  }
  if (cs.size() == 1)
  {
    if (attemptOffset(cs.front()))
      return;
  }
  if (cs.size() == 2)
  {
    if (attemptCenter(cs))
      return;
    if (attemptAxisAngle(cs))
      return;
  }
  if (cs.size() == 3)
  {
    if (attemptAverage3P(cs))
      return;
    if (attemptThrough3P(cs))
      return;
  }
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
}
