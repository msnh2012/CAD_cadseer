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
#include "message/msgnode.h"
#include "commandview/cmvparameterwidgets.h"
#include "commandview/cmvcsyswidget.h"
#include "parameter/prmparameter.h"
#include "tools/idtools.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrbase.h"
#include "commandview/cmvprimitive.h"

using boost::uuids::uuid;

using namespace cmv;

struct Primitive::Stow
{
  ftr::Base *feature;
  cmv::Primitive *view;
  cmv::ParameterWidget *parameterWidget = nullptr;
  cmv::CSysWidget *csysWidget = nullptr;
  
  Stow(ftr::Base *fIn, cmv::Primitive *vIn)
  : feature(fIn)
  , view(vIn)
  {
    buildGui();
    
    QSettings &settings = app::instance()->getUserSettings();
    settings.beginGroup("cmv::Primitive");
    //load settings
    settings.endGroup();
    
    glue();
  }
  
  void buildGui()
  {
    QSizePolicy adjust = view->sizePolicy();
    adjust.setVerticalPolicy(QSizePolicy::Maximum);
    view->setSizePolicy(adjust);
    
    QVBoxLayout *mainLayout = new QVBoxLayout();
    view->setLayout(mainLayout);
    
    parameterWidget = new cmv::ParameterWidget(view, feature->getParameters());
    mainLayout->addWidget(parameterWidget);
    prm::Parameter *csys = feature->getParameters(prm::Tags::CSys).front();
    csysWidget = dynamic_cast<cmv::CSysWidget*>(parameterWidget->getWidget(csys));
    assert(csysWidget);
    ftr::UpdatePayload payload = view->project->getPayload(feature->getId());
    std::vector<const ftr::Base*> inputs = payload.getFeatures(ftr::InputType::linkCSys);
    if (inputs.empty())
      csysWidget->setCSysLinkId(gu::createNilId());
    else
      csysWidget->setCSysLinkId(inputs.front()->getId());
    
    mainLayout->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
  }
  
  void glue()
  {
    QObject::connect(csysWidget, &CSysWidget::dirty, view, &Primitive::linkedCSysChanged);
    QObject::connect(parameterWidget, &ParameterBase::prmValueChanged, view, &Primitive::parameterChanged);
  }
  
  void goUpdate()
  {
    feature->updateModel(view->project->getPayload(feature->getId()));
    feature->updateVisual();
    feature->setModelDirty();
    view->node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  }
};

Primitive::Primitive(ftr::Base *fIn)
: Base("cmv::Primitive")
, stow(new Stow(fIn, this))
{}

Primitive::~Primitive() = default;

void Primitive::linkedCSysChanged()
{
  project->clearAllInputs(stow->feature->getId());
  uuid lId = stow->csysWidget->getCSysLinkId();
  if (!lId.is_nil())
    project->connect(lId, stow->feature->getId(), {ftr::InputType::linkCSys});
  stow->goUpdate();
}

void Primitive::parameterChanged()
{
  stow->goUpdate();
}
