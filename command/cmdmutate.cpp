/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2022 Thomas S. Anderson blobfish.at.gmx.com
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
#include "tools/featuretools.h"
#include "tools/occtools.h"
#include "project/prjproject.h"
#include "viewer/vwrwidget.h"
#include "annex/annseershape.h"
#include "annex/anncsysdragger.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "parameter/prmparameter.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrprimitive.h"
#include "feature/ftrmutate.h"
#include "dialogs/dlgparameter.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvmutate.h"
#include "command/cmdmutate.h"

using namespace cmd;

Mutate::Mutate()
: Base("cmd::Mutate")
, leafManager()
{
  feature = new ftr::Mutate::Feature();
  project->addFeature(std::unique_ptr<ftr::Mutate::Feature>(feature));
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  isEdit = false;
  isFirstRun = true;
}

Mutate::Mutate(ftr::Base *fIn)
: Base("cmd::Mutate")
, leafManager(fIn)
{
  feature = dynamic_cast<ftr::Mutate::Feature*>(fIn);
  assert(feature);
  viewBase = std::make_unique<cmv::Mutate>(this);
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  isEdit = true;
  isFirstRun = false;
}

Mutate::~Mutate() = default;

std::string Mutate::getStatusMessage()
{
  return QObject::tr("Select geometry for mutate feature").toStdString();
}

void Mutate::activate()
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
    if (!isEdit.get())
      node->sendBlocked(msg::buildSelectionFreeze(feature->getId()));
  }
  else
    sendDone();
}

void Mutate::deactivate()
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
    node->sendBlocked(msg::buildSelectionThaw(feature->getId()));
  }
  isActive = false;
}

bool Mutate::isValidShape(const slc::Message &mIn)
{
  if (!slc::isObjectType(mIn.type))
    return false;
    
  const ftr::Base *lf = project->findFeature(mIn.featureId);
  if (!lf->hasAnnex(ann::Type::SeerShape) || lf->getAnnex<ann::SeerShape>().isNull())
    return false;
  
  return true;
}

bool Mutate::isValidCSys(const slc::Message &mIn)
{
  if (!slc::isObjectType(mIn.type))
    return false;
    
  const ftr::Base *lf = project->findFeature(mIn.featureId);
  return !lf->getParameters(prm::Tags::CSysLinked).empty();
}

void Mutate::setSelections(const slc::Messages &targets)
{
  assert(isActive);
  
  project->clearAllInputs(feature->getId());
  auto *shape = feature->getParameter(ftr::Mutate::PrmTags::shape);
  shape->setValue(ftr::Picks());
  
  if (targets.empty())
    return;
  
  const ftr::Base *lf = project->findFeature(targets.front().featureId);
  auto sp = tls::convertToPick(targets.front(), *lf, project->getShapeHistory());
  sp.tag = indexTag(ftr::Mutate::PrmTags::shape, 0);
  project->connect(lf->getId(), feature->getId(), {sp.tag});
  shape->setValue({sp});
  
  if (targets.size() > 1)
  {
    const ftr::Base &parent = *project->findFeature(targets.at(1).featureId);
    auto pick = tls::convertToPick(targets.at(1), parent, project->getShapeHistory());
    pick.tag = indexTag(prm::Tags::CSysLinked, 0);
    feature->getParameter(prm::Tags::CSysLinked)->setValue({pick});
    project->connectInsert(parent.getId(), feature->getId(), {pick.tag, ftr::InputType::sever});
  }
}

//only for initial setting. no linking.
void Mutate::setCsys(const slc::Message &tm)
{
  ftr::Base *target = project->findFeature(tm.featureId);
  if (!target)
    return;
  
  auto goSet = [&](const osg::Matrixd &mIn)
  {
    feature->getParameter(prm::Tags::CSys)->setValue(mIn);
    feature->getAnnex<ann::CSysDragger>(ann::Type::CSysDragger).draggerUpdate();
  };
  
  //try to set the csys parameter to target features csys parameter.
  prm::Parameters csysPrms = target->getParameters(prm::Tags::CSys);
  if (!csysPrms.empty())
  {
    goSet(csysPrms.front()->getMatrix());
    return;
  }
  
  //try to set the csys parameter to the center of shape.
  if (target->hasAnnex(ann::Type::SeerShape))
  {
    const auto &targetShape = target->getAnnex<ann::SeerShape>().getRootOCCTShape();
    if (!targetShape.IsNull())
    {
      occt::BoundingBox bb(targetShape);
      osg::Matrixd m = osg::Matrixd::identity();
      m.setTrans(gu::toOsg(bb.getCenter()));
      goSet(m);
      return;
    }
  }
  
  //if all else fails, set to the current system.
  goSet(viewer->getCurrentSystem());
}

void Mutate::localUpdate()
{
  assert(isActive);
  feature->updateModel(project->getPayload(feature->getId()));
  feature->updateVisual();
  feature->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
}

void Mutate::go()
{
  const slc::Containers &cs = eventHandler->getSelections();
  
  std::optional<slc::Message> shape;
  std::optional<slc::Message> csys;
  for (const auto &c : cs)
  {
    auto m = slc::EventHandler::containerToMessage(c);
    if (!shape && isValidShape(m))
    {
      shape = m;
      continue;
    }
    if (shape && !csys && isValidCSys(m))
      csys = m;
  }
  
  if (shape)
  {
    std::vector<slc::Message> tm;
    tm.push_back(*shape);
    if (csys)
    {
      tm.push_back(*csys);
      feature->getParameter(prm::Tags::CSysType)->setValue(ftr::Primitive::Linked);
    }
    else
      setCsys(tm.front());
    setSelections(tm);
    node->sendBlocked(msg::buildStatusMessage("Mutate Added", 2.0));
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    node->sendBlocked(msg::buildHideThreeD(shape->featureId));
    node->sendBlocked(msg::buildHideOverlay(shape->featureId));
    dlg::Parameter *pDialog = new dlg::Parameter(feature->getParameter(prm::Tags::Scale), feature->getId());
    pDialog->show();
    pDialog->raise();
    pDialog->activateWindow();
    return;
  }
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  node->sendBlocked(msg::buildShowThreeD(feature->getId()));
  node->sendBlocked(msg::buildShowOverlay(feature->getId()));
  viewBase = std::make_unique<cmv::Mutate>(this);
}
