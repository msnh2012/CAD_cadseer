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
#include <QLineEdit>

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "annex/annseershape.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "message/msgmessage.h"
#include "message/msgnode.h"
#include "dialogs/dlgselectionbutton.h"
#include "dialogs/dlgselectionlist.h"
#include "dialogs/dlgselectionwidget.h"
#include "dialogs/dlgparameterwidget.h"
#include "parameter/prmparameter.h"
#include "expressions/exprmanager.h"
#include "expressions/exprstringtranslator.h"
#include "expressions/exprvalue.h"
#include "library/lbrplabel.h"
#include "tools/featuretools.h"
#include "tools/idtools.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrdraft.h"
#include "dialogs/dlgdraft.h"

using boost::uuids::uuid;

using namespace dlg;

struct Draft::Stow
{
  Stow(ftr::Draft *fIn)
  : feature(fIn)
  , parameter(std::make_shared<prm::Parameter>(*feature->getAngleParameter()))
  {}
  
  ftr::Draft *feature;
  SelectionWidget *selectionWidget = nullptr;
  ParameterWidget *parameterWidget = nullptr;
  std::shared_ptr<prm::Parameter> parameter;
};

Draft::Draft(ftr::Draft *fIn, QWidget *pIn, bool isEditIn)
: Base(fIn, pIn, isEditIn)
, stow(new Stow(fIn))
{
  init();
}

Draft::~Draft() = default;

void Draft::init()
{
  buildGui();
  
  QSettings &settings = app::instance()->getUserSettings();
  settings.beginGroup("dlg::Draft");

  settings.endGroup();
  
  loadFeatureData();
  stow->selectionWidget->activate(0);
}

void Draft::buildGui()
{
  QVBoxLayout *mainLayout = new QVBoxLayout();
  
  std::vector<SelectionWidgetCue> cues;
  SelectionWidgetCue cue;
  
  cue.name = tr("Neutral");
  cue.singleSelection = true;
  cue.mask = slc::FacesEnabled | slc::FacesSelectable;
  cue.statusPrompt = tr("Select Neutral Face");
  cues.push_back(cue);
  
  cue.name = tr("Draft");
  cue.singleSelection = false;
  cue.mask = slc::FacesEnabled | slc::FacesSelectable;
  cue.statusPrompt = tr("Select Faces To Draft");
  cues.push_back(cue);
  
  stow->selectionWidget = new SelectionWidget(this, cues);
  mainLayout->addWidget(stow->selectionWidget);
  
  //angle parameter
  QHBoxLayout *eLayout = new QHBoxLayout();
  eLayout->addStretch();
  stow->parameterWidget = new ParameterWidget(this, std::vector<prm::Parameter*>(1, stow->parameter.get()));
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

void Draft::loadFeatureData()
{
  if (isEditDialog)
  {
    ftr::UpdatePayload pl = app::instance()->getProject()->getPayload(stow->feature->getId());
    tls::Resolver resolver(pl);
    if (resolver.resolve(stow->feature->getNeutralPick()))
      stow->selectionWidget->getButton(0)->addMessages(resolver.convertToMessages());
    for (const auto &p : stow->feature->getTargetPicks())
    {
      if (resolver.resolve(p))
        stow->selectionWidget->getButton(1)->addMessages(resolver.convertToMessages());
    }
    auto features = pl.getFeatures("");
    if (!features.empty())
    {
      leafChildren = app::instance()->getProject()->getLeafChildren(features.front()->getId());
      app::instance()->getProject()->setCurrentLeaf(features.front()->getId());
    }
  }
}

void Draft::reject()
{
  isAccepted = false;
  finishDialog();
  QDialog::reject();
}

void Draft::accept()
{
  isAccepted = true;
  finishDialog();
  QDialog::accept();
}

void Draft::finishDialog()
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
  
  const auto &nms = stow->selectionWidget->getMessages(0);
  const auto &dms = stow->selectionWidget->getMessages(1);
  if (nms.empty() || dms.empty())
  {
    bail();
    node->sendBlocked(msg::buildStatusMessage("Empty selections", 2.0));
    return;
  }
  
  uuid fId = gu::createNilId();
  ftr::Picks neutralPicks;
  ftr::Picks draftPicks;
  tls::Connector connector;
  for (const auto &s : nms)
  {
    ftr::Base *fb = p->findFeature(s.featureId);
    if (!fb)
      continue;
    if (!fb->hasAnnex(ann::Type::SeerShape) || fb->getAnnex<ann::SeerShape>().isNull())
      continue;
    if (fId.is_nil())
      fId = s.featureId;
    if (fId != s.featureId)
      continue;
    neutralPicks.push_back(tls::convertToPick(s, fb->getAnnex<ann::SeerShape>(), p->getShapeHistory()));
    neutralPicks.back().tag = ftr::Draft::neutral;
    connector.add(s.featureId, neutralPicks.back().tag);
    break; //just one for now.
  }
  for (const auto &s : dms)
  {
    ftr::Base *fb = p->findFeature(s.featureId);
    if (!fb)
      continue;
    if (!fb->hasAnnex(ann::Type::SeerShape) || fb->getAnnex<ann::SeerShape>().isNull())
      continue;
    if (fId.is_nil())
      fId = s.featureId;
    if (fId != s.featureId)
      continue;
    auto ds = draftPicks.size();
    draftPicks.push_back(tls::convertToPick(s, fb->getAnnex<ann::SeerShape>(), p->getShapeHistory()));
    draftPicks.back().tag = ftr::InputType::createIndexedTag(ftr::InputType::target, ds);
    connector.add(s.featureId, draftPicks.back().tag);
  }
  if (neutralPicks.empty() || draftPicks.empty())
  {
    bail();
    node->sendBlocked(msg::buildStatusMessage("Empty Picks", 2.0));
    return;
  }
  
  //go with alteration of feature.
  stow->parameterWidget->syncLinks();
  p->clearAllInputs(stow->feature->getId());
  stow->feature->setNeutralPick(neutralPicks.front());
  stow->feature->setTargetPicks(draftPicks);
  stow->feature->setAngleParameter(stow->parameter);
  for (const auto &pr : connector.pairs)
    p->connectInsert(pr.first, stow->feature->getId(), {pr.second});
  
  node->sendBlocked(msg::buildHideThreeD(nms.front().featureId));
  node->sendBlocked(msg::buildHideOverlay(nms.front().featureId));
  
  queueUpdate();
  commandDone();
  restoreState();
}
