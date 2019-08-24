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

#include <TopoDS_Face.hxx>
#include <TopoDS.hxx>

#include <osg/Geometry> //yuck

#include "application/appmainwindow.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "annex/annseershape.h"
#include "dialogs/dlgextract.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrextract.h"
#include "tools/featuretools.h"
#include "command/cmdextract.h"

using namespace cmd;
using boost::uuids::uuid;

Extract::Extract() : Base() {}

Extract::~Extract()
{
  if (dialog)
    dialog->deleteLater();
}

std::string Extract::getStatusMessage()
{
  return QObject::tr("Select geometry to extract").toStdString();
}

void Extract::activate()
{
  isActive = true;
  
  /* first time the command is activated we will check for a valid preselection.
   * if there is one then we will just build a simple blend feature and call
   * the parameter dialog. Otherwise we will call the blend dialog.
   */
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

void Extract::deactivate()
{
  isActive = false;
}

void Extract::go()
{
  auto goDialog = [&]()
  {
    node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));

    std::shared_ptr<ftr::Extract> extract = std::make_shared<ftr::Extract>();
    project->addFeature(extract);
    dialog = new dlg::Extract(extract.get(), mainWindow);

    node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
    node->sendBlocked(msg::buildStatusMessage("invalid pre selection", 2.0));
    shouldUpdate = false;
  };
  
  bool created = false;
  const slc::Containers &containers = eventHandler->getSelections();
  std::vector<slc::Containers> splits = slc::split(containers); //grouped by featureId.
  for (const auto &cs : splits)
  {
    assert(!cs.empty());
    uuid fId = cs.front().featureId;
    ftr::Base *baseFeature = project->findFeature(fId);
    assert(baseFeature);
    if (!baseFeature->hasAnnex(ann::Type::SeerShape))
      continue;
    const ann::SeerShape &tss = baseFeature->getAnnex<ann::SeerShape>();
    int pickCount = -1;
    ftr::Picks picks;
    tls::Connector connector;
    for (const auto c : cs)
    {
      pickCount++;
      assert(c.featureId == fId);
      ftr::Pick pick = tls::convertToPick(c, tss, project->getShapeHistory());
      pick.tag = ftr::InputType::createIndexedTag(ftr::InputType::target, picks.size());
      connector.add(fId, pick.tag);
      picks.push_back(pick);
    }
    
    std::shared_ptr<ftr::Extract> extract = std::make_shared<ftr::Extract>();
    extract->setPicks(picks);
    project->addFeature(extract);
    for (const auto &p : connector.pairs)
      project->connectInsert(p.first, extract->getId(), {p.second});
    created = true;
    node->sendBlocked(msg::buildHideThreeD(baseFeature->getId()));
    node->sendBlocked(msg::buildHideOverlay(baseFeature->getId()));
    extract->setColor(baseFeature->getColor());
  }
  if (!created)
  {
    goDialog();
  }
  else
    node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

ExtractEdit::ExtractEdit(ftr::Base *in) : Base()
{
  feature = dynamic_cast<ftr::Extract*>(in);
  assert(feature);
  shouldUpdate = false; //dialog controls.
}

ExtractEdit::~ExtractEdit()
{
  if (dialog)
    dialog->deleteLater();
}

std::string ExtractEdit::getStatusMessage()
{
  return QObject::tr("Editing draft").toStdString();
}

void ExtractEdit::activate()
{
  isActive = true;
  if (!dialog)
  {
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    dialog = new dlg::Extract(feature, mainWindow, true);
  }

  dialog->show();
  dialog->raise();
  dialog->activateWindow();
}

void ExtractEdit::deactivate()
{
  dialog->hide();
  isActive = false;
}
