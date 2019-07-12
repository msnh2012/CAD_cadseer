/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2018  Thomas S. Anderson blobfish.at.gmx.com
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

#include <memory>

#include "project/prjproject.h"
#include "application/appmainwindow.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "viewer/vwrwidget.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrsubtract.h"
#include "dialogs/dlgboolean.h"
#include "command/cmdsubtract.h"

using namespace cmd;

Subtract::Subtract() : Base() {}
Subtract::~Subtract()
{
  if (dialog)
    dialog->deleteLater();
}

std::string Subtract::getStatusMessage()
{
  return QObject::tr("Select feature or geometry for boolean subtract").toStdString();
}

void Subtract::activate()
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

void Subtract::deactivate()
{
  if (dialog)
    dialog->hide();
  isActive = false;
}

void Subtract::go()
{
  auto goDialog = [&]()
  {
    node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
    std::shared_ptr<ftr::Subtract> subtract(new ftr::Subtract());
    project->addFeature(subtract);
    node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
    dialog = new dlg::Boolean(subtract.get(), mainWindow);
    node->sendBlocked(msg::buildStatusMessage("invalid pre selection", 2.0));
  };
  
  
  const slc::Containers &containers = eventHandler->getSelections();
  if (containers.empty())
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
  
  std::shared_ptr<ftr::Subtract> subtract(new ftr::Subtract());
  ftr::Pick targetPick;
  ftr::Picks targetPicks;
  if (!containers.front().shapeId.is_nil())
  {
    targetPick.shapeHistory = project->getShapeHistory().createDevolveHistory(containers.front().shapeId);
    targetPicks.push_back(targetPick);
    subtract->setTargetPicks(targetPicks);
  }
  subtract->setColor(tf->getColor());
  
  project->addFeature(subtract);
  project->connectInsert(tf->getId(), subtract->getId(), ftr::InputType{ftr::InputType::target});
  
  node->sendBlocked(msg::buildHideThreeD(tf->getId()));
  node->sendBlocked(msg::buildHideOverlay(tf->getId()));
  
  ftr::Picks toolPicks;
  for (std::size_t index = 1; index < containers.size(); ++index)
  {
    ftr::Pick toolPick;
    if (!containers.at(index).shapeId.is_nil())
    {
      toolPick.shapeHistory = project->getShapeHistory().createDevolveHistory(containers.at(index).shapeId);
      toolPicks.push_back(toolPick);
    }
    project->connect(containers.at(index).featureId, subtract->getId(), ftr::InputType{ftr::InputType::tool});
    node->sendBlocked(msg::buildHideThreeD(containers.at(index).featureId));
    node->sendBlocked(msg::buildHideOverlay(containers.at(index).featureId));
    
  }
  subtract->setToolPicks(toolPicks);
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
}


SubtractEdit::SubtractEdit(ftr::Base *in) : Base()
{
  //command manager edit dispatcher dispatches on ftr::type, so we know the type of 'in'
  subtract = dynamic_cast<ftr::Subtract*>(in);
  assert(subtract);
}

SubtractEdit::~SubtractEdit()
{
  if (dialog)
    dialog->deleteLater();
}

std::string SubtractEdit::getStatusMessage()
{
  return QObject::tr("Editing subtract").toStdString();
}

void SubtractEdit::activate()
{
  isActive = true;
  if (!dialog)
  {
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    dialog = new dlg::Boolean(subtract, mainWindow);
    dialog->setEditDialog();
  }

  dialog->show();
  dialog->raise();
  dialog->activateWindow();
}

void SubtractEdit::deactivate()
{
  dialog->hide();
  isActive = false;
}
