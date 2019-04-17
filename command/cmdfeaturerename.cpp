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

#include <sstream>

#include <QInputDialog>

#include "tools/idtools.h"
#include "application/appapplication.h"
#include "application/appmainwindow.h"
#include "message/msgnode.h"
#include "message/msgsift.h"
#include "selection/slceventhandler.h"
#include "project/prjproject.h"
#include "feature/ftrmessage.h"
#include "feature/ftrbase.h"
#include "command/cmdfeaturerename.h"

using namespace cmd;

using boost::uuids::uuid;

FeatureRename::FeatureRename() : Base(), id(gu::createNilId()), name()
{
  sift = std::make_unique<msg::Sift>();
  sift->name = "cmd::FeatureRename";
  node->setHandler(std::bind(&msg::Sift::receive, sift.get(), std::placeholders::_1));
  
  setupDispatcher();
  
  shouldUpdate = false;
}

FeatureRename::~FeatureRename() {}

std::string FeatureRename::getStatusMessage()
{
  return QObject::tr("Select object to rename").toStdString();
}

void FeatureRename::activate()
{
  isActive = true;
  go();
}

void FeatureRename::deactivate()
{
  isActive = false;
}

void FeatureRename::setFromMessage(const msg::Message &mIn)
{
  ftr::Message fm = mIn.getFTR();
  id = fm.featureId;
  name = fm.string;
}

void FeatureRename::go() //re-enters from dispatch.
{
  if (id.is_nil()) //try to get feature id from selection.
  {
    const slc::Containers &containers = eventHandler->getSelections();
    for (const auto &container : containers)
    {
      if (container.selectionType != slc::Type::Object)
        continue;
      id = container.featureId;
      break; //only works on the first selected object.
    }
  }
  
  if (!id.is_nil())
  {
    if(name.isEmpty())
    {
      ftr::Base *feature = project->findFeature(id);
      QString oldName = feature->getName();
      
      name = QInputDialog::getText
      (
        application->getMainWindow(),
        QString("Rename Feature"),
        QString("Rename Feature"),
        QLineEdit::Normal,
        oldName
      );
    }
    
    if (!name.isEmpty())
      goRename();
    
    sendDone();
  }
  else
    node->send(msg::buildSelectionMask(slc::ObjectsEnabled | slc::ObjectsSelectable));
}

void FeatureRename::goRename()
{
  ftr::Base *feature = project->findFeature(id);
  assert(feature);
  QString oldName = feature->getName();
  feature->setName(name);
      
  //setting the name doesn't make the feature dirty and thus doesn't
  //serialize it. Here we force a serialization so rename is in sync
  //with git, but doesn't trigger an unneeded update.
  feature->serialWrite(project->getSaveDirectory());
  
  std::ostringstream gitStream;
  gitStream << "Rename feature id: " << gu::idToShortString(feature->getId())
  << "    From: " << oldName.toStdString()
  << "    To: " << name.toStdString();
  node->send(msg::buildGitMessage(gitStream.str()));
}

void FeatureRename::setupDispatcher()
{
  sift->insert
  (
    msg::Response | msg::Post | msg::Selection | msg::Add
    , std::bind(&FeatureRename::selectionAdditionDispatched, this, std::placeholders::_1)
  );
}

void FeatureRename::selectionAdditionDispatched(const msg::Message&)
{
  if (!isActive)
    return;
  
  go();
}
