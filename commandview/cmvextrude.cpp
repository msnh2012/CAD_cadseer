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
#include <QVBoxLayout>
#include <QStackedWidget>

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "dialogs/dlgselectionbutton.h"
#include "dialogs/dlgselectionwidget.h"
#include "commandview/cmvparameterwidgets.h"
#include "parameter/prmparameter.h"
#include "tools/featuretools.h"
#include "feature/ftrextrude.h"
#include "command/cmdextrude.h"
#include "commandview/cmvextrude.h"

using boost::uuids::uuid;

using namespace cmv;

struct Extrude::Stow
{
  cmd::Extrude *command;
  cmv::Extrude *view;
  QComboBox *combo = nullptr;
  QStackedWidget *stackedWidget = nullptr;
  dlg::SelectionWidget *inferSelection = nullptr; //used by parameter also
  dlg::SelectionWidget *picksSelection = nullptr;
  cmv::ParameterWidget *parameterWidget = nullptr;
  std::vector<prm::Observer> observers;
  int lastComboIndex = 0; // used to sync profile selections between selection widgets.
  
  Stow(cmd::Extrude *cIn, cmv::Extrude *vIn)
  : command(cIn)
  , view(vIn)
  {
    buildGui();
    
    QSettings &settings = app::instance()->getUserSettings();
    settings.beginGroup("cmv::Extrude");
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
    combo->addItem(QObject::tr("Infer Direction"));
    combo->addItem(QObject::tr("Pick  Direction"));
    combo->addItem(QObject::tr("Parameter Direction"));
    mainLayout->addWidget(combo);
    
    stackedWidget = new QStackedWidget(view);
    mainLayout->addWidget(stackedWidget);
    
    std::vector<dlg::SelectionWidgetCue> cues;
    dlg::SelectionWidgetCue cue;
    cue.name = tr("Profile");
    cue.singleSelection = true; //eventually false.
    cue.mask = slc::ObjectsEnabled | slc::ObjectsSelectable | slc::FacesEnabled  | slc::FacesSelectable
      | slc::WiresEnabled | slc::EdgesEnabled | slc::PointsEnabled | slc::PointsSelectable | slc::AllPointsEnabled;
    cue.statusPrompt = tr("Select Profile Geometry");
    cue.showAccrueColumn = false;
    cues.push_back(cue);
    inferSelection = new dlg::SelectionWidget(stackedWidget, cues);
    stackedWidget->addWidget(inferSelection);
    
    cue.name = tr("Axis");
    cue.singleSelection = false;
    cue.mask = slc::ObjectsEnabled | slc::ObjectsSelectable | slc::FacesEnabled  | slc::FacesSelectable
      | slc::PointsEnabled | slc::PointsSelectable | slc::AllPointsEnabled;
    cue.statusPrompt = tr("Select Points, Face Or Datum For Axis");
    cues.push_back(cue);
    picksSelection = new dlg::SelectionWidget(stackedWidget, cues);
    stackedWidget->addWidget(picksSelection);
    
    parameterWidget = new cmv::ParameterWidget(view, command->feature->getParameters());
    mainLayout->addWidget(parameterWidget);
    for (auto *p : command->feature->getParameters())
    {
      observers.emplace_back(std::bind(&Stow::parameterChanged, this));
      p->connect(observers.back());
    }
    
    mainLayout->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    
    QObject::connect(combo, SIGNAL(currentIndexChanged(int)), view, SLOT(comboChanged(int)));
    QObject::connect(inferSelection->getButton(0), &dlg::SelectionButton::dirty, view, &Extrude::profileSelectionChanged);
    QObject::connect(picksSelection->getButton(0), &dlg::SelectionButton::dirty, view, &Extrude::profileSelectionChanged);
    QObject::connect(picksSelection->getButton(1), &dlg::SelectionButton::dirty, view, &Extrude::axisSelectionChanged);
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
    
    inferSelection->initializeButton(0, picksToMessages(command->feature->getPicks()));
    picksSelection->initializeButton(0, picksToMessages(command->feature->getPicks()));
    
    ftr::Extrude::DirectionType dt = command->feature->getDirectionType();
    if (dt == ftr::Extrude::DirectionType::Picks)
    {
      picksSelection->initializeButton(1, picksToMessages(command->feature->getAxisPicks()));
      stackedWidget->setCurrentIndex(1);
      picksSelection->activate(0);
    }
    else
    {
      stackedWidget->setCurrentIndex(0);
      inferSelection->activate(0);
    }
    
    QSignalBlocker stackBlocker(combo);
    combo->setCurrentIndex(static_cast<int>(dt));
    lastComboIndex = combo->currentIndex();
    
  }
  
