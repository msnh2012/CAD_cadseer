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

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "commandview/cmvselectioncue.h"
#include "commandview/cmvtable.h"
#include "tools/featuretools.h"
#include "feature/ftrrevolve.h"
#include "command/cmdrevolve.h"
#include "commandview/cmvrevolve.h"

using boost::uuids::uuid;

using namespace cmv;

struct Revolve::Stow
{
  cmd::Revolve *command;
  cmv::Revolve *view;
  prm::Parameters parameters;
  cmv::tbl::Model *prmModel = nullptr;
  cmv::tbl::View *prmView = nullptr;
  
  Stow(cmd::Revolve *cIn, cmv::Revolve *vIn)
  : command(cIn)
  , view(vIn)
  {
    parameters = command->feature->getParameters();
    buildGui();
    connect(prmModel, &tbl::Model::dataChanged, view, &Revolve::modelChanged);
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
    cue.singleSelection = true; //temp. eventually false.
    cue.mask = slc::ObjectsBoth | slc::FacesBoth | slc::WiresEnabled | slc::EdgesEnabled | slc::AllPointsEnabled;
    cue.statusPrompt = tr("Select Profile Geometry");
    cue.accrueEnabled = false;
    cue.forceTangentAccrue = false;
    prmModel->setCue(command->feature->getParameter(ftr::Revolve::PrmTags::profilePicks), cue);
    
    cue.singleSelection = false;
    cue.mask = slc::ObjectsBoth | slc::FacesEnabled | slc::EdgesBoth | slc::AllPointsEnabled;
    cue.statusPrompt = tr("Select Points, Edge, Face Or Datum For Axis");
    prmModel->setCue(command->feature->getParameter(ftr::Revolve::PrmTags::axisPicks), cue);
  }
};

Revolve::Revolve(cmd::Revolve *cIn)
: Base("cmv::Revolve")
, stow(new Stow(cIn, this))
{
  goSelectionToolbar();
  goMaskDefault();
}

Revolve::~Revolve() = default;

void Revolve::modelChanged(const QModelIndex &index, const QModelIndex&)
{
  if (!index.isValid())
    return;
  
  auto *profilePicks = stow->command->feature->getParameter(ftr::Revolve::PrmTags::profilePicks);
  auto *axisPicks = stow->command->feature->getParameter(ftr::Revolve::PrmTags::axisPicks);
  
  auto goPicks = [&]()
  {
    const auto &profile = stow->prmModel->getMessages(profilePicks);
    const auto &axis = stow->prmModel->getMessages(axisPicks);
    stow->command->setToAxisPicks(profile, axis);
  };
  
  auto goParameters = [&]()
  {
    const auto &profile = stow->prmModel->getMessages(profilePicks);
    stow->command->setToAxisParameter(profile);
  };
  
  auto tag = stow->parameters.at(index.row())->getTag();
  if
  (
    tag == ftr::Revolve::PrmTags::axisType
    || tag == ftr::Revolve::PrmTags::profilePicks
    || tag == ftr::Revolve::PrmTags::axisPicks
  )
  {
    int type = stow->command->feature->getParameter(ftr::Revolve::PrmTags::axisType)->getInt();
    if (type == 0)
      goPicks();
    else if (type == 1)
      goParameters();
  }
  stow->command->localUpdate();
  stow->prmView->updateHideInactive();
  node->sendBlocked(msg::buildStatusMessage(stow->command->getStatusMessage()));
  goMaskDefault();
}
