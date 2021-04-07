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
#include "commandview/cmvparameterwidgets.h"
#include "parameter/prmparameter.h"
#include "tools/featuretools.h"
#include "feature/ftrdraft.h"
#include "command/cmddraft.h"
#include "commandview/cmvdraft.h"

using boost::uuids::uuid;

using namespace cmv;

struct Draft::Stow
{
  cmd::Draft *command;
  cmv::Draft *view;
  dlg::SelectionWidget *selectionWidget = nullptr;
  cmv::ParameterWidget *parameterWidget = nullptr;
  
  Stow(cmd::Draft *cIn, cmv::Draft *vIn)
  : command(cIn)
  , view(vIn)
  {
    buildGui();
    
    QSettings &settings = app::instance()->getUserSettings();
    settings.beginGroup("cmv::Draft");
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
    
    cue.name = tr("Neutral");
    cue.singleSelection = true;
    cue.mask = slc::FacesEnabled | slc::FacesSelectable;
    cue.statusPrompt = tr("Select Neutral Face");
    cue.showAccrueColumn = false;
    cues.push_back(cue);
    
    cue.name = tr("Draft");
    cue.singleSelection = false;
    cue.statusPrompt = tr("Select Faces To Draft");
    cues.push_back(cue);
    
    selectionWidget = new dlg::SelectionWidget(view, cues);
    mainLayout->addWidget(selectionWidget);
    
    parameterWidget = new ParameterWidget(view, command->feature->getParameters());
    mainLayout->addWidget(parameterWidget);
    mainLayout->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
  }
  
  void loadFeatureData()
  {
    ftr::UpdatePayload up = view->project->getPayload(command->feature->getId());
    tls::Resolver resolver(up);
    
    auto picksToMessages = [&](const ftr::Picks &pIn) -> slc::Messages
    {
      slc::Messages out;
      for (const auto &p : pIn)
      {
        if (resolver.resolve(p))
        {
          auto msgs = resolver.convertToMessages();
          out.insert(out.end(), msgs.begin(), msgs.end());
        }
      }
      return out;
    };
    selectionWidget->initializeButton(0, picksToMessages(ftr::Picks(1, command->feature->getNeutralPick())));
    selectionWidget->initializeButton(1, picksToMessages(command->feature->getTargetPicks()));
  }
  
  void glue()
  {
    connect(selectionWidget->getButton(0), &dlg::SelectionButton::dirty, view, &Draft::selectionChanged);
    connect(selectionWidget->getButton(1), &dlg::SelectionButton::dirty, view, &Draft::selectionChanged);
    connect(parameterWidget, &ParameterBase::prmValueChanged, view, &Draft::parameterChanged);
  }
};

Draft::Draft(cmd::Draft *cIn)
: Base("cmv::Draft")
, stow(new Stow(cIn, this))
{}

Draft::~Draft() = default;

void Draft::selectionChanged()
{
  stow->command->setSelections(stow->selectionWidget->getButton(0)->getMessages(), stow->selectionWidget->getButton(1)->getMessages());
  stow->command->localUpdate();
}

void Draft::parameterChanged()
{
  stow->command->localUpdate();
}
