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
#include <QComboBox>
// #include <QStackedWidget>
#include <QVBoxLayout>
// #include <QHBoxLayout>
// #include <QLineEdit>

#include "application/appapplication.h"
#include "project/prjproject.h"
// #include "annex/annseershape.h"
// #include "preferences/preferencesXML.h"
// #include "preferences/prfmanager.h"
// #include "message/msgmessage.h"
// #include "message/msgnode.h"
// #include "dialogs/dlgselectionbutton.h"
// #include "dialogs/dlgselectionlist.h"
#include "dialogs/dlgselectionwidget.h"
#include "dialogs/dlgselectionbutton.h"
#include "commandview/cmvparameterwidgets.h"
#include "parameter/prmparameter.h"
// #include "expressions/exprmanager.h"
// #include "expressions/exprstringtranslator.h"
// #include "expressions/exprvalue.h"
// #include "library/lbrplabel.h"
#include "tools/featuretools.h"
// #include "tools/idtools.h"
// #include "feature/ftrinputtype.h"
#include "feature/ftrrevolve.h"
#include "command/cmdrevolve.h"
#include "commandview/cmvrevolve.h"

using boost::uuids::uuid;

using namespace cmv;

struct Revolve::Stow
{
  cmd::Revolve *command;
  cmv::Revolve *view;
  QComboBox *combo = nullptr;
  dlg::SelectionWidget *selection = nullptr;
  cmv::ParameterWidget *parameterWidget = nullptr;
  std::vector<prm::Observer> observers;
  
  Stow(cmd::Revolve *cIn, cmv::Revolve *vIn)
  : command(cIn)
  , view(vIn)
  {
    buildGui();
    
    QSettings &settings = app::instance()->getUserSettings();
    settings.beginGroup("cmv::Revolve");
    //load settings
    settings.endGroup();
    
    loadFeatureData();
  }
  
  void buildGui()
  {
    QSizePolicy adjust = view->sizePolicy();
    adjust.setVerticalPolicy(QSizePolicy::Maximum);
    view->setSizePolicy(adjust);
    
    QVBoxLayout *mainLayout = new QVBoxLayout();
    view->setLayout(mainLayout);
    
    combo = new QComboBox(view);
    combo->addItem(QObject::tr("Pick Axis"));
    combo->addItem(QObject::tr("Parameter Axis"));
    mainLayout->addWidget(combo);
    
    std::vector<dlg::SelectionWidgetCue> cues;
    dlg::SelectionWidgetCue cue;
    cue.name = tr("Profile");
    cue.singleSelection = true; //eventually false.
    cue.mask = slc::ObjectsEnabled | slc::ObjectsSelectable | slc::FacesEnabled  | slc::FacesSelectable
      | slc::WiresEnabled | slc::EdgesEnabled | slc::PointsEnabled | slc::PointsSelectable | slc::AllPointsEnabled;
    cue.statusPrompt = tr("Select Profile Geometry");
    cue.showAccrueColumn = false;
    cues.push_back(cue);
    
    cue.name = tr("Axis");
    cue.singleSelection = false;
    cue.mask = slc::ObjectsEnabled | slc::ObjectsSelectable | slc::FacesEnabled | slc::EdgesEnabled | slc::EdgesSelectable
      | slc::PointsEnabled | slc::PointsSelectable | slc::AllPointsEnabled;
    cue.statusPrompt = tr("Select Points, Edge, Face Or Datum For Axis");
    cues.push_back(cue);
    selection = new dlg::SelectionWidget(view, cues);
    mainLayout->addWidget(selection);
    
    parameterWidget = new cmv::ParameterWidget(view, command->feature->getParameters());
    mainLayout->addWidget(parameterWidget);
    for (auto *p : command->feature->getParameters())
    {
      observers.emplace_back(std::bind(&Stow::parameterChanged, this));
      p->connect(observers.back());
    }
    
    mainLayout->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    
    QObject::connect(combo, SIGNAL(currentIndexChanged(int)), view, SLOT(comboChanged(int)));
    QObject::connect(selection->getButton(0), &dlg::SelectionButton::dirty, view, &Revolve::profileSelectionChanged);
    QObject::connect(selection->getButton(1), &dlg::SelectionButton::dirty, view, &Revolve::axisSelectionChanged);
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
    
    selection->initializeButton(0, picksToMessages(command->feature->getPicks()));
    
    ftr::Revolve::AxisType at = command->feature->getAxisType();
    if (at == ftr::Revolve::AxisType::Parameter)
      selection->hideEntry(1);
    else
      selection->initializeButton(1, picksToMessages(command->feature->getAxisPicks()));
    
    QSignalBlocker stackBlocker(combo);
    combo->setCurrentIndex(static_cast<int>(at));
    selection->activate(0);
  }
  
  void parameterChanged()
  {
    if (!parameterWidget->isVisible())
      return;
    goUpdate();
  }
  
  void goAxisPicks()
  {
    command->setToAxisPicks(selection->getMessages(0), selection->getMessages(1));
    goUpdate();
  }
  
  void goAxisParameter()
  {
    command->setToAxisParameter(selection->getMessages(0));
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

Revolve::Revolve(cmd::Revolve *cIn)
: Base("cmv::Revolve")
, stow(new Stow(cIn, this))
{}

Revolve::~Revolve() = default;

void Revolve::comboChanged(int cIndex)
{
  if (cIndex == 0)
  {
    stow->selection->showEntry(1);
    stow->goAxisPicks();
  }
  else if (cIndex == 1)
  {
    stow->selection->hideEntry(1);
    stow->goAxisParameter();
  }
  
  //we will want to enable/disable the direction parameter.
  //we can't parse vectors yet, so will leave it disabled.
}

void Revolve::profileSelectionChanged()
{
  int current = stow->combo->currentIndex();
  if (current == 0)
    stow->goAxisPicks();
  else if (current == 1)
    stow->goAxisParameter();
}

void Revolve::axisSelectionChanged()
{
  stow->goAxisPicks();
}
