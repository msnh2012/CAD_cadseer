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
#include <QVBoxLayout>

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "dialogs/dlgselectionbutton.h"
#include "dialogs/dlgselectionwidget.h"
#include "tools/featuretools.h"
#include "feature/ftrmappcurve.h"
#include "command/cmdmappcurve.h"
#include "commandview/cmvmappcurve.h"

using boost::uuids::uuid;

using namespace cmv;

struct MapPCurve::Stow
{
  cmd::MapPCurve *command;
  cmv::MapPCurve *view;
  dlg::SelectionWidget *selectionWidget = nullptr;
  
  Stow(cmd::MapPCurve *cIn, cmv::MapPCurve *vIn)
  : command(cIn)
  , view(vIn)
  {
    buildGui();
    
    QSettings &settings = app::instance()->getUserSettings();
    settings.beginGroup("cmv::MapPCurve");
    //load settings
    settings.endGroup();
    
    loadFeatureData();
    glue();
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
    cue.name = tr("Face");
    cue.singleSelection = true;
    cue.mask = slc::ObjectsEnabled | slc::ObjectsSelectable | slc::FacesEnabled | slc::FacesSelectable;
    cue.statusPrompt = tr("Select Target Face");
    cue.showAccrueColumn = false;
    cues.push_back(cue);
    cue.name = tr("Edges");
    cue.singleSelection = false;
    cue.mask = slc::ObjectsEnabled | slc::ObjectsSelectable | slc::WiresEnabled | slc::WiresSelectable | slc::EdgesEnabled | slc::EdgesSelectable;
    cue.statusPrompt = tr("Select Edges To Map");
    cues.push_back(cue);
    selectionWidget = new dlg::SelectionWidget(view, cues);
    mainLayout->addWidget(selectionWidget);
    
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
    
    selectionWidget->initializeButton(0, picksToMessages({command->feature->getFacePick()}));
    selectionWidget->initializeButton(1, picksToMessages(command->feature->getEdgePicks()));
    selectionWidget->activate(0);
  }
  
  void glue()
  {
    QObject::connect(selectionWidget->getButton(0), &dlg::SelectionButton::dirty, view, &MapPCurve::selectionChanged);
    QObject::connect(selectionWidget->getButton(1), &dlg::SelectionButton::dirty, view, &MapPCurve::selectionChanged);
  }
};

MapPCurve::MapPCurve(cmd::MapPCurve *cIn)
: Base("cmv::MapPCurve")
, stow(new Stow(cIn, this))
{}

MapPCurve::~MapPCurve() = default;

void MapPCurve::selectionChanged()
{
  stow->command->setSelections(stow->selectionWidget->getMessages(0), stow->selectionWidget->getMessages(1));
  stow->command->localUpdate();
}
