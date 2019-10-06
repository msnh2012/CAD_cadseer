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

#include <osg/Switch>

#include <QSettings>
#include <QDialogButtonBox>
#include <QVBoxLayout>

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "dialogs/dlgparameterwidget.h"
#include "parameter/prmparameter.h"
#include "feature/ftrsphere.h"
#include "dialogs/dlgsphere.h"

using boost::uuids::uuid;

using namespace dlg;

struct Sphere::Stow
{
  Stow(ftr::Sphere *fIn)
  : feature(fIn)
  , radius(feature->getRadius())
  , overlayWasOn(feature->isVisibleOverlay())
  {}
  
  ftr::Sphere *feature;
  ParameterWidget *parameterWidget = nullptr;
  prm::Parameter radius;
  bool overlayWasOn = true;
};

Sphere::Sphere(ftr::Sphere *fIn, QWidget *pIn, bool isEditIn)
: Base(fIn, pIn, isEditIn)
, stow(new Stow(fIn))
{
  init();
}

Sphere::~Sphere() = default;

void Sphere::init()
{
  stow->feature->getOverlaySwitch()->setAllChildrenOff();
  
  buildGui();
  
  QSettings &settings = app::instance()->getUserSettings();
  settings.beginGroup("dlg::Sphere");

  settings.endGroup();
}

void Sphere::buildGui()
{
  std::vector<prm::Parameter*> prms({&stow->radius});
  stow->parameterWidget = new ParameterWidget(this, prms);
  
  QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
  connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
  
  QVBoxLayout *l = new QVBoxLayout();
  l->addWidget(stow->parameterWidget);
  l->addStretch();
  l->addWidget(buttons);
  
  this->setLayout(l);
}

void Sphere::reject()
{
  isAccepted = false;
  finishDialog();
  QDialog::reject();
}

void Sphere::accept()
{
  isAccepted = true;
  finishDialog();
  QDialog::accept();
}

void Sphere::finishDialog()
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
  stow->feature->setRadius(static_cast<double>(stow->radius));
  
  queueUpdate();
  commandDone();
  restoreState();
}
