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
#include "feature/ftroffset.h"
#include "command/cmdoffset.h"
#include "commandview/cmvoffset.h"

using boost::uuids::uuid;

using namespace cmv;

struct Offset::Stow
{
  cmd::Offset *command;
  cmv::Offset *view;
  dlg::SelectionWidget *selectionWidget = nullptr;
  cmv::ParameterWidget *parameterWidget = nullptr;
  
  Stow(cmd::Offset *cIn, cmv::Offset *vIn)
  : command(cIn)
  , view(vIn)
  {
    buildGui();
    
    QSettings &settings = app::instance()->getUserSettings();
    settings.beginGroup("cmv::Offset");
    //load settings
    settings.endGroup();
    
    loadFeatureData();
    glue();
    selectionWidget->activate(0);
  }
  
  void buildGui()
  {
    QSizePolicy adjust = view->sizePolicy();
    adjust.setVerticalPolicy(QSizePolicy::Maximum);
    view->setSizePolicy(adjust);
    
    QVBoxLayout *mainLayout = new QVBoxLayout();
    view->setLayout(mainLayout);
    
    std::vector<dlg::SelectionWidgetCue> cues;
    dlg::SelectionWidgetCue cue;
    cue.name = tr("Target");
    cue.singleSelection = false;
    cue.mask = slc::ObjectsEnabled | slc::ObjectsSelectable | slc::FacesEnabled | slc::FacesSelectable;
    cue.statusPrompt = tr("Select Object Or Faces To Offset ");
    cue.showAccrueColumn = false;
    cues.push_back(cue);
    selectionWidget = new dlg::SelectionWidget(view, cues);
    mainLayout->addWidget(selectionWidget);
    
    parameterWidget = new cmv::ParameterWidget(view, command->feature->getParameters());
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
    
    selectionWidget->initializeButton(0, picksToMessages(command->feature->getPicks()));
  }
  
  void glue()
  {
    QObject::connect(selectionWidget->getButton(0), &dlg::SelectionButton::dirty, view, &Offset::selectionChanged);
    QObject::connect(parameterWidget, &ParameterBase::prmValueChanged, view, &Offset::parameterChanged);
  }
  
  void goSelections()
  {
    command->setSelections(selectionWidget->getMessages(0));
    goUpdate();
  }
  
  void goUpdate()
  {
    command->localUpdate();
  }
};

Offset::Offset(cmd::Offset *cIn)
: Base("cmv::Offset")
, stow(new Stow(cIn, this))
{}

Offset::~Offset() = default;

void Offset::selectionChanged()
{
  stow->goSelections();
}

void Offset::parameterChanged()
{
  stow->goUpdate();
}
