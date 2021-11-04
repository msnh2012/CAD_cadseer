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
#include "commandview/cmvtable.h"
#include "dialogs/dlgwidgetgeometry.h"
#include "dialogs/dlgparameter.h"

using namespace dlg;

struct Parameter::Stow
{
  dlg::Parameter *dialog = nullptr;
  ftr::Base *feature = nullptr;
  prm::Parameter *parameter = nullptr;
  prm::Parameters parameters;
  prm::Observer pObserver;
  msg::Node node;
  msg::Sift sift;
  cmv::tbl::Model *model = nullptr;
  cmv::tbl::View *view = nullptr;
  
  Stow(dlg::Parameter *dIn, ftr::Base *fIn, prm::Parameter *pIn)
  : dialog(dIn)
  , feature(fIn)
  , parameter(pIn)
  {
    assert(dialog);
    assert(feature);
    assert(parameter);
    
    parameters.push_back(parameter);
    
    pObserver.activeHandler = std::bind(&Stow::activeHasChanged, this);
    pIn->connect(pObserver);
    
    node.connect(msg::hub());
    sift.name = "dlg::Parameter";
    sift.insert
    (
      msg::Response | msg::Pre | msg::Remove | msg::Feature
      , std::bind(&Stow::featureRemovedDispatched, this, std::placeholders::_1)
    );
    node.setHandler(std::bind(&msg::Sift::receive, &sift, std::placeholders::_1));
    
    buildGui();
    activeHasChanged(); //just sets window title
    connect(model, &cmv::tbl::Model::dataChanged, dialog, &Parameter::modelChanged);
    
    view->setCurrentIndex(model->index(0, 1));
    view->edit(model->index(0, 1));
  }
  
  void buildGui()
  {
    QVBoxLayout *mainLayout = new QVBoxLayout(dialog);
    dialog->setLayout(mainLayout);
    
    model = new cmv::tbl::Model(dialog, feature, parameters);
    view = new cmv::tbl::View(dialog, model);
    view->setMinimumHeight(10);
    mainLayout->addWidget(view);
  }
  
  void valueHasChanged()
  {
    std::ostringstream gitStream;
    gitStream
    << QObject::tr("Feature: ").toStdString() << feature->getName().toStdString()
    << QObject::tr("    Parameter ").toStdString() << parameter->getName().toStdString()
    << QObject::tr("    changed to: ").toStdString() << parameter->adaptToString();
    node.sendBlocked(msg::buildGitMessage(gitStream.str()));
    if (prf::manager().rootPtr->dragger().triggerUpdateOnFinish())
      node.sendBlocked(msg::Mask(msg::Request | msg::Project | msg::Update));
  }
  
  void activeHasChanged()
  {
    //the widgets themselves enable disable accordingly
    QString title = feature->getName() + ": ";
    title += QString::fromStdString(parameter->getValueTypeString());
    if (!parameter->isActive())
      title += tr(" INACTIVE");
    dialog->setWindowTitle(title);
  }
  
  //constant state change doesn't matter to the dialog. The widget will act
  //accordingly and we will get value changed message if applicable.
  
  void featureRemovedDispatched(const msg::Message &messageIn)
  {
    prj::Message pMessage = messageIn.getPRJ();
    if(pMessage.feature->getId() == feature->getId())
      dialog->reject();
  }
};

Parameter::Parameter(prm::Parameter *parameterIn, const boost::uuids::uuid &idIn)
: QDialog(app::instance()->getMainWindow())
, stow(std::make_unique<Stow>(this, app::instance()->getProject()->findFeature(idIn), parameterIn))
{
  WidgetGeometry *filter = new WidgetGeometry(this, "dlg::Parameter");
  this->installEventFilter(filter);
  
  this->setAttribute(Qt::WA_DeleteOnClose);
}

Parameter::~Parameter() = default;

void Parameter::modelChanged(const QModelIndex &index, const QModelIndex&)
{
  if (!index.isValid())
    return;
  
  stow->valueHasChanged();
}
