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
#include "dialogs/dlgselectionbutton.h"
#include "dialogs/dlgselectionwidget.h"
#include "commandview/cmvparameterwidgets.h"
#include "parameter/prmparameter.h"
#include "tools/featuretools.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrdatumplane.h"
#include "command/cmddatumplane.h"
#include "commandview/cmvdatumplane.h"

using boost::uuids::uuid;

using namespace cmv;

struct DatumPlane::Stow
{
  cmd::DatumPlane *command;
  cmv::DatumPlane *view;
  QComboBox *combo = nullptr;
  QStackedWidget *stackedWidget = nullptr;
  dlg::SelectionWidget *planarOffsetSelection = nullptr;
  dlg::SelectionWidget *planarCenterSelection = nullptr;
  dlg::SelectionWidget *axisAngleSelection = nullptr;
  dlg::SelectionWidget *average3PlaneSelection = nullptr;
  dlg::SelectionWidget *through3PointsSelection = nullptr;
  cmv::ParameterWidget *parameterWidget = nullptr;
  
  Stow(cmd::DatumPlane *cIn, cmv::DatumPlane *vIn)
  : command(cIn)
  , view(vIn)
  {
    buildGui();
    
    QSettings &settings = app::instance()->getUserSettings();
    settings.beginGroup("cmv::DatumPlane");
    //load settings
    settings.endGroup();
    
    loadFeatureData();
    glue();
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
    combo->addItem(QObject::tr("Planar Offset"));
    combo->addItem(QObject::tr("Planar Center"));
    combo->addItem(QObject::tr("Axis Angle"));
    combo->addItem(QObject::tr("3 Plane Average"));
    combo->addItem(QObject::tr("Through 3 Points"));
    mainLayout->addWidget(combo);
    
    parameterWidget = new cmv::ParameterWidget(view, command->feature->getParameters());
    mainLayout->addWidget(parameterWidget);
    
    stackedWidget = new QStackedWidget(view);
    mainLayout->addWidget(stackedWidget);
    
    //constant to be empty widget
    QWidget *dummy = new QWidget(stackedWidget);
    dummy->setSizePolicy(QSizePolicy(dummy->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding));
    stackedWidget->addWidget(dummy);
    
    //planar offset
    {
      std::vector<dlg::SelectionWidgetCue> cues;
      dlg::SelectionWidgetCue cue;
      cue.name = tr("Face/Datum 1");
      cue.singleSelection = true;
      cue.mask = slc::ObjectsEnabled | slc::FacesEnabled | slc::ObjectsSelectable | slc::FacesSelectable;
      cue.statusPrompt = tr("Select Planar Face Or Datum Plane");
      cue.showAccrueColumn = false;
      cues.push_back(cue);
      planarOffsetSelection = new dlg::SelectionWidget(stackedWidget, cues);
      planarOffsetSelection->setObjectName("selection");
      stackedWidget->addWidget(planarOffsetSelection);
    }
    
    //planar Center
    {
      std::vector<dlg::SelectionWidgetCue> cues;
      dlg::SelectionWidgetCue cue;
      cue.name = tr("Face/Datum 1");
      cue.singleSelection = true;
      cue.mask = slc::ObjectsEnabled | slc::FacesEnabled | slc::ObjectsSelectable | slc::FacesSelectable;
      cue.statusPrompt = tr("Select Planar Face Or Datum Plane");
      cue.showAccrueColumn = false;
      cues.push_back(cue);
      cues.push_back(cue);
      cues.back().name = tr("Face/Datum 2");
      planarCenterSelection = new dlg::SelectionWidget(stackedWidget, cues);
      planarCenterSelection->setObjectName("selection");
      stackedWidget->addWidget(planarCenterSelection);
    }
    
    //axis angle
    {
      std::vector<dlg::SelectionWidgetCue> cues;
      dlg::SelectionWidgetCue cue;
      cue.name = tr("Axis");
      cue.singleSelection = true;
      cue.mask = slc::ObjectsEnabled | slc::FacesEnabled | slc::EdgesEnabled | slc::ObjectsSelectable | slc::FacesSelectable | slc::EdgesSelectable;
      cue.statusPrompt = tr("Select Datum Axis, Face Or Edge");
      cue.showAccrueColumn = false;
      cues.push_back(cue);
      cues.push_back(cue);
      cues.back().name = tr("Plane");
      cues.back().mask = slc::ObjectsEnabled | slc::FacesEnabled | slc::ObjectsSelectable | slc::FacesSelectable;
      cues.back().statusPrompt = tr("Select Datum Plane Or Planar Face");
      axisAngleSelection = new dlg::SelectionWidget(stackedWidget, cues);
      axisAngleSelection->setObjectName("selection");
      stackedWidget->addWidget(axisAngleSelection);
    }
    
    //average 3 planes
    {
      std::vector<dlg::SelectionWidgetCue> cues;
      dlg::SelectionWidgetCue cue;
      cue.name = tr("Plane 1");
      cue.singleSelection = true;
      cue.mask = slc::ObjectsEnabled | slc::FacesEnabled | slc::ObjectsSelectable | slc::FacesSelectable;
      cue.statusPrompt = tr("Select First Datum Plane Or Face");
      cue.showAccrueColumn = false;
      cues.push_back(cue);
      cues.push_back(cue);
      cues.back().name = tr("Plane 2");
      cues.back().statusPrompt = tr("Select Second Datum Plane Or Face");
      cues.push_back(cue);
      cues.back().name = tr("Plane 3");
      cues.back().statusPrompt = tr("Select Third Datum Plane Or Face");
      average3PlaneSelection = new dlg::SelectionWidget(stackedWidget, cues);
      average3PlaneSelection->setObjectName("selection");
      stackedWidget->addWidget(average3PlaneSelection);
    }
    
    //through 3 points
    {
      std::vector<dlg::SelectionWidgetCue> cues;
      dlg::SelectionWidgetCue cue;
      cue.name = tr("Point 1");
      cue.singleSelection = true;
      cue.mask = slc::PointsEnabled | slc::AllPointsEnabled | slc::PointsSelectable | slc::EndPointsSelectable | slc::CenterPointsSelectable;
      cue.statusPrompt = tr("Select First Point");
      cue.showAccrueColumn = false;
      cues.push_back(cue);
      cues.push_back(cue);
      cues.back().name = tr("Point 2");
      cues.back().statusPrompt = tr("Select Second Point");
      cues.push_back(cue);
      cues.back().name = tr("Point 3");
      cues.back().statusPrompt = tr("Select Third Point");
      through3PointsSelection = new dlg::SelectionWidget(stackedWidget, cues);
      through3PointsSelection->setObjectName("selection");
      stackedWidget->addWidget(through3PointsSelection);
    }
  }
  
