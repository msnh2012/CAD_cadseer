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
#include "message/msgsift.h"
#include "message/msgmessage.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvbase.h"
#include "command/cmdbase.h"

using namespace cmd;

Base::Base()
: node(std::make_unique<msg::Node>())
, sift(std::make_unique<msg::Sift>())
{
  node->connect(msg::hub());
  node->setHandler(std::bind(&msg::Sift::receive, sift.get(), std::placeholders::_1));
  sift->name = "cmd::Base"; //set derived class or use other constructor
  
  sift->insert
  (
    {
      std::make_pair
      (
        msg::Response | msg::Command | msg::View | msg::Update
        , std::bind(&Base::splitterDispatched, this, std::placeholders::_1)
      )
    }
  );
  
  application = app::instance(); assert(application);
  mainWindow = application->getMainWindow(); assert(mainWindow);
  project = application->getProject(); assert(project);
  selectionManager = mainWindow->getSelectionManager(); assert(selectionManager);
  viewer = mainWindow->getViewer(); assert(viewer);
  eventHandler = viewer->getSelectionEventHandler(); assert(eventHandler);
  
  isActive = false;
}

Base::Base(const std::string &siftNameIn)
: Base()
{
  sift->name = siftNameIn;
}

Base::~Base()
{
}

void Base::splitterDispatched(const msg::Message &mIn)
{
  if (!viewBase || !isActive)
    return;
  assert(mIn.isCMV());
  viewBase->setPaneWidth(mIn.getCMV().paneWidth);
}

void Base::sendDone()
{
  msg::Message mOut(msg::Mask(msg::Request | msg::Command | msg::Done));
  app::instance()->queuedMessage(mOut);
}

std::string Base::indexTag(std::string_view sView, std::size_t index)
{
  return std::string(sView) + std::to_string(index);
}
