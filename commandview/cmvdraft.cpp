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
#include <QVBoxLayout>

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "message/msgmessage.h"
#include "message/msgnode.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "commandview/cmvselectioncue.h"
#include "commandview/cmvtable.h"
#include "tools/featuretools.h"
#include "feature/ftrdraft.h"
#include "command/cmddraft.h"
#include "commandview/cmvdraft.h"

using boost::uuids::uuid;

using namespace cmv;

struct Draft::Stow
{
  cmd::Draft *command;
  cmv::Draft *view;
  prm::Parameters parameters;
  cmv::tbl::Model *prmModel = nullptr;
  cmv::tbl::View *prmView = nullptr;
  
  Stow(cmd::Draft *cIn, cmv::Draft *vIn)
  : command(cIn)
  , view(vIn)
  {
    parameters = command->feature->getParameters();
    buildGui();
    connect(prmModel, &tbl::Model::dataChanged, view, &Draft::modelChanged);
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
    cue.mask = slc::ObjectsEnabled | slc::FacesBoth | slc::EdgesEnabled;
    cue.statusPrompt = tr("Select Neutral Plane");
    cue.accrueEnabled = false;
    prmModel->setCue(command->feature->getParameter(ftr::Draft::PrmTags::neutralPick), cue);
    
    tbl::SelectionCue targetCue;
    targetCue.singleSelection = false;
    targetCue.mask = slc::FacesBoth;
    targetCue.statusPrompt = tr("Select Target Faces");
    targetCue.accrueEnabled = false;
    targetCue.forceTangentAccrue = true;
    prmModel->setCue(command->feature->getParameter(ftr::Draft::PrmTags::targetPicks), targetCue);
  }
};

Draft::Draft(cmd::Draft *cIn)
: Base("cmv::Draft")
, stow(new Stow(cIn, this))
{}

Draft::~Draft() = default;

void Draft::modelChanged(const QModelIndex &index, const QModelIndex&)
{
  if (!index.isValid())
    return;
  auto changedTag = stow->parameters.at(index.row())->getTag();
  if (changedTag == ftr::Draft::PrmTags::neutralPick || changedTag == ftr::Draft::PrmTags::targetPicks)
  {
    const auto &neutralPick = stow->prmModel->getMessages(stow->command->feature->getParameter(ftr::Draft::PrmTags::neutralPick));
    const auto &targetPicks = stow->prmModel->getMessages(stow->command->feature->getParameter(ftr::Draft::PrmTags::targetPicks));
    stow->command->setSelections(neutralPick, targetPicks);
  }
  
  stow->command->localUpdate();
  node->sendBlocked(msg::buildStatusMessage(stow->command->getStatusMessage()));
}
