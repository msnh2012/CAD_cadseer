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

#include <boost/filesystem.hpp>

#include <QSettings>
#include <QTabWidget>
#include <QComboBox>
#include <QButtonGroup>
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QFileDialog>

#include "application/appapplication.h"
#include "application/appmainwindow.h"
#include "project/prjproject.h"
#include "viewer/vwrwidget.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "message/msgnode.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "commandview/cmvselectioncue.h"
#include "commandview/cmvtable.h"
#include "tools/idtools.h"
#include "feature/ftrquote.h"
#include "command/cmdquote.h"
#include "commandview/cmvquote.h"

using boost::uuids::uuid;

using namespace cmv;

struct Quote::Stow
{
  cmd::Quote *command;
  cmv::Quote *view;
  prm::Parameters parameters;
  cmv::tbl::Model *prmModel = nullptr;
  cmv::tbl::View *prmView = nullptr;
  
  Stow(cmd::Quote *cIn, cmv::Quote *vIn)
  : command(cIn)
  , view(vIn)
  {
    parameters = command->feature->getParameters();
    buildGui();
    connect(prmModel, &tbl::Model::dataChanged, view, &Quote::modelChanged);
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
    cue.statusPrompt = tr("Select Strip Feature");
    prmModel->setCue(command->feature->getParameter(ftr::Quote::PrmTags::stripPick), cue);
    cue.statusPrompt = tr("Select Die Set Feature");
    prmModel->setCue(command->feature->getParameter(ftr::Quote::PrmTags::diesetPick), cue);
  }
};

Quote::Quote(cmd::Quote *cIn)
: Base("cmv::Quote")
, stow(new Stow(cIn, this))
{
  goSelectionToolbar();
  goMaskDefault();
}

Quote::~Quote() = default;

void Quote::modelChanged(const QModelIndex &index, const QModelIndex&)
{
  if (!index.isValid())
    return;
  auto tag = stow->parameters.at(index.row())->getTag();
  if (tag == ftr::Quote::PrmTags::stripPick || tag == ftr::Quote::PrmTags::diesetPick)
  {
    const auto &sps = stow->prmModel->getMessages(stow->command->feature->getParameter(ftr::Quote::PrmTags::stripPick));
    const auto &dps = stow->prmModel->getMessages(stow->command->feature->getParameter(ftr::Quote::PrmTags::diesetPick));
    stow->command->setSelections(sps, dps);
  }
  stow->command->localUpdate();
  node->sendBlocked(msg::buildStatusMessage(stow->command->getStatusMessage()));
  goMaskDefault();
}
