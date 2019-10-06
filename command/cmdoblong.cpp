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

#include <osg/Matrixd>

#include "application/appmainwindow.h"
#include "viewer/vwrwidget.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "dialogs/dlgoblong.h"
#include "feature/ftroblong.h"
#include "command/cmdoblong.h"

using namespace cmd;

Oblong::Oblong() : Base() {}

Oblong::~Oblong()
{
  if (dialog)
    dialog->deleteLater();
}

std::string Oblong::getStatusMessage()
{
  return QObject::tr("Edit Oblong feature").toStdString();
}

void Oblong::activate()
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

void Oblong::deactivate()
{
  if (dialog)
    dialog->hide();
  isActive = false;
}

void Oblong::go()
{
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  std::shared_ptr<ftr::Oblong> oblong = std::make_shared<ftr::Oblong>();
  oblong->setCSys(viewer->getCurrentSystem());
  project->addFeature(oblong);
  node->sendBlocked(msg::buildStatusMessage("Oblong created", 2.0));
  oblong->updateModel(project->getPayload(oblong->getId()));
  oblong->updateVisual();
  oblong->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  dialog = new dlg::Oblong(oblong.get(), mainWindow);

  shouldUpdate = false;
}

OblongEdit::OblongEdit(ftr::Base *in) : Base()
{
  //command manager edit dispatcher dispatches on ftr::type, so we know the type of 'in'
  feature = dynamic_cast<ftr::Oblong*>(in);
  assert(feature);
  shouldUpdate = false; //dialog controls.
}

OblongEdit::~OblongEdit()
{
  if (dialog)
    dialog->deleteLater();
}

std::string OblongEdit::getStatusMessage()
{
  return QObject::tr("Editing oblong").toStdString();
}

void OblongEdit::activate()
{
  isActive = true;
  if (!dialog)
  {
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    dialog = new dlg::Oblong(feature, mainWindow, true);
  }

  dialog->show();
  dialog->raise();
  dialog->activateWindow();
}

void OblongEdit::deactivate()
{
  dialog->hide();
  isActive = false;
}
