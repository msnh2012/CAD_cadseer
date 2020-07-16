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
#include "message/msgmessage.h"
#include "message/msgnode.h"
#include "project/prjmessage.h"
#include "dialogs/dlgselectionbutton.h"
#include "dialogs/dlgselectionwidget.h"
#include "command/cmdremove.h"
#include "commandview/cmvremove.h"

using boost::uuids::uuid;

using namespace cmv;

struct Remove::Stow
{
  cmd::Remove *command;
  cmv::Remove *view;
  dlg::SelectionWidget *selectionWidget = nullptr;
  
  Stow(cmd::Remove *cIn, cmv::Remove *vIn)
  : command(cIn)
  , view(vIn)
  {
    buildGui();
    
    QSettings &settings = app::instance()->getUserSettings();
    settings.beginGroup("cmv::Remove");
    //load settings
    settings.endGroup();
    
    glue();
    selectionWidget->activate(0);
  }
  
  void buildGui()
  {
    QVBoxLayout *mainLayout = new QVBoxLayout();
  
    std::vector<dlg::SelectionWidgetCue> cues;
    dlg::SelectionWidgetCue cue;
    cue.name = tr("Features");
    cue.singleSelection = true;
    cue.showAccrueColumn = false;
    cue.mask = slc::ObjectsEnabled | slc::ObjectsSelectable;
    cue.statusPrompt = tr("Select Features To Remove");
    cues.push_back(cue);
    selectionWidget = new dlg::SelectionWidget(view, cues);
    mainLayout->addWidget(selectionWidget);
    
    mainLayout->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    
    view->setLayout(mainLayout);
  }
  
  void glue()
  {
    connect(selectionWidget->getButton(0), &dlg::SelectionButton::dirty, view, &Remove::selectionChanged);
  }
};

Remove::Remove(cmd::Remove *cIn)
: Base("cmv::Remove")
, stow(new Stow(cIn, this))
{}

Remove::~Remove() = default;

void Remove::selectionChanged()
{
  const auto &msgs = stow->selectionWidget->getButton(0)->getMessages();
  if (msgs.empty() || !project->hasFeature(msgs.front().featureId))
    return;

  prj::Message payload;
  payload.featureIds.push_back(msgs.front().featureId);
  msg::Message removeMessage(msg::Request | msg::Remove | msg::Feature, payload);
  
  //this should recurse through button, but empty check above should force return;
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  node->sendBlocked(removeMessage);
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
}
