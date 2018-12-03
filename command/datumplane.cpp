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

#include <globalutilities.h>
#include <application/application.h>
#include <application/mainwindow.h>
#include <viewer/widget.h>
#include <message/node.h>
#include <selection/eventhandler.h>
#include <project/project.h>
#include <annex/seershape.h>
#include <tools/featuretools.h>
#include <tools/occtools.h>
#include <feature/inputtype.h>
#include <feature/datumplane.h>
#include <command/datumplane.h>

using namespace cmd;
using boost::uuids::uuid;


//caller needs to set pick tag and shapehistory.
static boost::optional<ftr::Pick> buildPlanePick(const slc::Container &cIn, const ftr::Base *f)
{
  if (cIn.shapeId.is_nil())
    return boost::none;
  if (!f->hasAnnex(ann::Type::SeerShape))
    return boost::none;
  const ann::SeerShape &ss = f->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
  const TopoDS_Shape &shape = ss.getOCCTShape(cIn.shapeId);
  if (shape.ShapeType() != TopAbs_FACE)
    return boost::none;
  BRepAdaptor_Surface adaptor(TopoDS::Face(shape));
  if (adaptor.GetType() != GeomAbs_Plane)
    return boost::none;
  
  ftr::Pick p = tls::convertToPick(cIn, ss);
  return p;
};




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
  
  if (f->getType() == ftr::Type::DatumPlane)
  {
    //offset from another plane.
    std::shared_ptr<ftr::DatumPlane> dp(new ftr::DatumPlane());
    dp->setDPType(ftr::DatumPlane::DPType::POffset);
    dp->setAutoSize(true);
    project->addFeature(dp);
    project->connectInsert(f->getId(), dp->getId(), ftr::InputType{ftr::InputType::create});
    node->sendBlocked(msg::buildStatusMessage("Offset datum plane added", 2.0));
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return true;
  }
  
  if (cIn.selectionType != slc::Type::Face)
    return false;
  if (!f->hasAnnex(ann::Type::SeerShape))
    return false;
  const ann::SeerShape &ss = f->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
  assert(ss.hasId(cIn.shapeId));
  const TopoDS_Shape &fs = ss.getOCCTShape(cIn.shapeId);
  assert(fs.ShapeType() == TopAbs_FACE);
  BRepAdaptor_Surface sa(TopoDS::Face(fs));
  if (sa.GetType() == GeomAbs_Plane)
  {
    //a single plane is an offset.
    std::shared_ptr<ftr::DatumPlane> dp(new ftr::DatumPlane());
    ftr::Pick pick = tls::convertToPick(cIn, ss);
    pick.shapeHistory = project->getShapeHistory().createDevolveHistory(cIn.shapeId);
    dp->setPicks(ftr::Picks({pick}));
    dp->setDPType(ftr::DatumPlane::DPType::POffset);
    dp->setAutoSize(true);
    project->addFeature(dp);
    project->connectInsert(f->getId(), dp->getId(), ftr::InputType{ftr::InputType::create});
    node->sendBlocked(msg::buildStatusMessage("Offset datum plane added", 2.0));
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return true;
  }
  return false;
}

bool DatumPlane::attemptCenter(const std::vector<slc::Container> &csIn)
{
  int pc = 0;
  ftr::Picks picks;
  std::vector<const ftr::Base*> parents;
  for (const auto &s : csIn)
  {
    const ftr::Base *f = project->findFeature(s.featureId);
    if (f->getType() == ftr::Type::DatumPlane)
    {
      pc++;
      parents.push_back(f);
    }
    else
    {
      if (!f->hasAnnex(ann::Type::SeerShape))
        continue;
      const ann::SeerShape &ss = f->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
      assert(ss.hasId(s.shapeId));
      const TopoDS_Shape &fs = ss.getOCCTShape(s.shapeId);
      if (fs.ShapeType() != TopAbs_FACE)
        continue;
      BRepAdaptor_Surface sa(TopoDS::Face(fs));
      if (sa.GetType() == GeomAbs_Plane)
      {
        pc++;
        ftr::Pick pick = tls::convertToPick(s, ss);
        pick.shapeHistory = project->getShapeHistory().createDevolveHistory(s.shapeId);
        picks.push_back(pick);
        parents.push_back(f);
      }
    }
  }
  if (pc == 2)
  {
    std::shared_ptr<ftr::DatumPlane> dp(new ftr::DatumPlane());
    dp->setDPType(ftr::DatumPlane::DPType::PCenter);
    dp->setPicks(picks);
    dp->setAutoSize(true);
    project->addFeature(dp);
    gu::uniquefy(parents);
    for (const auto &p : parents)
      project->connectInsert(p->getId(), dp->getId(), ftr::InputType{ftr::InputType::create});
    node->sendBlocked(msg::buildStatusMessage("Center datum plane added", 2.0));
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return true;
  }
  return false;
}