  void parameterChanged()
  {
//     updateWidgetState();
      
    // lets make sure we don't trigger updates 'behind the scenes'
    if (!parameterWidget->isVisible())
      return;
    goUpdate();
  }
  
  void goAxisInfer()
  {
    command->setToAxisInfer(inferSelection->getMessages(0));
    goUpdate();
  }
  
  void goAxisPicks()
  {
    command->setToAxisPicks(picksSelection->getMessages(0), picksSelection->getMessages(1));
    goUpdate();
  }
  
  void goAxisParameter()
  {
    command->setToAxisParameter(inferSelection->getMessages(0));
    goUpdate();
  }
  
  void goUpdate()
  {
    //extrude can and will change the direction parameter. Break cycle.
    std::vector<std::unique_ptr<prm::ObserverBlocker>> blockers;
    for (auto &o : observers)
      blockers.push_back(std::make_unique<prm::ObserverBlocker>(o));
    command->localUpdate();
  }
};

Extrude::Extrude(cmd::Extrude *cIn)
: Base("cmv::Extrude")
, stow(new Stow(cIn, this))
{}

Extrude::~Extrude() = default;

void Extrude::comboChanged(int cIndex)
{
  //we have 2 selection widgets with distinct profile selections.
  //we want to keep these in sync for a better user experience.
  if (cIndex == 0)
  {
    if (stow->lastComboIndex == 1)
    {
      stow->inferSelection->getButton(0)->setMessages(stow->picksSelection->getButton(0)->getMessages());
      stow->inferSelection->getButton(0)->mask = stow->picksSelection->getButton(0)->mask;
      stow->inferSelection->activate(0);
    }
    stow->stackedWidget->setCurrentIndex(0);
    stow->goAxisInfer();
  }
  else if (cIndex == 1)
  {
    if (stow->lastComboIndex != 1)
    {
      stow->picksSelection->getButton(0)->setMessages(stow->inferSelection->getButton(0)->getMessages());
      stow->picksSelection->getButton(0)->mask = stow->inferSelection->getButton(0)->mask;
      stow->picksSelection->activate(0);
    }
    stow->stackedWidget->setCurrentIndex(1);
    stow->goAxisPicks();
  }
  else if (cIndex == 2)
  {
    if (stow->lastComboIndex == 1)
    {
      stow->inferSelection->getButton(0)->setMessages(stow->picksSelection->getButton(0)->getMessages());
      stow->inferSelection->getButton(0)->mask = stow->picksSelection->getButton(0)->mask;
      stow->inferSelection->activate(0);
    }
    stow->stackedWidget->setCurrentIndex(0);
    stow->goAxisParameter();
  }
  
  stow->lastComboIndex = cIndex;
  
  //we will want to enable/disable the direction parameter.
  //we can't parse vectors yet, so will leave it disabled.
}

void Extrude::profileSelectionChanged()
{
  int current = stow->combo->currentIndex();
  if (current == 0)
    stow->goAxisInfer();
  else if (current == 1)
    stow->goAxisPicks();
  else if (current == 2)
    stow->goAxisParameter();
}

void Extrude::axisSelectionChanged()
{
  stow->goAxisPicks();
}
