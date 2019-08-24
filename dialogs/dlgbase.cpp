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

#include "application/appapplication.h"
#include "application/appmainwindow.h"
#include "project/prjproject.h"
#include "feature/ftrbase.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "message/msgnode.h"
#include "message/msgsift.h"
#include "dialogs/dlgwidgetgeometry.h"
#include "dialogs/dlgbase.h"

using namespace dlg;

Base::Base(ftr::Base *fIn, bool isEditIn)
: Base(fIn, app::instance()->getMainWindow(), isEditIn)
{}

Base::Base(ftr::Base *fIn, QWidget *pIn, bool isEditIn)
: QDialog(pIn)
, isEditDialog(isEditIn)
, node(std::make_unique<msg::Node>())
, sift(std::make_unique<msg::Sift>())
{
  std::string fts = fIn->getTypeString();
  std::string uniqueName = "dlg::";
  uniqueName += fts;
  
  node->connect(msg::hub());
  sift->name = uniqueName;
  node->setHandler(std::bind(&msg::Sift::receive, sift.get(), std::placeholders::_1));
  
  QString title = QString::fromStdString(fts);
  title += " ";
  if (isEditDialog)
    title += tr("Edit");
  else
    title += tr("Construct");
  setWindowTitle(title);
  
  WidgetGeometry *filter = new WidgetGeometry(this, QString::fromStdString(uniqueName));
  this->installEventFilter(filter);
  
  project = app::instance()->getProject();
}

Base::~Base() = default;

void Base::queueUpdate()
{
  if (prf::manager().rootPtr->dragger().triggerUpdateOnFinish())
    app::instance()->queuedMessage(msg::Message(msg::Request | msg::Project | msg::Update));
}

void Base::commandDone()
{
  app::instance()->queuedMessage(msg::Message(msg::Mask(msg::Request | msg::Command | msg::Done)));
}
