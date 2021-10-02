/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2021 Thomas S. Anderson blobfish.at.gmx.com
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
#include "message/msgmessage.h"
#include "message/msgnode.h"
#include "commandview/cmvselectioncue.h"
#include "commandview/cmvtable.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "feature/ftrundercut.h"
#include "command/cmdundercut.h"
#include "commandview/cmvundercut.h"

using boost::uuids::uuid;

using namespace cmv;

struct UnderCut::Stow
{
  cmd::UnderCut *command;
  cmv::UnderCut *view;
  prm::Parameters parameters;
  cmv::tbl::Model *prmModel = nullptr;
  cmv::tbl::View *prmView = nullptr;
  
  Stow(cmd::UnderCut *cIn, cmv::UnderCut *vIn)
  : command(cIn)
  , view(vIn)
  {
    parameters = command->feature->getParameters();
    buildGui();
    connect(prmModel, &tbl::Model::dataChanged, view, &UnderCut::modelChanged);
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
    
    tbl::SelectionCue cue;
    cue.singleSelection = true;
    cue.mask = slc::ObjectsBoth;
    cue.statusPrompt = tr("Select Object To Analyse");
    cue.accrueEnabled = false;
    prmModel->setCue(command->feature->getParameter(ftr::UnderCut::PrmTags::sourcePick), cue);
    
    cue.singleSelection = false;
    cue.mask = slc::ObjectsBoth | slc::AllPointsEnabled; //we can do more.
    cue.statusPrompt = tr("Select Datum Axis Or Points For Direction");
    prmModel->setCue(command->feature->getParameter(ftr::UnderCut::PrmTags::directionPicks), cue);
  }
};

UnderCut::UnderCut(cmd::UnderCut *cIn)
: Base("cmv::UnderCut")
, stow(std::make_unique<Stow>(cIn, this))
{
  goSelectionToolbar();
  goMaskDefault();
}

UnderCut::~UnderCut() = default;

void UnderCut::modelChanged(const QModelIndex &index, const QModelIndex&)
{
  if (!index.isValid())
    return;
  auto changedTag = stow->parameters.at(index.row())->getTag();
  if (changedTag == ftr::UnderCut::PrmTags::sourcePick || changedTag == ftr::UnderCut::PrmTags::directionPicks)
  {
    const auto &source = stow->prmModel->getMessages(stow->command->feature->getParameter(ftr::UnderCut::PrmTags::sourcePick));
    const auto &direction = stow->prmModel->getMessages(stow->command->feature->getParameter(ftr::UnderCut::PrmTags::directionPicks));
    stow->command->setSelections(source, direction);
  }
  if (changedTag == ftr::UnderCut::PrmTags::directionType)
    stow->prmView->updateHideInactive();
  stow->command->localUpdate();
  node->sendBlocked(msg::buildStatusMessage(stow->command->getStatusMessage()));
  goMaskDefault();
}
