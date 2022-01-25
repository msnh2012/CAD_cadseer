/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2022 Thomas S. Anderson blobfish.at.gmx.com
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
// #include <QComboBox>
// #include <QPushButton>
// #include <QLabel>
// #include <QLineEdit>
// #include <QStackedWidget>
// #include <QGridLayout>
#include <QVBoxLayout>
// #include <QHBoxLayout>

#include "application/appapplication.h"
#include "project/prjproject.h"
// #include "annex/annseershape.h"
// #include "preferences/preferencesXML.h"
// #include "preferences/prfmanager.h"
#include "message/msgmessage.h"
#include "message/msgnode.h"
#include "commandview/cmvselectioncue.h"
#include "commandview/cmvtable.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
// #include "expressions/exprmanager.h"
// #include "expressions/exprvalue.h"
// #include "library/lbrplabel.h"
// #include "tools/featuretools.h"
// #include "tools/idtools.h"
// #include "feature/ftrinputtype.h"
#include "feature/ftrprimitive.h"
#include "feature/ftrmutate.h"
#include "command/cmdmutate.h"
#include "commandview/cmvmutate.h"

using boost::uuids::uuid;

using namespace cmv;

struct Mutate::Stow
{
  cmd::Mutate *command;
  cmv::Mutate *view;
  prm::Parameters parameters;
  cmv::tbl::Model *prmModel = nullptr;
  cmv::tbl::View *prmView = nullptr;
  
  Stow(cmd::Mutate *cIn, cmv::Mutate *vIn)
  : command(cIn)
  , view(vIn)
  {
    parameters = command->feature->getParameters();
    buildGui();
    connect(prmModel, &tbl::Model::dataChanged, view, &Mutate::modelChanged);
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
    cue.statusPrompt = tr("Select Shape Feature");
    cue.accrueEnabled = false;
    prmModel->setCue(command->feature->getParameter(ftr::Mutate::PrmTags::shape), cue);
    
    cue.statusPrompt = tr("Select Feature To Link CSys");
    prmModel->setCue(command->feature->getParameter(prm::Tags::CSysLinked), cue);
  }
  
  void goSelection()
  {
    slc::Messages msgs = prmModel->getMessages(command->feature->getParameter(ftr::Mutate::PrmTags::shape));
    if (msgs.empty())
    {
      command->setSelections(msgs);
      return;
    }
    auto csysType = static_cast<ftr::Primitive::CSysType>(command->feature->getParameter(prm::Tags::CSysType)->getInt());
    if (csysType == ftr::Primitive::Linked)
    {
      auto linkMsgs = prmModel->getMessages(command->feature->getParameter(prm::Tags::CSysLinked));
      msgs.insert(msgs.end(), linkMsgs.begin(), linkMsgs.end());
    }
    command->setSelections(msgs);
  }
};

Mutate::Mutate(cmd::Mutate *cIn)
: Base("cmv::Mutate")
, stow(std::make_unique<Stow>(cIn, this))
{
  goSelectionToolbar();
  goMaskDefault();
}

Mutate::~Mutate() = default;

void Mutate::modelChanged(const QModelIndex &index, const QModelIndex&)
{
  if (!index.isValid())
    return;
  
  auto changedTag = stow->parameters.at(index.row())->getTag();
  if
  (
    changedTag == prm::Tags::CSysLinked
    || changedTag == ftr::Mutate::PrmTags::shape
    || changedTag == prm::Tags::CSysType
  )
  {
    stow->goSelection();
    stow->prmView->updateHideInactive();
  }
  
  stow->command->localUpdate();
  node->sendBlocked(msg::buildStatusMessage(stow->command->getStatusMessage()));
  goMaskDefault();
}
