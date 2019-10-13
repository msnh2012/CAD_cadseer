/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2019 Thomas S. Anderson blobfish.at.gmx.com
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

#include <QUrl>
#include <QDesktopServices>

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "annex/annseershape.h"
#include "feature/ftrbase.h"
#include "command/cmdshapegraphdump.h"

using namespace cmd;

ShapeGraphDump::ShapeGraphDump() : Base() {}

ShapeGraphDump::~ShapeGraphDump() = default;

std::string ShapeGraphDump::getStatusMessage()
{
  return QObject::tr("Select feature for Shape Graph Dump").toStdString();
}

void ShapeGraphDump::activate()
{
  isActive = true;
  go();
  sendDone();
}

void ShapeGraphDump::deactivate()
{
  isActive = false;
}

void ShapeGraphDump::go()
{
  
  assert(project);
  const slc::Containers &containers = eventHandler->getSelections();
  if (containers.empty())
    return;
  
  boost::filesystem::path path = app::instance()->getApplicationDirectory();
  for (const auto &container : containers)
  {
    if (!slc::isObjectType(container.selectionType))
        continue;
    ftr::Base *feature = project->findFeature(container.featureId);
    if (!feature->hasAnnex(ann::Type::SeerShape))
        continue;
    path /= gu::idToString(feature->getId()) + ".dot";
    const ann::SeerShape &shape = feature->getAnnex<ann::SeerShape>();
    shape.dumpGraph(path.string());
    
    QDesktopServices::openUrl(QUrl(QString::fromStdString(path.string())));
  }
  node->sendBlocked(msg::buildStatusMessage("Shape graph dump created", 2.0));
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
}
