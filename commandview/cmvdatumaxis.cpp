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

#include <QVBoxLayout>

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "message/msgmessage.h"
#include "message/msgnode.h"
#include "commandview/cmvselectioncue.h"
#include "commandview/cmvtable.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "feature/ftrdatumaxis.h"
#include "command/cmddatumaxis.h"
#include "commandview/cmvdatumaxis.h"

using boost::uuids::uuid;

using namespace cmv;

struct DatumAxis::Stow
{
  cmd::DatumAxis *command = nullptr;
  cmv::DatumAxis *view = nullptr;
  prm::Parameters parameters;
  cmv::tbl::Model *prmModel = nullptr;
  cmv::tbl::View *prmView = nullptr;
  
  Stow(cmd::DatumAxis *cIn, cmv::DatumAxis *vIn)
  : command(cIn)
  , view(vIn)
  {
    parameters = command->feature->getParameters();
    buildGui();
    connect(prmModel, &tbl::Model::dataChanged, view, &DatumAxis::modelChanged);
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
    //no cues for parameter
    {
      //linked
      tbl::SelectionCue cue;
      cue.singleSelection = true;
      cue.mask = slc::ObjectsBoth;
      cue.statusPrompt = tr("Select Feature To Link Axis");
      cue.accrueEnabled = false;
      prmModel->setCue(ft->getParameter(ftr::DatumAxis::Tags::Linked), cue);
    }
    {
      //points
      tbl::SelectionCue cue;
      cue.singleSelection = false;
      cue.mask = slc::PointsBoth | slc::AllPointsEnabled | slc::EndPointsSelectable;
      cue.statusPrompt = tr("Select 2 Points For Axis");
      cue.accrueEnabled = false;
      prmModel->setCue(ft->getParameter(ftr::DatumAxis::Tags::Points), cue);
    }
    {
      //intersection
      tbl::SelectionCue cue;
      cue.singleSelection = false;
      cue.mask = slc::ObjectsBoth | slc::FacesBoth;
      cue.statusPrompt = tr("Select 2 Planes For Intersection Axis");
      cue.accrueEnabled = false;
      prmModel->setCue(ft->getParameter(ftr::DatumAxis::Tags::Intersection), cue);
    }
    {
      //geometry
      tbl::SelectionCue cue;
      cue.singleSelection = false;
      cue.mask = slc::FacesBoth | slc::EdgesBoth;
      cue.statusPrompt = tr("Select 2 Planes For Intersection Axis");
      cue.accrueEnabled = false;
      prmModel->setCue(ft->getParameter(ftr::DatumAxis::Tags::Geometry), cue);
    }
  }
};

DatumAxis::DatumAxis(cmd::DatumAxis *cIn)
: Base("cmv::DatumAxis")
, stow(new Stow(cIn, this))
{}

DatumAxis::~DatumAxis() = default;

void DatumAxis::modelChanged(const QModelIndex &index, const QModelIndex&)
{
  if (!index.isValid())
    return;
  
  prm::Parameter *par = stow->parameters.at(index.row());
  if (par->getTag() == ftr::DatumAxis::Tags::AxisType)
  {
    namespace FDA = ftr::DatumAxis;
    using FDAT = FDA::AxisType;
    const auto *ft = stow->command->feature;
    auto axisType = static_cast<ftr::DatumAxis::AxisType>(par->getInt());
    switch(axisType)
    {
      case FDAT::Constant:{
        stow->command->setToConstant();
        break;}
      case FDAT::Parameters:{
        stow->command->setToParameters();
        break;}
      case FDAT::Linked:{
        const auto *p = ft->getParameter(FDA::Tags::Linked);
        stow->command->setToLinked(stow->prmModel->getMessages(p));
        break;}
      case FDAT::Points:{
        const auto *p = ft->getParameter(FDA::Tags::Points);
        stow->command->setToPoints(stow->prmModel->getMessages(p));
        break;}
      case FDAT::Intersection:{
        const auto *p = ft->getParameter(FDA::Tags::Intersection);
        stow->command->setToIntersection(stow->prmModel->getMessages(p));
        break;}
      case FDAT::Geometry:{
        const auto *p = ft->getParameter(FDA::Tags::Geometry);
        stow->command->setToGeometry(stow->prmModel->getMessages(p));
        break;}
    }
    stow->prmView->updateHideInactive();
  }
  
  //for picks we will need to call command function to handle graph connections.
  //everything else will be ok with the simple stow->command->localUpdate.
  if (par->getTag() == ftr::DatumAxis::Tags::Linked)
    stow->command->setToLinked(stow->prmModel->getMessages(par));
  else if (par->getTag() == ftr::DatumAxis::Tags::Points)
    stow->command->setToPoints(stow->prmModel->getMessages(par));
  else if (par->getTag() == ftr::DatumAxis::Tags::Intersection)
    stow->command->setToIntersection(stow->prmModel->getMessages(par));
  else if (par->getTag() == ftr::DatumAxis::Tags::Geometry)
    stow->command->setToGeometry(stow->prmModel->getMessages(par));
  else if (par->getTag() == prm::Tags::AutoSize)
    stow->prmView->updateHideInactive();
  
  stow->command->localUpdate();
  node->sendBlocked(msg::buildStatusMessage(stow->command->getStatusMessage()));
}
