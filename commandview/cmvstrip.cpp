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
#include <QLabel>
#include <QVBoxLayout>
#include <QListWidget>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QDragLeaveEvent>

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "dialogs/dlgselectionbutton.h"
#include "dialogs/dlgselectionwidget.h"
#include "commandview/cmvparameterwidgets.h"
#include "parameter/prmparameter.h"
#include "tools/idtools.h"
#include "feature/ftrstrip.h"
#include "command/cmdstrip.h"
#include "commandview/cmvstrip.h"

namespace
{
    /*! This is a simple QLabel with a picture
   * to absorb drags.
   */
  class TrashCan : public QLabel
  {
  public:
    TrashCan() = delete;
    TrashCan(QWidget *parent) : QLabel(parent)
    {
      setMinimumSize(128, 128);
      setFrameStyle(QFrame::Sunken | QFrame::StyledPanel);
      setAlignment(Qt::AlignCenter);
      setAcceptDrops(true);
      setAutoFillBackground(true);
      
      setPixmap(QPixmap(":resources/images/editRemove.svg").scaled(128, 128, Qt::KeepAspectRatio));
    }
  protected:
    void dragEnterEvent(QDragEnterEvent *event) override
    {
      setBackgroundRole(QPalette::Highlight);
      event->acceptProposedAction();
    }
    void dragMoveEvent(QDragMoveEvent *event) override
    {
      event->acceptProposedAction();
    }
    void dragLeaveEvent(QDragLeaveEvent *event) override
    {
      setBackgroundRole(QPalette::Dark);
      event->accept();
    }
    void dropEvent(QDropEvent *event) override
    {
      setBackgroundRole(QPalette::Dark);
      event->acceptProposedAction();
    }
  };
}

using boost::uuids::uuid;
using namespace cmv;

struct Strip::Stow
{
  cmd::Strip *command;
  cmv::Strip *view;
  dlg::SelectionWidget *selectionWidget = nullptr;
  cmv::ParameterWidget *parameterWidget = nullptr;
  std::vector<prm::Observer> observers;
  QListWidget *stationsList = nullptr;
  
  Stow(cmd::Strip *cIn, cmv::Strip *vIn)
  : command(cIn)
  , view(vIn)
  {
    buildGui();
    
    QSettings &settings = app::instance()->getUserSettings();
    settings.beginGroup("cmv::Strip");
    //load settings
    settings.endGroup();
    
    loadFeatureData();
    glue();
    selectionWidget->activate(0);
  }
  
  void buildGui()
  {
    QSizePolicy adjust = view->sizePolicy();
    adjust.setVerticalPolicy(QSizePolicy::Expanding);
    view->setSizePolicy(adjust);
    
    QVBoxLayout *mainLayout = new QVBoxLayout();
    view->setLayout(mainLayout);
    
    std::vector<dlg::SelectionWidgetCue> cues;
    dlg::SelectionWidgetCue cue;
    cue.name = QObject::tr("Part");
    cue.singleSelection = true;
    cue.mask = slc::ObjectsEnabled | slc::ObjectsSelectable;
    cue.statusPrompt = tr("Select Part Object");
    cue.showAccrueColumn = false;
    cues.push_back(cue);
    cue.name = QObject::tr("Blank");
    cue.statusPrompt = tr("Select Blank Object");
    cues.push_back(cue);
    cue.name = QObject::tr("Nest");
    cue.statusPrompt = tr("Select Nest Object");
    cues.push_back(cue);
    selectionWidget = new dlg::SelectionWidget(view, cues);
    mainLayout->addWidget(selectionWidget);
    
    parameterWidget = new cmv::ParameterWidget(view, command->feature->getParameters());
    mainLayout->addWidget(parameterWidget);
    for (auto *p : command->feature->getParameters())
    {
      observers.emplace_back(std::bind(&Stow::parameterChanged, this));
      p->connect(observers.back());
    }
    
    mainLayout->addWidget(buildStationWidget());
    
//     mainLayout->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
  }
  
  
  QWidget* buildStationWidget()
  {
    QWidget *out = new QWidget(view);
    out->setContentsMargins(0, 0, 0, 0);
    
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setContentsMargins(0, 0, 0, 0);
    out->setLayout(mainLayout);
    
    QHBoxLayout *hl = new QHBoxLayout();
    mainLayout->addLayout(hl);
//     hl->setContentsMargins(0, 0, 0, 0);
    
    QSizePolicy adjust = view->sizePolicy();
    adjust.setVerticalPolicy(QSizePolicy::Expanding);
    out->setSizePolicy(adjust);
    
    QVBoxLayout *vl = new QVBoxLayout();
    hl->addLayout(vl);
    
    stationsList = new QListWidget(out);
    stationsList->setSelectionMode(QAbstractItemView::SingleSelection);
    stationsList->setDragEnabled(true);
    stationsList->setAcceptDrops(true);
    stationsList->setDropIndicatorShown(true);
    stationsList->setDragDropMode(QAbstractItemView::DragDrop);
    stationsList->setDefaultDropAction(Qt::MoveAction);
    stationsList->setSizePolicy(adjust);
    vl->addWidget(stationsList);
    
    TrashCan *trash = new TrashCan(out);
    vl->addWidget(trash);
    
    QListWidget *protoList = new QListWidget(out);
    protoList->addItem(tr("Aerial Cam"));
    protoList->addItem(tr("Blank"));
    protoList->addItem(tr("Cam"));
    protoList->addItem(tr("Coin"));
    protoList->addItem(tr("Cutoff"));
    protoList->addItem(tr("Draw"));
    protoList->addItem(tr("Flange"));
    protoList->addItem(tr("Form"));
    protoList->addItem(tr("Idle"));
    protoList->addItem(tr("Pierce"));
    protoList->addItem(tr("Restrike"));
    protoList->addItem(tr("Tip"));
    protoList->addItem(tr("Trim"));
    hl->addWidget(protoList);
    protoList->setSelectionMode(QAbstractItemView::SingleSelection);
    protoList->setDragEnabled(true);
    protoList->setDragDropMode(QAbstractItemView::DragOnly);
    protoList->setSizePolicy(adjust);
    
    return out;
  }
  
