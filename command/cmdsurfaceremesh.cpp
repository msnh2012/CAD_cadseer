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

#include "application/appmainwindow.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrsurfaceremesh.h"
#include "command/cmdsurfaceremesh.h"

using namespace cmd;

SurfaceReMesh::SurfaceReMesh() : Base() {}

SurfaceReMesh::~SurfaceReMesh()
{
//   if (dialog)
//     dialog->deleteLater();
}

std::string SurfaceReMesh::getStatusMessage()
{
  return QObject::tr("Select geometry for SurfaceReMesh feature").toStdString();
}

void SurfaceReMesh::activate()
{
  isActive = true;
  
  if (firstRun)
  {
    firstRun = false;
    go();
  }
  sendDone(); //while no dialog
  
//   if (dialog)
//   {
//     dialog->show();
//     dialog->raise();
//     dialog->activateWindow();
//   }
//   else sendDone();
}

void SurfaceReMesh::deactivate()
{
//   if (dialog)
//     dialog->hide();
  isActive = false;
}

void SurfaceReMesh::go()
{
  assert(project);
  shouldUpdate = false;
  
  auto goDialog = [&]()
  {
    node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));

//     auto feature = std::make_shared<ftr::SurfaceReMesh>();
//     project->addFeature(feature);
//     dialog = new dlg::SurfaceReMesh(feature.get(), mainWindow);
// 
//     node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
    node->sendBlocked(msg::buildStatusMessage("invalid pre selection", 2.0));
  };
  
  const slc::Containers &cs = eventHandler->getSelections();
  if (cs.size() != 1)
  {
    goDialog();
    return;
  }
  
  const ftr::Base *bf = project->findFeature(cs.front().featureId);
  if (!bf || !bf->hasAnnex(ann::Type::SurfaceMesh))
  {
    goDialog();
    return;
  }
  
  auto f = std::make_shared<ftr::SurfaceReMesh>();
  project->addFeature(f);
  project->connectInsert(cs.front().featureId, f->getId(), {ftr::InputType::target});
  
  node->sendBlocked(msg::buildHideThreeD(cs.front().featureId));
  node->sendBlocked(msg::buildHideOverlay(cs.front().featureId));
  node->sendBlocked(msg::buildStatusMessage("SurfaceReMesh created", 2.0));
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  shouldUpdate = true;
}

/*
SurfaceReMeshEdit::SurfaceReMeshEdit(ftr::Base *in) : Base()
{
  //command manager edit dispatcher dispatches on ftr::type, so we know the type of 'in'
  feature = dynamic_cast<ftr::SurfaceReMesh*>(in);
  assert(feature);
  shouldUpdate = false; //dialog controls.
}

SurfaceReMeshEdit::~SurfaceReMeshEdit()
{
  if (dialog)
    dialog->deleteLater();
}

std::string SurfaceReMeshEdit::getStatusMessage()
{
  return QObject::tr("Editing SurfaceReMesh").toStdString();
}

void SurfaceReMeshEdit::activate()
{
  isActive = true;
  if (!dialog)
  {
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    dialog = new dlg::SurfaceReMesh(feature, mainWindow, true);
  }

  dialog->show();
  dialog->raise();
  dialog->activateWindow();
}

void SurfaceReMeshEdit::deactivate()
{
  dialog->hide();
  isActive = false;
}
*/
