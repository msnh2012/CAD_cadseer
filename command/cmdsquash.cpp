/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#include "application/appapplication.h"
#include "application/appmainwindow.h"
#include "selection/slceventhandler.h"
#include "message/msgnode.h"
#include "project/prjproject.h"
#include "annex/annseershape.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrsquash.h"
#include "tools/occtools.h"
#include "tools/idtools.h"
#include "tools/featuretools.h"
#include "command/cmdsquash.h"

using namespace cmd;
using boost::uuids::uuid;

Squash::Squash() : Base() {}

Squash::~Squash(){}

std::string Squash::getStatusMessage()
{
  return QObject::tr("Select feature to squash").toStdString();
}

void Squash::activate()
{
  isActive = true;
  
  go();
  
  sendDone();
}

void Squash::deactivate()
{
  isActive = false;
}

void Squash::go()
{
  //only works with preselection for now.
  ftr::Base *f = nullptr;
  ftr::Picks fps;
  const slc::Containers &containers = eventHandler->getSelections();
  for (const auto &container : containers)
  {
    if (container.selectionType != slc::Type::Face)
      continue;
    ftr::Base *tf = project->findFeature(container.featureId);
    assert(tf);
    if (!tf->hasAnnex(ann::Type::SeerShape))
      continue;
    if (!f)
    {
      f = tf;
    }
    else
    {
      if (f->getId() != tf->getId())
        continue;
    }
    
    const ann::SeerShape &ss = f->getAnnex<ann::SeerShape>();
    TopoDS_Face face = TopoDS::Face(ss.getOCCTShape(container.shapeId));  
    ftr::Pick pick = tls::convertToPick(container, ss, project->getShapeHistory());
    pick.tag = ftr::InputType::target;
    fps.push_back(pick);
  }
  if(!f)
  {
    std::cout << "Squash: no feature found" << std::endl;
    node->send(msg::buildStatusMessage("Squash: no feature found", 2.0));
    shouldUpdate = false;
    return;
  }
  if(fps.empty())
  {
    std::cout << "Squash: no faces" << std::endl;
    node->send(msg::buildStatusMessage("Squash: no faces", 2.0));
    shouldUpdate = false;
    return;
  }
  auto *squash = new ftr::Squash();
  project->addFeature(std::unique_ptr<ftr::Squash>(squash));
  project->connect(f->getId(), squash->getId(), ftr::InputType{ftr::InputType::target});
  squash->setPicks(fps);
  squash->setColor(f->getColor());
  
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
}
