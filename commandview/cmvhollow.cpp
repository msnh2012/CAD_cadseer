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
#include "tools/featuretools.h"
// #include "tools/idtools.h"
// #include "feature/ftrinputtype.h"
#include "feature/ftrhollow.h"
#include "command/cmdhollow.h"
#include "commandview/cmvhollow.h"

using boost::uuids::uuid;

using namespace cmv;

struct Hollow::Stow
{
  cmd::Hollow *command;
  cmv::Hollow *view;
  dlg::SelectionWidget *selectionWidget = nullptr;
  cmv::ParameterWidget *parameterWidget = nullptr;
  std::vector<prm::Observer> observers;
  
  Stow(cmd::Hollow *cIn, cmv::Hollow *vIn)
  : command(cIn)
  , view(vIn)
  {
    buildGui();
    
    QSettings &settings = app::instance()->getUserSettings();
    settings.beginGroup("cmv::Hollow");
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
    
    cue.name = tr("Pierce Faces");
    cue.singleSelection = false;
    cue.mask = slc::FacesEnabled | slc::FacesSelectable;
    cue.statusPrompt = tr("Select Pierce Faces");
    cue.showAccrueColumn = false;
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
    selectionWidget->initializeButton(0, picksToMessages(command->feature->getHollowPicks()));
  }
  
  void glue()
  {
    connect(selectionWidget->getButton(0), &dlg::SelectionButton::dirty, view, &Hollow::selectionChanged);
  }
  
  void parameterChanged()
  {
    // lets make sure we don't trigger updates 'behind the scenes'
    if (!parameterWidget->isVisible())
      return;
    command->localUpdate();
  }
};

Hollow::Hollow(cmd::Hollow *cIn)
: Base("cmv::Hollow")
, stow(new Stow(cIn, this))
{}

Hollow::~Hollow() = default;

void Hollow::selectionChanged()
{
  stow->command->setSelections(stow->selectionWidget->getButton(0)->getMessages());
  stow->command->localUpdate();
}
