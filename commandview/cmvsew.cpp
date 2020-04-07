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
#include "tools/featuretools.h"
#include "feature/ftrsew.h"
#include "command/cmdsew.h"
#include "commandview/cmvsew.h"

using boost::uuids::uuid;

using namespace cmv;

struct Sew::Stow
{
  cmd::Sew *command;
  cmv::Sew *view;
  dlg::SelectionWidget *selectionWidget = nullptr;
  
  Stow(cmd::Sew *cIn, cmv::Sew *vIn)
  : command(cIn)
  , view(vIn)
  {
    buildGui();
    
    QSettings &settings = app::instance()->getUserSettings();
    settings.beginGroup("cmv::Sew");
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
    
    std::vector<dlg::SelectionWidgetCue> cues;
    dlg::SelectionWidgetCue cue;
    cue.name = tr("Items");
    cue.singleSelection = false;
    cue.mask = slc::ObjectsEnabled | slc::ObjectsSelectable | slc::FacesEnabled | slc::FacesSelectable;
    cue.statusPrompt = tr("Select Items");
    cue.showAccrueColumn = false;
    cues.push_back(cue);
    
    selectionWidget = new dlg::SelectionWidget(view, cues);
    mainLayout->addWidget(selectionWidget);
    
    mainLayout->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    
    QObject::connect(selectionWidget->getButton(0), &dlg::SelectionButton::dirty, view, &Sew::selectionChanged);
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
    selectionWidget->activate(0);
  }
  
  void goSelections()
  {
    command->setSelections(selectionWidget->getMessages(0));
    command->localUpdate();
  }
};

Sew::Sew(cmd::Sew *cIn)
: Base("cmv::Sew")
, stow(new Stow(cIn, this))
{}

Sew::~Sew() = default;

void Sew::selectionChanged()
{
  stow->goSelections();
}
