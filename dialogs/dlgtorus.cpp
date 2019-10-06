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
#include "feature/ftrtorus.h"
#include "dialogs/dlgtorus.h"

using boost::uuids::uuid;

using namespace dlg;

struct Torus::Stow
{
  Stow(ftr::Torus *fIn)
  : feature(fIn)
  , radius1(feature->getRadius1())
  , radius2(feature->getRadius2())
  , overlayWasOn(feature->isVisibleOverlay())
  {}
  
  ftr::Torus *feature;
  ParameterWidget *parameterWidget = nullptr;
  prm::Parameter radius1;
  prm::Parameter radius2;
  bool overlayWasOn = true;
};

Torus::Torus(ftr::Torus *fIn, QWidget *pIn, bool isEditIn)
: Base(fIn, pIn, isEditIn)
, stow(new Stow(fIn))
{
  init();
}

Torus::~Torus() = default;

void Torus::init()
{
  stow->feature->getOverlaySwitch()->setAllChildrenOff();
  
  buildGui();
  
  QSettings &settings = app::instance()->getUserSettings();
  settings.beginGroup("dlg::Torus");

  settings.endGroup();
}

void Torus::buildGui()
{
  std::vector<prm::Parameter*> prms({&stow->radius1, &stow->radius2});
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

void Torus::reject()
{
  isAccepted = false;
  finishDialog();
  QDialog::reject();
}

void Torus::accept()
{
  isAccepted = true;
  finishDialog();
  QDialog::accept();
}

void Torus::finishDialog()
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
  stow->feature->setRadius1(static_cast<double>(stow->radius1));
  stow->feature->setRadius2(static_cast<double>(stow->radius2));
  
  queueUpdate();
  commandDone();
  restoreState();
}
