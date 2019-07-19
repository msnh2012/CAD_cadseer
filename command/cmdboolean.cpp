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

#include "application/appmainwindow.h"
#include "project/prjproject.h"
#include "viewer/vwrwidget.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "dialogs/dlgboolean.h"
#include "tools/featuretools.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrintersect.h"
#include "feature/ftrsubtract.h"
#include "feature/ftrunion.h"
#include "command/cmdboolean.h"

using namespace cmd;

Boolean::Boolean(const ftr::Type &tIn)
: Base()
, ftrType(tIn)
{
  assert
  (
    (ftrType == ftr::Type::Intersect)
    || (ftrType == ftr::Type::Subtract)
    || (ftrType == ftr::Type::Union)
  );
}

Boolean::~Boolean() {}

std::string Boolean::getCommandName()
{
  if (ftrType == ftr::Type::Intersect)
    return "Intersect";
  else if (ftrType == ftr::Type::Subtract)
    return "Subtract";
  else if (ftrType == ftr::Type::Union)
    return "Union";
  
  return "ERROR";
}

std::string Boolean::getStatusMessage()
{
  std::string message = std::string("Select geometry for ") + getCommandName() + std::string(" feature");
  return message;
}

void Boolean::activate()
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

void Boolean::deactivate()
{
  if (dialog)
    dialog->hide();
  isActive = false;
}

void Boolean::go()
{
  auto goDialog = [&]()
  {
    node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
    if (ftrType == ftr::Type::Intersect)
    {
      std::shared_ptr<ftr::Intersect> intersect(new ftr::Intersect());
      project->addFeature(intersect);
      dialog = new dlg::Boolean(intersect.get(), mainWindow);
    }
    else if (ftrType == ftr::Type::Subtract)
    {
      std::shared_ptr<ftr::Subtract> subtract(new ftr::Subtract());
      project->addFeature(subtract);
      dialog = new dlg::Boolean(subtract.get(), mainWindow);
    }
    else if (ftrType == ftr::Type::Union)
    {
      std::shared_ptr<ftr::Union> onion(new ftr::Union());
      project->addFeature(onion);
      dialog = new dlg::Boolean(onion.get(), mainWindow);
    }
    node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
    node->sendBlocked(msg::buildStatusMessage("invalid pre selection", 2.0));
  };
  
  const slc::Containers &containers = eventHandler->getSelections();
  if (containers.size() < 2)
  {
    goDialog();
    return;
  }
  
  ftr::Base *tf = project->findFeature(containers.front().featureId);
  if (!tf->hasAnnex(ann::Type::SeerShape))
  {
    goDialog();
    return;
  }
  
  ftr::Pick targetPick = tls::convertToPick(containers.front(), tf->getAnnex<ann::SeerShape>(), project->getShapeHistory());
  targetPick.tag = ftr::InputType::target;
  
  std::shared_ptr<ftr::Base> basePtr = nullptr;
  std::shared_ptr<ftr::Intersect> iPtr;
  std::shared_ptr<ftr::Subtract> sPtr;
  std::shared_ptr<ftr::Union> uPtr;
  if (ftrType == ftr::Type::Intersect)
  {
    iPtr = std::make_shared<ftr::Intersect>();
    iPtr->setTargetPicks(ftr::Picks(1, targetPick));
    basePtr = iPtr;
  }
  else if (ftrType == ftr::Type::Subtract)
  {
    sPtr = std::make_shared<ftr::Subtract>();
    sPtr->setTargetPicks(ftr::Picks(1, targetPick));
    basePtr = sPtr;
  }
  else if (ftrType == ftr::Type::Union)
  {
    uPtr = std::make_shared<ftr::Union>();
    uPtr->setTargetPicks(ftr::Picks(1, targetPick));
    basePtr = uPtr;
  }
  
  basePtr->setColor(tf->getColor());
  project->addFeature(basePtr);
  project->connectInsert(tf->getId(), basePtr->getId(), ftr::InputType{targetPick.tag});
  node->sendBlocked(msg::buildHideThreeD(tf->getId()));
  node->sendBlocked(msg::buildHideOverlay(tf->getId()));
  
  
  ftr::Picks toolPicks;
  int toolCount = 0;
  for (std::size_t index = 1; index < containers.size(); ++index, ++toolCount)
  {
    const slc::Container &cc = containers.at(index); // current container
    ftr::Base *tool = project->findFeature(cc.featureId);
    if (!tool->hasAnnex(ann::Type::SeerShape))
      continue;
    
    ftr::Pick toolPick = tls::convertToPick(cc, tf->getAnnex<ann::SeerShape>(), project->getShapeHistory());
    toolPick.tag = std::string(ftr::InputType::tool) + std::to_string(toolCount);
    toolPicks.push_back(toolPick);
    
    project->connect(cc.featureId, basePtr->getId(), ftr::InputType{toolPick.tag});
    node->sendBlocked(msg::buildHideThreeD(cc.featureId));
    node->sendBlocked(msg::buildHideOverlay(cc.featureId));
  }
  
  if (ftrType == ftr::Type::Intersect)
    iPtr->setToolPicks(toolPicks);
  else if (ftrType == ftr::Type::Subtract)
    sPtr->setToolPicks(toolPicks);
  else if (ftrType == ftr::Type::Union)
    uPtr->setToolPicks(toolPicks);
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

BooleanEdit::BooleanEdit(ftr::Base *in) : Base()
{
  //command manager edit dispatcher dispatches on ftr::type, so we know the type of 'in'
  if (in->getType() == ftr::Type::Intersect)
  {
    iPtr = dynamic_cast<ftr::Intersect*>(in);
    assert(iPtr);
  }
  else if (in->getType() == ftr::Type::Subtract)
  {
    sPtr = dynamic_cast<ftr::Subtract*>(in);
    assert(sPtr);
  }
  else if (in->getType() == ftr::Type::Union)
  {
    uPtr = dynamic_cast<ftr::Union*>(in);
    assert(uPtr);
  }
}

BooleanEdit::~BooleanEdit()
{
  if (dialog)
    dialog->deleteLater();
}

std::string BooleanEdit::getStatusMessage()
{
  if (iPtr)
    return QObject::tr("Editing Intersect").toStdString();
  else if (sPtr)
    return QObject::tr("Editing Subtract").toStdString();
  else if (uPtr)
    return QObject::tr("Editing Union").toStdString();
  
  return "ERROR";
}

void BooleanEdit::activate()
{
  isActive = true;
  if (!dialog)
  {
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    
    if (iPtr)
      dialog = new dlg::Boolean(iPtr, mainWindow);
    else if (sPtr)
      dialog = new dlg::Boolean(sPtr, mainWindow);
    else if (uPtr)
      dialog = new dlg::Boolean(uPtr, mainWindow);
    
    dialog->setEditDialog();
  }

  dialog->show();
  dialog->raise();
  dialog->activateWindow();
}

void BooleanEdit::deactivate()
{
  dialog->hide();
  isActive = false;
}
