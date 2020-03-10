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

#include <boost/optional/optional.hpp>

#include <QSettings>
#include <QComboBox>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "message/msgmessage.h"
#include "dialogs/dlgselectionbutton.h"
#include "dialogs/dlgselectionwidget.h"
#include "tools/featuretools.h"
#include "parameter/prmparameter.h"
#include "feature/ftrdatumaxis.h"
#include "command/cmddatumaxis.h"
#include "commandview/cmvparameterwidgets.h"
#include "commandview/cmvdatumaxis.h"

using boost::uuids::uuid;

using namespace cmv;

struct DatumAxis::Stow
{
  cmd::DatumAxis *command = nullptr;
  cmv::DatumAxis *view = nullptr;
  QComboBox *combo = nullptr;
  QStackedWidget *stackedWidget = nullptr;
  dlg::SelectionWidget *pointsSelectionWidget = nullptr;
  dlg::SelectionWidget *intersectionSelectionWidget = nullptr;
  dlg::SelectionWidget *geometrySelectionWidget = nullptr;
  cmv::ParameterWidget *parameterWidget = nullptr;
  std::vector<prm::Observer> observers;
  
  Stow(cmd::DatumAxis *cIn, cmv::DatumAxis *vIn)
  : command(cIn)
  , view(vIn)
  {
    buildGui();
    
    QSettings &settings = app::instance()->getUserSettings();
    settings.beginGroup("cmv::DatumAxis");
    //load settings
    settings.endGroup();
    
    loadFeatureData();
  }
  
  void buildGui()
  {
    QSizePolicy adjust = view->sizePolicy();
    adjust.setVerticalPolicy(QSizePolicy::Maximum);
    view->setSizePolicy(adjust);
    
    QVBoxLayout *mainLayout = new QVBoxLayout();
    view->setLayout(mainLayout);
    
    combo = new QComboBox(view);
    combo->addItem(QObject::tr("Constant"));
    combo->addItem(QObject::tr("Points"));
    combo->addItem(QObject::tr("Intersection"));
    combo->addItem(QObject::tr("Geometry"));
    mainLayout->addWidget(combo);
    
    stackedWidget = new QStackedWidget(view);
    mainLayout->addWidget(stackedWidget);
    
    //constant to be empty widget
    QWidget *dummy = new QWidget(stackedWidget);
    dummy->setSizePolicy(QSizePolicy(dummy->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding));
    stackedWidget->addWidget(dummy);
    
    //points just has selection widget
    std::vector<dlg::SelectionWidgetCue> cues;
    dlg::SelectionWidgetCue cue;
    cue.name = tr("Item 1");
    cue.singleSelection = true;
    cue.mask = slc::PointsEnabled | slc::AllPointsEnabled | slc::PointsSelectable | slc::EndPointsSelectable | slc::CenterPointsSelectable;
    cue.statusPrompt = tr("Select First Item");
    cue.showAccrueColumn = false;
    cues.push_back(cue);
    cue.name = tr("Item 2");
    cue.statusPrompt = tr("Select Second Item");
    cues.push_back(cue);
    pointsSelectionWidget = new dlg::SelectionWidget(stackedWidget, cues);
    pointsSelectionWidget->setObjectName("selection");
    stackedWidget->addWidget(pointsSelectionWidget);
    
    //intersection just has selection widget. Cheat and use existing cues.
    cues.front().mask = cues.back().mask = slc::ObjectsEnabled | slc::ObjectsSelectable | slc::FacesEnabled | slc::FacesSelectable;
    intersectionSelectionWidget = new dlg::SelectionWidget(stackedWidget, cues);
    intersectionSelectionWidget->setObjectName("selection");
    stackedWidget->addWidget(intersectionSelectionWidget);
    
    //geometry just has selection widget. Cheat again.
    cues.pop_back();
    cues.front().mask = slc::FacesEnabled | slc::FacesSelectable | slc::EdgesEnabled | slc::EdgesSelectable;
    geometrySelectionWidget = new dlg::SelectionWidget(stackedWidget, cues);
    geometrySelectionWidget->setObjectName("selection");
    stackedWidget->addWidget(geometrySelectionWidget);
    
    parameterWidget = new cmv::ParameterWidget(view, command->feature->getParameters());
    mainLayout->addWidget(parameterWidget);
    for (auto *p : command->feature->getParameters())
    {
      observers.emplace_back(std::bind(&Stow::parameterChanged, this));
      p->connect(observers.back());
    }
    if (static_cast<bool>(*command->feature->getAutoSizeParameter()))
      parameterWidget->disableWidget(command->feature->getSizeParameter());
    
    mainLayout->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    
    QObject::connect(combo, SIGNAL(currentIndexChanged(int)), stackedWidget, SLOT(setCurrentIndex(int)));
    QObject::connect(stackedWidget, SIGNAL(currentChanged(int)), view, SLOT(stackedChanged(int)));
    QObject::connect(pointsSelectionWidget->getButton(0), &dlg::SelectionButton::dirty, view, &DatumAxis::pointSelectionChanged);
    QObject::connect(pointsSelectionWidget->getButton(1), &dlg::SelectionButton::dirty, view, &DatumAxis::pointSelectionChanged);
    QObject::connect(intersectionSelectionWidget->getButton(0), &dlg::SelectionButton::dirty, view, &DatumAxis::intersectionSelectionChanged);
    QObject::connect(intersectionSelectionWidget->getButton(1), &dlg::SelectionButton::dirty, view, &DatumAxis::intersectionSelectionChanged);
    QObject::connect(geometrySelectionWidget->getButton(0), &dlg::SelectionButton::dirty, view, &DatumAxis::geometrySelectionChanged);
  }
  
