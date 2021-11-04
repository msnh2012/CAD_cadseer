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
#include <QVBoxLayout>
#include <QStackedWidget>

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "message/msgmessage.h"
#include "message/msgnode.h"
#include "commandview/cmvselectioncue.h"
#include "commandview/cmvtable.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "tools/featuretools.h"
#include "feature/ftrextrude.h"
#include "command/cmdextrude.h"
#include "commandview/cmvextrude.h"

using boost::uuids::uuid;

using namespace cmv;

struct Extrude::Stow
{
  cmd::Extrude *command;
  cmv::Extrude *view;
  prm::Parameters parameters;
  cmv::tbl::Model *prmModel = nullptr;
  cmv::tbl::View *prmView = nullptr;
  
  Stow(cmd::Extrude *cIn, cmv::Extrude *vIn)
  : command(cIn)
  , view(vIn)
  {
    parameters = command->feature->getParameters();
    buildGui();
    connect(prmModel, &tbl::Model::dataChanged, view, &Extrude::modelChanged);
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
    
    {
      tbl::SelectionCue cue;
      cue.singleSelection = false;
      cue.mask = slc::ObjectsBoth | slc::FacesBoth | slc::WiresEnabled | slc::EdgesBoth | slc::AllPointsEnabled;
      cue.statusPrompt = tr("Select Profiles To Extrude");
      cue.accrueEnabled = false;
      prmModel->setCue(command->feature->getParameter(ftr::Extrude::PrmTags::profilePicks), cue);
    }
    
    {
      tbl::SelectionCue cue;
      cue.singleSelection = false;
      cue.mask = slc::ObjectsBoth | slc::FacesEnabled | slc::AllPointsEnabled | slc::EndPointsSelectable;
      cue.statusPrompt = tr("Select Axis. Two Points, One Face or One Datum Axis");
      cue.accrueEnabled = false;
      prmModel->setCue(command->feature->getParameter(ftr::Extrude::PrmTags::axisPicks), cue);
    }
  }
};

Extrude::Extrude(cmd::Extrude *cIn)
: Base("cmv::Extrude")
, stow(new Stow(cIn, this))
{
  goSelectionToolbar();
  goMaskDefault();
}

Extrude::~Extrude() = default;

void Extrude::modelChanged(const QModelIndex &index, const QModelIndex&)
{
  if (!index.isValid())
    return;
  
  using ftr::Extrude::PrmTags::profilePicks;
  using ftr::Extrude::PrmTags::axisPicks;
  using ftr::Extrude::PrmTags::extrusionType;
  using ftr::Extrude::ExtrusionType;
  
  const ftr::Base *feature = stow->command->feature;
  const auto &profiles = stow->prmModel->getMessages(feature->getParameter(profilePicks));
  const auto &axes = stow->prmModel->getMessages(feature->getParameter(axisPicks));
  
  auto dispatch = [&]()
  {
    switch(static_cast<ExtrusionType>(feature->getParameter(extrusionType)->getInt()))
    {
      case ExtrusionType::Constant: stow->command->setToAxisParameter(profiles); break;
      case ExtrusionType::Infer: stow->command->setToAxisInfer(profiles); break;
      case ExtrusionType::Picks: stow->command->setToAxisPicks(profiles, axes); break;
      default: assert(0); break; //unrecognized extrude type.
    }
  };
  
  auto changedTag = stow->parameters.at(index.row())->getTag();
  if (changedTag == extrusionType)
  {
    dispatch();
    stow->prmView->updateHideInactive();
  }
  else if (changedTag == profilePicks || changedTag == axisPicks)
    dispatch();
  
  stow->command->localUpdate();
  node->sendBlocked(msg::buildStatusMessage(stow->command->getStatusMessage()));
  goMaskDefault();
}
