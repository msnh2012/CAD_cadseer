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

#include "globalutilities.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "viewer/vwrwidget.h"
#include "tools/tlsosgtools.h"
#include "tools/occtools.h"
#include "annex/annseershape.h"
#include "feature/ftrbase.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "command/cmdsystemtoselection.h"

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
  
  return tls::matrixFrom3Points(origin, xPoint, yPoint);
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
  
  if (containers.size() == 1)
  {
    const ftr::Base *f = project->findFeature(containers.front().featureId);
    auto csysPrms = f->getParameters(prm::Tags::CSys);
    if (!csysPrms.empty())
    {
      viewer->setCurrentSystem(csysPrms.front()->getMatrix());
      node->sendBlocked(msg::buildStatusMessage("Current system set to datum plane", 2.0));
      return;
    }
    else if (containers.front().selectionType == slc::Type::Edge)
    {
      assert(f->hasAnnex(ann::Type::SeerShape));
      const ann::SeerShape &ss = f->getAnnex<ann::SeerShape>();
      assert(ss.hasId(containers.front().shapeId));
      const TopoDS_Shape &edge = ss.getOCCTShape(containers.front().shapeId);
      auto sp = occt::gleanSystem(edge);
      if (sp.second)
      {
        viewer->setCurrentSystem(gu::toOsg(sp.first));
        node->sendBlocked(msg::buildStatusMessage("Current system set to edge", 2.0));
        return;
      }
    }
  }
  else if (containers.size() == 3)
  {
    boost::optional<osg::Matrixd> ocsys = from3Points(containers);
    if (ocsys)
    {
      node->sendBlocked(msg::buildStatusMessage("Current system set to 3 points", 2.0));
      viewer->setCurrentSystem(*ocsys);
      return;
    }
  }
  
  node->sendBlocked(msg::buildStatusMessage("Selection not supported for system derivation", 2.0));
}
