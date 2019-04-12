/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2019 Thomas S. Anderson blobfish.at.gmx.com
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

#include <cassert>

#include <TopoDS.hxx>

#include "tools/featuretools.h"
// #include "application/mainwindow.h"
// #include "application/application.h"
#include "project/project.h"
#include "message/node.h"
#include "selection/eventhandler.h"
// #include "parameter/parameter.h"
// #include "dialogs/parameter.h"
// #include "dialogs/transitioncurve.h"
#include "annex/seershape.h"
#include "feature/inputtype.h"
#include "feature/ftrtransitioncurve.h"
#include "command/cmdtransitioncurve.h"

using namespace cmd;

TransitionCurve::TransitionCurve() : Base() {}

TransitionCurve::~TransitionCurve() {}

std::string TransitionCurve::getStatusMessage()
{
  return QObject::tr("Select geometry for TransitionCurve feature").toStdString();
}

void TransitionCurve::activate()
{
  isActive = true;
  go();
  sendDone();
}

void TransitionCurve::deactivate()
{
  isActive = false;
}

void TransitionCurve::go()
{
  const slc::Containers &cs = eventHandler->getSelections();
  if (cs.size() != 2)
  {
    node->sendBlocked(msg::buildStatusMessage("Incorrect selection for TransitionCurve", 2.0));
    shouldUpdate = false;
    return;
  }
  
  auto isValidPoint = [](const slc::Container &cIn) -> bool
  {
    if
    (
      cIn.selectionType == slc::Type::StartPoint
      || cIn.selectionType == slc::Type::EndPoint
      || cIn.selectionType == slc::Type::MidPoint
      || cIn.selectionType == slc::Type::QuadrantPoint
      || cIn.selectionType == slc::Type::NearestPoint
    )
      return true;
      
    return false;
  };
  if (!isValidPoint(cs.front()))
  {
    node->sendBlocked(msg::buildStatusMessage("Incorrect first selection for TransitionCurve", 2.0));
    shouldUpdate = false;
    return;
  }
  if (!isValidPoint(cs.back()))
  {
    node->sendBlocked(msg::buildStatusMessage("Incorrect last selection for TransitionCurve", 2.0));
    shouldUpdate = false;
    return;
  }
  
  const ftr::Base *bf0 = project->findFeature(cs.front().featureId);
  if (!bf0 || !bf0->hasAnnex(ann::Type::SeerShape))
  {
    node->sendBlocked(msg::buildStatusMessage("Invalid first selection for TransitionCurve", 2.0));
    shouldUpdate = false;
    return;
  }
  const ann::SeerShape &ss0 = bf0->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
  
  ftr::Base const *bf1 = project->findFeature(cs.back().featureId);
  if (!bf1 || !bf1->hasAnnex(ann::Type::SeerShape))
  {
    node->sendBlocked(msg::buildStatusMessage("Invalid second selection for TransitionCurve", 2.0));
    shouldUpdate = false;
    return;
  }
  const ann::SeerShape &ss1 = bf1->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
  
  ftr::Picks picks;
  picks.push_back(tls::convertToPick(cs.front(), ss0));
  if (!cs.front().shapeId.is_nil())
    picks.back().shapeHistory = project->getShapeHistory().createDevolveHistory(cs.front().shapeId);
  picks.push_back(tls::convertToPick(cs.back(), ss1));
  if (!cs.back().shapeId.is_nil())
    picks.back().shapeHistory = project->getShapeHistory().createDevolveHistory(cs.back().shapeId);
  
  assert(ss0.hasId(cs.front().shapeId));
  assert(ss0.findShape(cs.front().shapeId).ShapeType() == TopAbs_EDGE);
  assert(ss1.hasId(cs.back().shapeId));
  assert(ss1.findShape(cs.back().shapeId).ShapeType() == TopAbs_EDGE);
  TopoDS_Edge e0 = TopoDS::Edge(ss0.getOCCTShape(cs.front().shapeId));
  TopoDS_Edge e1 = TopoDS::Edge(ss1.getOCCTShape(cs.back().shapeId));
  
  auto f = std::make_shared<ftr::TransitionCurve>();
  f->setPicks(picks);
  f->gleanDirection0(e0);
  f->gleanDirection1(e1);
  project->addFeature(f);
  project->connectInsert(bf0->getId(), f->getId(), ftr::InputType{ftr::TransitionCurve::pickZero});
  project->connectInsert(bf1->getId(), f->getId(), ftr::InputType{ftr::TransitionCurve::pickOne});
  
  node->sendBlocked(msg::buildStatusMessage("TransitionCurve created", 2.0));
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
}
