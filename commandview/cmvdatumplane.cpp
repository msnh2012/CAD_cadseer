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

#include "message/msgmessage.h"
#include "message/msgnode.h"
#include "commandview/cmvselectioncue.h"
#include "commandview/cmvtable.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "feature/ftrdatumplane.h"
#include "command/cmddatumplane.h"
#include "commandview/cmvdatumplane.h"

using boost::uuids::uuid;

using namespace cmv;

struct DatumPlane::Stow
{
  cmd::DatumPlane *command;
  cmv::DatumPlane *view;
  prm::Parameters parameters;
  cmv::tbl::Model *prmModel = nullptr;
  cmv::tbl::View *prmView = nullptr;
  
  Stow(cmd::DatumPlane *cIn, cmv::DatumPlane *vIn)
  : command(cIn)
  , view(vIn)
  {
    buildGui();
    connect(prmModel, &tbl::Model::dataChanged, view, &DatumPlane::modelChanged);
  }
  
  prm::Parameter* getTaggedParameter(std::string_view tag)
  {
    auto prms = command->feature->getParameters(tag); assert(prms.size() == 1);
    return prms.front();
  }
  
  const slc::Messages& getTaggedMessages(std::string_view tag)
  {
    return prmModel->getMessages(getTaggedParameter(tag));
  }
  
  void buildGui()
  {
    QVBoxLayout *mainLayout = new QVBoxLayout();
    view->setLayout(mainLayout);
    Base::clearContentMargins(view);
    view->setSizePolicy(view->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);
    
    parameters = command->feature->getParameters();
    prmModel = new tbl::Model(view, command->feature);
    prmView = new tbl::View(view, prmModel, true);
    mainLayout->addWidget(prmView);
    
    //Reusing cue and only changing what is different from previous, so don't reorder this.
    //linked
    tbl::SelectionCue cue;
    cue.singleSelection = true;
    cue.mask = slc::ObjectsEnabled | slc::ObjectsSelectable;
    cue.statusPrompt = tr("Select Feature To Link CSys");
    cue.accrueEnabled = false;
    prmModel->setCue(getTaggedParameter(prm::Tags::CSysLinked), cue);
    
    //offset
    cue.mask |= slc::FacesEnabled | slc::FacesSelectable;
    cue.statusPrompt = tr("Select Planar Face Or Datum Plane For Offset");
    prmModel->setCue(getTaggedParameter(ftr::DatumPlane::PrmTags::offsetPicks), cue);
    
    //center / bisect
    cue.singleSelection = false;
    cue.statusPrompt = tr("Select Planar Faces Or Datum Planes For Center Or Bisect");
    prmModel->setCue(getTaggedParameter(ftr::DatumPlane::PrmTags::centerPicks), cue);
    
    //axis angle
    cue.mask |= slc::EdgesEnabled | slc::EdgesSelectable;
    cue.statusPrompt = tr("Select Axis First Then Select Plane For Axis Angle");
    prmModel->setCue(getTaggedParameter(ftr::DatumPlane::PrmTags::axisAnglePicks), cue);
    
    //average
    cue.mask &= ~slc::EdgesEnabled & ~slc::EdgesSelectable;
    cue.statusPrompt = tr("Select 3 Planes For Average");
    prmModel->setCue(getTaggedParameter(ftr::DatumPlane::PrmTags::averagePicks), cue);
    
    //points
    cue.mask = slc::PointsEnabled | slc::AllPointsEnabled | slc::PointsSelectable | slc::EndPointsSelectable;
    cue.statusPrompt = tr("Select Origin Point, Then X Vector Point, Then Z Vector Point");
    prmModel->setCue(getTaggedParameter(ftr::DatumPlane::PrmTags::pointsPicks), cue);
  }
};

DatumPlane::DatumPlane(cmd::DatumPlane *cIn)
: Base("cmv::DatumPlane")
, stow(new Stow(cIn, this))
{
  node->sendBlocked(msg::buildStatusMessage("Double Click Parameter To Edit"));
}

DatumPlane::~DatumPlane() = default;

void DatumPlane::modelChanged(const QModelIndex &index, const QModelIndex&)
{
  if (!index.isValid())
    return;
  
  prm::Parameter *par = stow->parameters.at(index.row());
  if (par->getTag() == ftr::DatumPlane::PrmTags::datumPlaneType)
  {
    //change the parameter to 'trick' lower into updating new type
    switch(par->getInt())
    {
      case 0: par = stow->getTaggedParameter(prm::Tags::CSys); break;
      case 1: par = stow->getTaggedParameter(prm::Tags::CSysLinked); break;
      case 2: par = stow->getTaggedParameter(ftr::DatumPlane::PrmTags::offsetPicks); break;
      case 3: par = stow->getTaggedParameter(ftr::DatumPlane::PrmTags::centerPicks); break;
      case 4: par = stow->getTaggedParameter(ftr::DatumPlane::PrmTags::axisAnglePicks); break;
      case 5: par = stow->getTaggedParameter(ftr::DatumPlane::PrmTags::averagePicks); break;
      case 6: par = stow->getTaggedParameter(ftr::DatumPlane::PrmTags::pointsPicks); break;
      default: assert(0); break; //unrecognized datum plane type.
    }
    stow->prmView->updateHideInactive();
  }
  
  if (par->getTag() == prm::Tags::CSys)
  {
    stow->command->setToConstant();
  }
  else if (par->getTag() == prm::Tags::CSysLinked)
  {
    stow->command->setLinked(stow->prmModel->getMessages(par));
  }
  else if (par->getTag() == ftr::DatumPlane::PrmTags::offsetPicks)
  {
    stow->command->setToPlanarOffset(stow->prmModel->getMessages(par));
  }
  else if (par->getTag() == ftr::DatumPlane::PrmTags::centerPicks)
  {
    stow->command->setToPlanarCenter(stow->prmModel->getMessages(par));
  }
  else if (par->getTag() == ftr::DatumPlane::PrmTags::axisAnglePicks)
  {
    stow->command->setToAxisAngle(stow->prmModel->getMessages(par));
  }
  else if (par->getTag() == ftr::DatumPlane::PrmTags::averagePicks)
  {
    stow->command->setToAverage3Plane(stow->prmModel->getMessages(par));
  }
  else if (par->getTag() == ftr::DatumPlane::PrmTags::pointsPicks)
  {
    stow->command->setToThrough3Points(stow->prmModel->getMessages(par));
  }
  
  stow->command->localUpdate();
  node->sendBlocked(msg::buildStatusMessage(stow->command->getStatusMessage()));
}
