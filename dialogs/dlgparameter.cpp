/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2015  Thomas S. Anderson blobfish.at.gmx.com
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

#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "application/appapplication.h"
#include "project/prjproject.h"
#include "project/prjmessage.h"
#include "application/appmainwindow.h"
#include "feature/ftrbase.h"
#include "parameter/prmparameter.h"
#include "message/msgnode.h"
#include "message/msgsift.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "commandview/cmvparameterwidgets.h"
#include "dialogs/dlgwidgetgeometry.h"
#include "dialogs/dlgparameter.h"

using namespace dlg;

Parameter::Parameter(prm::Parameter *parameterIn, const boost::uuids::uuid &idIn):
  QDialog(app::instance()->getMainWindow()),
  parameter(parameterIn)
  , pObserver
  (
    new prm::Observer //widgets take care of themselves.
    (
      prm::Handler()
      , prm::Handler()
      , std::bind(&Parameter::activeHasChanged, this) //just edit dialog title
    )
  )
{
  assert(parameter);
  
  feature = app::instance()->getProject()->findFeature(idIn);
  assert(feature);
  buildGui();
  
  parameter->connect(*pObserver);
  
  node = std::make_unique<msg::Node>();
  node->connect(msg::hub());
  sift = std::make_unique<msg::Sift>();
  sift->name = "dlg::Parameter";
  sift->insert
  (
    msg::Response | msg::Pre | msg::Remove | msg::Feature
    , std::bind(&Parameter::featureRemovedDispatched, this, std::placeholders::_1)
  );
  node->setHandler(std::bind(&msg::Sift::receive, sift.get(), std::placeholders::_1));
  
  WidgetGeometry *filter = new WidgetGeometry(this, "dlg::Parameter");
  this->installEventFilter(filter);
  
  this->setAttribute(Qt::WA_DeleteOnClose);
}

Parameter::~Parameter(){}

void Parameter::buildGui()
{
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  
  QHBoxLayout *editLayout = new QHBoxLayout();
  QLabel *nameLabel = new QLabel(parameter->getName(), this);
  editLayout->addWidget(nameLabel);
  
  editWidget = cmv::ParameterWidget::buildWidget(this, parameter);
  connect(editWidget, &cmv::ParameterBase::prmValueChanged, this, &Parameter::valueHasChanged);
  connect(editWidget, &cmv::ParameterBase::prmConstantChanged, this, &Parameter::constantHasChanged);
  editLayout->addWidget(editWidget);
  
  mainLayout->addLayout(editLayout);
  activeHasChanged(); //just sets window title
}

void Parameter::valueHasChanged()
{
  std::ostringstream gitStream;
  gitStream
    << QObject::tr("Feature: ").toStdString() << feature->getName().toStdString()
    << QObject::tr("    Parameter ").toStdString() << parameter->getName().toStdString()
    << QObject::tr("    changed to: ").toStdString() << static_cast<QString>(*parameter).toStdString();
  node->sendBlocked(msg::buildGitMessage(gitStream.str()));
  if (prf::manager().rootPtr->dragger().triggerUpdateOnFinish())
    node->sendBlocked(msg::Mask(msg::Request | msg::Project | msg::Update));
}

void Parameter::constantHasChanged()
{
  // no git messages here because the expression manager takes care of them.
  if (parameter->isConstant() && prf::manager().rootPtr->dragger().triggerUpdateOnFinish())
    node->sendBlocked(msg::Mask(msg::Request | msg::Project | msg::Update));
}

void Parameter::featureRemovedDispatched(const msg::Message &messageIn)
{
  prj::Message pMessage = messageIn.getPRJ();
  if(pMessage.feature->getId() == feature->getId())
    this->reject();
}

void Parameter::activeHasChanged()
{
  //the widgets themselves enable disable accordingly
  
  QString title = feature->getName() + ": ";
  title += QString::fromStdString(parameter->getValueTypeString());
  if (!parameter->isActive())
    title += tr(" INACTIVE");
  this->setWindowTitle(title);
}
