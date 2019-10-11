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

#include <osg/Geometry>

// #include "tools/featuretools.h"
#include "application/appmainwindow.h"
// #include "application/appapplication.h"
#include "project/prjproject.h"
// #include "viewer/vwrwidget.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "parameter/prmparameter.h"
#include "dialogs/dlgparameter.h"
#include "dialogs/dlghollow.h"
#include "annex/annseershape.h"
#include "tools/featuretools.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrhollow.h"
#include "command/cmdhollow.h"

using boost::uuids::uuid;

using namespace cmd;

Hollow::Hollow() : Base() {}

Hollow::~Hollow()
{
  if (dialog)
    dialog->deleteLater();
}

std::string Hollow::getStatusMessage()
{
  return QObject::tr("Select faces for Hollow feature").toStdString();
}

void Hollow::activate()
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

void Hollow::deactivate()
{
  if (dialog)
    dialog->hide();
  isActive = false;
}

void Hollow::go()
{
  auto goDialog = [&]()
  {
    node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));

    auto hollow = std::make_shared<ftr::Hollow>();
    project->addFeature(hollow);
    dialog = new dlg::Hollow(hollow.get(), mainWindow);

    node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
    node->sendBlocked(msg::buildStatusMessage("invalid pre selection", 2.0));
    shouldUpdate = false;
  };
  
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
  
  const slc::Containers &cs = eventHandler->getSelections();
  if (cs.empty())
  {
    goDialog();
    return;
  }
  
  uuid fId = gu::createNilId();
  ftr::Picks picks;
  tls::Connector connector;
  for (const auto &m : cs)
  {
    if (m.selectionType != slc::Type::Face)
      continue;
    const ftr::Base *f = getFeature(m.featureId);
    if (!f)
      continue;
    if (fId.is_nil())
      fId = m.featureId;
    if (fId != m.featureId)
      continue;
    auto ps = picks.size();
    picks.push_back(tls::convertToPick(m, f->getAnnex<ann::SeerShape>(), project->getShapeHistory()));
    picks.back().tag = ftr::InputType::createIndexedTag(ftr::InputType::target, ps);
    connector.add(fId, picks.back().tag);
  }
  if (picks.empty() || connector.pairs.empty())
  {
    goDialog();
    return;
  }
  
  auto f = std::make_shared<ftr::Hollow>();
  f->setHollowPicks(picks);
  project->addFeature(f);
  for (const auto &pr : connector.pairs)
    project->connectInsert(pr.first, f->getId(), {pr.second});
  
  node->sendBlocked(msg::buildHideThreeD(fId));
  node->sendBlocked(msg::buildHideOverlay(fId));
  node->sendBlocked(msg::buildStatusMessage("Hollow created", 2.0));
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));

  dlg::Parameter *pDialog = new dlg::Parameter(&f->getOffset(), f->getId());
  pDialog->show();
  pDialog->raise();
  pDialog->activateWindow();
}

HollowEdit::HollowEdit(ftr::Base *in) : Base()
{
  //command manager edit dispatcher dispatches on ftr::type, so we know the type of 'in'
  feature = dynamic_cast<ftr::Hollow*>(in);
  assert(feature);
  shouldUpdate = false; //dialog controls.
}

HollowEdit::~HollowEdit()
{
  if (dialog)
    dialog->deleteLater();
}

std::string HollowEdit::getStatusMessage()
{
  return QObject::tr("Editing hollow").toStdString();
}

void HollowEdit::activate()
{
  isActive = true;
  if (!dialog)
  {
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    dialog = new dlg::Hollow(feature, mainWindow, true);
  }

  dialog->show();
  dialog->raise();
  dialog->activateWindow();
}

void HollowEdit::deactivate()
{
  dialog->hide();
  isActive = false;
}
