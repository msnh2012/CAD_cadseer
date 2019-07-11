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

#include <boost/optional.hpp>

#include "tools/featuretools.h"
#include "application/appmainwindow.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "library/lbrplabel.h"
#include "dialogs/dlgsweep.h"
#include "annex/annseershape.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrsweep.h"
#include "command/cmdsweep.h"

using boost::uuids::uuid;

using namespace cmd;

Sweep::Sweep() : Base() {}

Sweep::~Sweep() {}

std::string Sweep::getStatusMessage()
{
  return QObject::tr("Select geometry for Sweep feature").toStdString();
}

void Sweep::activate()
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

void Sweep::deactivate()
{
  if (dialog)
    dialog->hide();
  isActive = false;
}

void Sweep::go()
{
  auto goDialog = [&]()
  {
    shouldUpdate = false;
    node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
    auto sweep = std::make_shared<ftr::Sweep>();
    project->addFeature(sweep);
    node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
    node->sendBlocked(msg::buildHideOverlay(sweep->getId()));
    dialog = new dlg::Sweep(sweep.get(), mainWindow);
    node->sendBlocked(msg::buildStatusMessage("invalid pre selection", 2.0));
  };
  
  const slc::Containers &cs = eventHandler->getSelections();
  if (cs.size() < 2)
  {
    goDialog();
    return;
  }
  
  std::vector<std::pair<uuid, std::string>> inputStringPair; //feature id and input tag
  boost::optional<ftr::Pick> spinePick;
  ftr::Picks profiles;
  for (const auto &s : cs)
  {
    if
    (
      (s.selectionType != slc::Type::Object)
      && (s.selectionType != slc::Type::Wire)
      && (s.selectionType != slc::Type::Edge)
//       && (!slc::isPointType(s.selectionType)) //profile can be a point, but not now.
    )
    {
      goDialog();
      return;
    }
    const ftr::Base *bf = project->findFeature(s.featureId);
    if (!bf || !bf->hasAnnex(ann::Type::SeerShape))
    {
      goDialog();
      return;
    }
    const ann::SeerShape &ss = bf->getAnnex<ann::SeerShape>();
    if (ss.isNull())
    {
      goDialog();
      return;
    }
    
    ftr::Pick cp = tls::convertToPick(s, ss, project->getShapeHistory());
    if (!spinePick)
    {
      cp.tag = ftr::Sweep::spineTag;
      spinePick = cp;
    }
    else
    {
      cp.tag = ftr::Sweep::profileTag + std::to_string(profiles.size());
      profiles.push_back(cp);
    }
    inputStringPair.push_back(std::make_pair(s.featureId, cp.tag));
  }
  
  if (!spinePick || profiles.empty())
  {
    goDialog();
    return;
  }
  
  auto f = std::make_shared<ftr::Sweep>();
  ftr::SweepData data;
  data.spine = spinePick.get();
  ftr::SweepProfiles sp;
  for (const auto &prof : profiles)
    data.profiles.push_back(ftr::SweepProfile(prof));
  f->setSweepData(data);
  project->addFeature(f);
  
  for (const auto &p : inputStringPair)
    project->connectInsert(p.first, f->getId(), ftr::InputType{p.second});
  
  node->sendBlocked(msg::buildStatusMessage("Sweep created", 2.0));
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

SweepEdit::SweepEdit(ftr::Base *feature) : Base()
{
  sweep = dynamic_cast<ftr::Sweep*>(feature);
  assert(sweep);
}

SweepEdit::~SweepEdit()
{
  if (sweepDialog)
    sweepDialog->deleteLater();
}

std::string SweepEdit::getStatusMessage()
{
  return "Editing Sweep Feature";
}

void SweepEdit::activate()
{
  if (!sweepDialog)
    sweepDialog = new dlg::Sweep(sweep, mainWindow, true);
  
  isActive = true;
  sweepDialog->show();
  sweepDialog->raise();
  sweepDialog->activateWindow();
  
  shouldUpdate = false;
}

void SweepEdit::deactivate()
{
  sweepDialog->hide();
  isActive = false;
}
