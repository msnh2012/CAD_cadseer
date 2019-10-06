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
#include "dialogs/dlgcone.h"
#include "feature/ftrcone.h"
#include "command/cmdcone.h"

using namespace cmd;

Cone::Cone() : Base() {}

Cone::~Cone()
{
  if (dialog)
    dialog->deleteLater();
}

std::string Cone::getStatusMessage()
{
  return QObject::tr("Edit Cone feature").toStdString();
}

void Cone::activate()
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

void Cone::deactivate()
{
  if (dialog)
    dialog->hide();
  isActive = false;
}

void Cone::go()
{
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  std::shared_ptr<ftr::Cone> cone = std::make_shared<ftr::Cone>();
  cone->setCSys(viewer->getCurrentSystem());
  project->addFeature(cone);
  node->sendBlocked(msg::buildStatusMessage("Cone created", 2.0));
  cone->updateModel(project->getPayload(cone->getId()));
  cone->updateVisual();
  cone->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  dialog = new dlg::Cone(cone.get(), mainWindow);

  shouldUpdate = false;
}

ConeEdit::ConeEdit(ftr::Base *in) : Base()
{
  //command manager edit dispatcher dispatches on ftr::type, so we know the type of 'in'
  feature = dynamic_cast<ftr::Cone*>(in);
  assert(feature);
  shouldUpdate = false; //dialog controls.
}

ConeEdit::~ConeEdit()
{
  if (dialog)
    dialog->deleteLater();
}

std::string ConeEdit::getStatusMessage()
{
  return QObject::tr("Editing cone").toStdString();
}

void ConeEdit::activate()
{
  isActive = true;
  if (!dialog)
  {
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    dialog = new dlg::Cone(feature, mainWindow, true);
  }

  dialog->show();
  dialog->raise();
  dialog->activateWindow();
}

void ConeEdit::deactivate()
{
  dialog->hide();
  isActive = false;
}
