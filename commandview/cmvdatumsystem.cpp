/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2020 Thomas S. Anderson blobfish.at.gmx.com
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

#include <QSettings>
#include <QComboBox>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "message/msgmessage.h"
#include "message/msgnode.h"
#include "commandview/cmvselectioncue.h"
#include "commandview/cmvtable.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "tools/featuretools.h"
#include "tools/idtools.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrdatumsystem.h"
#include "command/cmddatumsystem.h"
#include "commandview/cmvdatumsystem.h"

using boost::uuids::uuid;

using namespace cmv;

struct DatumSystem::Stow
{
  cmd::DatumSystem *command;
  cmv::DatumSystem *view;
  prm::Parameters parameters;
  cmv::tbl::Model *prmModel = nullptr;
  cmv::tbl::View *prmView = nullptr;
 
  Stow(cmd::DatumSystem *cIn, cmv::DatumSystem *vIn)
  : command(cIn)
  , view(vIn)
  {
    parameters = command->feature->getParameters();
    buildGui();
    connect(prmModel, &tbl::Model::dataChanged, view, &DatumSystem::modelChanged);
  }
  
  void buildGui()
  {
    QVBoxLayout *mainLayout = new QVBoxLayout();
    view->setLayout(mainLayout);
    Base::clearContentMargins(view);
    view->setSizePolicy(view->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);
    
    prmModel = new tbl::Model(view, command->feature);
    prmView = new tbl::View(view, prmModel, true);
    mainLayout->addWidget(prmView);
    
    const auto *ft = command->feature;
    
    //no cues for constant
    {
      //linked
      tbl::SelectionCue cue;
      cue.singleSelection = true;
      cue.mask = slc::ObjectsBoth;
      cue.statusPrompt = tr("Select Feature To Link Axis");
      cue.accrueEnabled = false;
      prmModel->setCue(ft->getParameter(ftr::DatumSystem::Tags::Linked), cue);
    }
    {
      //points
      tbl::SelectionCue cue;
      cue.singleSelection = false;
      cue.mask = slc::AllPointsEnabled | slc::EndPointsSelectable;
      cue.statusPrompt = tr("Select 2 Points For Axis");
      cue.accrueEnabled = false;
      prmModel->setCue(ft->getParameter(ftr::DatumSystem::Tags::Points), cue);
    }
  }
};

DatumSystem::DatumSystem(cmd::DatumSystem *cIn)
: Base("cmv::DatumSystem")
, stow(new Stow(cIn, this))
{
  node->sendBlocked(msg::buildStatusMessage("Double Click Parameter To Edit"));
  goSelectionToolbar();
  goMaskDefault();
}

DatumSystem::~DatumSystem() = default;

void DatumSystem::modelChanged(const QModelIndex &index, const QModelIndex&)
{
  if (!index.isValid())
    return;
  
  prm::Parameter *par = stow->parameters.at(index.row());
  if (par->getTag() == ftr::DatumSystem::Tags::SystemType)
  {
    const auto *ft = stow->command->feature;
    auto systemType = static_cast<ftr::DatumSystem::SystemType>(par->getInt());
    switch(systemType)
    {
      case ftr::DatumSystem::SystemType::Constant:{
        stow->command->setConstant();
        break;}
      case ftr::DatumSystem::SystemType::Linked:{
        const auto *p = ft->getParameter(ftr::DatumSystem::Tags::Linked);
        stow->command->setLinked(stow->prmModel->getMessages(p));
        break;}
      case ftr::DatumSystem::SystemType::Through3Points:{
        const auto *p = ft->getParameter(ftr::DatumSystem::Tags::Points);
        stow->command->set3Points(stow->prmModel->getMessages(p));
        break;}
    }
    stow->prmView->updateHideInactive();
  }
  
  if (par->getTag() == ftr::DatumSystem::Tags::Linked)
    stow->command->setLinked(stow->prmModel->getMessages(par));
  else if (par->getTag() == ftr::DatumSystem::Tags::Points)
    stow->command->set3Points(stow->prmModel->getMessages(par));
  else if (par->getTag() == prm::Tags::AutoSize)
    stow->prmView->updateHideInactive();
  
  stow->command->localUpdate();
  node->sendBlocked(msg::buildStatusMessage(stow->command->getStatusMessage()));
  goMaskDefault();
}
