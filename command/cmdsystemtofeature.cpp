/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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

#include "project/prjproject.h"
#include "application/appmainwindow.h"
#include "selection/slceventhandler.h"
#include "globalutilities.h"
#include "viewer/vwrwidget.h"
#include "annex/anncsysdragger.h"
#include "parameter/prmparameter.h"
#include "feature/ftrbase.h"
#include "command/cmdsystemtofeature.h"

using namespace cmd;

SystemToFeature::SystemToFeature() : Base()
{
  shouldUpdate = false;
}

SystemToFeature::~SystemToFeature(){}

std::string SystemToFeature::getStatusMessage()
{
  return QObject::tr("Select feature to reposition the current coordinate system").toStdString();
}

void SystemToFeature::activate()
{
  isActive = true;
  
  const slc::Containers &containers = eventHandler->getSelections();
  for (const auto &container : containers)
  {
    if (container.selectionType != slc::Type::Object)
      continue;
    
    ftr::Base *baseFeature = project->findFeature(container.featureId);
    assert(baseFeature);
    
    if (!baseFeature->hasAnnex(ann::Type::CSysDragger))
      continue;
    ann::CSysDragger &da = baseFeature->getAnnex<ann::CSysDragger>(ann::Type::CSysDragger);
    mainWindow->getViewer()->setCurrentSystem(da.parameter->getMatrix());
    break;
  }
  
  sendDone();
}

void SystemToFeature::deactivate()
{
  isActive = false;
}
