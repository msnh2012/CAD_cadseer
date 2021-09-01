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
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "annex/annseershape.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrtransitioncurve.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvtransitioncurve.h"
#include "command/cmdtransitioncurve.h"

using namespace cmd;

TransitionCurve::TransitionCurve()
: Base("cmd::TransitionCurve")
, leafManager()
{
  feature = new ftr::TransitionCurve();
  project->addFeature(std::unique_ptr<ftr::TransitionCurve>(feature));
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  isEdit = false;
  isFirstRun = true;
  gleanDirection = true;
}

TransitionCurve::TransitionCurve(ftr::Base *fIn)
: Base("cmd::TransitionCurve")
, leafManager(fIn)
{
  feature = dynamic_cast<ftr::TransitionCurve*>(fIn);
  assert(feature);
  viewBase = std::make_unique<cmv::TransitionCurve>(this);
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  isEdit = true;
  isFirstRun = false;
  gleanDirection = false;
}

TransitionCurve::~TransitionCurve() = default;

std::string TransitionCurve::getStatusMessage()
{
  return QObject::tr("Select geometry for TransitionCurve feature").toStdString();
}

void TransitionCurve::activate()
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

void TransitionCurve::deactivate()
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

bool TransitionCurve::isValidSelection(const slc::Message &mIn)
{
  const ftr::Base *lf = project->findFeature(mIn.featureId);
  if (!lf->hasAnnex(ann::Type::SeerShape) || lf->getAnnex<ann::SeerShape>().isNull())
    return false;
  
  if
  (
    mIn.type == slc::Type::StartPoint
    || mIn.type == slc::Type::EndPoint
    || mIn.type == slc::Type::MidPoint
    || mIn.type == slc::Type::QuadrantPoint
    || mIn.type == slc::Type::NearestPoint
  )
    return true;
    
  return false;
}

void TransitionCurve::setSelections(const std::vector<slc::Message> &targets)
{
  assert(isActive);
  
  auto reset = [&]()
  {
    project->clearAllInputs(feature->getId());
    feature->setPicks(ftr::Picks());
  };
  reset();
  
  if (targets.size() != 2)
    return;
  
  ftr::Picks freshPicks;
  for (const auto &m : targets)
  {
    if (!isValidSelection(m))
      continue;

    const ftr::Base *lf = project->findFeature(m.featureId);
    freshPicks.push_back(tls::convertToPick(m, *lf, project->getShapeHistory()));
    freshPicks.back().tag = ftr::InputType::createIndexedTag(ftr::InputType::target, freshPicks.size() - 1);
    //conversion to pick will resolve shape history to vertices for end points.
    //we can't have that because transition needs edge also.
    //so here we will hack back in the edge history.
    //are we trying to do too much with one pick?
    if (m.type == slc::Type::StartPoint || m.type == slc::Type::EndPoint)
      freshPicks.back().shapeHistory = project->getShapeHistory().createDevolveHistory(m.shapeId);
    
    project->connect(lf->getId(), feature->getId(), {freshPicks.back().tag});
  }
  if (freshPicks.size() != 2)
  {
    reset();
    return;
  }
  
  feature->setPicks(freshPicks);
  if (gleanDirection)
  {
    gleanDirection = false;
     auto getEdge = [&](const slc::Message &mIn) -> TopoDS_Edge
    {
      const ftr::Base *lf = project->findFeature(mIn.featureId);
      const ann::SeerShape &ss = lf->getAnnex<ann::SeerShape>();
      return TopoDS::Edge(ss.getOCCTShape(mIn.shapeId));
    };
    feature->gleanDirection0(getEdge(targets.front()));
    feature->gleanDirection1(getEdge(targets.back()));
  }
}

void TransitionCurve::localUpdate()
{
  assert(isActive);
  feature->updateModel(project->getPayload(feature->getId()));
  feature->updateVisual();
  feature->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
}

void TransitionCurve::go()
{
  const slc::Containers &cs = eventHandler->getSelections();
  
  std::vector<slc::Message> tm;
  for (const auto &c : cs)
  {
    auto m = slc::EventHandler::containerToMessage(c);
    if (isValidSelection(m))
      tm.push_back(m);
  }
  
  if (tm.size() == 2)
  {
    gleanDirection = true;
    setSelections(tm);
    
    node->sendBlocked(msg::buildStatusMessage("Transition Curve Added", 2.0));
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return;
  }
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  node->sendBlocked(msg::buildShowThreeD(feature->getId()));
  node->sendBlocked(msg::buildShowOverlay(feature->getId()));
  viewBase = std::make_unique<cmv::TransitionCurve>(this);
}
