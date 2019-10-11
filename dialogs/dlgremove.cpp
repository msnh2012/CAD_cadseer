/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2019 Thomas S. Anderson blobfish.at.gmx.com
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
#include <QDialogButtonBox>
#include <QVBoxLayout>

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "message/msgmessage.h"
#include "project/prjmessage.h"
#include "dialogs/dlgselectionbutton.h"
#include "dialogs/dlgselectionwidget.h"
#include "dialogs/dlgremove.h"

using boost::uuids::uuid;

using namespace dlg;

struct Remove::Stow
{
  Stow()
  {}
  SelectionWidget *selectionWidget = nullptr;
};

Remove::Remove(QWidget *pIn)
: QDialog(pIn)
, stow(new Stow())
{
  init();
}

Remove::~Remove() = default;

void Remove::init()
{
  buildGui();
  
  QSettings &settings = app::instance()->getUserSettings();
  settings.beginGroup("dlg::Remove");

  settings.endGroup();
  
  stow->selectionWidget->activate(0);
}

void Remove::buildGui()
{
  QVBoxLayout *mainLayout = new QVBoxLayout();
  
  std::vector<SelectionWidgetCue> cues;
  SelectionWidgetCue cue;
  cue.name = tr("Features");
  cue.singleSelection = false;
  cue.showAccrueColumn = false;
  cue.mask = slc::ObjectsEnabled | slc::ObjectsSelectable;
  cue.statusPrompt = tr("Select Features To Remove");
  cues.push_back(cue);
  stow->selectionWidget = new SelectionWidget(this, cues);
  mainLayout->addWidget(stow->selectionWidget);
  
  //dialog box buttons
  QDialogButtonBox *buttons = new QDialogButtonBox
    (QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
  QHBoxLayout *dButtonLayout = new QHBoxLayout();
  dButtonLayout->addStretch();
  dButtonLayout->addWidget(buttons);
  connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
  mainLayout->addLayout(dButtonLayout);
  
  this->setLayout(mainLayout);
}

void Remove::reject()
{
  QDialog::reject();
  app::instance()->queuedMessage(msg::Message(msg::Mask(msg::Request | msg::Command | msg::Done)));
}

void Remove::accept()
{
  //make a copy just in case button messages get changed from deletion of features.
  slc::Messages messages = stow->selectionWidget->getButton(0)->getMessages();
  
  QDialog::accept();
  
  prj::Project *p = app::instance()->getProject();
  for (const auto &current : stow->selectionWidget->getButton(0)->getMessages())
  {
    if (!slc::isObjectType(current.type))
      continue;
    if (!p->hasFeature(current.featureId)) //just in case feature has been deleted
      continue;
    prj::Message payload;
    payload.featureIds.push_back(current.featureId);
    msg::Message removeMessage(msg::Request | msg::Remove | msg::Feature, payload);
    app::instance()->messageSlot(removeMessage);
  }
  app::instance()->messageSlot(msg::buildStatusMessage("Features Removed", 2.0));
  
  app::instance()->queuedMessage(msg::Message(msg::Mask(msg::Request | msg::Command | msg::Done)));
  if (prf::manager().rootPtr->dragger().triggerUpdateOnFinish())
    app::instance()->queuedMessage(msg::Message(msg::Request | msg::Project | msg::Update));
}
