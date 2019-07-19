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

#include "tools/featuretools.h"
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
  shouldUpdate = false;
  const slc::Containers &containers = eventHandler->getSelections();
  if (containers.empty() || containers.size() > 3)
  {
    node->sendBlocked(msg::buildStatusMessage("Wrong pre-selection for extrude", 2.0));
    return;
  }
  
  //target/shape
  tls::Connector connector;
  ftr::Base *tf = project->findFeature(containers.front().featureId);
  if (!tf->hasAnnex(ann::Type::SeerShape))
  {
    node->sendBlocked(msg::buildStatusMessage("First pre-selection needs to be shape", 2.0));
    return;
  }
  ftr::Pick shapePick = tls::convertToPick(containers.front(), tf->getAnnex<ann::SeerShape>(), project->getShapeHistory());
  shapePick.tag = ftr::InputType::createIndexedTag(ftr::InputType::target, 0);
  connector.add(tf->getId(), shapePick.tag);
  osg::Vec4 color = tf->getColor();
  
  //axis
  ftr::Picks axisPicks;
  auto addAxisPick = [&](const slc::Container &cIn, const std::string &tagIn) -> ftr::Base*
  {
    ftr::Base *bf = project->findFeature(cIn.featureId);
    assert(bf);
    ftr::Pick tp = tls::convertToPick(cIn, *bf, project->getShapeHistory());
    tp.tag = tagIn;
    axisPicks.push_back(tp);
    connector.add(bf->getId(), tp.tag);
    return bf;
  };
  if (containers.size() == 2)
  {
    ftr::Base *bf = addAxisPick(containers.at(1), ftr::InputType::createIndexedTag(ftr::Extrude::axisName, 0));
    if
    (
      slc::isObjectType(containers.at(1).selectionType)
      && bf->getType() != ftr::Type::DatumAxis
      && bf->getType() != ftr::Type::DatumPlane
    )
    {
      node->sendBlocked(msg::buildStatusMessage("Wrong feature type for 1 pick axis", 2.0));
      return;
    }
  }
  else if (containers.size() == 3)//3 picks. need 2 points
  {
    if (!slc::isPointType(containers.at(1).selectionType) || !slc::isPointType(containers.at(2).selectionType))
    {
      node->sendBlocked(msg::buildStatusMessage("Need 2 points for 2 pick axis definition", 2.0));
      return;
    }
    addAxisPick(containers.at(1), ftr::InputType::createIndexedTag(ftr::Extrude::axisName, 0));
    addAxisPick(containers.at(2), ftr::InputType::createIndexedTag(ftr::Extrude::axisName, 1));
  }
  
  std::shared_ptr<ftr::Extrude> extrude(new ftr::Extrude());
  extrude->setPicks(ftr::Picks{shapePick});
  if (!axisPicks.empty())
    extrude->setAxisPicks(axisPicks); //sets the appropriate type also.
  extrude->setColor(color);
  project->addFeature(extrude);
  for (const auto &p : connector.pairs)
    project->connectInsert(p.first, extrude->getId(), {p.second});
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  shouldUpdate = true;
}
