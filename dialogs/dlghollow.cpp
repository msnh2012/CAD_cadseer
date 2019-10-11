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
#include <QHBoxLayout>

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "annex/annseershape.h"
#include "message/msgnode.h"
#include "dialogs/dlgselectionbutton.h"
#include "dialogs/dlgselectionwidget.h"
#include "dialogs/dlgparameterwidget.h"
#include "parameter/prmparameter.h"
#include "tools/featuretools.h"
#include "tools/idtools.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrhollow.h"
#include "dialogs/dlghollow.h"

using boost::uuids::uuid;

using namespace dlg;

struct Hollow::Stow
{
  Stow(ftr::Hollow *fIn)
  : feature(fIn)
  , offset(std::make_shared<prm::Parameter>(feature->getOffset()))
  {}
  
  ftr::Hollow *feature;
  SelectionWidget *selectionWidget = nullptr;
  ParameterWidget *parameterWidget = nullptr;
  std::shared_ptr<prm::Parameter> offset;
};

Hollow::Hollow(ftr::Hollow *fIn, QWidget *pIn, bool isEditIn)
: Base(fIn, pIn, isEditIn)
, stow(new Stow(fIn))
{
  init();
}

Hollow::~Hollow() = default;

void Hollow::init()
{
  buildGui();
  
  QSettings &settings = app::instance()->getUserSettings();
  settings.beginGroup("dlg::Hollow");

  settings.endGroup();
  
  loadFeatureData();
  stow->selectionWidget->activate(0);
}

void Hollow::buildGui()
{
  QVBoxLayout *mainLayout = new QVBoxLayout();
  
  std::vector<SelectionWidgetCue> cues;
  SelectionWidgetCue cue;
  
  cue.name = tr("Pierce Faces");
  cue.singleSelection = false;
  cue.mask = slc::FacesEnabled | slc::FacesSelectable;
  cue.statusPrompt = tr("Select Pierce Faces");
  cue.showAccrueColumn = false;
  cues.push_back(cue);
  
  stow->selectionWidget = new SelectionWidget(this, cues);
  mainLayout->addWidget(stow->selectionWidget);
  
  //angle parameter
  QHBoxLayout *eLayout = new QHBoxLayout();
  eLayout->addStretch();
  stow->parameterWidget = new ParameterWidget(this, std::vector<prm::Parameter*>(1, stow->offset.get()));
  eLayout->addWidget(stow->parameterWidget);
  mainLayout->addLayout(eLayout);
  
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

void Hollow::loadFeatureData()
{
  if (isEditDialog)
  {
    ftr::UpdatePayload pl = app::instance()->getProject()->getPayload(stow->feature->getId());
    tls::Resolver resolver(pl);
    for (const auto &p : stow->feature->getHollowPicks())
    {
      if (resolver.resolve(p))
        stow->selectionWidget->getButton(0)->addMessages(resolver.convertToMessages());
    }
    auto features = pl.getFeatures("");
    if (!features.empty())
    {
      leafChildren = app::instance()->getProject()->getLeafChildren(features.front()->getId());
      app::instance()->getProject()->setCurrentLeaf(features.front()->getId());
    }
  }
}

void Hollow::reject()
{
  isAccepted = false;
  finishDialog();
  QDialog::reject();
}

void Hollow::accept()
{
  isAccepted = true;
  finishDialog();
  QDialog::accept();
}

void Hollow::finishDialog()
{
  prj::Project *p = app::instance()->getProject();
  
  auto restoreState = [&]()
  {
    if (isEditDialog)
    {
      for (const auto &id : leafChildren)
        p->setCurrentLeaf(id);
    }
  };
  auto bail = [&]()
  {
    if (!isEditDialog)
      p->removeFeature(stow->feature->getId());
    commandDone();
    restoreState();
  };
  
  if(!isAccepted)
  {
    bail();
    return;
  }
  
  const auto &hms = stow->selectionWidget->getMessages(0);
  if (hms.empty())
  {
    bail();
    node->sendBlocked(msg::buildStatusMessage("Empty selections", 2.0));
    return;
  }
  
  auto getFeature = [&](const uuid &fId) -> const ftr::Base*
  {
    if (!p->hasFeature(fId))
      return nullptr;
    const ftr::Base *out = p->findFeature(fId);
    if (!out->hasAnnex(ann::Type::SeerShape))
      return nullptr;
    if (out->getAnnex<ann::SeerShape>().isNull())
      return nullptr;
    return out;
  };
  
  uuid fId = gu::createNilId();
  ftr::Picks picks;
  tls::Connector connector;
  for (const auto &m : hms)
  {
    const ftr::Base *f = getFeature(m.featureId);
    if (!f)
      continue;
    if (fId.is_nil())
      fId = m.featureId;
    if (fId != m.featureId)
      continue;
    auto ps = picks.size();
    picks.push_back(tls::convertToPick(m, f->getAnnex<ann::SeerShape>(), p->getShapeHistory()));
    picks.back().tag = ftr::InputType::createIndexedTag(ftr::InputType::target, ps);
    connector.add(fId, picks.back().tag);
  }
  
  stow->parameterWidget->syncLinks();
  p->clearAllInputs(stow->feature->getId());
  stow->feature->setHollowPicks(picks);
  stow->feature->setOffset(static_cast<double>(*stow->offset));
  for (const auto &pr : connector.pairs)
    p->connectInsert(pr.first, stow->feature->getId(), {pr.second});
  
  node->sendBlocked(msg::buildHideThreeD(fId));
  node->sendBlocked(msg::buildHideOverlay(fId));

  queueUpdate();
  commandDone();
  restoreState();
}
