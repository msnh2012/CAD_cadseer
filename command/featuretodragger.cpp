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

#include <project/project.h>
#include <feature/csysbase.h>
#include <application/mainwindow.h>
#include <viewer/viewerwidget.h>
#include <selection/eventhandler.h>
#include <command/featuretodragger.h>

using namespace cmd;

FeatureToDragger::FeatureToDragger() : Base()
{
}

FeatureToDragger::~FeatureToDragger(){}

std::string FeatureToDragger::getStatusMessage()
{
  return QObject::tr("Select feature to locate feature to dragger").toStdString();
}

void FeatureToDragger::activate()
{
  isActive = true;
  
  const slc::Containers &containers = eventHandler->getSelections();
  for (const auto &container : containers)
  {
    if (container.selectionType != slc::Type::Object)
      continue;
    
    ftr::Base *baseFeature = project->findFeature(container.featureId);
    assert(baseFeature);
    ftr::CSysBase *csysFeature = dynamic_cast<ftr::CSysBase*>(baseFeature);
    if (!csysFeature)
      continue;
    
    csysFeature->setSystem(csysFeature->getDragger().getMatrix());
  }
  
  sendDone();
}

void FeatureToDragger::deactivate()
{
  isActive = false;
}