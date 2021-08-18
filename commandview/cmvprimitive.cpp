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
#include "commandview/cmvselectioncue.h"
#include "commandview/cmvtable.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "tools/idtools.h"
#include "selection/slcmessage.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrbase.h"
#include "feature/ftrprimitive.h"
#include "command/cmdprimitive.h"
#include "commandview/cmvprimitive.h"

using boost::uuids::uuid;

using namespace cmv;

struct Primitive::Stow
{
  cmd::Primitive *command;
  cmv::Primitive *view;
  ftr::Base *feature;
  prm::Parameters parameters;
  cmv::tbl::Model *prmModel = nullptr;
  cmv::tbl::View *prmView = nullptr;
  
  Stow(cmd::Primitive *cIn, cmv::Primitive *vIn)
  : command(cIn)
  , view(vIn)
  , feature(command->feature)
  {
    parameters = feature->getParameters();
    buildGui();
    connect(prmModel, &tbl::Model::dataChanged, view, &Primitive::modelChanged);
  }
  
  void buildGui()
  {
    QVBoxLayout *mainLayout = new QVBoxLayout();
    view->setLayout(mainLayout);
    Base::clearContentMargins(view);
    view->setSizePolicy(view->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);
    
    prmModel = new tbl::Model(view, feature);
    prmView = new tbl::View(view, prmModel, true);
    mainLayout->addWidget(prmView);
    
    tbl::SelectionCue cue;
    cue.singleSelection = true;
    cue.mask = slc::ObjectsBoth;
    cue.statusPrompt = tr("Select Feature To Link CSys");
    cue.accrueEnabled = false;
    prmModel->setCue(feature->getParameter(prm::Tags::CSysLinked), cue);
  }
};

Primitive::Primitive(cmd::Primitive *cmdIn)
: Base("cmv::Primitive")
, stow(new Stow(cmdIn, this))
{
  goSelectionToolbar();
  goMaskDefault();
}

Primitive::~Primitive() = default;

void Primitive::modelChanged(const QModelIndex &index, const QModelIndex&)
{
  if (!index.isValid())
    return;
  
  prm::Parameter *par = stow->parameters.at(index.row());
  if (par->getTag() == prm::Tags::CSysType)
  {
    if (static_cast<ftr::Primitive::CSysType>(par->getInt()) == ftr::Primitive::Linked)
      par = stow->feature->getParameter(prm::Tags::CSysLinked); //trick next if.
    else
      stow->command->setToConstant();
    stow->prmView->updateHideInactive();
  }
  if (par->getTag() == prm::Tags::CSysLinked)
  {
    stow->command->setToLinked(stow->prmModel->getMessages(par));
  }
  stow->feature->updateModel(project->getPayload(stow->feature->getId()));
  stow->feature->updateVisual();
  stow->feature->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  goMaskDefault();
}
