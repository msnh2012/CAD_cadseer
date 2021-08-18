/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) %YEAR% Thomas S. Anderson blobfish.at.gmx.com
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
// #include <QVBoxLayout>
// #include <QHBoxLayout>

#include "application/appapplication.h"
#include "project/prjproject.h"
// #include "annex/annseershape.h"
// #include "preferences/preferencesXML.h"
// #include "preferences/prfmanager.h"
// #include "message/msgmessage.h"
// #include "message/msgnode.h"
// #include "commandview/cmvselectioncue.h"
// #include "commandview/cmvtable.h"
// #include "parameter/prmconstants.h"
// #include "parameter/prmparameter.h"
// #include "expressions/exprmanager.h"
// #include "expressions/exprvalue.h"
// #include "library/lbrplabel.h"
// #include "tools/featuretools.h"
// #include "tools/idtools.h"
// #include "feature/ftrinputtype.h"
#include "feature/ftr%CLASSNAMELOWERCASE%.h"
#include "command/cmd%CLASSNAMELOWERCASE%.h"
#include "commandview/cmv%CLASSNAMELOWERCASE%.h"

using boost::uuids::uuid;

using namespace cmv;

struct %CLASSNAME%::Stow
{
  cmd::%CLASSNAME% *command;
  cmv::%CLASSNAME% *view;
//   prm::Parameters parameters;
//   cmv::tbl::Model *prmModel = nullptr;
//   cmv::tbl::View *prmView = nullptr;
//   std::vector<prm::Observer> observers;
  
  Stow(cmd::%CLASSNAME% *cIn, cmv::%CLASSNAME% *vIn)
  : command(cIn)
  , view(vIn)
  {
//     parameters = command->feature->getParameters();
    buildGui();
//     connect(prmModel, &tbl::Model::dataChanged, view, &%CLASSNAME%::modelChanged);
  }
  
  void buildGui()
  {
    /*
    QVBoxLayout *mainLayout = new QVBoxLayout();
    view->setLayout(mainLayout);
    Base::clearContentMargins(view);
    view->setSizePolicy(view->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);
    
    prmModel = new tbl::Model(view, command->feature);
    prmView = new tbl::View(view, prmModel, true);
    mainLayout->addWidget(prmView);
    
    tbl::SelectionCue cue;
    cue.singleSelection = false;
    cue.mask = slc::AllEnabled & ~slc::PointsEnabled;
    cue.statusPrompt = tr("Select Entities");
    cue.accrueEnabled = true;
    prmModel->setCue(command->feature->getParameter(prm::Tags::Picks), cue);
    */
  }
};

%CLASSNAME%::%CLASSNAME%(cmd::%CLASSNAME% *cIn)
: Base("cmv::%CLASSNAME%")
, stow(std::make_uniue<Stow>(cIn, this))
{
  goSelectionToolbar();
  goMaskDefault();
}

%CLASSNAME%::~%CLASSNAME%() = default;

void %CLASSNAME%::modelChanged(const QModelIndex &index, const QModelIndex&)
{
  if (!index.isValid())
    return;
  /*
  auto changedTag = stow->parameters.at(index.row())->getTag();
  if (changedTag == prm::Tags::Picks)
  {
    const auto &picks = stow->prmModel->getMessages(stow->command->feature->getParameter(prm::Tags::Picks));
    stow->command->setSelections(picks);

    stow->prmView->updateHideInactive();
  }
  */
  stow->command->localUpdate();
  node->sendBlocked(msg::buildStatusMessage(stow->command->getStatusMessage()));
  goMaskDefault();
}
