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
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrdatumplane.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvdatumplane.h"
#include "command/cmddatumplane.h"

using namespace cmd;
using boost::uuids::uuid;

DatumPlane::DatumPlane()
: Base()
, leafManager()
{
  feature = new ftr::DatumPlane::Feature();
  project->addFeature(std::unique_ptr<ftr::DatumPlane::Feature>(feature));
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  isEdit = false;
  isFirstRun = true;
}

DatumPlane::DatumPlane(ftr::Base *fIn)
: Base()
, leafManager(fIn)
{
  feature = dynamic_cast<ftr::DatumPlane::Feature*>(fIn);
  assert(feature);
  viewBase = std::make_unique<cmv::DatumPlane>(this);
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  isEdit = true;
  isFirstRun = false;
}

DatumPlane::~DatumPlane() {}

std::string DatumPlane::getStatusMessage()
{
  return QObject::tr("Double Click Parameter To Edit Or Drop Expression").toStdString();
}

void DatumPlane::activate()
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

void DatumPlane::deactivate()
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

void DatumPlane::setToConstant()
{
  assert(isActive);
  project->clearAllInputs(feature->getId());
}

void DatumPlane::setLinked(const slc::Messages &msIn)
{
  assert(isActive);
  project->clearAllInputs(feature->getId());
  if (msIn.empty())
    return;
  
  const ftr::Base *f = project->findFeature(msIn.front().featureId);
  ftr::Pick pick = tls::convertToPick(msIn.front(), *f, project->getShapeHistory());
  pick.tag = ftr::InputType::linkCSys;
  
  auto ps = feature->getParameters(prm::Tags::CSysLinked);
  assert(ps.size() == 1);
  ps.front()->setValue(pick);
  project->connect(msIn.front().featureId, feature->getId(), {ftr::InputType::linkCSys});
}

void DatumPlane::setToPlanarOffset(const std::vector<slc::Message> &msIn)
{
  assert(isActive);
  project->clearAllInputs(feature->getId());
  if (msIn.size() != 1)
    return;
  assert(project->hasFeature(msIn.front().featureId));
  
  const ftr::Base *f = project->findFeature(msIn.front().featureId);
  ftr::Pick pick = tls::convertToPick(msIn.front(), *f, project->getShapeHistory());
  pick.tag = ftr::DatumPlane::InputTags::plane;
  
  auto ps = feature->getParameters(ftr::DatumPlane::PrmTags::offsetPicks);
  assert(ps.size() == 1);
  ps.front()->setValue(pick);
  project->connectInsert(f->getId(), feature->getId(), {pick.tag});
}

void DatumPlane::setToPlanarCenter(const std::vector<slc::Message> &msIn)
{
  assert(isActive);
  
  project->clearAllInputs(feature->getId());
  if (msIn.size() != 2)
    return;
  assert(project->hasFeature(msIn.front().featureId));
  assert(project->hasFeature(msIn.back().featureId));
  
  ftr::Picks picks;
  for (const auto &mIn : msIn)
  {
    const ftr::Base *f = project->findFeature(mIn.featureId);
    ftr::Pick pick = tls::convertToPick(mIn, *f, project->getShapeHistory());
    pick.tag = indexTag(ftr::DatumPlane::InputTags::center, picks.size());
    picks.push_back(pick);
    project->connectInsert(f->getId(), feature->getId(), {pick.tag});
  };
  auto ps = feature->getParameters(ftr::DatumPlane::PrmTags::centerPicks);
  assert(ps.size() == 1);
  ps.front()->setValue(picks);
}

void DatumPlane::setToAxisAngle(const std::vector<slc::Message> &msIn)
{
  assert(isActive);
  project->clearAllInputs(feature->getId());
  if (msIn.size() != 2)
    return;
  assert(project->hasFeature(msIn.front().featureId));
  assert(project->hasFeature(msIn.back().featureId));
  
  //understood that first selection message is axis and second is plane.
  ftr::Picks picks;
  for (const auto &mIn : msIn)
  {
    const ftr::Base *f = project->findFeature(mIn.featureId);
    ftr::Pick pick = tls::convertToPick(mIn, *f, project->getShapeHistory());
    pick.tag = (picks.empty()) ? ftr::DatumPlane::InputTags::axis : ftr::DatumPlane::InputTags::plane;
    picks.push_back(pick);
    project->connectInsert(f->getId(), feature->getId(), {pick.tag});
  };
  auto ps = feature->getParameters(ftr::DatumPlane::PrmTags::axisAnglePicks);
  assert(ps.size() == 1);
  ps.front()->setValue(picks);
}

