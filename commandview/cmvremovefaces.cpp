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
// #include "commandview/cmvparameterwidgets.h"
// #include "parameter/prmparameter.h"
// #include "expressions/exprmanager.h"
// #include "expressions/exprstringtranslator.h"
// #include "expressions/exprvalue.h"
// #include "library/lbrplabel.h"
#include "tools/featuretools.h"
// #include "tools/idtools.h"
// #include "feature/ftrinputtype.h"
#include "feature/ftrremovefaces.h"
#include "command/cmdremovefaces.h"
#include "commandview/cmvremovefaces.h"

using boost::uuids::uuid;

using namespace cmv;

struct RemoveFaces::Stow
{
  cmd::RemoveFaces *command;
  cmv::RemoveFaces *view;
  dlg::SelectionWidget *selectionWidget = nullptr;
//   cmv::ParameterWidget *parameterWidget = nullptr;
//   std::vector<prm::Observer> observers;
  
  Stow(cmd::RemoveFaces *cIn, cmv::RemoveFaces *vIn)
  : command(cIn)
  , view(vIn)
  {
    buildGui();
    
    QSettings &settings = app::instance()->getUserSettings();
    settings.beginGroup("cmv::RemoveFaces");
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
    cue.name = tr("Faces");
    cue.singleSelection = false;
    cue.mask = slc::FacesEnabled | slc::FacesSelectable;
    cue.statusPrompt = tr("Select Faces To Remove");
    cue.showAccrueColumn = false; //true in future
    cues.push_back(cue);
    selectionWidget = new dlg::SelectionWidget(view, cues);
    mainLayout->addWidget(selectionWidget);
    
    mainLayout->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    
    QObject::connect(selectionWidget->getButton(0), &dlg::SelectionButton::dirty, view, &RemoveFaces::selectionChanged);
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

RemoveFaces::RemoveFaces(cmd::RemoveFaces *cIn)
: Base("cmv::RemoveFaces")
, stow(new Stow(cIn, this))
{}

RemoveFaces::~RemoveFaces() = default;

void RemoveFaces::selectionChanged()
{
  stow->goSelections();
}
