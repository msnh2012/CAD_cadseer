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

#include <TopoDS.hxx>
#include <BRepAdaptor_Surface.hxx>

#include <globalutilities.h>
#include <application/application.h>
#include <application/mainwindow.h>
#include <viewer/widget.h>
#include <message/observer.h>
#include <selection/eventhandler.h>
#include <project/project.h>
#include <annex/seershape.h>
#include <tools/featuretools.h>
#include <tools/occtools.h>
#include <feature/datumplane.h>
#include <command/datumplane.h>

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

void DatumPlane::go()
{
  assert(project);
  
  const slc::Containers &cs = eventHandler->getSelections();
  if (cs.size() == 1 && cs.front().selectionType == slc::Type::Face)
  {
    const ftr::Base *f = project->findFeature(cs.front().featureId);
    assert(f->hasAnnex(ann::Type::SeerShape));
    const ann::SeerShape &ss = f->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
    assert(ss.hasId(cs.front().shapeId));
    const TopoDS_Shape &fs = ss.getOCCTShape(cs.front().shapeId);
    
    BRepAdaptor_Surface sa(TopoDS::Face(fs));
    if (sa.GetType() == GeomAbs_Plane)
    {
      //a single plane is an offset.
      std::shared_ptr<ftr::DatumPlane> dp(new ftr::DatumPlane());
      ftr::Pick pick = tls::convertToPick(cs.front(), ss);
      pick.shapeHistory = project->getShapeHistory().createDevolveHistory(cs.front().shapeId);
      dp->setPicks(ftr::Picks({pick}));
      dp->setDPType(ftr::DatumPlane::DPType::POffset);
      dp->setAutoSize(true);
      project->addFeature(dp);
      project->connectInsert(f->getId(), dp->getId(), ftr::InputType{ftr::InputType::create});
    }
  }
  
  if (cs.size() == 1 && cs.front().selectionType == slc::Type::Object)
  {
    const ftr::Base *f = project->findFeature(cs.front().featureId);
    if (f->getType() == ftr::Type::DatumPlane)
    {
      //offset from another plane.
      std::shared_ptr<ftr::DatumPlane> dp(new ftr::DatumPlane());
      dp->setDPType(ftr::DatumPlane::DPType::POffset);
      dp->setAutoSize(true);
      project->addFeature(dp);
      project->connectInsert(f->getId(), dp->getId(), ftr::InputType{ftr::InputType::create});
    }
  }
  
  if (cs.size() == 2)
  {
    //count planars and if we have 2 build center.
    int pc = 0;
    ftr::Picks picks;
    std::vector<const ftr::Base*> parents;
    for (const auto &s : cs)
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
    }
    else //don't have 2 planars. lets try axis and angle.
    {
      auto isAxis = [&](const slc::Container &cIn) -> bool
      {
        const ftr::Base *f = project->findFeature(cIn.featureId);
        if (f->getType() == ftr::Type::DatumAxis)
          return true;
        if (cIn.shapeId.is_nil())
          return false;
        if (!f->hasAnnex(ann::Type::SeerShape))
          return false;
        const ann::SeerShape &ss = f->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
        const TopoDS_Shape &shape = ss.getOCCTShape(cIn.shapeId);
        auto glean = occt::gleanAxis(shape);
        if (!glean.second)
          return false;
        return true;
      };
      
      auto isPlane = [&](const slc::Container &cIn) -> bool
      {
        const ftr::Base *f = project->findFeature(cIn.featureId);
        if (f->getType() == ftr::Type::DatumPlane)
          return true;
        if (cIn.shapeId.is_nil())
          return false;
        if (!f->hasAnnex(ann::Type::SeerShape))
          return false;
        const ann::SeerShape &ss = f->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
        const TopoDS_Shape &shape = ss.getOCCTShape(cIn.shapeId);
        if (shape.ShapeType() != TopAbs_FACE)
          return false;
        BRepAdaptor_Surface adaptor(TopoDS::Face(shape));
        if (adaptor.GetType() != GeomAbs_Plane)
          return false;
        return true;
      };
      
      if (!cs.front().shapeId.is_nil())
      {
        ftr::Pick p = tls::convertToPick(cs.front(), project->findFeature(cs.front().featureId)->getAnnex<ann::SeerShape>(ann::Type::SeerShape));
        p.shapeHistory = project->getShapeHistory().createDevolveHistory(cs.front().shapeId);
        if (isAxis(cs.front()))
          p.tag = ftr::DatumPlane::rotationAxis;
        if (isPlane(cs.front()))
          p.tag = ftr::DatumPlane::plane;
        picks.push_back(p);
      }
      
      if (!cs.back().shapeId.is_nil())
      {
        ftr::Pick p = tls::convertToPick(cs.back(), project->findFeature(cs.back().featureId)->getAnnex<ann::SeerShape>(ann::Type::SeerShape));
        p.shapeHistory = project->getShapeHistory().createDevolveHistory(cs.back().shapeId);
        if (isAxis(cs.back()))
          p.tag = ftr::DatumPlane::rotationAxis;
        if (isPlane(cs.back()))
          p.tag = ftr::DatumPlane::plane;
        picks.push_back(p);
      }
        
      if (isAxis(cs.front()) && isPlane(cs.back()))
      {
        std::shared_ptr<ftr::DatumPlane> dp(new ftr::DatumPlane());
        dp->setDPType(ftr::DatumPlane::DPType::AAngleP);
        dp->setPicks(picks);
        dp->setAutoSize(true);
        project->addFeature(dp);
        project->connectInsert(cs.front().featureId, dp->getId(), ftr::InputType{ftr::InputType::create, ftr::DatumPlane::rotationAxis});
        project->connectInsert(cs.back().featureId, dp->getId(), ftr::InputType{ftr::InputType::create, ftr::DatumPlane::plane});
      }
      else if (isPlane(cs.front()) && isAxis(cs.back()))
      {
        std::shared_ptr<ftr::DatumPlane> dp(new ftr::DatumPlane());
        dp->setDPType(ftr::DatumPlane::DPType::AAngleP);
        dp->setPicks(picks);
        dp->setAutoSize(true);
        project->addFeature(dp);
        project->connectInsert(cs.front().featureId, dp->getId(), ftr::InputType{ftr::InputType::create, ftr::DatumPlane::plane});
        project->connectInsert(cs.back().featureId, dp->getId(), ftr::InputType{ftr::InputType::create, ftr::DatumPlane::rotationAxis});
      }
    }
  }
  
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
}