bool DatumPlane::attemptAxisAngle(const std::vector<slc::Container> &csIn)
{
  auto buildAxisPick = [&](const slc::Container &cIn) -> boost::optional<ftr::Pick>
  {
    const ftr::Base *f = project->findFeature(cIn.featureId);
    if (cIn.shapeId.is_nil())
      return boost::none;
    if (!f->hasAnnex(ann::Type::SeerShape))
      return boost::none;
    const ann::SeerShape &ss = f->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
    const TopoDS_Shape &shape = ss.getOCCTShape(cIn.shapeId);
    auto glean = occt::gleanAxis(shape);
    if (!glean.second)
      return boost::none;
    
    ftr::Pick p = tls::convertToPick(cIn, project->findFeature(cIn.featureId)->getAnnex<ann::SeerShape>(ann::Type::SeerShape));
    p.shapeHistory = project->getShapeHistory().createDevolveHistory(cIn.shapeId);
    p.tag = ftr::DatumPlane::rotationAxis;
    return p;
  };
  
  typedef boost::optional<std::tuple<ftr::InputType, const ftr::Base*>> Group;
  Group axis, plane;
  ftr::Picks aaPicks;
  for (const auto &s : csIn)
  {
    const ftr::Base *f = project->findFeature(s.featureId);
    if (f->getType() == ftr::Type::DatumAxis)
    {
      axis = std::make_tuple
      (
        ftr::InputType{ftr::InputType::create, ftr::DatumPlane::rotationAxis}
        , f
      );
    }
    else if (f->getType() == ftr::Type::DatumPlane)
    {
      plane = std::make_tuple
      (
        ftr::InputType{ftr::InputType::create, ftr::DatumPlane::plane1}
        , f
      );
    }
    else
    {
      auto p = buildAxisPick(s);
      if (p)
      {
        aaPicks.push_back(p.get());
        axis = std::make_tuple
        (
          ftr::InputType{ftr::InputType::create, ftr::DatumPlane::rotationAxis}
          , f
        );
      }
      else
      {
        auto pp = buildPlanePick(s, f);
        if (pp)
        {
          pp.get().shapeHistory = project->getShapeHistory().createDevolveHistory(s.shapeId);
          pp.get().tag = ftr::DatumPlane::plane1;
          aaPicks.push_back(pp.get());
          plane = std::make_tuple
          (
            ftr::InputType{ftr::InputType::create, ftr::DatumPlane::plane1}
            , f
          );
        }
      }
    }
  }
  
  if (axis && plane)
  {
    std::shared_ptr<ftr::DatumPlane> dp(new ftr::DatumPlane());
    dp->setDPType(ftr::DatumPlane::DPType::AAngleP);
    
    dp->setPicks(aaPicks);
    dp->setAutoSize(true);
    project->addFeature(dp);
    project->connectInsert(std::get<1>(axis.get())->getId(), dp->getId(), std::get<0>(axis.get()));
    project->connectInsert(std::get<1>(plane.get())->getId(), dp->getId(), std::get<0>(plane.get()));
    node->sendBlocked(msg::buildStatusMessage("Axis angle datum plane added", 2.0));
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return true;
  }
  return false;
}

