/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2020 Thomas S. Anderson blobfish.at.gmx.com
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

#include "tools/featuretools.h"
#include "project/prjproject.h"
#include "annex/annseershape.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "parameter/prmparameter.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrmappcurve.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvmappcurve.h"
#include "command/cmdmappcurve.h"

using namespace cmd;

MapPCurve::MapPCurve()
: Base("cmd::MapPCurve")
, leafManager()
{
  feature = new ftr::MapPCurve::Feature();
  project->addFeature(std::unique_ptr<ftr::MapPCurve::Feature>(feature));
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  isEdit = false;
  isFirstRun = true;
}

MapPCurve::MapPCurve(ftr::Base *fIn)
: Base("cmd::MapPCurve")
, leafManager(fIn)
{
  feature = dynamic_cast<ftr::MapPCurve::Feature*>(fIn);
  assert(feature);
  viewBase = std::make_unique<cmv::MapPCurve>(this);
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  isEdit = true;
  isFirstRun = false;
}

MapPCurve::~MapPCurve() = default;

std::string MapPCurve::getStatusMessage()
{
  return QObject::tr("Select geometry for mappcurve feature").toStdString();
}

void MapPCurve::activate()
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

void MapPCurve::deactivate()
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

bool MapPCurve::isValidFaceSelection(const slc::Message &mIn)
{
  if (mIn.type == slc::Type::Face)
    return true;
  if (slc::isObjectType(mIn.type))
  {
    const ftr::Base *lf = project->findFeature(mIn.featureId);
    if (lf->hasAnnex(ann::Type::SeerShape) && !lf->getAnnex<ann::SeerShape>().isNull())
      return true;
  }
  
  return false;
}

bool MapPCurve::isValidEdgeSelection(const slc::Message &mIn)
{
  if (mIn.type == slc::Type::Edge || mIn.type == slc::Type::Wire)
    return true;
  if (slc::isObjectType(mIn.type))
  {
    if (mIn.featureType == ftr::Type::Sketch)
      return true;
    
    const ftr::Base *lf = project->findFeature(mIn.featureId);
    if (lf->hasAnnex(ann::Type::SeerShape) && !lf->getAnnex<ann::SeerShape>().isNull())
      return true;
  }
  
  return false;
}

//assumes messages have been validated.
void MapPCurve::setSelections(const std::vector<slc::Message> &faces, const std::vector<slc::Message> &edges)
{
  assert(isActive);
  
  project->clearAllInputs(feature->getId());
  auto *facePick = feature->getParameter(ftr::MapPCurve::Tags::facePick);
  auto *edgePicks = feature->getParameter(ftr::MapPCurve::Tags::edgePicks);
  facePick->setValue(ftr::Picks());
  edgePicks->setValue(ftr::Picks());
  if (faces.empty() || edges.empty())
    return;
  
  ftr::Pick face = tls::convertToPick(faces.front(), *project->findFeature(faces.front().featureId), project->getShapeHistory());
  face.tag = indexTag(ftr::MapPCurve::Tags::facePick, 0);
  facePick->setValue(face);
  project->connect(faces.front().featureId, feature->getId(), {face.tag});

  ftr::Picks te;
  for (const auto &m : edges)
  {
    const ftr::Base *lf = project->findFeature(m.featureId);
    te.push_back(tls::convertToPick(m, *lf, project->getShapeHistory()));
    te.back().tag = indexTag(ftr::MapPCurve::Tags::edgePicks, te.size() - 1);
    project->connect(lf->getId(), feature->getId(), {te.back().tag});
  }
  edgePicks->setValue(te);
}

void MapPCurve::localUpdate()
{
  assert(isActive);
  feature->updateModel(project->getPayload(feature->getId()));
  feature->updateVisual();
  feature->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
}

void MapPCurve::go()
{
  //first pick is face and the rest is edges.
  
  const slc::Containers &cs = eventHandler->getSelections();
  
  boost::optional<slc::Message> fm; //face message
  std::vector<slc::Message> ems; //edge messages
  for (const auto &c : cs)
  {
    auto m = slc::EventHandler::containerToMessage(c);
    if (!fm)
    {
      if (!isValidFaceSelection(m))
        continue;
      fm = m;
      continue;
    }
    if (isValidEdgeSelection(m))
      ems.push_back(m);
  }
  
  if (fm && !ems.empty())
  {
    setSelections({fm.get()}, ems);
    node->sendBlocked(msg::buildStatusMessage("MapPCurve Added", 2.0));
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return;
  }
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  node->sendBlocked(msg::buildShowThreeD(feature->getId()));
  node->sendBlocked(msg::buildShowOverlay(feature->getId()));
  viewBase = std::make_unique<cmv::MapPCurve>(this);
}
