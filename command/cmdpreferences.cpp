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

#include <memory>

#include "application/appmainwindow.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "dialogs/dlgpreferences.h"
#include "command/cmdpreferences.h"

using namespace cmd;

Preferences::Preferences() : Base() {}

Preferences::~Preferences() = default;

std::string Preferences::getStatusMessage()
{
  return QObject::tr("Editing Preferences").toStdString();
}

void Preferences::activate()
{
  isActive = true;
  go();
  sendDone();
}

void Preferences::deactivate()
{
  isActive = false;
}

void Preferences::go()
{
  shouldUpdate = false;
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  auto localDialog = std::make_unique<dlg::Preferences>(&prf::manager(), mainWindow);
  localDialog->setModal(true);
  if (!localDialog->exec())
    return;
  if (localDialog->isVisualDirty())
  {
    assert(project);
    project->setAllVisualDirty();
    project->updateVisual();
  }
  else if(localDialog->isHiddenLinesDirty())
  {
    if (prf::manager().rootPtr->visual().display().showHiddenLines())
      node->sendBlocked(msg::Message(msg::Request | msg::View | msg::Show | msg::HiddenLine));
    else
      node->sendBlocked(msg::Message(msg::Request | msg::View | msg::Hide | msg::HiddenLine));
  }
  
  node->sendBlocked(msg::buildStatusMessage("Preferences Edited", 2.0));
  
  msg::Message prfResponse(msg::Response | msg::Preferences);
  node->sendBlocked(prfResponse);
}
