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
#include "message/msgnode.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "commandview/cmvselectioncue.h"
#include "commandview/cmvtable.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrtrim.h"
#include "command/cmdtrim.h"
#include "commandview/cmvtrim.h"

using boost::uuids::uuid;

using namespace cmv;

struct Trim::Stow
{
  cmd::Trim *command;
  cmv::Trim *view;
  prm::Parameters parameters;
  cmv::tbl::Model *prmModel = nullptr;
  cmv::tbl::View *prmView = nullptr;
  
  Stow(cmd::Trim *cIn, cmv::Trim *vIn)
  : command(cIn)
  , view(vIn)
  {
    parameters = command->feature->getParameters();
    buildGui();
    connect(prmModel, &tbl::Model::dataChanged, view, &Trim::modelChanged);
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
    cue.mask = slc::ObjectsBoth | slc::SolidsEnabled | slc::ShellsEnabled | slc::FacesEnabled;
    cue.statusPrompt = tr("Select Entity To Trim");
    cue.accrueEnabled = false;
    cue.forceTangentAccrue = false;
    prmModel->setCue(command->feature->getParameter(ftr::Trim::PrmTags::targetPicks), cue);
    
    cue.statusPrompt = tr("Select Trimming Entity");
    cue.mask = slc::ObjectsBoth | slc::ShellsEnabled | slc::FacesEnabled;
    prmModel->setCue(command->feature->getParameter(ftr::Trim::PrmTags::toolPicks), cue);
  }
};

Trim::Trim(cmd::Trim *cIn)
: Base("cmv::Trim")
, stow(new Stow(cIn, this))
{
  goSelectionToolbar();
  goMaskDefault();
}

Trim::~Trim() = default;

void Trim::modelChanged(const QModelIndex &index, const QModelIndex&)
{
  if (!index.isValid())
    return;
  auto tag = stow->parameters.at(index.row())->getTag();
  if (tag == ftr::Trim::PrmTags::targetPicks || tag == ftr::Trim::PrmTags::toolPicks)
  {
    const auto &targets = stow->prmModel->getMessages(stow->command->feature->getParameter(ftr::Trim::PrmTags::targetPicks));
    const auto &tools = stow->prmModel->getMessages(stow->command->feature->getParameter(ftr::Trim::PrmTags::toolPicks));
    stow->command->setSelections(targets, tools);
  }
  stow->command->localUpdate();
  node->sendBlocked(msg::buildStatusMessage(stow->command->getStatusMessage()));
  goMaskDefault();
}
