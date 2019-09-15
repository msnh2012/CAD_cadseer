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

// #include "tools/featuretools.h"
#include "application/appmainwindow.h"
#include "project/prjproject.h"
// #include "viewer/vwrwidget.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "parameter/prmparameter.h"
#include "dialogs/dlgparameter.h"
#include "dialogs/dlgchamfer.h"
#include "annex/annseershape.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrchamfer.h"
#include "tools/featuretools.h"
#include "command/cmdchamfer.h"

using namespace cmd;

Chamfer::Chamfer() : Base() {}

Chamfer::~Chamfer()
{
  if (dialog)
    dialog->deleteLater();
}

std::string Chamfer::getStatusMessage()
{
  return QObject::tr("Select geometry for Chamfer feature").toStdString();
}

void Chamfer::activate()
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

void Chamfer::deactivate()
{
  if (dialog)
    dialog->hide();
  isActive = false;
}

void Chamfer::go()
{
  auto goDialog = [&]()
  {
    node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));

    std::shared_ptr<ftr::Chamfer> chamfer(new ftr::Chamfer());
    project->addFeature(chamfer);
    dialog = new dlg::Chamfer(chamfer.get(), mainWindow);

    node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
    node->sendBlocked(msg::buildStatusMessage("invalid pre selection", 2.0));
    shouldUpdate = false;
  };
  
  const slc::Containers &cs = eventHandler->getSelections();
  if (cs.empty())
  {
    goDialog();
    return;
  }
  
  const ftr::Base *bf = project->findFeature(cs.front().featureId);
  if (!bf || !bf->hasAnnex(ann::Type::SeerShape) || bf->getAnnex<ann::SeerShape>().isNull())
  {
    goDialog();
    return;
  }
  const ann::SeerShape &ss = bf->getAnnex<ann::SeerShape>();
  
  tls::Connector connector;
  ftr::Chamfer::Cue::Entry entry = ftr::Chamfer::Cue::Entry::buildDefaultSymmetric();
  for (const auto &c : cs)
  {
    //make sure we are all on the same solid
    if (c.featureId != cs.front().featureId)
      continue;
    int index = entry.edgePicks.size();
    entry.edgePicks.push_back(tls::convertToPick(c, ss, project->getShapeHistory()));
    entry.edgePicks.back().tag = ftr::InputType::createIndexedTag(ftr::InputType::target, index);
    connector.add(c.featureId, entry.edgePicks.back().tag);
  }
  
  ftr::Chamfer::Cue cue;
  cue.mode = ftr::Chamfer::Mode::Classic;
  cue.entries.push_back(entry);
  auto f = std::make_shared<ftr::Chamfer>();
  f->setCue(cue);
  project->addFeature(f);
  for (const auto &p : connector.pairs)
    project->connectInsert(p.first, f->getId(), ftr::InputType{p.second});
  
  f->setColor(bf->getColor());
  
  node->sendBlocked(msg::buildHideThreeD(cs.front().featureId));
  node->sendBlocked(msg::buildHideOverlay(cs.front().featureId));
  
  node->sendBlocked(msg::buildStatusMessage("Chamfer created", 2.0));
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  dlg::Parameter *dialog = new dlg::Parameter(cue.entries.back().parameter1.get(), f->getId());
  dialog->show();
  dialog->raise();
  dialog->activateWindow();
}

ChamferEdit::ChamferEdit(ftr::Base *in) : Base()
{
  //command manager edit dispatcher dispatches on ftr::type, so we know the type of 'in'
  feature = dynamic_cast<ftr::Chamfer*>(in);
  assert(feature);
  shouldUpdate = false; //dialog controls.
}

ChamferEdit::~ChamferEdit()
{
  if (dialog)
    dialog->deleteLater();
}

std::string ChamferEdit::getStatusMessage()
{
  return QObject::tr("Editing chamfer").toStdString();
}

void ChamferEdit::activate()
{
  isActive = true;
  if (!dialog)
  {
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    dialog = new dlg::Chamfer(feature, mainWindow, true);
  }

  dialog->show();
  dialog->raise();
  dialog->activateWindow();
}

void ChamferEdit::deactivate()
{
  dialog->hide();
  isActive = false;
}