void DatumPlane::setToAverage3Plane(const std::vector<slc::Message> &msIn)
{
  assert(isActive);
  project->clearAllInputs(feature->getId());
  if (msIn.size() != 3)
    return;
  assert(project->hasFeature(msIn.at(0).featureId));
  assert(project->hasFeature(msIn.at(1).featureId));
  assert(project->hasFeature(msIn.at(2).featureId));
  
  ftr::Picks picks;
  for (const auto &mIn : msIn)
  {
    const ftr::Base *f = project->findFeature(mIn.featureId);
    ftr::Pick pick = tls::convertToPick(mIn, *f, project->getShapeHistory());
    pick.tag = indexTag(ftr::DatumPlane::InputTags::plane, picks.size());
    picks.push_back(pick);
    project->connectInsert(f->getId(), feature->getId(), {pick.tag});
  };
  auto ps = feature->getParameters(ftr::DatumPlane::PrmTags::averagePicks);
  assert(ps.size() == 1);
  ps.front()->setValue(picks);
}

void DatumPlane::setToThrough3Points(const std::vector<slc::Message> &msIn)
{
  assert(isActive);
  project->clearAllInputs(feature->getId());
  if (msIn.size() != 3)
    return;
  assert(project->hasFeature(msIn.at(0).featureId));
  assert(project->hasFeature(msIn.at(1).featureId));
  assert(project->hasFeature(msIn.at(2).featureId));
  
  ftr::Picks picks;
  for (const auto &mIn : msIn)
  {
    const ftr::Base *f = project->findFeature(mIn.featureId);
    ftr::Pick pick = tls::convertToPick(mIn, *f, project->getShapeHistory());
    pick.tag = indexTag(ftr::DatumPlane::InputTags::point, picks.size());
    picks.push_back(pick);
    project->connectInsert(f->getId(), feature->getId(), {pick.tag});
  }
  auto ps = feature->getParameters(ftr::DatumPlane::PrmTags::pointsPicks);
  assert(ps.size() == 1);
  ps.front()->setValue(picks);
}

void DatumPlane::localUpdate()
{
  assert(isActive);
  feature->updateModel(project->getPayload(feature->getId()));
  feature->updateVisual();
  feature->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
}

