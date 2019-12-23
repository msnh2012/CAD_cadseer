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

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStackedLayout>
#include <QLabel>
#include <QRadioButton>

#include "application/appapplication.h"
#include "tools/idtools.h"
#include "parameter/prmparameter.h"
#include "message/msgmessage.h"
#include "selection/slcmessage.h"
#include "dialogs/dlgselectionbutton.h"
#include "dialogs/dlgparameterwidget.h"
#include "dialogs/dlgselectionwidget.h"
#include "dialogs/dlgvizfilter.h"
#include "dialogs/dlgcsyswidget.h"

namespace dlg
{
  struct CSysWidget::Stow
  {
    Stow() = delete;
    Stow(QWidget *parentIn, prm::Parameter* parameterIn)
    : parent(parentIn)
    , parameter(parameterIn)
    {
      buildGui();
    }
    
    void buildGui()
    {
      QVBoxLayout *mainLayout = new QVBoxLayout();
      mainLayout->setContentsMargins(0, 0, 0, 0);
      parent->setLayout(mainLayout);
      parent->setContentsMargins(0, 0, 0, 0);
      
      //the label and radio buttons
      byConstant = new QRadioButton(tr("Parameter"), parent);
      byFeature = new QRadioButton(tr("Feature"), parent);
      QHBoxLayout *csysLayout = new QHBoxLayout();
      csysLayout->addWidget(byConstant);
      csysLayout->addWidget(byFeature);
      csysLayout->addStretch();
      mainLayout->addLayout(csysLayout);
      
      //stacked widgets
      parameterWidget = new ParameterWidget(parent, std::vector<prm::Parameter*>(1, parameter));
      parameterWidget->setDisabled(true); //we can't parse coordinate systems yet
      SelectionWidgetCue cue;
      cue.name.clear();
      cue.singleSelection = true;
      cue.showAccrueColumn = false;
      cue.mask = slc::ObjectsEnabled | slc::ObjectsSelectable;
      cue.statusPrompt = tr("Select Feature To Link Coordinate System");
      QWidget *dummy = new QWidget(parent); //to keep parameter widget pushed 'up'
      dummy->setContentsMargins(0, 0, 0, 0);
      QVBoxLayout *dummyLayout = new QVBoxLayout();
      dummyLayout->setContentsMargins(0, 0, 0, 0);
      dummyLayout->addWidget(parameterWidget);
      dummyLayout->addStretch();
      dummy->setLayout(dummyLayout);
      VizFilter *vFilter = new VizFilter(parameterWidget);
      parameterWidget->installEventFilter(vFilter);
      connect(vFilter, &VizFilter::shown, [](){app::instance()->messageSlot(msg::buildSelectionMask(slc::None));});
      connect(vFilter, &VizFilter::shown, [](){app::instance()->messageSlot(msg::buildStatusMessage(tr("Enter Coordinate System Values").toStdString()));});
      
      selectionWidget = new SelectionWidget(parent, std::vector<SelectionWidgetCue>(1, cue));
      stackLayout = new QStackedLayout();
      stackLayout->addWidget(dummy);
      stackLayout->addWidget(selectionWidget);
      mainLayout->addLayout(stackLayout);
      mainLayout->addStretch();
      
      selectionWidget->getButton(0)->setChecked(true);
    }
    
    QWidget *parent = nullptr;
    prm::Parameter *parameter = nullptr;
    QRadioButton *byConstant = nullptr;
    QRadioButton *byFeature = nullptr;
    ParameterWidget *parameterWidget = nullptr;
    SelectionWidget *selectionWidget = nullptr;
    QStackedLayout *stackLayout = nullptr;
  };
}

using namespace dlg;

CSysWidget::CSysWidget(QWidget *parentIn, prm::Parameter *parameterIn)
: QWidget(parentIn)
, stow(std::make_unique<Stow>(this, parameterIn))
{
  connect(stow->byConstant, &QRadioButton::toggled, this, &CSysWidget::radioSlot);
  connect(stow->byFeature, &QRadioButton::toggled, this, &CSysWidget::radioSlot);
}

CSysWidget::~CSysWidget() = default;

void CSysWidget::syncLinks()
{
  stow->parameterWidget->syncLinks();
}

void CSysWidget::setCSysLinkId(const boost::uuids::uuid &idIn)
{
  if (idIn.is_nil())
  {
    stow->byConstant->setChecked(true);
    return;
  }
  //else
  slc::Message s;
  s.type = slc::Type::Feature;
  s.accrue = slc::Accrue::None;
  s.featureType = ftr::Type::Base; //we don't care what type of feature it is.
  s.featureId = idIn;
  s.shapeId = gu::createNilId();
  //don't care about point location
  //don't care about selection mask
  stow->selectionWidget->getButton(0)->setMessages(s);
  stow->byFeature->setChecked(true);
}

boost::uuids::uuid CSysWidget::getCSysLinkId() const
{
  const slc::Messages &msgs = stow->selectionWidget->getButton(0)->getMessages();
  if (!stow->byFeature->isChecked() || msgs.empty())
    return gu::createNilId();
  return msgs.front().featureId;
}

void CSysWidget::radioSlot()
{
  //gets called when states of both buttons change.
  //so we need both tests and can't use 'else'
  if (stow->byConstant->isChecked())
    stow->stackLayout->setCurrentIndex(0);
  if (stow->byFeature->isChecked())
    stow->stackLayout->setCurrentIndex(1);
}
