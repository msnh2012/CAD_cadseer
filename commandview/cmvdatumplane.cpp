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
#include "message/msgnode.h"
#include "dialogs/dlgselectionbutton.h"
#include "dialogs/dlgselectionwidget.h"
#include "commandview/cmvparameterwidgets.h"
#include "commandview/cmvcsyswidget.h"
#include "commandview/cmvselectioncue.h"
#include "commandview/cmvtable.h"
#include "parameter/prmconstants.h"
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
  prm::Parameters parameters;
  QStackedWidget *stackedWidget = nullptr;
  dlg::SelectionWidget *planarOffsetSelection = nullptr;
  dlg::SelectionWidget *planarCenterSelection = nullptr;
  dlg::SelectionWidget *axisAngleSelection = nullptr;
  dlg::SelectionWidget *average3PlaneSelection = nullptr;
  dlg::SelectionWidget *through3PointsSelection = nullptr;
  cmv::ParameterWidget *parameterWidget = nullptr;
  cmv::CSysWidget *csysWidget = nullptr;
  cmv::tbl::Model *prmModel = nullptr;
  cmv::tbl::View *prmView = nullptr;
  
  Stow(cmd::DatumPlane *cIn, cmv::DatumPlane *vIn)
  : command(cIn)
  , view(vIn)
  {
    buildGui();
    
    /*
    QSettings &settings = app::instance()->getUserSettings();
    settings.beginGroup("cmv::DatumPlane");
    //load settings
    settings.endGroup();
    
    loadFeatureData();
    */
    glue();
  }
  
  void buildGui()
  {
    QVBoxLayout *mainLayout = new QVBoxLayout();
    view->setLayout(mainLayout);
    Base::clearContentMargins(view);
    view->setSizePolicy(view->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);
    
    //this cue for offset, so kind of temp
    tbl::SelectionCue cue;
    cue.singleSelection = true;
    cue.mask = slc::ObjectsEnabled | slc::FacesEnabled | slc::ObjectsSelectable | slc::FacesSelectable;
    cue.statusPrompt = tr("Select Planar Face Or Datum Plane");
    cue.accrueEnabled = false;
    
    parameters = command->feature->getParameters();
    prmModel = new tbl::Model(view, command->feature);
    prmModel->setCue(command->feature->getParameters(prm::Tags::Picks).front(), cue);
    prmView = new tbl::View(view, prmModel, true);
    mainLayout->addWidget(prmView);
    
    //probably temp. move to glue.
//     connect(prmView, &tbl::View::prmValueChanged, view, &DatumPlane::parameterChanged);
    
    /*
    //remove hack param so doesn't show in widget
    auto hackPrms = command->feature->getParameters(ftr::DatumPlane::csysLinkHack);
    assert(hackPrms.size() == 1);
    auto theEnd = std::remove(allParams.begin(), allParams.end(), hackPrms.front());
    allParams.erase(theEnd, allParams.end());
    
    parameterWidget = new cmv::ParameterWidget(view, allParams);
    
    //set prm mode onto csysWidget.
    auto csysPrms = command->feature->getParameters(prm::Tags::CSys);
    assert(csysPrms.size() == 1);
    csysWidget = dynamic_cast<CSysWidget*>(parameterWidget->getWidget(csysPrms.front()));
    assert(csysWidget);
    csysWidget->setPrmMode(hackPrms.front());
    
    mainLayout->addWidget(parameterWidget);
    
    stackedWidget = new QStackedWidget(view);
    mainLayout->addWidget(stackedWidget);
    
    //constant to be empty widget
    QWidget *dummy = new QWidget(stackedWidget);
    dummy->setSizePolicy(QSizePolicy(dummy->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding));
    stackedWidget->addWidget(dummy);
    
    //Link to be empty widget for now.
    QWidget *dummy2 = new QWidget(stackedWidget);
    dummy2->setSizePolicy(QSizePolicy(dummy2->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding));
    stackedWidget->addWidget(dummy2);
    
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
    */
  }
  
  void loadFeatureData()
  {
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
    if (dpt == ftr::DatumPlane::DPType::Link)
    {
      const auto &pl = view->project->getPayload(command->feature->getId());
      auto features = pl.getFeatures(ftr::InputType::linkCSys);
      if (features.empty())
      {
        csysWidget->setCSysLinkId(gu::createNilId());
        view->node->sendBlocked(msg::buildStatusMessage("Couldn't find linked csys feature"));
      }
      else
        csysWidget->setCSysLinkId(features.front()->getId());
    }
    else if (dpt == ftr::DatumPlane::DPType::POffset)
    {
      auto msgs = goResolve(ftr::InputType::create);
      if (!msgs.empty()) //what if more than one?
        planarOffsetSelection->initializeButton(0, msgs.front());
    }
    else if (dpt == ftr::DatumPlane::DPType::PCenter)
    {
      auto msgs0 = goResolve(ftr::InputType::createIndexedTag(ftr::DatumPlane::center, 0));
      if (!msgs0.empty()) //what if more than one?
        planarCenterSelection->initializeButton(0, msgs0.front());
      auto msgs1 = goResolve(ftr::InputType::createIndexedTag(ftr::DatumPlane::center, 1));
      if (!msgs1.empty()) //what if more than one?
        planarCenterSelection->initializeButton(1, msgs1.front());
    }
    else if (dpt == ftr::DatumPlane::DPType::AAngleP)
    {
      auto msgsAxis = goResolve(ftr::DatumPlane::axis);
      if (!msgsAxis.empty()) //what if more than one?
        axisAngleSelection->initializeButton(0, msgsAxis.front());
      auto msgsPlane = goResolve(ftr::DatumPlane::plane);
      if (!msgsPlane.empty()) //what if more than one?
        axisAngleSelection->initializeButton(1, msgsPlane.front());
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
    }
    int dptInt = static_cast<int>(dpt);
    stackedWidget->setCurrentIndex(dptInt);
    activate(dptInt); //activate will handle it, if no selection widget for index.
  }
  
  void glue()
  {
//     connect(planarOffsetSelection->getButton(0), &dlg::SelectionButton::dirty, view, &DatumPlane::planarOffsetSelectionChanged);
//     connect(planarCenterSelection->getButton(0), &dlg::SelectionButton::dirty, view, &DatumPlane::planarCenterSelectionChanged);
//     connect(planarCenterSelection->getButton(1), &dlg::SelectionButton::dirty, view, &DatumPlane::planarCenterSelectionChanged);
//     connect(axisAngleSelection->getButton(0), &dlg::SelectionButton::dirty, view, &DatumPlane::axisAngleSelectionChanged);
//     connect(axisAngleSelection->getButton(1), &dlg::SelectionButton::dirty, view, &DatumPlane::axisAngleSelectionChanged);
//     connect(average3PlaneSelection->getButton(0), &dlg::SelectionButton::dirty, view, &DatumPlane::average3PlaneSelectionChanged);
//     connect(average3PlaneSelection->getButton(1), &dlg::SelectionButton::dirty, view, &DatumPlane::average3PlaneSelectionChanged);
//     connect(average3PlaneSelection->getButton(2), &dlg::SelectionButton::dirty, view, &DatumPlane::average3PlaneSelectionChanged);
//     connect(through3PointsSelection->getButton(0), &dlg::SelectionButton::dirty, view, &DatumPlane::through3PointsSelectionChanged);
//     connect(through3PointsSelection->getButton(1), &dlg::SelectionButton::dirty, view, &DatumPlane::through3PointsSelectionChanged);
//     connect(through3PointsSelection->getButton(2), &dlg::SelectionButton::dirty, view, &DatumPlane::through3PointsSelectionChanged);
//     connect(parameterWidget, &ParameterBase::prmValueChanged, view, &DatumPlane::parameterChanged);
//     connect(csysWidget, &CSysWidget::dirty, view, &DatumPlane::linkCSysChanged);
    
    connect(prmModel, &tbl::Model::dataChanged, view, &DatumPlane::modelChanged);
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
  
  void goLinked()
  {
    assert(csysWidget);
    auto id = csysWidget->getCSysLinkId();
    command->setLinked(id);
    command->localUpdate();
  }
  
  void goPlanarOffset()
  {
    std::vector<slc::Message> msgs0 = planarOffsetSelection->getMessages(0);
    command->setToPlanarOffset(msgs0);
    command->localUpdate();
  }
  
  void goPlanarCenter()
  {
    std::vector<slc::Message> msgs0 = planarCenterSelection->getMessages(0);
    std::vector<slc::Message> msgs1 = planarCenterSelection->getMessages(1);
    msgs0.insert(msgs0.end(), msgs1.begin(), msgs1.end());
    command->setToPlanarCenter(msgs0);
    command->localUpdate();
  }
  
  void goAxisAngle()
  {
    std::vector<slc::Message> msgs0 = axisAngleSelection->getMessages(0);
    std::vector<slc::Message> msgs1 = axisAngleSelection->getMessages(1);
    msgs0.insert(msgs0.end(), msgs1.begin(), msgs1.end());
    command->setToAxisAngle(msgs0);
    command->localUpdate();
  }
  
  void goAverage3Plane()
  {
    std::vector<slc::Message> msgs0 = average3PlaneSelection->getMessages(0);
    std::vector<slc::Message> msgs1 = average3PlaneSelection->getMessages(1);
    std::vector<slc::Message> msgs2 = average3PlaneSelection->getMessages(2);
    msgs0.insert(msgs0.end(), msgs1.begin(), msgs1.end());
    msgs0.insert(msgs0.end(), msgs2.begin(), msgs2.end());
    command->setToAverage3Plane(msgs0);
    command->localUpdate();
  }
  
  void goThrough3Points()
  {
    std::vector<slc::Message> msgs0 = through3PointsSelection->getMessages(0);
    std::vector<slc::Message> msgs1 = through3PointsSelection->getMessages(1);
    std::vector<slc::Message> msgs2 = through3PointsSelection->getMessages(2);
    msgs0.insert(msgs0.end(), msgs1.begin(), msgs1.end());
    msgs0.insert(msgs0.end(), msgs2.begin(), msgs2.end());
    command->setToThrough3Points(msgs0);
    command->localUpdate();
  }
};