  void loadFeatureData()
  {
    QSignalBlocker stackBlocker(stackedWidget);
    
    const ftr::Picks &picks = command->feature->getPicks();
    auto pickFromTag = [&](const std::string &tagIn) -> boost::optional<const ftr::Pick&>
    {
      for (const auto &p : picks)
      {
        if (p.tag == tagIn)
          return p;
      }
      return boost::none;
    };
    ftr::UpdatePayload up = app::instance()->getProject()->getPayload(command->feature->getId());
    tls::Resolver resolver(up);
    auto goResolve = [&](const std::string &tagIn) -> slc::Messages
    {
      auto op = pickFromTag(tagIn);
      if (op && resolver.resolve(op.get()))
        return resolver.convertToMessages();
      return slc::Messages();
    };
    
    ftr::DatumPlane::DPType dpt = command->feature->getDPType();
    if (dpt == ftr::DatumPlane::DPType::Constant)
      combo->setCurrentIndex(0);
    else if (dpt == ftr::DatumPlane::DPType::POffset)
    {
      auto msgs = goResolve(ftr::InputType::create);
      if (!msgs.empty()) //what if more than one?
        planarOffsetSelection->initializeButton(0, msgs.front());
      combo->setCurrentIndex(1);
      stackedWidget->setCurrentIndex(1);
      activate(1);
    }
    else if (dpt == ftr::DatumPlane::DPType::PCenter)
    {
      auto msgs0 = goResolve(ftr::InputType::createIndexedTag(ftr::DatumPlane::center, 0));
      if (!msgs0.empty()) //what if more than one?
        planarCenterSelection->initializeButton(0, msgs0.front());
      auto msgs1 = goResolve(ftr::InputType::createIndexedTag(ftr::DatumPlane::center, 1));
      if (!msgs1.empty()) //what if more than one?
        planarCenterSelection->initializeButton(1, msgs1.front());
      combo->setCurrentIndex(2);
      stackedWidget->setCurrentIndex(2);
      activate(2);
    }
    else if (dpt == ftr::DatumPlane::DPType::AAngleP)
    {
      auto msgsAxis = goResolve(ftr::DatumPlane::axis);
      if (!msgsAxis.empty()) //what if more than one?
        axisAngleSelection->initializeButton(0, msgsAxis.front());
      auto msgsPlane = goResolve(ftr::DatumPlane::plane);
      if (!msgsPlane.empty()) //what if more than one?
        axisAngleSelection->initializeButton(1, msgsPlane.front());
      combo->setCurrentIndex(3);
      stackedWidget->setCurrentIndex(3);
      activate(3);
    }
    else if (dpt == ftr::DatumPlane::DPType::Average3P)
    {
      auto msgs0 = goResolve(ftr::InputType::createIndexedTag(ftr::DatumPlane::plane, 0));
      if (!msgs0.empty()) //what if more than one?
        average3PlaneSelection->initializeButton(0, msgs0.front());
      auto msgs1 = goResolve(ftr::InputType::createIndexedTag(ftr::DatumPlane::plane, 1));
      if (!msgs1.empty()) //what if more than one?
        average3PlaneSelection->initializeButton(1, msgs1.front());
      auto msgs2 = goResolve(ftr::InputType::createIndexedTag(ftr::DatumPlane::plane, 2));
      if (!msgs2.empty()) //what if more than one?
        average3PlaneSelection->initializeButton(2, msgs2.front());
      combo->setCurrentIndex(4);
      stackedWidget->setCurrentIndex(4);
      activate(4);
    }
    else if (dpt == ftr::DatumPlane::DPType::Through3P)
    {
      auto msgs0 = goResolve(ftr::InputType::createIndexedTag(ftr::DatumPlane::point, 0));
      if (!msgs0.empty()) //what if more than one?
        through3PointsSelection->initializeButton(0, msgs0.front());
      auto msgs1 = goResolve(ftr::InputType::createIndexedTag(ftr::DatumPlane::point, 1));
      if (!msgs1.empty()) //what if more than one?
        through3PointsSelection->initializeButton(1, msgs1.front());
      auto msgs2 = goResolve(ftr::InputType::createIndexedTag(ftr::DatumPlane::point, 2));
      if (!msgs2.empty()) //what if more than one?
        through3PointsSelection->initializeButton(2, msgs2.front());
      combo->setCurrentIndex(5);
      stackedWidget->setCurrentIndex(5);
      activate(5);
    }
  }
  
