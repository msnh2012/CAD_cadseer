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
#include "dialogs/dlgselectionbutton.h"
#include "dialogs/dlgselectionwidget.h"
#include "tools/featuretools.h"
#include "feature/ftrface.h"
#include "command/cmdface.h"
#include "commandview/cmvface.h"

using boost::uuids::uuid;

using namespace cmv;

struct Face::Stow
{
  cmd::Face *command;
  cmv::Face *view;
  dlg::SelectionWidget *selectionWidget = nullptr;
  
  Stow(cmd::Face *cIn, cmv::Face *vIn)
  : command(cIn)
  , view(vIn)
  {
    buildGui();
    
    QSettings &settings = app::instance()->getUserSettings();
    settings.beginGroup("cmv::Face");
    //load settings
    settings.endGroup();
    
    loadFeatureData();
    glue();
    selectionWidget->activate(0);
  }
  
  void buildGui()
  {
    QVBoxLayout *mainLayout = new QVBoxLayout();
    view->setLayout(mainLayout);
  
    std::vector<dlg::SelectionWidgetCue> cues;
    dlg::SelectionWidgetCue cue;
    cue.name = tr("Edges");
    cue.singleSelection = false;
    cue.mask = slc::ObjectsEnabled | slc::WiresEnabled | slc::EdgesEnabled | slc::EdgesSelectable;
    cue.statusPrompt = tr("Select Edges To Build Face");
    cue.showAccrueColumn = false;
    cues.push_back(cue);
    
    selectionWidget = new dlg::SelectionWidget(view, cues);
    mainLayout->addWidget(selectionWidget);
    
    mainLayout->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
  }
  
  void loadFeatureData()
  {
    ftr::UpdatePayload up = view->project->getPayload(command->feature->getId());
    tls::Resolver resolver(up);
    slc::Messages msgs;
    for (const auto &p : command->feature->getPicks())
    {
      if (!resolver.resolve(p))
        continue;
      auto temp = resolver.convertToMessages();
      msgs.insert(msgs.end(), temp.begin(), temp.end());
    }
    selectionWidget->initializeButton(0, msgs);
  }
  
  void glue()
  {
    connect(selectionWidget->getButton(0), &dlg::SelectionButton::dirty, view, &Face::selectionChanged);
  }
};

Face::Face(cmd::Face *cIn)
: Base("cmv::Face")
, stow(new Stow(cIn, this))
{}

Face::~Face() = default;

void Face::selectionChanged()
{
  const auto &msgs = stow->selectionWidget->getButton(0)->getMessages();
  if (msgs.empty())
    return;
  stow->command->setSelections(stow->selectionWidget->getButton(0)->getMessages());
  stow->command->localUpdate();
}
