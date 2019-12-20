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

#include "tools/featuretools.h"
#include "application/appmainwindow.h"
#include "project/prjproject.h"
#include "viewer/vwrwidget.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "dialogs/dlgdatumsystem.h"
#include "annex/annseershape.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrdatumsystem.h"
#include "command/cmddatumsystem.h"

using boost::uuids::uuid;

using namespace cmd;
using namespace ftr::DatumSystem;

DatumSystem::DatumSystem() : Base() {}

DatumSystem::~DatumSystem()
{
  if (dialog)
    dialog->deleteLater();
}

std::string DatumSystem::getStatusMessage()
{
  return QObject::tr("Select geometry for DatumSystem feature").toStdString();
}

void DatumSystem::activate()
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

void DatumSystem::deactivate()
{
  if (dialog)
    dialog->hide();
  isActive = false;
}

void DatumSystem::go()
{
  assert(project);
  shouldUpdate = false;
  
  auto goDialog = [&]()
  {
    node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));

    auto feature = std::make_shared<Feature>();
    feature->setCSys(viewer->getCurrentSystem());
    feature->setSize(viewer->getDiagonalLength() / 4.0);
    project->addFeature(feature);
    dialog = new dlg::DatumSystem(feature.get(), mainWindow);

    node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
    node->sendBlocked(msg::buildStatusMessage("invalid pre selection", 2.0));
  };
  
  const slc::Containers &cs = eventHandler->getSelections();
  if (cs.empty())
  {
    goDialog();
    return;
  }
  if (cs.size() == 3)
  {
    if (attemptThrough3Points(cs))
      return;
  }
}

bool DatumSystem::attemptThrough3Points(const slc::Containers &csIn)
{
  assert(csIn.size() == 3);
  if
  (
    !slc::isPointType(csIn.at(0).selectionType)
    || !slc::isPointType(csIn.at(1).selectionType)
    || !slc::isPointType(csIn.at(2).selectionType)
  )
    return false;
    
  auto getFeature = [&](const uuid &fId) -> const ftr::Base*
  {
    if (!project->hasFeature(fId))
      return nullptr;
    const ftr::Base *out = project->findFeature(fId);
    if (!out->hasAnnex(ann::Type::SeerShape))
      return nullptr;
    if (out->getAnnex<ann::SeerShape>().isNull())
      return nullptr;
    return out;
  };
  
  Cue cue;
  cue.systemType = Through3Points;
  tls::Connector connector;
  
  auto process = [&](const slc::Container& cIn, const char *tag) -> bool
  {
    auto *featureIn = getFeature(cIn.featureId);
    if (!featureIn)
      return false;
    cue.picks.push_back(tls::convertToPick(cIn, featureIn->getAnnex<ann::SeerShape>(), project->getShapeHistory()));
    cue.picks.back().tag = tag;
    connector.add(cIn.featureId, cue.picks.back().tag);
    return true;
  };
  
  if (!process(csIn.at(0), point0))
    return false;
  if (!process(csIn.at(1), point1))
    return false;
  if (!process(csIn.at(2), point2))
    return false;
  
  auto feature = std::make_shared<Feature>();
  feature->setCue(cue);
  feature->setAutoSize(true);
  project->addFeature(feature);
  for (const auto &pr : connector.pairs)
    project->connectInsert(pr.first, feature->getId(), {pr.second});
  
  shouldUpdate = true;
  return true;
}

DatumSystemEdit::DatumSystemEdit(ftr::Base *in) : Base()
{
  //command manager edit dispatcher dispatches on ftr::type, so we know the type of 'in'
  feature = dynamic_cast<ftr::DatumSystem::Feature*>(in);
  assert(feature);
  shouldUpdate = false; //dialog controls.
}

DatumSystemEdit::~DatumSystemEdit()
{
  if (dialog)
    dialog->deleteLater();
}

std::string DatumSystemEdit::getStatusMessage()
{
  return QObject::tr("Editing Datum System").toStdString();
}

void DatumSystemEdit::activate()
{
  isActive = true;
  if (!dialog)
  {
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    dialog = new dlg::DatumSystem(feature, mainWindow, true);
  }

  dialog->show();
  dialog->raise();
  dialog->activateWindow();
}

void DatumSystemEdit::deactivate()
{
  dialog->hide();
  isActive = false;
}
