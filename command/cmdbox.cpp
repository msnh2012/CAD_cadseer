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

#include <osg/Matrixd>

#include "application/appmainwindow.h"
#include "viewer/vwrwidget.h"
#include "selection/slceventhandler.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "dialogs/dlgbox.h"
#include "feature/ftrinputtype.h"
#include "parameter/prmparameter.h"
#include "feature/ftrbox.h"
#include "command/cmdbox.h"

using namespace cmd;

Box::Box() : Base() {}

Box::~Box()
{
  if (dialog)
    dialog->deleteLater();
}

std::string Box::getStatusMessage()
{
  return QObject::tr("Edit Box feature").toStdString();
}

void Box::activate()
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

void Box::deactivate()
{
  if (dialog)
    dialog->hide();
  isActive = false;
}

void Box::go()
{
  std::shared_ptr<ftr::Box> box = std::make_shared<ftr::Box>();
  box->setCSys(viewer->getCurrentSystem());
  project->addFeature(box);
  
  for (const auto &c : eventHandler->getSelections())
  {
    ftr::Base *tf = project->findFeature(c.featureId);
    if (!tf || !tf->hasParameter(prm::Names::CSys))
      continue;
    project->connectInsert(tf->getId(), box->getId(), ftr::InputType{ftr::InputType::linkCSys});
    break;
  }
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  node->sendBlocked(msg::buildStatusMessage("Box created", 2.0));
  box->updateModel(project->getPayload(box->getId()));
  box->updateVisual();
  box->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  dialog = new dlg::Box(box.get(), mainWindow);

  shouldUpdate = false;
}

BoxEdit::BoxEdit(ftr::Base *in) : Base()
{
  //command manager edit dispatcher dispatches on ftr::type, so we know the type of 'in'
  feature = dynamic_cast<ftr::Box*>(in);
  assert(feature);
  shouldUpdate = false; //dialog controls.
}

BoxEdit::~BoxEdit()
{
  if (dialog)
    dialog->deleteLater();
}

std::string BoxEdit::getStatusMessage()
{
  return QObject::tr("Editing box").toStdString();
}

void BoxEdit::activate()
{
  isActive = true;
  if (!dialog)
  {
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    dialog = new dlg::Box(feature, mainWindow, true);
  }

  dialog->show();
  dialog->raise();
  dialog->activateWindow();
}

void BoxEdit::deactivate()
{
  dialog->hide();
  isActive = false;
}