DatumPlane::DatumPlane(cmd::DatumPlane *cIn)
: Base("cmv::DatumPlane")
, stow(new Stow(cIn, this))
{}

DatumPlane::~DatumPlane() = default;

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
  auto prmsType = stow->command->feature->getParameters(ftr::DatumPlane::datumPlaneType);
  assert(!prmsType.empty());
  auto *prmType = prmsType.front();
  int typeInt = prmType->getInt();
  /*
  assert(typeInt >= 0 && typeInt < stow->stackedWidget->count());
  
  if (stow->stackedWidget->currentIndex() == typeInt)
  {
    //minor parameter change.
    stow->command->localUpdate();
    return;
  }
  
  //major type change.
  stow->stackedWidget->setCurrentIndex(typeInt);
  stow->activate(typeInt);
  */
  
  using DPType = ftr::DatumPlane::DPType;
  switch (static_cast<DPType>(typeInt))
  {
    case DPType::Constant:
    {
      stow->command->setToConstant();
      stow->command->localUpdate();
      break;
    }
    case DPType::Link:
    {
      stow->goLinked();
      break;
    }
    case DPType::POffset:
    {
      stow->goPlanarOffset();
      break;
    }
    case DPType::PCenter:
    {
      stow->goPlanarCenter();
      break;
    }
    case DPType::AAngleP:
    {
      stow->goAxisAngle();
      break;
    }
    case DPType::Average3P:
    {
      stow->goAverage3Plane();
      break;
    }
    case DPType::Through3P:
    {
      stow->goThrough3Points();
      break;
    }
    default:
    {
      assert(0); //unrecognized DatumPlane type.
      break;
    }
  }
}

void DatumPlane::linkCSysChanged()
{
  stow->command->feature->setModelDirty();
  stow->goLinked();
}

void DatumPlane::modelChanged(const QModelIndex &index, const QModelIndex&)
{
  if (!index.isValid())
    return;
  
  prm::Parameter *prm = stow->parameters.at(index.row());
  std::cout << "Prm: " << prm->getName().toStdString() << " changed" << std::endl;
  
  if (prm->getValueType() == typeid(ftr::Picks))
  {
    if (stow->command->feature->getDPType() == ftr::DatumPlane::DPType::POffset)
      stow->command->setToPlanarOffset(stow->prmModel->getMessages(index));
  }
  stow->command->localUpdate();
}
