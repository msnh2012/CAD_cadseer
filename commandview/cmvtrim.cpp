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
// #include "message/msgmessage.h"
// #include "message/msgnode.h"
#include "dialogs/dlgselectionbutton.h"
// #include "dialogs/dlgselectionlist.h"
#include "dialogs/dlgselectionwidget.h"
#include "commandview/cmvparameterwidgets.h"
#include "parameter/prmparameter.h"
// #include "expressions/exprmanager.h"
// #include "expressions/exprstringtranslator.h"
// #include "expressions/exprvalue.h"
// #include "library/lbrplabel.h"
// #include "tools/featuretools.h"
// #include "tools/idtools.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrtrim.h"
#include "command/cmdtrim.h"
#include "commandview/cmvtrim.h"

using boost::uuids::uuid;

using namespace cmv;

struct Trim::Stow
{
  cmd::Trim *command;
  cmv::Trim *view;
  dlg::SelectionWidget *selectionWidget = nullptr;
  cmv::ParameterWidget *parameterWidget = nullptr;
  std::vector<prm::Observer> observers;
  
  Stow(cmd::Trim *cIn, cmv::Trim *vIn)
  : command(cIn)
  , view(vIn)
  {
    buildGui();
    
    QSettings &settings = app::instance()->getUserSettings();
    settings.beginGroup("cmv::Trim");
    //load settings
    settings.endGroup();
    
    loadFeatureData();
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
    cue.singleSelection = true;
    cue.mask = slc::ObjectsEnabled | slc::ObjectsSelectable; //limited to current feature
    cue.statusPrompt = tr("Select Object To Trim ");
    cue.showAccrueColumn = false;
    cues.push_back(cue);
    cue.name = tr("Tool");
    cue.statusPrompt = tr("Select Trimming Object");
    cues.push_back(cue);
    selectionWidget = new dlg::SelectionWidget(view, cues);
    mainLayout->addWidget(selectionWidget);
    
    parameterWidget = new cmv::ParameterWidget(view, command->feature->getParameters());
    mainLayout->addWidget(parameterWidget);
    for (auto *p : command->feature->getParameters())
    {
      observers.emplace_back(std::bind(&Stow::parameterChanged, this));
      p->connect(observers.back());
    }
    
    mainLayout->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    
    QObject::connect(selectionWidget->getButton(0), &dlg::SelectionButton::dirty, view, &Trim::selectionChanged);
    QObject::connect(selectionWidget->getButton(1), &dlg::SelectionButton::dirty, view, &Trim::selectionChanged);
  }
  
  void loadFeatureData()
  {
    auto pl = view->project->getPayload(command->feature->getId());
    
    auto buildMessage = [](const ftr::Base *fIn) -> slc::Message
    {
      slc::Message m;
      m.type = slc::Type::Object;
      m.featureType = fIn->getType();
      m.featureId = fIn->getId();
      
      return m;
    };
    
    auto tfs = pl.getFeatures(ftr::InputType::target);
    if (!tfs.empty())
    {
      slc::Message tm = buildMessage(tfs.front());
      selectionWidget->initializeButton(0, tm);
    }
    
    auto trfs = pl.getFeatures(ftr::InputType::tool);
    if (!trfs.empty())
    {
      slc::Message tm = buildMessage(trfs.front());
      selectionWidget->initializeButton(1, tm);
    }
  }
  
  void parameterChanged()
  {
    if (!parameterWidget->isVisible())
      return;
    goUpdate();
  }
  
  void goSelections()
  {
    command->setSelections(selectionWidget->getMessages(0), selectionWidget->getMessages(1));
    goUpdate();
  }
  
  void goUpdate()
  {
    //Break cycle.
    std::vector<std::unique_ptr<prm::ObserverBlocker>> blockers;
    for (auto &o : observers)
      blockers.push_back(std::make_unique<prm::ObserverBlocker>(o));
    command->localUpdate();
  }
};

Trim::Trim(cmd::Trim *cIn)
: Base("cmv::Trim")
, stow(new Stow(cIn, this))
{}

Trim::~Trim() = default;

void Trim::selectionChanged()
{
  stow->goSelections();
}
