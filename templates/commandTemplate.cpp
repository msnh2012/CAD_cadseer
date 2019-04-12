/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) %YEAR% Thomas S. Anderson blobfish.at.gmx.com
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

// #include "tools/featuretools.h"
// #include "application/mainwindow.h"
// #include "application/application.h"
// #include "project/project.h"
// #include "message/node.h"
// #include "selection/eventhandler.h"
// #include "parameter/parameter.h"
// #include "dialogs/parameter.h"
// #include "dialogs/%CLASSNAMELOWERCASE%.h"
// #include "annex/seershape.h"
// #include "feature/inputtype.h"
#include "feature/ftr%CLASSNAMELOWERCASE%.h"
#include "command/cmd%CLASSNAMELOWERCASE%.h"

using namespace cmd;

%CLASSNAME%::%CLASSNAME%() : Base() {}

%CLASSNAME%::~%CLASSNAME%() {}

std::string %CLASSNAME%::getStatusMessage()
{
  return QObject::tr("Select geometry for %CLASSNAME% feature").toStdString();
}

void %CLASSNAME%::activate()
{
  isActive = true;
  go();
  sendDone();
}

void %CLASSNAME%::deactivate()
{
  isActive = false;
}

void %CLASSNAME%::go()
{
//   const slc::Containers &cs = eventHandler->getSelections();
//   if (cs.size() != 2)
//   {
//     node->sendBlocked(msg::buildStatusMessage("Incorrect selection for %CLASSNAME%", 2.0));
//     shouldUpdate = false;
//     return;
//   }
//   
//   const ftr::Base *bf0 = project->findFeature(cs.front().featureId);
//   if (!bf0 || !bf0->hasAnnex(ann::Type::SeerShape))
//   {
//     node->sendBlocked(msg::buildStatusMessage("Invalid first selection for %CLASSNAME%", 2.0));
//     shouldUpdate = false;
//     return;
//   }
//   const ann::SeerShape &ss0 = bf0->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
//   
//   ftr::Picks picks;
//   picks.push_back(tls::convertToPick(cs.front(), ss0));
//   picks.at(0).shapeHistory = project->getShapeHistory().createDevolveHistory(cs.at(0).shapeId);
//   
//   auto f = std::make_shared<ftr::%CLASSNAME%>();
//   f->setPicks(picks);
//   project->addFeature(f);
//   project->connectInsert(cs.front().featureId, f->getId(), ftr::InputType{"something"});
//   
//   node->sendBlocked(msg::buildStatusMessage("%CLASSNAME% created", 2.0));
//   node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
}
