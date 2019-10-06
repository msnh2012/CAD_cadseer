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

#include "application/appmainwindow.h"
#include "project/prjproject.h"
#include "viewer/vwrwidget.h"
#include "message/msgnode.h"
#include "dialogs/dlgsphere.h"
#include "feature/ftrsphere.h"
#include "command/cmdsphere.h"

using namespace cmd;

Sphere::Sphere() : Base() {}

Sphere::~Sphere()
{
  if (dialog)
    dialog->deleteLater();
}

std::string Sphere::getStatusMessage()
{
  return QObject::tr("Edit Sphere feature").toStdString();
}

void Sphere::activate()
{
  isActive = true;
  
  if (firstRun)
  {
    firstRun = false;
    go();
  }
  
  if (dialog)
  {
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
  }
  else sendDone();
}

void Sphere::deactivate()
{
  if (dialog)
    dialog->hide();
  isActive = false;
}

void Sphere::go()
{
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  std::shared_ptr<ftr::Sphere> sphere = std::make_shared<ftr::Sphere>();
  sphere->setCSys(viewer->getCurrentSystem());
  project->addFeature(sphere);
  node->sendBlocked(msg::buildStatusMessage("Sphere created", 2.0));
  sphere->updateModel(project->getPayload(sphere->getId()));
  sphere->updateVisual();
  sphere->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  dialog = new dlg::Sphere(sphere.get(), mainWindow);

  shouldUpdate = false;
}

SphereEdit::SphereEdit(ftr::Base *in) : Base()
{
  //command manager edit dispatcher dispatches on ftr::type, so we know the type of 'in'
  feature = dynamic_cast<ftr::Sphere*>(in);
  assert(feature);
  shouldUpdate = false; //dialog controls.
}

SphereEdit::~SphereEdit()
{
  if (dialog)
    dialog->deleteLater();
}

std::string SphereEdit::getStatusMessage()
{
  return QObject::tr("Editing sphere").toStdString();
}

void SphereEdit::activate()
{
  isActive = true;
  if (!dialog)
  {
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    dialog = new dlg::Sphere(feature, mainWindow, true);
  }

  dialog->show();
  dialog->raise();
  dialog->activateWindow();
}

void SphereEdit::deactivate()
{
  dialog->hide();
  isActive = false;
}