  void loadFeatureData()
  {
    boost::optional<slc::Message> msg0;
    boost::optional<slc::Message> msg1;
    const auto &picks = command->feature->getPicks();
    ftr::UpdatePayload up = app::instance()->getProject()->getPayload(command->feature->getId());
    tls::Resolver resolver(up);
    if (picks.size() >= 1 && resolver.resolve(picks.at(0)))
    {
      auto msgs = resolver.convertToMessages();
      if (!msgs.empty()) //what if more than one?
        msg0 = msgs.front();
    }
    if (picks.size() >= 2 && resolver.resolve(picks.at(1)))
    {
      auto msgs = resolver.convertToMessages();
      if (!msgs.empty()) //what if more than one?
        msg1 = msgs.front();
    }
    
    QSignalBlocker stackBlocker(stackedWidget);
    ftr::DatumAxis::AxisType at = command->feature->getAxisType();
    if (at == ftr::DatumAxis::AxisType::Constant)
    {
      combo->setCurrentIndex(0);
    }
    else if (at == ftr::DatumAxis::AxisType::Points)
    {
      if (msg0)
        pointsSelectionWidget->initializeButton(0, msg0.get());
      if (msg1)
        pointsSelectionWidget->initializeButton(1, msg1.get());
      combo->setCurrentIndex(1);
      activate(1);
    }
    else if (at == ftr::DatumAxis::AxisType::Intersection)
    {
      if (msg0)
        intersectionSelectionWidget->initializeButton(0, msg0.get());
      if (msg1)
        intersectionSelectionWidget->initializeButton(1, msg1.get());
      combo->setCurrentIndex(2);
      activate(2);
    }
    else if (at == ftr::DatumAxis::AxisType::Geometry)
    {
      if (msg0)
        geometrySelectionWidget->initializeButton(0, msg0.get());
      combo->setCurrentIndex(3);
      activate(3);
    }
  }
  
  void activate(int index)
  {
    if (index < 0 || index >= stackedWidget->count())
      return;
    QWidget *w = stackedWidget->widget(index);
    assert(w);
    dlg::SelectionWidget *sw = dynamic_cast<dlg::SelectionWidget*>(w);
    if (!sw)
      sw = w->findChild<dlg::SelectionWidget*>("selection");
    if (sw)
      sw->activate(0);
  }
  
  void goPoints()
  {
    std::vector<slc::Message> msgs0 = pointsSelectionWidget->getMessages(0);
    std::vector<slc::Message> msgs1 = pointsSelectionWidget->getMessages(1);
    msgs0.insert(msgs0.end(), msgs1.begin(), msgs1.end());
    if (msgs0.size() == 2)
      command->setToPoints(msgs0);
  }
  
  void goIntersection()
  {
    std::vector<slc::Message> msgs0 = intersectionSelectionWidget->getMessages(0);
    std::vector<slc::Message> msgs1 = intersectionSelectionWidget->getMessages(1);
    msgs0.insert(msgs0.end(), msgs1.begin(), msgs1.end());
    if (msgs0.size() == 2)
      command->setToIntersection(msgs0);
  }
  
  void goGeometry()
  {
    std::vector<slc::Message> msgs = geometrySelectionWidget->getMessages(0);
    if (msgs.size() == 1)
      command->setToGeometry(msgs);
  }
  
  void parameterChanged()
  {
    if (static_cast<bool>(*command->feature->getAutoSizeParameter()))
      parameterWidget->disableWidget(command->feature->getSizeParameter());
    else
      parameterWidget->enableWidget(command->feature->getSizeParameter());
      
    //lets make sure we don't trigger updates 'behind the scenes'
    if (!parameterWidget->isVisible())
      return;
    command->localUpdate();
  }
};

DatumAxis::DatumAxis(cmd::DatumAxis *cIn)
: Base("cmv::DatumAxis")
, stow(new Stow(cIn, this))
{}

DatumAxis::~DatumAxis() = default;

void DatumAxis::stackedChanged(int index)
{
  if (index < 0 || index >= stow->stackedWidget->count())
    return;
  stow->activate(index);
  
  if (index == 0) //constant
    stow->command->setToConstant();
  else if (index == 1) //points
    stow->goPoints();
  else if (index == 2) //intersection
    stow->goIntersection();
  else if (index == 3) //geometry
    stow->goGeometry();
}

void DatumAxis::pointSelectionChanged()
{
  stow->goPoints();
}

void DatumAxis::intersectionSelectionChanged()
{
  stow->goIntersection();
}

void DatumAxis::geometrySelectionChanged()
{
  stow->goGeometry();
}
