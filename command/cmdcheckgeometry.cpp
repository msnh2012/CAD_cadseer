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
#include "application/appapplication.h"
#include "application/appmainwindow.h"
#include "feature/ftrbase.h"
#include "selection/slceventhandler.h"
#include "selection/slcmanager.h"
#include "message/msgnode.h"
#include "message/msgsift.h"
#include "dialogs/dlgcheckgeometry.h"
#include "command/cmdcheckgeometry.h"

using namespace cmd;

CheckGeometry::CheckGeometry() : Base("cmd::CheckGeometry")
{
  setupDispatcher();
  shouldUpdate = false;
}

CheckGeometry::~CheckGeometry()
{
  if (dialog)
    dialog->deleteLater();
}

std::string CheckGeometry::getStatusMessage()
{
  return QObject::tr("Select geometry to check").toStdString();
}

void CheckGeometry::activate()
{
  isActive = true;
  
  if (!hasRan)
  {
    node->send(msg::buildSelectionMask(slc::ObjectsEnabled | slc::ObjectsSelectable));
    go();
  }

  if (dialog)
  {
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
  }
}

void CheckGeometry::deactivate()
{
  isActive = false;
  
  if (dialog)
    dialog->hide();
}

void CheckGeometry::go()
{
  const slc::Containers &containers = eventHandler->getSelections();
  if
  (
    (!containers.empty()) &&
    (containers.front().selectionType == slc::Type::Object)
  )
  {
    ftr::Base *feature = project->findFeature(containers.front().featureId);
    assert(feature);
    if (feature->hasAnnex(ann::Type::SeerShape))
    {
      assert(!dialog);
      dialog = new dlg::CheckGeometry(*feature, application->getMainWindow());
      QString freshTitle = dialog->windowTitle() + " --" + feature->getName() + "--";
      dialog->setWindowTitle(freshTitle);
      hasRan = true;
      dialog->go();
      node->send(msg::buildSelectionMask(~slc::All));
      node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
      return;
    }
  }
  
  //here we didn't have an acceptable pre seleciton.
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  node->send(msg::buildStatusMessage("Select object to check"));
}

void CheckGeometry::setupDispatcher()
{
  sift->insert
  (
    msg::Response | msg::Post | msg::Selection | msg::Add
    , std::bind(&CheckGeometry::selectionAdditionDispatched, this, std::placeholders::_1)
  );
}

void CheckGeometry::selectionAdditionDispatched(const msg::Message&)
{
  if (hasRan)
    return;
  go();
  if (dialog)
  {
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
  }
}
