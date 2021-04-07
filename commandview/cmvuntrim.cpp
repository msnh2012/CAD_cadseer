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
#include "feature/ftruntrim.h"
#include "command/cmduntrim.h"
#include "commandview/cmvuntrim.h"

using boost::uuids::uuid;

using namespace cmv;

struct Untrim::Stow
{
  cmd::Untrim *command;
  cmv::Untrim *view;
  dlg::SelectionWidget *selectionWidget = nullptr;
  cmv::ParameterWidget *parameterWidget = nullptr;
  
  Stow(cmd::Untrim *cIn, cmv::Untrim *vIn)
  : command(cIn)
  , view(vIn)
  {
    buildGui();
    
    QSettings &settings = app::instance()->getUserSettings();
    settings.beginGroup("cmv::Untrim");
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
    cue.name = tr("Face");
    cue.singleSelection = true;
    cue.mask = slc::FacesEnabled | slc::FacesSelectable;
    cue.statusPrompt = tr("Select Face To Untrim");
    cue.showAccrueColumn = false;
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
    
    if (resolver.resolve(command->feature->getPick()))
      selectionWidget->initializeButton(0, resolver.convertToMessages());
  }
  
  void glue()
  {
    connect(selectionWidget->getButton(0), &dlg::SelectionButton::dirty, view, &Untrim::selectionChanged);
    connect(parameterWidget, &ParameterBase::prmValueChanged, view, &Untrim::parameterChanged);
  }
};

Untrim::Untrim(cmd::Untrim *cIn)
: Base("cmv::Untrim")
, stow(new Stow(cIn, this))
{}

Untrim::~Untrim() = default;

void Untrim::selectionChanged()
{
  const auto &msgs = stow->selectionWidget->getButton(0)->getMessages();
  if (msgs.empty())
    return;
  stow->command->setSelection(stow->selectionWidget->getButton(0)->getMessages().front());
  stow->command->localUpdate();
}

void Untrim::parameterChanged()
{
  stow->command->localUpdate();
}
