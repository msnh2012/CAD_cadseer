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
// #include "tools/featuretools.h"
// #include "tools/idtools.h"
// #include "feature/ftrinputtype.h"
#include "feature/ftrsubtract.h"
#include "feature/ftrintersect.h"
#include "feature/ftrunion.h"
#include "command/cmdboolean.h"
#include "commandview/cmvboolean.h"

using boost::uuids::uuid;

using namespace cmv;

struct Boolean::Stow
{
  cmd::Boolean *command;
  cmv::Boolean *view;
  dlg::SelectionWidget *selectionWidget = nullptr;
//   cmv::ParameterWidget *parameterWidget = nullptr;
//   std::vector<prm::Observer> observers;
  
  Stow(cmd::Boolean *cIn, cmv::Boolean *vIn)
  : command(cIn)
  , view(vIn)
  {
    buildGui();
    
    QSettings &settings = app::instance()->getUserSettings();
    settings.beginGroup("cmv::Boolean");
    //load settings
    settings.endGroup();
    
    loadFeatureData();
    glue();
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
    cue.mask = slc::ObjectsEnabled | slc::ObjectsSelectable | slc::SolidsEnabled;
    cue.statusPrompt = tr("Select Target Object Or Solid");
    cue.showAccrueColumn = false;
    cues.push_back(cue);
    cue.name = tr("Tools");
    cue.singleSelection = false;
    cue.mask = slc::ObjectsEnabled | slc::ObjectsSelectable | slc::SolidsEnabled;
    cue.statusPrompt = tr("Select Tool Object Or Solid");
    cues.push_back(cue);
    selectionWidget = new dlg::SelectionWidget(view, cues);
    mainLayout->addWidget(selectionWidget);
    
    mainLayout->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
  }
  
  void loadFeatureData()
  {
    slc::Messages targets;
    slc::Messages tools;
    std::tie(targets, tools) = command->getSelections();
    
    selectionWidget->initializeButton(0, targets);
    selectionWidget->initializeButton(1, tools);
  }
  
  void glue()
  {
    connect(selectionWidget->getButton(0), &dlg::SelectionButton::dirty, view, &Boolean::selectionChanged);
    connect(selectionWidget->getButton(1), &dlg::SelectionButton::dirty, view, &Boolean::selectionChanged);
  }
};

Boolean::Boolean(cmd::Boolean *cIn)
: Base("cmv::Boolean")
, stow(new Stow(cIn, this))
{}

Boolean::~Boolean() = default;

void Boolean::selectionChanged()
{
  stow->command->setSelections(stow->selectionWidget->getMessages(0), stow->selectionWidget->getMessages(1));
  stow->command->localUpdate();
}