  void loadFeatureData()
  {
    for (const auto &s : command->feature->stations)
      stationsList->addItem(s);
    
    auto payload = view->project->getPayload(command->feature->getId());
    
    auto buildMessage = [](const std::vector<const ftr::Base*> &parents) -> slc::Message
    {
      slc::Message mOut;
      mOut.type = slc::Type::Object;
      mOut.featureType = parents.front()->getType();
      mOut.featureId = parents.front()->getId();
      return mOut;
    };
    
    {
      auto partParents = payload.getFeatures(ftr::Strip::part);
      if (!partParents.empty())
      {
        slc::Message mOut = buildMessage(partParents);
        selectionWidget->initializeButton(0, mOut);
      }
    }
    
    {
      auto blankParents = payload.getFeatures(ftr::Strip::blank);
      if (!blankParents.empty())
      {
        slc::Message mOut = buildMessage(blankParents);
        selectionWidget->initializeButton(1, mOut);
      }
    }
    
    {
      auto nestParents = payload.getFeatures(ftr::Strip::nest);
      if (!nestParents.empty())
      {
        slc::Message mOut = buildMessage(nestParents);
        selectionWidget->initializeButton(2, mOut);
      }
    }
  }
  
  void glue()
  {
    QObject::connect(selectionWidget->getButton(0), &dlg::SelectionButton::dirty, view, &Strip::selectionChanged);
    QObject::connect(selectionWidget->getButton(1), &dlg::SelectionButton::dirty, view, &Strip::selectionChanged);
    QObject::connect(selectionWidget->getButton(2), &dlg::SelectionButton::dirty, view, &Strip::selectionChanged);
    QObject::connect(stationsList->model(), SIGNAL(rowsRemoved(const QModelIndex&, int, int)), view, SLOT(stationsChanged(const QModelIndex&, int, int)));
    QObject::connect(stationsList->model(), SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&, const QVector<int>&)), view, SLOT(dataChanged(const QModelIndex&, const QModelIndex&, const QVector<int>&)));
  }
  
  void parameterChanged()
  {
    if (!parameterWidget->isVisible())
      return;
    goUpdate();
  }
  
  void goUpdate()
  {
    //Break cycle.
    std::vector<std::unique_ptr<prm::ObserverBlocker>> blockers;
    for (auto &o : observers)
      blockers.push_back(std::make_unique<prm::ObserverBlocker>(o));
    command->localUpdate();
  }
};

Strip::Strip(cmd::Strip *cIn)
: Base("cmv::Strip")
, stow(new Stow(cIn, this))
{}

Strip::~Strip() = default;

void Strip::selectionChanged()
{
  auto *sw = stow->selectionWidget;
  stow->command->setSelections(sw->getMessages(0), sw->getMessages(1), sw->getMessages(2));
  stow->goUpdate();
}

void Strip::stationsChanged(const QModelIndex&, int, int)
{
  eyeroll();
}

void Strip::dataChanged(const QModelIndex&, const QModelIndex&, const QVector<int>&)
{
  eyeroll();
}

void Strip::eyeroll()
{
  //couldn't find a single signal from the listwidget model that would trigger for adding a station or removing a station.
  //dataChanged does trigger when removing.
  //rowsRemoved and rowsInserted has the same signature so could connect to same slot.
  //however rowsInserted happens before the data is set and we get an empty string for the inserted row.
  stow->command->feature->stations.clear();
  for (int i = 0; i < stow->stationsList->count(); ++i)
    stow->command->feature->stations.push_back(stow->stationsList->item(i)->text());
  //ftr::Strip::stations is NOT a parameter so setting it doesn't dirty the feature. So do it.
  stow->command->feature->setModelDirty();
  stow->goUpdate();
}
