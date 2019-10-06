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
#include "dialogs/dlgtorus.h"
#include "feature/ftrtorus.h"
#include "command/cmdtorus.h"

using namespace cmd;

Torus::Torus() : Base() {}

Torus::~Torus()
{
  if (dialog)
    dialog->deleteLater();
}

std::string Torus::getStatusMessage()
{
  return QObject::tr("Edit Torus feature").toStdString();
}

void Torus::activate()
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

void Torus::deactivate()
{
  if (dialog)
    dialog->hide();
  isActive = false;
}

void Torus::go()
{
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  std::shared_ptr<ftr::Torus> torus = std::make_shared<ftr::Torus>();
  torus->setCSys(viewer->getCurrentSystem());
  project->addFeature(torus);
  node->sendBlocked(msg::buildStatusMessage("Torus created", 2.0));
  torus->updateModel(project->getPayload(torus->getId()));
  torus->updateVisual();
  torus->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  dialog = new dlg::Torus(torus.get(), mainWindow);

  shouldUpdate = false;
}

TorusEdit::TorusEdit(ftr::Base *in) : Base()
{
  //command manager edit dispatcher dispatches on ftr::type, so we know the type of 'in'
  feature = dynamic_cast<ftr::Torus*>(in);
  assert(feature);
  shouldUpdate = false; //dialog controls.
}

TorusEdit::~TorusEdit()
{
  if (dialog)
    dialog->deleteLater();
}

std::string TorusEdit::getStatusMessage()
{
  return QObject::tr("Editing torus").toStdString();
}

void TorusEdit::activate()
{
  isActive = true;
  if (!dialog)
  {
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    dialog = new dlg::Torus(feature, mainWindow, true);
  }

  dialog->show();
  dialog->raise();
  dialog->activateWindow();
}

void TorusEdit::deactivate()
{
  dialog->hide();
  isActive = false;
}