void DatumPlane::go()
{
  assert(project);
  const slc::Containers &cs = eventHandler->getSelections();
  if (cs.size() == 1)
  {
    if (cs.front().selectionType == slc::Type::Face || cs.front().featureType == ftr::Type::DatumPlane)
    {
      if (attemptOffset(cs.front()))
        return;
    }
    if (slc::isObjectType(cs.front().selectionType))
    {
      if (attemptLink(cs.front()))
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
  auto prms = feature->getParameters(prm::Tags::CSys); assert(prms.size() == 1);
  prms.front()->setValue(app::instance()->getMainWindow()->getViewer()->getCurrentSystem());
  prms = feature->getParameters(prm::Tags::Size); assert(prms.size() == 1);
  prms.front()->setValue(viewer->getDiagonalLength() / 4.0);
  localUpdate();
  node->sendBlocked(msg::buildStatusMessage("Constant datum plane added at current system", 2.0));
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  viewBase = std::make_unique<cmv::DatumPlane>(this);
  return;
}

bool DatumPlane::isPlanarFace(const slc::Container &cIn)
{
  const ftr::Base *f = project->findFeature(cIn.featureId);
  if (!f->hasAnnex(ann::Type::SeerShape))
    return false;
  const ann::SeerShape &ss = f->getAnnex<ann::SeerShape>();
  assert(ss.hasId(cIn.shapeId));
  const TopoDS_Shape &fs = ss.getOCCTShape(cIn.shapeId);
  if (fs.ShapeType() != TopAbs_FACE)
    return false;
  //TODO GeomLib_IsPlanarSurface
  BRepAdaptor_Surface sa(TopoDS::Face(fs));
  if (sa.GetType() == GeomAbs_Plane)
    return true;
  return false;
}

bool DatumPlane::isAxis(const slc::Container &cIn)
{
  const ftr::Base *f = project->findFeature(cIn.featureId);
  if (!f->hasAnnex(ann::Type::SeerShape))
    return false;
  const ann::SeerShape &ss = f->getAnnex<ann::SeerShape>();
  if (ss.isNull())
    return false;
  assert(ss.hasId(cIn.shapeId));
  const TopoDS_Shape &s = ss.getOCCTShape(cIn.shapeId);
  return occt::gleanAxis(s).second;
}

void DatumPlane::setType(int typeIn)
{
  auto typePrms = feature->getParameters(ftr::DatumPlane::PrmTags::datumPlaneType);
  assert(typePrms.size() == 1);
  typePrms.front()->setValue(typeIn);
}

bool DatumPlane::attemptLink(const slc::Container &cIn)
{
  const ftr::Base *f = project->findFeature(cIn.featureId);
  if (!f->getParameters(prm::Tags::CSys).empty())
  {
    setType(static_cast<int>(ftr::DatumPlane::DPType::Linked));
    setLinked({slc::EventHandler::containerToMessage(cIn)});
    auto prms = feature->getParameters(prm::Tags::Size); assert(prms.size() == 1);
    prms.front()->setValue(viewer->getDiagonalLength() / 4.0);
    return true;
  }
  return false;
}

bool DatumPlane::attemptOffset(const slc::Container &cIn)
{
  //we only get here if the single selection is a face or a datum plane.
  //Datum will always work, so we only need to check for a planar face.
  
  std::vector<slc::Message> msgs;
  msgs.push_back(slc::EventHandler::containerToMessage(cIn));
  auto build = [&]()
  {
    setType(static_cast<int>(ftr::DatumPlane::DPType::Offset));
    setToPlanarOffset(msgs);
    auto prms = feature->getParameters(prm::Tags::AutoSize); assert(prms.size() == 1);
    prms.front()->setValue(true);
    node->sendBlocked(msg::buildStatusMessage("Offset datum plane added", 2.0));
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  };
  
  if (cIn.featureType == ftr::Type::DatumPlane || isPlanarFace(cIn))
  {
    build();
    return true;
  }
  
  return false;
}

bool DatumPlane::attemptCenter(const std::vector<slc::Container> &csIn)
{
  int count = 0;
  for (const auto &s : csIn)
  {
    if (s.featureType == ftr::Type::DatumPlane || isPlanarFace(s))
      count++;
  }
  if (count == 2)
  {
    std::vector<slc::Message> msgs;
    for (const auto &c : csIn)
      msgs.push_back(slc::EventHandler::containerToMessage(c));
    setType(static_cast<int>(ftr::DatumPlane::DPType::Center));
    setToPlanarCenter(msgs);
    auto prms = feature->getParameters(prm::Tags::AutoSize); assert(prms.size() == 1);
    prms.front()->setValue(true);
    node->sendBlocked(msg::buildStatusMessage("Center datum plane added", 2.0));
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return true;
  }
  return false;
}

bool DatumPlane::attemptAxisAngle(const std::vector<slc::Container> &csIn)
{
  boost::optional<slc::Message> axisMsg;
  boost::optional<slc::Message> planeMsg;
  for (const auto &c : csIn)
  {
    slc::Message cm = slc::EventHandler::containerToMessage(c);
    if (!axisMsg && c.featureType == ftr::Type::DatumAxis)
      axisMsg = cm;
    if (!planeMsg && c.featureType == ftr::Type::DatumPlane)
      planeMsg = cm;
    if (!axisMsg && isAxis(c))
      axisMsg = cm;
    if (!planeMsg && isPlanarFace(c))
      planeMsg = cm;
  }
  if (!axisMsg || !planeMsg)
    return false;
  
  std::vector<slc::Message> msgs{axisMsg.get(), planeMsg.get()};
  setType(static_cast<int>(ftr::DatumPlane::DPType::AxisAngle));
  setToAxisAngle(msgs);
  auto prms = feature->getParameters(prm::Tags::AutoSize); assert(prms.size() == 1);
  prms.front()->setValue(true);
  node->sendBlocked(msg::buildStatusMessage("Axis angle datum plane added", 2.0));
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  return true;
}

bool DatumPlane::attemptAverage3P(const std::vector<slc::Container> &csIn)
{
  std::vector<slc::Message> msgs;
  for (const auto &c : csIn)
  {
    if (c.featureType == ftr::Type::DatumPlane || isPlanarFace(c))
      msgs.push_back(slc::EventHandler::containerToMessage(c));
  }
  if (msgs.size() != 3)
    return false;
  setType(static_cast<int>(ftr::DatumPlane::DPType::Average));
  setToAverage3Plane(msgs);
  auto prms = feature->getParameters(prm::Tags::AutoSize); assert(prms.size() == 1);
  prms.front()->setValue(true);
  node->sendBlocked(msg::buildStatusMessage("3 plane average datum plane added", 2.0));
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  return true;
}

bool DatumPlane::attemptThrough3P(const std::vector<slc::Container> &csIn)
{
  std::vector<slc::Message> msgs;
  for (const auto &c : csIn)
  {
    if (c.isPointType())
      msgs.push_back(slc::EventHandler::containerToMessage(c));
  }
  if (msgs.size() != 3)
    return false;
  setType(static_cast<int>(ftr::DatumPlane::DPType::Points));
  setToThrough3Points(msgs);
  auto prms = feature->getParameters(prm::Tags::AutoSize); assert(prms.size() == 1);
  prms.front()->setValue(true);
  node->sendBlocked(msg::buildStatusMessage("Through 3 points datum plane added", 2.0));
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  return true;
}
