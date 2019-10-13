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

#include <iostream>

// #include "tools/featuretools.h"
// #include "application/appmainwindow.h"
// #include "application/appapplication.h"
#include "project/prjproject.h"
// #include "viewer/vwrwidget.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
// #include "parameter/prmparameter.h"
#include "annex/annseershape.h"
#include "feature/ftrbase.h"
#include "command/cmdfeaturedump.h"

using namespace cmd;

FeatureDump::FeatureDump() : Base() {}

FeatureDump::~FeatureDump() = default;

std::string FeatureDump::getStatusMessage()
{
  return QObject::tr("Select feature").toStdString();
}

void FeatureDump::activate()
{
  isActive = true;
  go();
  sendDone();
}

void FeatureDump::deactivate()
{
  isActive = false;
}

void FeatureDump::go()
{
  assert(project);
  const slc::Containers &containers = eventHandler->getSelections();
  if (containers.empty())
    return;
  
  std::cout << std::endl << std::endl << "begin feature dump:" << std::endl;
  
  for (const auto &container : containers)
  {
    if (container.selectionType != slc::Type::Object)
      continue;
    ftr::Base *feature = project->findFeature(container.featureId);
    assert(feature);
    if (!feature->hasAnnex(ann::Type::SeerShape))
      continue;
    const ann::SeerShape &seerShape = feature->getAnnex<ann::SeerShape>();
    std::cout << std::endl;
    std::cout << "feature name: " << feature->getName().toStdString() << "    feature id: " << gu::idToString(feature->getId()) << std::endl;
    std::cout << "shape id container:" << std::endl; seerShape.dumpShapeIdContainer(std::cout); std::cout << std::endl;
    std::cout << "shape evolve container:" << std::endl; seerShape.dumpEvolveContainer(std::cout); std::cout << std::endl;
    std::cout << "feature tag container:" << std::endl; seerShape.dumpFeatureTagContainer(std::cout); std::cout << std::endl;
  }
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
}
