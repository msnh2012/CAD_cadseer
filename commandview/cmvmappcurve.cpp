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
#include "feature/ftrmappcurve.h"
#include "command/cmdmappcurve.h"
#include "commandview/cmvmappcurve.h"

using boost::uuids::uuid;

using namespace cmv;

struct MapPCurve::Stow
{
  cmd::MapPCurve *command;
  cmv::MapPCurve *view;
  prm::Parameters parameters;
  cmv::tbl::Model *prmModel = nullptr;
  cmv::tbl::View *prmView = nullptr;
  
  Stow(cmd::MapPCurve *cIn, cmv::MapPCurve *vIn)
  : command(cIn)
  , view(vIn)
  {
    parameters = command->feature->getParameters();
    buildGui();
    connect(prmModel, &tbl::Model::dataChanged, view, &MapPCurve::modelChanged);
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
      cue.singleSelection = true;
      cue.mask = slc::ObjectsEnabled | slc::FacesBoth;
      cue.statusPrompt = tr("Select Face");
      cue.accrueEnabled = false;
      prmModel->setCue(command->feature->getParameter(ftr::MapPCurve::Tags::facePick), cue);
    }
    
    {
      tbl::SelectionCue cue;
      cue.singleSelection = false;
      cue.mask = slc::ObjectsBoth;
      cue.statusPrompt = tr("Select Sketch To Project");
      cue.accrueEnabled = false;
      prmModel->setCue(command->feature->getParameter(ftr::MapPCurve::Tags::edgePicks), cue);
    }
  }
};

MapPCurve::MapPCurve(cmd::MapPCurve *cIn)
: Base("cmv::MapPCurve")
, stow(new Stow(cIn, this))
{
  goSelectionToolbar();
  goMaskDefault();
}

MapPCurve::~MapPCurve() = default;

void MapPCurve::modelChanged(const QModelIndex &index, const QModelIndex&)
{
  if (!index.isValid())
    return;
  auto tag = stow->parameters.at(index.row())->getTag();
  if (tag == ftr::MapPCurve::Tags::facePick || tag == ftr::MapPCurve::Tags::edgePicks)
  {
    const auto &fs = stow->prmModel->getMessages(stow->command->feature->getParameter(ftr::MapPCurve::Tags::facePick));
    const auto &es = stow->prmModel->getMessages(stow->command->feature->getParameter(ftr::MapPCurve::Tags::edgePicks));
    stow->command->setSelections(fs, es);
  }
  
  stow->command->localUpdate();
  node->sendBlocked(msg::buildStatusMessage(stow->command->getStatusMessage()));
  goMaskDefault();
}
