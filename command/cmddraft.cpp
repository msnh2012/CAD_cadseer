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
#include "application/appapplication.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "parameter/prmparameter.h"
#include "dialogs/dlgparameter.h"
#include "dialogs/dlgdraft.h"
#include "annex/annseershape.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrdraft.h"
#include "command/cmddraft.h"

using namespace cmd;

Draft::Draft() : Base() {}

Draft::~Draft()
{
  if (dialog)
    dialog->deleteLater();
}

std::string Draft::getStatusMessage()
{
  return QObject::tr("Select geometry for Draft feature").toStdString();
}

void Draft::activate()
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

void Draft::deactivate()
{
  dialog->hide();
  isActive = false;
}

void Draft::go()
{
  auto goDialog = [&]()
  {
    node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));

    std::shared_ptr<ftr::Draft> draft(new ftr::Draft());
    project->addFeature(draft);
    dialog = new dlg::Draft(draft.get(), mainWindow);

    node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
    node->sendBlocked(msg::buildStatusMessage("invalid pre selection", 2.0));
    shouldUpdate = false;
  };
  
  const slc::Containers &cs = eventHandler->getSelections();
  if (cs.size() < 2)
  {
    goDialog();
    return;
  }
  const ftr::Base *bf = project->findFeature(cs.front().featureId);
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
  
  tls::Connector connector;
  ftr::Pick np = tls::convertToPick(cs.front(), ss, project->getShapeHistory());
  np.tag = ftr::Draft::neutral;
  connector.add(cs.front().featureId, np.tag);
  
  ftr::Picks draftPicks;
  auto it = cs.begin() + 1;
  for (; it != cs.end(); ++it)
  {
    if (it->featureId != cs.front().featureId)
      continue; //can't apply draft across solids. Is this really a limitation in occt?
    ftr::Pick tp = tls::convertToPick(*it, ss, project->getShapeHistory());
    tp.tag = ftr::InputType::createIndexedTag(ftr::InputType::target, draftPicks.size());
    connector.add(it->featureId, tp.tag);
    draftPicks.push_back(tp);
  }
  
  if (draftPicks.empty())
  {
    goDialog();
    return;
  }
  
  auto f = std::make_shared<ftr::Draft>();
  project->addFeature(f);
  f->setNeutralPick(np);
  f->setTargetPicks(draftPicks);
  for (const auto &p : connector.pairs)
    project->connectInsert(p.first, f->getId(), {p.second});
  f->setColor(bf->getColor());
  
  node->sendBlocked(msg::buildHideThreeD(cs.front().featureId));
  node->sendBlocked(msg::buildHideOverlay(cs.front().featureId));
  
  node->sendBlocked(msg::buildStatusMessage("Draft created", 2.0));
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  dlg::Parameter *dialog = new dlg::Parameter(f->getAngleParameter().get(), f->getId());
  dialog->show();
  dialog->raise();
  dialog->activateWindow();
}

DraftEdit::DraftEdit(ftr::Base *in) : Base()
{
  //command manager edit dispatcher dispatches on ftr::type, so we know the type of 'in'
  feature = dynamic_cast<ftr::Draft*>(in);
  assert(feature);
  shouldUpdate = false; //dialog controls.
}

DraftEdit::~DraftEdit()
{
  if (dialog)
    dialog->deleteLater();
}

std::string DraftEdit::getStatusMessage()
{
  return QObject::tr("Editing draft").toStdString();
}

void DraftEdit::activate()
{
  isActive = true;
  if (!dialog)
  {
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    dialog = new dlg::Draft(feature, mainWindow, true);
  }

  dialog->show();
  dialog->raise();
  dialog->activateWindow();
}

void DraftEdit::deactivate()
{
  dialog->hide();
  isActive = false;
}
