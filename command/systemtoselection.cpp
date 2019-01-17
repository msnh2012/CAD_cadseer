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

#include <boost/type_traits/is_volatile.hpp>
#include <boost/type_traits/is_assignable.hpp>
#include <boost/optional.hpp>

#include <project/project.h>
#include <message/node.h>
#include <selection/eventhandler.h>
#include <viewer/widget.h>
#include <feature/datumplane.h>
#include <command/systemtoselection.h>

using namespace cmd;

static boost::optional<osg::Matrixd> from3Points(const slc::Containers &csIn)
{
  if (csIn.size() != 3)
    return boost::none;
  for (const auto &c : csIn)
  {
    if (!slc::isPointType(c.selectionType))
      return boost::none;
  }
  osg::Vec3d origin = csIn.at(0).pointLocation;
  osg::Vec3d xPoint = csIn.at(1).pointLocation;
  osg::Vec3d yPoint = csIn.at(2).pointLocation;
  
  osg::Vec3d xVector = xPoint - origin;
  osg::Vec3d yVector = yPoint - origin;
  if (xVector.isNaN() || yVector.isNaN())
    return boost::none;
  xVector.normalize();
  yVector.normalize();
  if ((1 - std::fabs(xVector * yVector)) < std::numeric_limits<float>::epsilon())
    return boost::none;
  osg::Vec3d zVector = xVector ^ yVector;
  if (zVector.isNaN())
    return boost::none;
  zVector.normalize();
  yVector = zVector ^ xVector;
  yVector.normalize();

  osg::Matrixd fm;
  fm(0,0) = xVector.x(); fm(0,1) = xVector.y(); fm(0,2) = xVector.z();
  fm(1,0) = yVector.x(); fm(1,1) = yVector.y(); fm(1,2) = yVector.z();
  fm(2,0) = zVector.x(); fm(2,1) = zVector.y(); fm(2,2) = zVector.z();
  fm.setTrans(origin);
  
  return fm;
}

SystemToSelection::SystemToSelection() : Base()
{
  shouldUpdate = false;
}

SystemToSelection::~SystemToSelection() {}

std::string SystemToSelection::getStatusMessage()
{
  return QObject::tr("Select geometry for derived system").toStdString();
}

void SystemToSelection::activate()
{
  isActive = true;
  go();
  sendDone();
}

void SystemToSelection::deactivate()
{
  isActive = false;
}

void SystemToSelection::go()
{
  const slc::Containers &containers = eventHandler->getSelections();
  if (containers.empty())
  {
    node->sendBlocked(msg::buildStatusMessage("No selection for system derivation", 2.0));
    return;
  }
  
  boost::optional<osg::Matrixd> ocsys;
  ocsys = from3Points(containers);
  if (ocsys)
  {
    node->sendBlocked(msg::buildStatusMessage("Current system set to 3 points", 2.0));
    viewer->setCurrentSystem(*ocsys);
    return;
  }
  if (containers.size() == 1 && containers.front().featureType == ftr::Type::DatumPlane)
  {
    const ftr::DatumPlane *dp = dynamic_cast<const ftr::DatumPlane*>(project->findFeature(containers.front().featureId));
    assert(dp);
    viewer->setCurrentSystem(dp->getSystem());
    node->sendBlocked(msg::buildStatusMessage("Current system set to datum plane", 2.0));
    return;
  }
  
  node->sendBlocked(msg::buildStatusMessage("Selection not supported for system derivation", 2.0));
}
