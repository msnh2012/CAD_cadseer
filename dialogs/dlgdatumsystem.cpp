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
#include <QComboBox>
#include <QStackedWidget>
#include <QTabWidget>

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "annex/annseershape.h"
#include "message/msgnode.h"
#include "dialogs/dlgselectionbutton.h"
#include "dialogs/dlgselectionwidget.h"
#include "dialogs/dlgparameterwidget.h"
#include "dialogs/dlgcsyswidget.h"
#include "parameter/prmparameter.h"
#include "selection/slccontainer.h"
#include "tools/featuretools.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrdatumsystem.h"
#include "dialogs/dlgdatumsystem.h"

using boost::uuids::uuid;

using namespace dlg;

struct DatumSystem::Stow
{
  Stow(ftr::DatumSystem::Feature *fIn)
  : feature(fIn)
  , csys(std::make_shared<prm::Parameter>(*feature->getParameter(prm::Names::CSys)))
  , offset(std::make_shared<prm::Parameter>(*feature->getParameter(prm::Names::Offset)))
  {}
  
  ftr::DatumSystem::Feature *feature = nullptr;
  std::shared_ptr<prm::Parameter> csys;
  std::shared_ptr<prm::Parameter> offset;
  QTabWidget *tabWidget;
  QComboBox *typeCombo = nullptr;
  QStackedWidget *stackedWidget = nullptr;
  SelectionWidget *selectionWidget3P = nullptr;
  CSysWidget *csysWidget = nullptr;
  ParameterWidget *offsetParameterWidget = nullptr;
};

DatumSystem::DatumSystem(ftr::DatumSystem::Feature *fIn, QWidget *pIn, bool isEditIn)
: Base(fIn, pIn, isEditIn)
, stow(new Stow(fIn))
{
  init();
}

DatumSystem::~DatumSystem() = default;

void DatumSystem::init()
{
  buildGui();
  
  QSettings &settings = app::instance()->getUserSettings();
  settings.beginGroup("dlg::DatumSystem");

  settings.endGroup();
  
  loadFeatureData();
  node->send(msg::buildSelectionMask(slc::None));
}

