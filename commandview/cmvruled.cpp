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
#include "feature/ftrruled.h"
#include "command/cmdruled.h"
#include "commandview/cmvruled.h"

using boost::uuids::uuid;

using namespace cmv;

struct Ruled::Stow
{
  cmd::Ruled *command;
  cmv::Ruled *view;
  dlg::SelectionWidget *selectionWidget = nullptr;
  
  Stow(cmd::Ruled *cIn, cmv::Ruled *vIn)
  : command(cIn)
  , view(vIn)
  {
    buildGui();
    
    QSettings &settings = app::instance()->getUserSettings();
    settings.beginGroup("cmv::Ruled");
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
    cue.name = tr("Item 1");
    cue.singleSelection = true;
    cue.mask = slc::ObjectsEnabled | slc::ObjectsSelectable | slc::WiresEnabled | slc::WiresSelectable | slc::EdgesEnabled | slc::EdgesSelectable;
    cue.statusPrompt = tr("Select Item 1");
    cue.showAccrueColumn = false;
    cues.push_back(cue);
    cue.name = tr("Item 2");
    cue.statusPrompt = tr("Select Item 2");
    cues.push_back(cue);
    
    selectionWidget = new dlg::SelectionWidget(view, cues);
    mainLayout->addWidget(selectionWidget);
    
    mainLayout->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    
    QObject::connect(selectionWidget->getButton(0), &dlg::SelectionButton::dirty, view, &Ruled::selectionChanged);
    QObject::connect(selectionWidget->getButton(1), &dlg::SelectionButton::dirty, view, &Ruled::selectionChanged);
  }
  
  void loadFeatureData()
  {
    ftr::UpdatePayload up = view->project->getPayload(command->feature->getId());
    tls::Resolver resolver(up);
    
    auto pickToMessages = [&](const ftr::Pick &pIn) -> slc::Messages
    {
      slc::Messages out;
      if (resolver.resolve(pIn))
      {
        auto msgs = resolver.convertToMessages();
        out.insert(out.end(), msgs.begin(), msgs.end());
      }
      return out;
    };
    
    const auto &cp = command->feature->getPicks();
    if (!cp.empty())
      selectionWidget->initializeButton(0, pickToMessages(cp.front()));
    if (cp.size() > 1)
      selectionWidget->initializeButton(1, pickToMessages(cp.at(1)));
    selectionWidget->activate(0);
  }
  
  void goSelections()
  {
    auto msgs = selectionWidget->getMessages(0);
    msgs.insert(msgs.begin(), selectionWidget->getMessages(1).begin(), selectionWidget->getMessages(1).end());
    command->setSelections(msgs);
    command->localUpdate();
  }
};

Ruled::Ruled(cmd::Ruled *cIn)
: Base("cmv::Ruled")
, stow(new Stow(cIn, this))
{}

Ruled::~Ruled() = default;

void Ruled::selectionChanged()
{
  stow->goSelections();
}
