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

#include <boost/variant.hpp>

#include "application/mainwindow.h"
#include "message/node.h"
#include "project/project.h"
#include "feature/sketch.h"
#include "dialogs/sketch.h"
#include "command/sketch.h"

using boost::uuids::uuid;

using namespace cmd;

Sketch::Sketch() : Base() {}
Sketch::~Sketch()
{
  if (dialog)
    dialog->deleteLater();
}

std::string Sketch::getStatusMessage()
{
  return QObject::tr("Select plane for sketch").toStdString();
}

void Sketch::activate()
{
  isActive = true;
  
  if (!dialog)
    go();
  
  dialog->show();
  dialog->raise();
  dialog->activateWindow();
}

void Sketch::deactivate()
{
  isActive = false;
}

void Sketch::go()
{
  std::shared_ptr<ftr::Sketch> nf(new ftr::Sketch());
  nf->buildDefault();
  project->addFeature(nf);
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  
  shouldUpdate = false;
  
  dialog = new dlg::Sketch(nf.get(), mainWindow);
}


SketchEdit::SketchEdit(ftr::Base *fIn) : Base()
{
  feature = dynamic_cast<ftr::Sketch*>(fIn);
  assert(feature);
}

SketchEdit::~SketchEdit()
{
  if (dialog)
    dialog->deleteLater();
}

std::string SketchEdit::getStatusMessage()
{
  return QObject::tr("Edit sketch feature").toStdString();
}

void SketchEdit::activate()
{
  if (!dialog)
  {
    feature->setModelDirty();
    dialog = new dlg::Sketch(feature, mainWindow);
  }
  
  isActive = true;
  dialog->show();
  dialog->raise();
  dialog->activateWindow();
  
  shouldUpdate = false;
}

void SketchEdit::deactivate()
{
  dialog->hide();
  isActive = false;
}