void DatumSystem::buildGui()
{
  auto clearContentMargins = [](QWidget *w) //assumes layout present
  {
    w->setContentsMargins(0, 0, 0, 0);
    w->layout()->setContentsMargins(0, 0, 0, 0);
  };
  
  QVBoxLayout *mainLayout = new QVBoxLayout();
  
  stow->tabWidget = new QTabWidget(this);
  mainLayout->addWidget(stow->tabWidget);
  connect(stow->tabWidget, &QTabWidget::currentChanged, this, &DatumSystem::tabChanged);
  
  QWidget *page0 = new QWidget(stow->tabWidget);
  QVBoxLayout *page0Layout = new QVBoxLayout();
  page0->setLayout(page0Layout);
  clearContentMargins(page0);
  stow->tabWidget->addTab(page0, tr("Definition"));
  
  stow->typeCombo = new QComboBox(page0);
  stow->typeCombo->addItem(tr("Parameter"));
  stow->typeCombo->addItem(tr("Through 3 points"));
  page0Layout->addWidget(stow->typeCombo);
  connect(stow->typeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(comboChanged(int)));
  
  stow->stackedWidget = new QStackedWidget(page0);
  page0Layout->addWidget(stow->stackedWidget);
  
  //csys parameter widget
  stow->csysWidget = new CSysWidget(page0, stow->csys.get());
  QWidget *dummy0 = new QWidget(page0);
  QVBoxLayout *dummyLayout0 =  new QVBoxLayout();
  dummy0->setLayout(dummyLayout0);
  clearContentMargins(dummy0);
  dummyLayout0->addWidget(stow->csysWidget);
  dummyLayout0->addStretch();
  stow->stackedWidget->addWidget(dummy0);
  
  //through 3 points
  SelectionWidgetCue cue;
  cue.name.clear();
  cue.singleSelection = false;
  cue.showAccrueColumn = false;
  cue.mask = slc::PointsEnabled | slc::PointsSelectable | slc::EndPointsEnabled
    | slc::MidPointsEnabled | slc::CenterPointsEnabled | slc::QuadrantPointsEnabled;
  cue.statusPrompt = tr("Select 3 points");
  stow->selectionWidget3P = new SelectionWidget(page0, std::vector<SelectionWidgetCue>(1, cue));
  stow->selectionWidget3P->getButton(0)->setChecked(true);
  stow->stackedWidget->addWidget(stow->selectionWidget3P);
  
  //offset page.
  QWidget *page1 = new QWidget(stow->tabWidget);
  QVBoxLayout *page1Layout = new QVBoxLayout();
  page1->setLayout(page1Layout);
  clearContentMargins(page1);
  stow->tabWidget->addTab(page1, tr("Offset"));
  
  stow->offsetParameterWidget = new ParameterWidget(page1, prm::Parameters(1, stow->offset.get()));
  QWidget *dummy1 = new QWidget(page1);
  QVBoxLayout *dummyLayout1 =  new QVBoxLayout();
  dummy1->setLayout(dummyLayout1);
  clearContentMargins(dummy1);
  dummyLayout1->addWidget(stow->offsetParameterWidget);
  dummyLayout1->addStretch();
  page1Layout->addWidget(dummy1);
  
  mainLayout->addStretch();
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

void DatumSystem::loadFeatureData()
{
  const auto &cue = stow->feature->getCue();
  int index = static_cast<int>(cue.systemType);
  stow->typeCombo->setCurrentIndex(index);
  stow->csysWidget->setCSysLinkId(gu::createNilId()); //turn on if needed
  
  if (cue.systemType == ftr::DatumSystem::SystemType::Through3Points)
  {
    //for all types?
    const auto &pl = project->getPayload(stow->feature->getId());
    tls::Resolver resolver(pl);
    for (const auto &p : cue.picks)
    {
      if (!resolver.resolve(p))
        continue;
      stow->selectionWidget3P->getButton(0)->addMessages(resolver.convertToMessages());
    }
  }
  else if (cue.systemType == ftr::DatumSystem::SystemType::Linked)
  {
    const auto &pl = project->getPayload(stow->feature->getId());
    auto features = pl.getFeatures(ftr::InputType::linkCSys);
    if (features.empty())
      stow->csysWidget->setCSysLinkId(gu::createNilId());
    else
      stow->csysWidget->setCSysLinkId(features.front()->getId());
  }
}

void DatumSystem::reject()
{
  isAccepted = false;
  finishDialog();
  QDialog::reject();
}

void DatumSystem::accept()
{
  isAccepted = true;
  finishDialog();
  QDialog::accept();
}

void DatumSystem::finishDialog()
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
  
  stow->feature->getParameter(prm::Names::CSys)->setValue(static_cast<osg::Matrixd>(*stow->csys));
  stow->feature->getParameter(prm::Names::Offset)->setValue(static_cast<osg::Vec3d>(*stow->offset));
  project->clearAllInputs(stow->feature->getId());
  tls::Connector connector;
  ftr::DatumSystem::Cue cue;
  int ti = stow->typeCombo->currentIndex(); //type index
  if (ti == 0)
  {
    uuid linkId = stow->csysWidget->getCSysLinkId();
    if (linkId.is_nil())
    {
      cue.systemType = ftr::DatumSystem::SystemType::Constant;
    }
    else
    {
      cue.systemType = ftr::DatumSystem::SystemType::Linked;
      connector.add(linkId, {ftr::InputType::linkCSys});
    }
  }
  else
    cue.systemType = static_cast<ftr::DatumSystem::SystemType>(stow->typeCombo->currentIndex());
  
  auto process = [&](const slc::Message& mIn, const char *tag)
  {
    if (!project->hasFeature(mIn.featureId))
      return;
    const ftr::Base *out = project->findFeature(mIn.featureId);
    if (!out->hasAnnex(ann::Type::SeerShape))
      return;
    if (out->getAnnex<ann::SeerShape>().isNull())
      return;
    
    auto *featureIn = project->findFeature(mIn.featureId);
    cue.picks.push_back(tls::convertToPick(mIn, featureIn->getAnnex<ann::SeerShape>(), project->getShapeHistory()));
    cue.picks.back().tag = tag;
    connector.add(mIn.featureId, cue.picks.back().tag);
    return;
  };
  
  if (cue.systemType == ftr::DatumSystem::SystemType::Through3Points)
  {
    const auto msgs = stow->selectionWidget3P->getMessages(0);
    if (msgs.size() > 0)
      process(msgs.at(0), ftr::DatumSystem::point0);
    if (msgs.size() > 1)
      process(msgs.at(1), ftr::DatumSystem::point1);
    if (msgs.size() > 2)
      process(msgs.at(2), ftr::DatumSystem::point2);
  }
  
  stow->feature->setCue(cue);
  for (const auto &pr : connector.pairs)
    p->connectInsert(pr.first, stow->feature->getId(), {pr.second});
  
  stow->csysWidget->syncLinks();
  
  queueUpdate();
  commandDone();
  restoreState();
}

void DatumSystem::comboChanged(int index)
{
  stow->stackedWidget->setCurrentIndex(index);
  if (index != 1)
    node->send(msg::buildSelectionMask(slc::None));
}

void DatumSystem::tabChanged(int index)
{
  if (index == 1)
    node->send(msg::buildSelectionMask(slc::None));
}
