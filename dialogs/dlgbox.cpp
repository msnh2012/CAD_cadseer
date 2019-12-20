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
#include <QTabWidget>

#include <osg/Switch>

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "feature/ftrinputtype.h"
#include "tools/idtools.h"
#include "message/msgnode.h"
#include "dialogs/dlgparameterwidget.h"
#include "dialogs/dlgcsyswidget.h"
#include "parameter/prmparameter.h"
#include "feature/ftrbox.h"
#include "dialogs/dlgbox.h"

using boost::uuids::uuid;

using namespace dlg;

struct Box::Stow
{
  Stow(ftr::Box *fIn)
  : feature(fIn)
  , length(std::make_shared<prm::Parameter>(*feature->getLength()))
  , width(std::make_shared<prm::Parameter>(*feature->getWidth()))
  , height(std::make_shared<prm::Parameter>(*feature->getHeight()))
  , csys(std::make_shared<prm::Parameter>(*feature->getParameter(prm::Names::CSys)))
  , overlayWasOn(feature->isVisibleOverlay())
  {}
  
  ftr::Box *feature;
  QTabWidget *tabWidget = nullptr;
  ParameterWidget *parameterWidget = nullptr;
  CSysWidget *csysWidget = nullptr;
  std::shared_ptr<prm::Parameter> length;
  std::shared_ptr<prm::Parameter> width;
  std::shared_ptr<prm::Parameter> height;
  std::shared_ptr<prm::Parameter> csys;
  bool overlayWasOn = true;
};

Box::Box(ftr::Box *fIn, QWidget *pIn, bool isEditIn)
: Base(fIn, pIn, isEditIn)
, stow(new Stow(fIn))
{
  init();
}

Box::~Box() = default;

void Box::init()
{
  stow->feature->getOverlaySwitch()->setAllChildrenOff();
  
  buildGui();
  
  QSettings &settings = app::instance()->getUserSettings();
  settings.beginGroup("dlg::Box");

  settings.endGroup();
}

void Box::buildGui()
{
  stow->tabWidget = new QTabWidget(this);
  connect(stow->tabWidget, &QTabWidget::currentChanged, this, &Box::currentTabChanged);
  
  std::vector<prm::Parameter*> prms({stow->length.get(), stow->width.get(), stow->height.get()});
  stow->parameterWidget = new ParameterWidget(this, prms);
  QWidget *dummy = new QWidget(this);
  dummy->setContentsMargins(0, 0, 0, 0);
  QVBoxLayout *dummyLayout =  new QVBoxLayout();
  dummyLayout->setContentsMargins(0, 0, 0, 0);
  dummy->setLayout(dummyLayout);
  dummyLayout->addWidget(stow->parameterWidget);
  dummyLayout->addStretch();
  stow->tabWidget->addTab(dummy, tr("Parameters"));
  
  stow->csysWidget = new CSysWidget(this, stow->csys.get());
  ftr::UpdatePayload payload = project->getPayload(stow->feature->getId());
  std::vector<const ftr::Base*> inputs = payload.getFeatures(ftr::InputType::linkCSys);
  if (inputs.empty())
    stow->csysWidget->setCSysLinkId(gu::createNilId());
  else
    stow->csysWidget->setCSysLinkId(inputs.front()->getId());
  stow->tabWidget->addTab(stow->csysWidget, tr("CSys"));
  
  QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
  connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
  
  QVBoxLayout *l = new QVBoxLayout();
  l->addWidget(stow->tabWidget);
  l->addWidget(buttons);
  
  this->setLayout(l);
}

void Box::reject()
{
  isAccepted = false;
  finishDialog();
  QDialog::reject();
}

void Box::accept()
{
  isAccepted = true;
  finishDialog();
  QDialog::accept();
}

void Box::finishDialog()
{
  if (stow->overlayWasOn)
    stow->feature->getOverlaySwitch()->setAllChildrenOn();
  else
    stow->feature->getOverlaySwitch()->setAllChildrenOff();
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
  
  stow->parameterWidget->syncLinks();
  stow->csysWidget->syncLinks();
  stow->feature->setLength(static_cast<double>(*stow->length));
  stow->feature->setWidth(static_cast<double>(*stow->width));
  stow->feature->setHeight(static_cast<double>(*stow->height));
  stow->feature->setCSys(static_cast<osg::Matrixd>(*stow->csys));
  
  std::vector<const ftr::Base*> inputs = project->getPayload(stow->feature->getId()).getFeatures(ftr::InputType::linkCSys);
  uuid oldLink = gu::createNilId();
  if (!inputs.empty())
    oldLink = inputs.front()->getId();
  
  uuid newLinked = stow->csysWidget->getCSysLinkId();
  if (newLinked != oldLink)
  {
    stow->feature->setModelDirty();
    project->clearAllInputs(stow->feature->getId());
    if (!newLinked.is_nil())
      project->connectInsert(newLinked, stow->feature->getId(), ftr::InputType{ftr::InputType::linkCSys});
  }
  
  queueUpdate();
  commandDone();
  restoreState();
}

void Box::currentTabChanged(int index)
{
  if (index == 0)
    node->sendBlocked(msg::buildSelectionMask(slc::None));
  //we let csys widget handle when it is shown.
}
