/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#include <QTextStream>

#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>

#include <osg/AutoTransform>

#include "message/msgnode.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvmeasurelinear.h"
#include "command/cmdmeasurelinear.h"

using namespace cmd;

MeasureLinear::MeasureLinear()
: Base()
{
  viewBase = std::make_unique<cmv::MeasureLinear>(this);
  shouldUpdate = false;
}

MeasureLinear::~MeasureLinear(){}

std::string MeasureLinear::getStatusMessage()
{
  return QObject::tr("Select 2 objects for measure linear").toStdString();
}

void MeasureLinear::activate()
{
  isActive = true;
  cmv::Message vm(viewBase.get(), viewBase->getPaneWidth());
  msg::Message out(msg::Mask(msg::Request | msg::Command | msg::View | msg::Show), vm);
  node->sendBlocked(out);
}

void MeasureLinear::deactivate()
{
  isActive = false;
  msg::Message out(msg::Mask(msg::Request | msg::Command | msg::View | msg::Hide));
  node->sendBlocked(out);
}
