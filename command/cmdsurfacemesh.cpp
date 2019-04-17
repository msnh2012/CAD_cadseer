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
#include "project/prjproject.h"
#include "selection/slceventhandler.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrsurfacemesh.h"
#include "command/cmdsurfacemesh.h"

using namespace cmd;

SurfaceMesh::SurfaceMesh() : Base() {}
SurfaceMesh::~SurfaceMesh() {}

std::string SurfaceMesh::getStatusMessage()
{
  return QObject::tr("Select feature or geometry for surface mesh feature").toStdString();
}

void SurfaceMesh::activate()
{
  isActive = true;
  go();
  sendDone();
}

void SurfaceMesh::deactivate()
{
  isActive = false;
}

void SurfaceMesh::go()
{
  const slc::Containers &containers = eventHandler->getSelections();
  if (containers.empty())
  {
    //this temp. if no selection, import mesh.
    node->sendBlocked(msg::buildStatusMessage("No selection for surface mesh", 2.0));
    shouldUpdate = false;
    return;
  }
  //only considering first feature
  ftr::Base *bf = project->findFeature(containers.front().featureId);
  if (!bf->hasAnnex(ann::Type::SeerShape))
  {
    node->sendBlocked(msg::buildStatusMessage("Feature doesn't have seer shape", 2.0));
    shouldUpdate = false;
    return;
  }
  
  std::shared_ptr<ftr::SurfaceMesh> sfm(new ftr::SurfaceMesh());
  sfm->setMeshType(ftr::SurfaceMesh::MeshType::occt);
//   sfm->setMeshType(ftr::SurfaceMesh::MeshType::netgen);
//   sfm->setMeshType(ftr::SurfaceMesh::MeshType::gmsh);
  project->addFeature(sfm);
  project->connectInsert(bf->getId(), sfm->getId(), ftr::InputType{ftr::InputType::target});
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
}
