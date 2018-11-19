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

#include "message/observer.h"
#include "project/project.h"
#include "selection/eventhandler.h"
#include "annex/seershape.h"
#include "feature/revolve.h"
#include "command/revolve.h"

using boost::uuids::uuid;

using namespace cmd;

Revolve::Revolve() : Base() {}
Revolve::~Revolve() {}

std::string Revolve::getStatusMessage()
{
  return QObject::tr("Select feature or geometry for revolve feature").toStdString();
}

void Revolve::activate()
{
  isActive = true;
  go();
  sendDone();
}

void Revolve::deactivate()
{
  isActive = false;
}

void Revolve::go()
{
  const slc::Containers &containers = eventHandler->getSelections();
  if (containers.empty())
  {
    observer->outBlocked(msg::buildStatusMessage("Wrong pre selection for revolve", 2.0));
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
    observer->outBlocked(msg::buildStatusMessage("No feature id for revolve", 2.0));
    shouldUpdate = false;
    return;
  }
  
  std::shared_ptr<ftr::Revolve> revolve(new ftr::Revolve());
  revolve->setPicks(picks);
  if (color)
    revolve->setColor(color.get());
  project->addFeature(revolve);
  project->connectInsert(fId.get(), revolve->getId(), ftr::InputType{ftr::InputType::target});
  if (axisId)
  {
    project->connectInsert(axisId.get(), revolve->getId(), ftr::InputType{ftr::Revolve::axisName});
    revolve->setAxisType(ftr::Revolve::AxisType::Picks);
  }
  
  observer->outBlocked(msg::buildHideThreeD(fId.get()));
  observer->outBlocked(msg::buildHideOverlay(fId.get()));
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
}
