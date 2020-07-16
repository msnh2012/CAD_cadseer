/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2018  Thomas S. Anderson blobfish.at.gmx.com
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
#include "message/msgnode.h"
#include "commandview/cmvrevision.h"
#include "commandview/cmvmessage.h"
#include "command/cmdrevision.h"

using namespace cmd;

Revision::Revision() : Base()
{
  shouldUpdate = false;
  viewBase = std::make_unique<cmv::Revision>(this);
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

Revision::~Revision() = default;

std::string Revision::getStatusMessage()
{
  return QObject::tr("Manage Revisions").toStdString();
}

void Revision::activate()
{
  isActive = true;
  
  cmv::Message vm(viewBase.get(), viewBase->getPaneWidth());
  msg::Message out(msg::Mask(msg::Request | msg::Command | msg::View | msg::Show), vm);
  node->sendBlocked(out);
}

void Revision::deactivate()
{
  msg::Message out(msg::Mask(msg::Request | msg::Command | msg::View | msg::Hide));
  node->sendBlocked(out);
  isActive = false;
}

