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
#include "feature/ftrextract.h"
#include "dialogs/dlgextract.h"

using boost::uuids::uuid;

using namespace dlg;

struct Extract::Stow
{
  Stow(ftr::Extract *fIn, prm::Handler handler)
  : feature(fIn)
  , parameter(std::make_shared<prm::Parameter>(*feature->getAngleParameter()))
  , pObserver(handler)
  {
    parameter->connect(pObserver);
  }
  
  ftr::Extract *feature;
  SelectionWidget *selectionWidget = nullptr;
  ParameterWidget *parameterWidget = nullptr;
  std::shared_ptr<prm::Parameter> parameter;
  prm::Observer pObserver;
};

Extract::Extract(ftr::Extract *fIn, QWidget *pIn, bool isEditIn)
: Base(fIn, pIn, isEditIn)
, stow(std::make_unique<Stow>(fIn, std::bind(&Extract::valueChanged, this)))
{
  init();
}

Extract::~Extract() = default;

void Extract::valueChanged()
{
  stow->selectionWidget->setAngle(static_cast<double>(*stow->parameter));
}

void Extract::init()
{
  buildGui();
  
  QSettings &settings = app::instance()->getUserSettings();
  settings.beginGroup("dlg::Extract");

  settings.endGroup();
  
  loadFeatureData();
  stow->selectionWidget->activate(0);
}

void Extract::buildGui()
{
  QVBoxLayout *mainLayout = new QVBoxLayout();
  
  std::vector<SelectionWidgetCue> cues;
  SelectionWidgetCue cue;
  
  cue.name = tr("Entities");
  cue.singleSelection = false;
  cue.mask = slc::AllEnabled & ~slc::PointsEnabled;
  cue.statusPrompt = tr("Select Entities");
  cues.push_back(cue);
  
  stow->selectionWidget = new SelectionWidget(this, cues);
  connect(stow->selectionWidget, &SelectionWidget::accrueChanged, this, &Extract::accrueChanged);
  mainLayout->addWidget(stow->selectionWidget);
  
  //angle parameter
  QHBoxLayout *eLayout = new QHBoxLayout();
  eLayout->addStretch();
  stow->parameterWidget = new ParameterWidget(this, std::vector<prm::Parameter*>(1, stow->parameter.get()));
  eLayout->addWidget(stow->parameterWidget);
  mainLayout->addLayout(eLayout);
  stow->parameterWidget->setDisabled(true);
  
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

void Extract::accrueChanged()
{
  bool pwe = false; //parameter widget enabled
  
  auto hasTangent = [&](const SelectionButton *b) -> bool
  {
    for (const auto &m : b->getMessages())
    {
      if (m.accrue == slc::Accrue::Tangent)
        return true;
    }
    return false;
  };
  
  int bc = stow->selectionWidget->getButtonCount();
  for (int i = 0; i < bc; ++i)
  {
    SelectionButton *b = stow->selectionWidget->getButton(i);
    if (hasTangent(b))
    {
      pwe = true;
      break;
    }
  }
  stow->parameterWidget->setEnabled(pwe);
}

void Extract::loadFeatureData()
{
  if (isEditDialog)
  {
    ftr::UpdatePayload pl = app::instance()->getProject()->getPayload(stow->feature->getId());
    tls::Resolver resolver(pl);
    for (const auto &p : stow->feature->getPicks())
    {
      if (resolver.resolve(p))
      {
        stow->selectionWidget->getButton(0)->addMessages(resolver.convertToMessages());
      }
    }
    auto features = pl.getFeatures("");
    if (!features.empty())
    {
      leafChildren = app::instance()->getProject()->getLeafChildren(features.front()->getId());
      app::instance()->getProject()->setCurrentLeaf(features.front()->getId());
    }
    accrueChanged();
    valueChanged();
  }
}

void Extract::reject()
{
  isAccepted = false;
  finishDialog();
  QDialog::reject();
}

void Extract::accept()
{
  isAccepted = true;
  finishDialog();
  QDialog::accept();
}

void Extract::finishDialog()
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
  
  const auto &ems = stow->selectionWidget->getMessages(0);
  if (ems.empty())
  {
    bail();
    node->sendBlocked(msg::buildStatusMessage("Empty selections", 2.0));
    return;
  }
  
  uuid fId = gu::createNilId();
  ftr::Picks ePicks;
  tls::Connector connector;
  for (const auto &s : ems)
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
    auto ds = ePicks.size();
    ePicks.push_back(tls::convertToPick(s, fb->getAnnex<ann::SeerShape>(), p->getShapeHistory()));
    ePicks.back().tag = ftr::InputType::createIndexedTag(ftr::InputType::target, ds);
    connector.add(s.featureId, ePicks.back().tag);
  }
  
  if (ePicks.empty())
  {
    bail();
    node->sendBlocked(msg::buildStatusMessage("Empty Picks", 2.0));
    return;
  }
  stow->parameterWidget->syncLinks();
  p->clearAllInputs(stow->feature->getId());
  stow->feature->setPicks(ePicks);
  stow->feature->setAngleParameter(stow->parameter);
  for (const auto &pr : connector.pairs)
    p->connectInsert(pr.first, stow->feature->getId(), {pr.second});
  
  node->sendBlocked(msg::buildHideThreeD(ems.front().featureId));
  node->sendBlocked(msg::buildHideOverlay(ems.front().featureId));
  queueUpdate();
  commandDone();
  restoreState();
}
