/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2019  Thomas S. Anderson blobfish.at.gmx.com
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

#include "message/msgnode.h"
#include "application/appmainwindow.h"
#include "project/prjproject.h"
#include "annex/annseershape.h"
#include "selection/slceventhandler.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvsurfacemesh.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrsurfacemesh.h"
#include "command/cmdsurfacemesh.h"

using namespace cmd;

SurfaceMesh::SurfaceMesh()
: Base("cmd::SurfaceMesh")
, leafManager()
{
  auto nf = std::make_shared<ftr::SurfaceMesh>();
  project->addFeature(nf);
  feature = nf.get();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  isEdit = false;
  isFirstRun = true;
}

SurfaceMesh::SurfaceMesh(ftr::Base *fIn)
: Base("cmd::SurfaceMesh")
, leafManager(fIn)
{
  feature = dynamic_cast<ftr::SurfaceMesh*>(fIn);
  assert(feature);
  viewBase = std::make_unique<cmv::SurfaceMesh>(this);
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  isEdit = true;
  isFirstRun = false;
}

SurfaceMesh::~SurfaceMesh() = default;

std::string SurfaceMesh::getStatusMessage()
{
  return QObject::tr("Select feature or geometry for surface mesh feature").toStdString();
}

void SurfaceMesh::activate()
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

void SurfaceMesh::deactivate()
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

bool SurfaceMesh::isValidSelection(const slc::Message &mIn)
{
  if (slc::isObjectType(mIn.type))
  {
    const ftr::Base *lf = project->findFeature(mIn.featureId);
    if (lf->hasAnnex(ann::Type::SeerShape) && !lf->getAnnex<ann::SeerShape>().isNull())
      return true;
  }
  
  return false;
}

void SurfaceMesh::setSelection(const slc::Message &mIn)
{
  assert(isActive);
  
  project->clearAllInputs(feature->getId());
  if (isValidSelection(mIn))
    project->connect(mIn.featureId, feature->getId(), {ftr::InputType::target});
}

void SurfaceMesh::localUpdate()
{
  assert(isActive);
  feature->updateModel(project->getPayload(feature->getId()));
  feature->updateVisual();
  feature->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
}

void SurfaceMesh::go()
{
  const slc::Containers &cs = eventHandler->getSelections();
  
  boost::optional<slc::Message> target;
  for (const auto &c : cs)
  {
    auto m = slc::EventHandler::containerToMessage(c);
    if (isValidSelection(m))
    {
      target = m;
      break;
    }
  }
  
  if (target)
  {
    setSelection(target.get());
    node->sendBlocked(msg::buildStatusMessage("SurfaceMesh Added", 2.0));
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return;
  }
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  node->sendBlocked(msg::buildShowThreeD(feature->getId()));
  node->sendBlocked(msg::buildShowOverlay(feature->getId()));
  viewBase = std::make_unique<cmv::SurfaceMesh>(this);
}
