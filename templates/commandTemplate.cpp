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
// #include "application/appmainwindow.h"
// #include "application/appapplication.h"
// #include "project/prjproject.h"
// #include "viewer/vwrwidget.h"
// #include "message/msgnode.h"
// #include "selection/slceventhandler.h"
// #include "parameter/prmparameter.h"
// #include "dialogs/dlgparameter.h"
// #include "dialogs/dlg%CLASSNAMELOWERCASE%.h"
// #include "annex/annseershape.h"
// #include "feature/ftrinputtype.h"
#include "feature/ftr%CLASSNAMELOWERCASE%.h"
#include "command/cmd%CLASSNAMELOWERCASE%.h"

using namespace cmd;

%CLASSNAME%::%CLASSNAME%() : Base() {}

%CLASSNAME%::~%CLASSNAME%()
{
  if (dialog)
    dialog->deleteLater();
}

std::string %CLASSNAME%::getStatusMessage()
{
  return QObject::tr("Select geometry for %CLASSNAME% feature").toStdString();
}

void %CLASSNAME%::activate()
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

void %CLASSNAME%::deactivate()
{
  if (dialog)
    dialog->hide();
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
//   const ftr::Base *bf = project->findFeature(cs.front().featureId);
//   if (!bf || !bf->hasAnnex(ann::Type::SeerShape))
//   {
//     node->sendBlocked(msg::buildStatusMessage("Invalid first selection for %CLASSNAME%", 2.0));
//     shouldUpdate = false;
//     return;
//   }
//   const ann::SeerShape &ss = bf->getAnnex<ann::SeerShape>();
//   if (ss.isNull())
//   {
//     node->sendBlocked(msg::buildStatusMessage("Seershape is null for %CLASSNAME%", 2.0));
//     shouldUpdate = false;
//     return;
//   }
//   
//   ftr::Picks picks;
//   picks.push_back(tls::convertToPick(cs.front(), ss0, project->getShapeHistory()));
//   picks.back().tag = "something";
//   
//   auto f = std::make_shared<ftr::%CLASSNAME%>();
//   f->setPicks(picks);
//   project->addFeature(f);
//   project->connectInsert(cs.front().featureId, f->getId(), ftr::InputType{"something"});
//   
//   node->sendBlocked(msg::buildStatusMessage("%CLASSNAME% created", 2.0));
//   node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

%CLASSNAME%Edit::%CLASSNAME%Edit(ftr::Base *in) : Base()
{
  //command manager edit dispatcher dispatches on ftr::type, so we know the type of 'in'
  feature = dynamic_cast<ftr::%CLASSNAME%*>(in);
  assert(feature);
  shouldUpdate = false; //dialog controls.
}

%CLASSNAME%Edit::~%CLASSNAME%Edit()
{
  if (dialog)
    dialog->deleteLater();
}

std::string %CLASSNAME%Edit::getStatusMessage()
{
  return QObject::tr("Editing %CLASSNAMELOWERCASE%").toStdString();
}

void %CLASSNAME%Edit::activate()
{
  isActive = true;
  if (!dialog)
  {
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    dialog = new dlg::%CLASSNAME%(feature, mainWindow, true);
  }

  dialog->show();
  dialog->raise();
  dialog->activateWindow();
}

void %CLASSNAME%Edit::deactivate()
{
  dialog->hide();
  isActive = false;
}