  void glue()
  {
    connect(combo, SIGNAL(currentIndexChanged(int)), stackedWidget, SLOT(setCurrentIndex(int)));
    connect(stackedWidget, SIGNAL(currentChanged(int)), view, SLOT(stackedChanged(int)));
    connect(planarOffsetSelection->getButton(0), &dlg::SelectionButton::dirty, view, &DatumPlane::planarOffsetSelectionChanged);
    connect(planarCenterSelection->getButton(0), &dlg::SelectionButton::dirty, view, &DatumPlane::planarCenterSelectionChanged);
    connect(planarCenterSelection->getButton(1), &dlg::SelectionButton::dirty, view, &DatumPlane::planarCenterSelectionChanged);
    connect(axisAngleSelection->getButton(0), &dlg::SelectionButton::dirty, view, &DatumPlane::axisAngleSelectionChanged);
    connect(axisAngleSelection->getButton(1), &dlg::SelectionButton::dirty, view, &DatumPlane::axisAngleSelectionChanged);
    connect(average3PlaneSelection->getButton(0), &dlg::SelectionButton::dirty, view, &DatumPlane::average3PlaneSelectionChanged);
    connect(average3PlaneSelection->getButton(1), &dlg::SelectionButton::dirty, view, &DatumPlane::average3PlaneSelectionChanged);
    connect(average3PlaneSelection->getButton(2), &dlg::SelectionButton::dirty, view, &DatumPlane::average3PlaneSelectionChanged);
    connect(through3PointsSelection->getButton(0), &dlg::SelectionButton::dirty, view, &DatumPlane::through3PointsSelectionChanged);
    connect(through3PointsSelection->getButton(1), &dlg::SelectionButton::dirty, view, &DatumPlane::through3PointsSelectionChanged);
    connect(through3PointsSelection->getButton(2), &dlg::SelectionButton::dirty, view, &DatumPlane::through3PointsSelectionChanged);
    connect(parameterWidget, &ParameterBase::prmValueChanged, view, &DatumPlane::parameterChanged);
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
  
  void goPlanarOffset()
  {
    std::vector<slc::Message> msgs0 = planarOffsetSelection->getMessages(0);
    if (msgs0.size() == 1)
    {
      command->setToPlanarOffset(msgs0);
      command->localUpdate();
    }
  }
  
  void goPlanarCenter()
  {
    std::vector<slc::Message> msgs0 = planarCenterSelection->getMessages(0);
    std::vector<slc::Message> msgs1 = planarCenterSelection->getMessages(1);
    msgs0.insert(msgs0.end(), msgs1.begin(), msgs1.end());
    if (msgs0.size() == 2)
    {
      command->setToPlanarCenter(msgs0);
      command->localUpdate();
    }
  }
  
  void goAxisAngle()
  {
    std::vector<slc::Message> msgs0 = axisAngleSelection->getMessages(0);
    std::vector<slc::Message> msgs1 = axisAngleSelection->getMessages(1);
    msgs0.insert(msgs0.end(), msgs1.begin(), msgs1.end());
    if (msgs0.size() == 2)
    {
      command->setToAxisAngle(msgs0);
      command->localUpdate();
    }
  }
  
  void goAverage3Plane()
  {
    std::vector<slc::Message> msgs0 = average3PlaneSelection->getMessages(0);
    std::vector<slc::Message> msgs1 = average3PlaneSelection->getMessages(1);
    std::vector<slc::Message> msgs2 = average3PlaneSelection->getMessages(2);
    msgs0.insert(msgs0.end(), msgs1.begin(), msgs1.end());
    msgs0.insert(msgs0.end(), msgs2.begin(), msgs2.end());
    if (msgs0.size() == 3)
    {
      command->setToAverage3Plane(msgs0);
      command->localUpdate();
    }
  }
  
  void goThrough3Points()
  {
    std::vector<slc::Message> msgs0 = through3PointsSelection->getMessages(0);
    std::vector<slc::Message> msgs1 = through3PointsSelection->getMessages(1);
    std::vector<slc::Message> msgs2 = through3PointsSelection->getMessages(2);
    msgs0.insert(msgs0.end(), msgs1.begin(), msgs1.end());
    msgs0.insert(msgs0.end(), msgs2.begin(), msgs2.end());
    if (msgs0.size() == 3)
    {
      command->setToThrough3Points(msgs0);
      command->localUpdate();
    }
  }
};

DatumPlane::DatumPlane(cmd::DatumPlane *cIn)
: Base("cmv::DatumPlane")
, stow(new Stow(cIn, this))
{}

DatumPlane::~DatumPlane() = default;

void DatumPlane::stackedChanged(int index)
{
  if (index < 0 || index >= stow->stackedWidget->count())
    return;
  stow->activate(index);
  
  if (index == 0) //constant
  {
    stow->command->setToConstant();
    stow->command->localUpdate();
  }
  else if (index == 1)
    stow->goPlanarOffset();
  else if (index == 2)
    stow->goPlanarCenter();
  else if (index == 3)
    stow->goAxisAngle();
  else if (index == 4)
    stow->goAverage3Plane();
  else if (index == 5)
    stow->goThrough3Points();
}

void DatumPlane::planarOffsetSelectionChanged()
{
  stow->goPlanarOffset();
}

void DatumPlane::planarCenterSelectionChanged()
{
  stow->goPlanarCenter();
}

void DatumPlane::axisAngleSelectionChanged()
{
  stow->goAxisAngle();
}

void DatumPlane::average3PlaneSelectionChanged()
{
  stow->goAverage3Plane();
}

void DatumPlane::through3PointsSelectionChanged()
{
  stow->goThrough3Points();
}

void DatumPlane::parameterChanged()
{
  stow->command->localUpdate();
}
