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
#include <QStackedWidget>
#include <QVBoxLayout>

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "message/msgmessage.h"
#include "message/msgnode.h"
#include "dialogs/dlgselectionbutton.h"
#include "dialogs/dlgselectionwidget.h"
#include "commandview/cmvparameterwidgets.h"
#include "commandview/cmvcsyswidget.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "tools/featuretools.h"
#include "tools/idtools.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrdatumsystem.h"
#include "command/cmddatumsystem.h"
#include "commandview/cmvdatumsystem.h"

using boost::uuids::uuid;

using namespace cmv;

struct DatumSystem::Stow
{
  cmd::DatumSystem *command;
  cmv::DatumSystem *view;
  QComboBox *typeCombo = nullptr;
  QStackedWidget *stackedWidget = nullptr;
  CSysWidget *csysWidget = nullptr;
  dlg::SelectionWidget *selectionWidget3P = nullptr;
  ParameterWidget *parameterWidget = nullptr;
  prm::Parameter *csys = nullptr;
  prm::Parameter *autoSize = nullptr;
  prm::Parameter *size = nullptr;
  
  Stow(cmd::DatumSystem *cIn, cmv::DatumSystem *vIn)
  : command(cIn)
  , view(vIn)
  {
    buildGui();
    
    QSettings &settings = app::instance()->getUserSettings();
    settings.beginGroup("cmv::DatumSystem");
    //load settings
    settings.endGroup();
    
    loadFeatureData();
    glue();
  }
  
  void buildGui()
  {
    QVBoxLayout *mainLayout = new QVBoxLayout();
    view->setLayout(mainLayout);
    
    typeCombo = new QComboBox(view);
    typeCombo->addItem(tr("Parameter"));
    typeCombo->addItem(tr("Through 3 points"));
    mainLayout->addWidget(typeCombo);
    
    parameterWidget = new ParameterWidget(view, {command->feature->getParameters()});
    mainLayout->addWidget(parameterWidget);
    
    for (auto *p : command->feature->getParameters())
    {
      if (p->getName() == prm::Names::CSys)
        csys = p;
      if (p->getName() == prm::Names::Size)
        size = p;
      if (p->getName() == prm::Names::AutoSize)
        autoSize = p;
    }
    assert(csys);
    assert(size);
    assert(autoSize);
    
    csysWidget = dynamic_cast<cmv::CSysWidget*>(parameterWidget->getWidget(csys));
    assert(csysWidget);
    if (csysWidget)
    {
      ftr::UpdatePayload payload = view->project->getPayload(command->feature->getId());
      std::vector<const ftr::Base*> inputs = payload.getFeatures(ftr::InputType::linkCSys);
      if (inputs.empty())
        csysWidget->setCSysLinkId(gu::createNilId());
      else
        csysWidget->setCSysLinkId(inputs.front()->getId());
    }
    
    stackedWidget = new QStackedWidget(view);
    stackedWidget->setSizePolicy(stackedWidget->sizePolicy().horizontalPolicy(), QSizePolicy::Maximum);
    mainLayout->addWidget(stackedWidget);
    
    //add dummy widget for 'by parameter'
    stackedWidget->addWidget(new QWidget(stackedWidget));
    
    //through 3 points
    dlg::SelectionWidgetCue cue;
    cue.name.clear();
    cue.singleSelection = false;
    cue.showAccrueColumn = false;
    cue.mask = slc::PointsEnabled | slc::PointsSelectable | slc::EndPointsEnabled
      | slc::MidPointsEnabled | slc::CenterPointsEnabled | slc::QuadrantPointsEnabled;
    cue.statusPrompt = tr("Select 3 points");
    selectionWidget3P = new dlg::SelectionWidget(stackedWidget, {cue});
    selectionWidget3P->getButton(0)->setChecked(true);
    stackedWidget->addWidget(selectionWidget3P);
    
    mainLayout->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
  }
  
  void loadFeatureData()
  {
    csysWidget->setCSysLinkId(gu::createNilId()); //turn on if needed
    csysWidget->setEnabled(false); //default to off turn on if needed
    
    const auto &cue = command->feature->getCue();
    if (cue.systemType == ftr::DatumSystem::SystemType::Constant)
    {
      typeCombo->setCurrentIndex(0);
      stackedWidget->setCurrentIndex(0);
      csysWidget->setCSysLinkId(gu::createNilId());
      csysWidget->setEnabled(true);
    }
    else if (cue.systemType == ftr::DatumSystem::SystemType::Linked)
    {
      typeCombo->setCurrentIndex(0);
      stackedWidget->setCurrentIndex(0);
      csysWidget->setEnabled(true);
      const auto &pl = view->project->getPayload(command->feature->getId());
      auto features = pl.getFeatures(ftr::InputType::linkCSys);
      if (features.empty())
      {
        csysWidget->setCSysLinkId(gu::createNilId());
        view->node->sendBlocked(msg::buildStatusMessage("Couldn't find linked csys feature"));
      }
      else
        csysWidget->setCSysLinkId(features.front()->getId());
    }
    else if (cue.systemType == ftr::DatumSystem::SystemType::Through3Points)
    {
      typeCombo->setCurrentIndex(1);
      stackedWidget->setCurrentIndex(1);
      //for all types?
      const auto &pl = view->project->getPayload(command->feature->getId());
      tls::Resolver resolver(pl);
      for (const auto &p : cue.picks)
      {
        if (!resolver.resolve(p))
          continue;
        selectionWidget3P->getButton(0)->addMessages(resolver.convertToMessages());
      }
      selectionWidget3P->activate(0);
    }
    
    if (static_cast<bool>(*autoSize))
      parameterWidget->disableWidget(size);
    else
      parameterWidget->enableWidget(size);
  }
  
  void glue()
  {
    connect(typeCombo, SIGNAL(currentIndexChanged(int)), view, SLOT(comboChanged(int)));
    connect(csysWidget, &CSysWidget::dirty, view, &DatumSystem::linkCSysChanged);
    connect(selectionWidget3P->getButton(0), &dlg::SelectionButton::dirty, view, &DatumSystem::p3Changed);
    connect(parameterWidget, &ParameterBase::prmValueChanged, view, &DatumSystem::parameterChanged);
  }
  
  void goUpdate()
  {
    command->localUpdate();
  }
};

DatumSystem::DatumSystem(cmd::DatumSystem *cIn)
: Base("cmv::DatumSystem")
, stow(new Stow(cIn, this))
{}

DatumSystem::~DatumSystem() = default;

void DatumSystem::comboChanged(int index)
{
  stow->stackedWidget->setCurrentIndex(index);
  if (index == 0)
  {
    stow->csysWidget->setEnabled(true);
    linkCSysChanged();
  }
  else if (index == 1)
  {
    stow->csysWidget->setEnabled(false);
    p3Changed();
  }
}

void DatumSystem::linkCSysChanged()
{
  stow->command->setLinked(stow->csysWidget->getCSysLinkId());
  stow->goUpdate();
}

void DatumSystem::p3Changed()
{
  stow->command->set3Points(stow->selectionWidget3P->getButton(0)->getMessages());
  stow->goUpdate();
}

void DatumSystem::parameterChanged()
{
  stow->goUpdate();
}
