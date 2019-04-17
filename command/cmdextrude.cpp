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

#include "message/msgnode.h"
#include "project/prjproject.h"
#include "selection/slceventhandler.h"
#include "annex/annseershape.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrextrude.h"
#include "command/cmdextrude.h"

using boost::uuids::uuid;

using namespace cmd;

Extrude::Extrude() : Base() {}
Extrude::~Extrude() {}

std::string Extrude::getStatusMessage()
{
  return QObject::tr("Select feature or geometry for extrude feature").toStdString();
}

void Extrude::activate()
{
  isActive = true;
  go();
  sendDone();
}

void Extrude::deactivate()
{
  isActive = false;
}

void Extrude::go()
{
  const slc::Containers &containers = eventHandler->getSelections();
  if (containers.empty())
  {
    node->sendBlocked(msg::buildStatusMessage("Wrong pre selection for extrude", 2.0));
    shouldUpdate = false;
    return;
  }
  
  boost::optional<uuid> fId;
  boost::optional<uuid> axisId;
  ftr::Picks picks;
  boost::optional<osg::Vec4> color;
  for (const auto &c : containers)
  {
    //any axis definition from shape selection will have to be done in dialog.
    if (c.featureType == ftr::Type::DatumAxis && !axisId)
    {
      axisId = c.featureId;
      continue;
    }
    
    ftr::Base *bf = project->findFeature(c.featureId);
    if (!bf->hasAnnex(ann::Type::SeerShape))
      continue;
    
    if (!fId)
      fId = c.featureId;
    if (fId.get() != c.featureId)
      continue;
    if (!color)
      color = bf->getColor();
    const ann::SeerShape &ss = bf->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
    if (!c.shapeId.is_nil())
    {
      assert(ss.hasId(c.shapeId));
      if (!ss.hasId(c.shapeId))
        continue;
      //todo: something to turn end points into vertices.
      ftr::Pick p(c.shapeId, 0.0, 0.0);
      p.shapeHistory = project->getShapeHistory().createDevolveHistory(p.id);
      picks.push_back(p);
    }
  }
  if (!fId)
  {
    node->sendBlocked(msg::buildStatusMessage("No feature id for extrude", 2.0));
    shouldUpdate = false;
    return;
  }
  
  std::shared_ptr<ftr::Extrude> extrude(new ftr::Extrude());
  extrude->setPicks(picks);
  if (color)
    extrude->setColor(color.get());
  project->addFeature(extrude);
  project->connectInsert(fId.get(), extrude->getId(), ftr::InputType{ftr::InputType::target});
  if (axisId)
  {
    project->connectInsert(axisId.get(), extrude->getId(), ftr::InputType{ftr::Extrude::axisName});
    extrude->setDirectionType(ftr::Extrude::DirectionType::Picks);
  }
  
  node->sendBlocked(msg::buildHideThreeD(fId.get()));
  node->sendBlocked(msg::buildHideOverlay(fId.get()));
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
}
