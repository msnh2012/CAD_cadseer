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
#include "feature/ftrextract.h"
#include "command/cmdextract.h"
#include "commandview/cmvextract.h"

using boost::uuids::uuid;

using namespace cmv;

struct Extract::Stow
{
  cmd::Extract *command;
  cmv::Extract *view;
  dlg::SelectionWidget *selectionWidget = nullptr;
  cmv::ParameterWidget *parameterWidget = nullptr;
  
  Stow(cmd::Extract *cIn, cmv::Extract *vIn)
  : command(cIn)
  , view(vIn)
  {
    buildGui();
    
    QSettings &settings = app::instance()->getUserSettings();
    settings.beginGroup("cmv::Extract");
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
    
    cue.name = tr("Entities");
    cue.singleSelection = false;
    cue.mask = slc::AllEnabled & ~slc::PointsEnabled;
    cue.statusPrompt = tr("Select Entities");
    cues.push_back(cue);
    
    selectionWidget = new dlg::SelectionWidget(view, cues);
    mainLayout->addWidget(selectionWidget);
    
    //angle parameter
    QHBoxLayout *eLayout = new QHBoxLayout();
    eLayout->addStretch();
    parameterWidget = new ParameterWidget(view, command->feature->getParameters());
    eLayout->addWidget(parameterWidget);
    mainLayout->addLayout(eLayout);
    parameterWidget->setDisabled(true);
    
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
    
    selectionWidget->setAngle(command->feature->getAngleParameter()->getDouble());
  }
  
  void glue()
  {
    connect(selectionWidget, &dlg::SelectionWidget::accrueChanged, view, &Extract::accrueChanged);
    connect(selectionWidget->getButton(0), &dlg::SelectionButton::dirty, view, &Extract::selectionChanged);
    connect(parameterWidget, &ParameterBase::prmValueChanged, view, &Extract::parameterChanged);
  }
};

Extract::Extract(cmd::Extract *cIn)
: Base("cmv::Extract")
, stow(new Stow(cIn, this))
{
  enableParameterWidget();
}

Extract::~Extract() = default;

void Extract::accrueChanged()
{
  enableParameterWidget();
  stow->command->setSelections(stow->selectionWidget->getButton(0)->getMessages());
  stow->command->localUpdate();
}

void Extract::selectionChanged()
{
  stow->command->setSelections(stow->selectionWidget->getButton(0)->getMessages());
  stow->command->localUpdate();
}

void Extract::enableParameterWidget()
{
  bool pwe = false; //parameter widget disabled by default
  
  auto hasTangent = [&](const dlg::SelectionButton *b) -> bool
  {
    for (const auto &m : b->getMessages())
    {
      if (m.accrue == slc::Accrue::Tangent)
        return true;
    }
    return false;
  };
  
  int bc = stow->selectionWidget->getButtonCount();
  for (int i = 0; i < bc; ++i)
  {
    auto *b = stow->selectionWidget->getButton(i);
    if (hasTangent(b))
    {
      pwe = true;
      break;
    }
  }
  stow->parameterWidget->setEnabled(pwe);
}

void Extract::parameterChanged()
{
  stow->selectionWidget->setAngle(stow->command->feature->getAngleParameter()->getDouble());
  stow->selectionWidget->getButton(0)->syncToSelection();
  stow->command->localUpdate();
}
