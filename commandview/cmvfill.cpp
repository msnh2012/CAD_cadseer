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
#include "dialogs/dlgselectionbutton.h"
#include "dialogs/dlgselectionlist.h"
#include "dialogs/dlgselectionwidget.h"
#include "commandview/cmvparameterwidgets.h"
#include "parameter/prmparameter.h"
#include "tools/featuretools.h"
#include "feature/ftrfill.h"
#include "command/cmdfill.h"
#include "commandview/cmvfill.h"

using boost::uuids::uuid;

using namespace cmv;

struct Fill::Stow
{
  cmd::Fill *command;
  cmv::Fill *view;
  dlg::SelectionWidget *selectionWidget = nullptr;
  QStackedWidget *stacked = nullptr;
  std::vector<std::shared_ptr<prm::Parameter>> parameters;
  
  Stow(cmd::Fill *cIn, cmv::Fill *vIn)
  : command(cIn)
  , view(vIn)
  {
    buildGui();
    
    QSettings &settings = app::instance()->getUserSettings();
    settings.beginGroup("cmv::Fill");
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
    cue.name = tr("Boundaries");
    cue.singleSelection = false;
    cue.mask = slc::FacesEnabled | slc::FacesSelectable | slc::EdgesEnabled | slc::EdgesSelectable;
    cue.statusPrompt = tr("Select Boundaries");
    cue.showAccrueColumn = false;
    cues.push_back(cue);
    
    cue.name = tr("Internal");
    cue.mask = slc::EdgesEnabled | slc::EdgesSelectable;
    cue.statusPrompt = tr("Select Internal Edges");
    cues.push_back(cue);
    
    selectionWidget = new dlg::SelectionWidget(view, cues);
    mainLayout->addWidget(selectionWidget);
    
    stacked = new QStackedWidget(view);
    stacked->setDisabled(true); //turn on when we select something from boundaries.
    stacked->setSizePolicy(stacked->sizePolicy().horizontalPolicy(), QSizePolicy::Maximum);
    mainLayout->addWidget(stacked);
    
    mainLayout->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
  }
  
  void loadFeatureData()
  {
    ftr::UpdatePayload up = view->project->getPayload(command->feature->getId());
    tls::Resolver resolver(up);
    
    slc::Messages bMsgs;
    slc::Messages iMsgs;
    for (const auto &e : command->feature->getEntries())
    {
      if (e.edgePick)
      {
        if (resolver.resolve(*e.edgePick))
        {
          auto rm = resolver.convertToMessages();
          for (const auto &m : rm)
          {
            if (e.boundary)
            {
              bMsgs.emplace_back(m);
              //duplicate parameter in case of multiple resolutions to ensure sync between selections and parameters
              parameters.push_back(std::make_shared<prm::Parameter>(*e.continuity));
              stacked->addWidget(new QWidget(view));
            }
            else
              iMsgs.emplace_back(m);
          }
        }
      }
      if (e.facePick)
      {
        if (resolver.resolve(*e.facePick))
        {
          auto rm = resolver.convertToMessages();
          for (const auto &m : rm)
          {
            if (e.boundary)
            {
              bMsgs.emplace_back(m);
              parameters.push_back(std::make_shared<prm::Parameter>(*e.continuity));
              auto *w = new cmv::ParameterWidget(view, {parameters.back().get()});
              stacked->addWidget(w);
              connect(w, &ParameterBase::prmValueChanged, view, &Fill::continuityChanged);
            }
            else
              iMsgs.emplace_back(m);
          }
        }
      }
    }
    selectionWidget->initializeButton(0, bMsgs);
    selectionWidget->initializeButton(1, iMsgs);
  }
  
  void glue()
  {
    connect(selectionWidget->getButton(0), &dlg::SelectionButton::selectionAdded, view, &Fill::boundaryAdded);
    connect(selectionWidget->getButton(0), &dlg::SelectionButton::selectionRemoved, view, &Fill::boundaryRemoved);
    connect(selectionWidget->getView(0), &dlg::SelectionView::dirty, view, &Fill::boundarySelectionChanged);
    connect(selectionWidget->getButton(1), &dlg::SelectionButton::selectionAdded, view, &Fill::internalAdded);
    connect(selectionWidget->getButton(1), &dlg::SelectionButton::selectionRemoved, view, &Fill::internalRemoved);
  }
  
  void goUpdate()
  {
    cmd::Fill::Datas datas;
    int boundaryIndex = -1;
    for (const auto &boundary : selectionWidget->getButton(0)->getMessages())
    {
      boundaryIndex++;
      assert(boundaryIndex >= 0 && boundaryIndex < static_cast<int>(parameters.size()));
      datas.push_back({boundary, parameters.at(boundaryIndex), true});
    }
    for (const auto &internal : selectionWidget->getButton(1)->getMessages())
    {
      datas.push_back({internal, ftr::Fill::buildContinuityParameter(), false});
    }
    
    command->setSelections(datas);
    command->localUpdate();
  }
  
};

Fill::Fill(cmd::Fill *cIn)
: Base("cmv::Fill")
, stow(new Stow(cIn, this))
{}

Fill::~Fill() = default;

void Fill::boundaryAdded(int index)
{
  const auto &s = stow->selectionWidget->getButton(0)->getMessages();
  assert(index >= 0 && index < static_cast<int>(s.size()));
  
  auto freshParameter = ftr::Fill::buildContinuityParameter();
  stow->parameters.push_back(freshParameter);
  
  if (s.at(index).type == slc::Type::Edge)
  {
    freshParameter->setValue(0);
    stow->stacked->addWidget(new QWidget(this));
  }
  else if (s.at(index).type == slc::Type::Face)
  {
    freshParameter->setValue(1);
    auto *w = new cmv::ParameterWidget(this, {stow->parameters.back().get()});
    connect(w, &ParameterBase::prmValueChanged, this, &Fill::continuityChanged);
    stow->stacked->addWidget(w);
  }
  stow->goUpdate();
}

void Fill::boundaryRemoved(int index)
{
  assert(index >= 0);
  assert(index < stow->stacked->count());
  assert(index < static_cast<int>(stow->parameters.size()));
  delete stow->stacked->widget(index);
  
  stow->goUpdate();
}

void Fill::boundarySelectionChanged()
{
  int index = stow->selectionWidget->getView(0)->getSelectedIndex();
  if (index < 0)
  {
    stow->stacked->setDisabled(true);
    return;
  }
  assert(index >= 0 && index < stow->stacked->count());
  stow->stacked->setCurrentIndex(index);
  
  const auto &s = stow->selectionWidget->getButton(0)->getMessages();
  if (s.at(index).type == slc::Type::Face)
    stow->stacked->setEnabled(true);
  else
    stow->stacked->setDisabled(true);
}

void Fill::continuityChanged()
{
  stow->goUpdate();
}

void Fill::internalAdded(int)
{
  stow->goUpdate();
}

void Fill::internalRemoved(int)
{
  stow->goUpdate();
}
