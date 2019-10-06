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
#include "dialogs/dlgcylinder.h"
#include "feature/ftrcylinder.h"
#include "command/cmdcylinder.h"

using namespace cmd;

Cylinder::Cylinder() : Base() {}

Cylinder::~Cylinder()
{
  if (dialog)
    dialog->deleteLater();
}

std::string Cylinder::getStatusMessage()
{
  return QObject::tr("Edit Cylinder feature").toStdString();
}

void Cylinder::activate()
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

void Cylinder::deactivate()
{
  if (dialog)
    dialog->hide();
  isActive = false;
}

void Cylinder::go()
{
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  std::shared_ptr<ftr::Cylinder> cylinder = std::make_shared<ftr::Cylinder>();
  cylinder->setCSys(viewer->getCurrentSystem());
  project->addFeature(cylinder);
  node->sendBlocked(msg::buildStatusMessage("Cylinder created", 2.0));
  cylinder->updateModel(project->getPayload(cylinder->getId()));
  cylinder->updateVisual();
  cylinder->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  dialog = new dlg::Cylinder(cylinder.get(), mainWindow);

  shouldUpdate = false;
}

CylinderEdit::CylinderEdit(ftr::Base *in) : Base()
{
  //command manager edit dispatcher dispatches on ftr::type, so we know the type of 'in'
  feature = dynamic_cast<ftr::Cylinder*>(in);
  assert(feature);
  shouldUpdate = false; //dialog controls.
}

CylinderEdit::~CylinderEdit()
{
  if (dialog)
    dialog->deleteLater();
}

std::string CylinderEdit::getStatusMessage()
{
  return QObject::tr("Editing cylinder").toStdString();
}

void CylinderEdit::activate()
{
  isActive = true;
  if (!dialog)
  {
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    dialog = new dlg::Cylinder(feature, mainWindow, true);
  }

  dialog->show();
  dialog->raise();
  dialog->activateWindow();
}

void CylinderEdit::deactivate()
{
  dialog->hide();
  isActive = false;
}
