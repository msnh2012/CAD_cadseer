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
#include "commandview/cmvselectioncue.h"
#include "commandview/cmvtable.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "tools/featuretools.h"
#include "feature/ftrextract.h"
#include "command/cmdextract.h"
#include "commandview/cmvextract.h"

using boost::uuids::uuid;

using namespace cmv;

struct Extract::Stow
{
  cmd::Extract *command;
  cmv::Extract *view;
  prm::Parameters parameters;
  cmv::tbl::Model *prmModel = nullptr;
  cmv::tbl::View *prmView = nullptr;
  
  Stow(cmd::Extract *cIn, cmv::Extract *vIn)
  : command(cIn)
  , view(vIn)
  {
    parameters = command->feature->getParameters();
    buildGui();
    connect(prmModel, &tbl::Model::dataChanged, view, &Extract::modelChanged);
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
    cue.singleSelection = false;
    cue.mask = slc::AllEnabled;
    cue.statusPrompt = tr("Select Entities");
    cue.accrueEnabled = true;
    prmModel->setCue(command->feature->getParameter(prm::Tags::Picks), cue);
  }
};

Extract::Extract(cmd::Extract *cIn)
: Base("cmv::Extract")
, stow(new Stow(cIn, this))
{
  goSelectionToolbar();
  goMaskDefault();
}

Extract::~Extract() = default;

void Extract::modelChanged(const QModelIndex &index, const QModelIndex&)
{
  if (!index.isValid())
    return;
  
  auto changedTag = stow->parameters.at(index.row())->getTag();
  if (changedTag == prm::Tags::Picks)
  {
    const auto &picks = stow->prmModel->getMessages(stow->command->feature->getParameter(prm::Tags::Picks));
    stow->command->setSelections(picks);
    
    //enable or disable tangle angle parameter.
    bool enableAngle = false;
    for (const auto &p : picks)
    {
      if (p.accrue == slc::Accrue::Tangent)
      {
        enableAngle = true;
        break;
      }
    }
    stow->command->feature->getParameter(prm::Tags::Angle)->setActive(enableAngle);
    stow->prmView->updateHideInactive();
  }
  
  stow->command->localUpdate();
  node->sendBlocked(msg::buildStatusMessage(stow->command->getStatusMessage()));
  goMaskDefault();
}
