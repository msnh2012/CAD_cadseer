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

// #include <cassert>
// #include <boost/optional/optional.hpp>

#include <QSettings>
#include <QVBoxLayout>

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "commandview/cmvselectioncue.h"
#include "commandview/cmvtable.h"
#include "tools/featuretools.h"
#include "feature/ftrremovefaces.h"
#include "command/cmdremovefaces.h"
#include "commandview/cmvremovefaces.h"

using boost::uuids::uuid;

using namespace cmv;

struct RemoveFaces::Stow
{
  cmd::RemoveFaces *command;
  cmv::RemoveFaces *view;
  prm::Parameters parameters;
  cmv::tbl::Model *prmModel = nullptr;
  cmv::tbl::View *prmView = nullptr;
  
  Stow(cmd::RemoveFaces *cIn, cmv::RemoveFaces *vIn)
  : command(cIn)
  , view(vIn)
  {
    parameters = command->feature->getParameters();
    buildGui();
    connect(prmModel, &tbl::Model::dataChanged, view, &RemoveFaces::modelChanged);
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
    cue.mask = slc::FacesBoth;
    cue.statusPrompt = tr("Select Faces To Remove");
    cue.accrueEnabled = false;
    cue.forceTangentAccrue = false;
    prmModel->setCue(command->feature->getParameter(prm::Tags::Picks), cue);
  }
};

RemoveFaces::RemoveFaces(cmd::RemoveFaces *cIn)
: Base("cmv::RemoveFaces")
, stow(new Stow(cIn, this))
{
  goSelectionToolbar();
  goMaskDefault();
}

RemoveFaces::~RemoveFaces() = default;

void RemoveFaces::modelChanged(const QModelIndex &index, const QModelIndex&)
{
  if (!index.isValid())
    return;
  auto tag = stow->parameters.at(index.row())->getTag();
  if (tag == prm::Tags::Picks)
  {
    const auto &fs = stow->prmModel->getMessages(stow->parameters.at(index.row()));
    stow->command->setSelections(fs);
  }
  stow->command->localUpdate();
  node->sendBlocked(msg::buildStatusMessage(stow->command->getStatusMessage()));
  goMaskDefault();
}
