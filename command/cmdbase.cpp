/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2015  Thomas S. Anderson blobfish.at.gmx.com
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

#include <cassert>

#include "application/appapplication.h"
#include "application/appmainwindow.h"
#include "viewer/vwrwidget.h"
#include "project/prjproject.h"
#include "selection/slcmanager.h"
#include "selection/slceventhandler.h"
#include "selection/slcmessage.h"
#include "message/msgnode.h"
#include "command/cmdbase.h"

using namespace cmd;

Base::Base()
{
  node = std::make_unique<msg::Node>();
  node->connect(msg::hub());
  
  application = app::instance(); assert(application);
  mainWindow = application->getMainWindow(); assert(mainWindow);
  project = application->getProject(); assert(project);
  selectionManager = mainWindow->getSelectionManager(); assert(selectionManager);
  viewer = mainWindow->getViewer(); assert(viewer);
  eventHandler = viewer->getSelectionEventHandler(); assert(eventHandler);
  
  isActive = false;
}

Base::~Base()
{
}

void Base::sendDone()
{
  msg::Message mOut(msg::Mask(msg::Request | msg::Command | msg::Done));
  app::instance()->queuedMessage(mOut);
}