bool DatumPlane::attemptAverage3P(const std::vector<slc::Container> &csIn)
{
  typedef boost::optional<std::tuple<ftr::InputType, const ftr::Base*>> Group;
  Group plane1, plane2, plane3;
  ftr::Picks picks;
  for (const auto &s : csIn)
  {
    const ftr::Base *f = project->findFeature(s.featureId);
    
    auto createGroup = [&](const char *tag) -> Group
    {
      return std::make_tuple (ftr::InputType{ftr::InputType::create, tag} , f);
    };
    
    auto assign = [&]() -> const char*
    {
      if (!plane1)
      {
        plane1 = createGroup(ftr::DatumPlane::plane1);
        return ftr::DatumPlane::plane1;
      }
      else if (!plane2)
      {
        plane2 = createGroup(ftr::DatumPlane::plane2);
        return ftr::DatumPlane::plane2;
      }
      plane3 = createGroup(ftr::DatumPlane::plane3);
      return ftr::DatumPlane::plane3;
    };
    
    if (f->getType() == ftr::Type::DatumPlane)
    {
      assign();
    }
    else
    {
      auto pp = buildPlanePick(s, f);
      if (pp)
      {
        const char *tag = assign();
        pp.get().shapeHistory = project->getShapeHistory().createDevolveHistory(s.shapeId);
        pp.get().tag = tag;
        picks.push_back(pp.get());
      }
    }
  }
  
  if (plane1 && plane2 && plane3)
  {
    std::shared_ptr<ftr::DatumPlane> dp(new ftr::DatumPlane());
    dp->setDPType(ftr::DatumPlane::DPType::Average3P);
    
    dp->setPicks(picks);
    dp->setAutoSize(true);
    project->addFeature(dp);
    project->connectInsert(std::get<1>(plane1.get())->getId(), dp->getId(), std::get<0>(plane1.get()));
    project->connectInsert(std::get<1>(plane2.get())->getId(), dp->getId(), std::get<0>(plane2.get()));
    project->connectInsert(std::get<1>(plane3.get())->getId(), dp->getId(), std::get<0>(plane3.get()));
    node->sendBlocked(msg::buildStatusMessage("Average datum plane added", 2.0));
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return true;
  }
  return false;
}

bool DatumPlane::attemptThrough3P(const std::vector<slc::Container> &csIn)
{
  typedef boost::optional<std::tuple<ftr::InputType, const ftr::Base*>> Group;
  Group point1, point2, point3;
  ftr::Picks picks;
  for (const auto &s : csIn)
  {
    const ftr::Base *f = project->findFeature(s.featureId);
    if (!f->hasAnnex(ann::Type::SeerShape))
      continue;
    
    auto createGroup = [&](const char *tag) -> Group
    {
      return std::make_tuple (ftr::InputType{ftr::InputType::create, tag} , f);
    };
    
    auto assign = [&]() -> const char*
    {
      if (!point1)
      {
        point1 = createGroup(ftr::DatumPlane::point1);
        return ftr::DatumPlane::point1;
      }
      else if (!point2)
      {
        point2 = createGroup(ftr::DatumPlane::point2);
        return ftr::DatumPlane::point2;
      }
      point3 = createGroup(ftr::DatumPlane::point3);
      return ftr::DatumPlane::point3;
    };

    ftr::Pick p = tls::convertToPick(s, project->findFeature(s.featureId)->getAnnex<ann::SeerShape>(ann::Type::SeerShape));
    p.shapeHistory = project->getShapeHistory().createDevolveHistory(s.shapeId);
    p.tag = assign();
    picks.push_back(p);
  }
  
  if (point1 && point2 && point3 && picks.size() == 3)
  {
    std::shared_ptr<ftr::DatumPlane> dp(new ftr::DatumPlane());
    dp->setDPType(ftr::DatumPlane::DPType::Through3P);
    
    dp->setPicks(picks);
    dp->setAutoSize(true);
    project->addFeature(dp);
    project->connectInsert(std::get<1>(point1.get())->getId(), dp->getId(), std::get<0>(point1.get()));
    project->connectInsert(std::get<1>(point2.get())->getId(), dp->getId(), std::get<0>(point2.get()));
    project->connectInsert(std::get<1>(point3.get())->getId(), dp->getId(), std::get<0>(point3.get()));
    node->sendBlocked(msg::buildStatusMessage("Average datum plane added", 2.0));
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return true;
  }
  return false;
}

void DatumPlane::go()
{
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
    return;
  }
  if (cs.size() == 1)
  {
    if (attemptOffset(cs.front()))
    {
      node->sendBlocked(msg::buildStatusMessage("Offset datum plane added", 2.0));
      node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
      return;
    }
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
