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
#include <QComboBox>
#include <QAction>
#include <QStackedWidget>
#include <QListWidget>
#include <QVBoxLayout>
#include <QTimer>

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "message/msgmessage.h"
#include "message/msgnode.h"
#include "message/msgsift.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "commandview/cmvselectioncue.h"
#include "commandview/cmvtable.h"
#include "commandview/cmvtablelist.h"
#include "dialogs/dlgsplitterdecorated.h"
#include "tools/featuretools.h"
#include "feature/ftrchamfer.h"
#include "command/cmdchamfer.h"
#include "commandview/cmvchamfer.h"

using boost::uuids::uuid;

using namespace cmv;

struct Chamfer::Stow
{
  cmd::Chamfer *command;
  cmv::Chamfer *view;
  
  prm::Parameters parameters;
  tbl::Model *prmModel = nullptr;
  tbl::View *prmView = nullptr;
  dlg::SplitterDecorated *mainSplitter = nullptr;
  TableList *tableList = nullptr;
  
  QAction *addSymmetric = nullptr;
  QAction *addTwoDistances = nullptr;
  QAction *addDistanceAngle = nullptr;
  QAction *separator = nullptr;
  QAction *remove = nullptr;
  
  bool isGlobalBoundarySelection = false;
  
  Stow(cmd::Chamfer *cIn, cmv::Chamfer *vIn)
  : command(cIn)
  , view(vIn)
  {
    parameters = command->feature->getParameters();
    buildGui();
    
    loadFeatureData();
    glue();
    modeChanged();
    tableList->setSelectedDelayed();
  }
  
  void buildGui()
  {
    QVBoxLayout *mainLayout = new QVBoxLayout();
    view->setLayout(mainLayout);
    clearContentMargins(view);
    view->setSizePolicy(view->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);
    
    prmModel = new tbl::Model(view, command->feature, parameters);
    prmView = new tbl::View(view, prmModel, true);
    
    addSymmetric = new QAction(tr("Add Symmetric"), view);
    addTwoDistances = new QAction(tr("Add Two Distances"), view);
    addDistanceAngle = new QAction(tr("Add Distance Angle"), view);
    separator = new QAction(view);
    separator->setSeparator(true);
    remove = new QAction(tr("Remove"), view);
    
    tableList = new TableList(view, command->feature);
    tableList->restoreSettings("cmv::Chamfer::TableList");
    tableList->getListWidget()->addAction(addSymmetric);
    tableList->getListWidget()->addAction(addTwoDistances);
    tableList->getListWidget()->addAction(addDistanceAngle);
    tableList->getListWidget()->addAction(separator);
    tableList->getListWidget()->addAction(remove);
    
    mainSplitter = new dlg::SplitterDecorated(view);
    mainSplitter->setOrientation(Qt::Vertical);
    mainSplitter->setChildrenCollapsible(false);
    mainSplitter->addWidget(prmView);
    mainSplitter->addWidget(tableList);
    mainSplitter->restoreSettings("cmv::Chamfer::mainSplitter");
    mainLayout->addWidget(mainSplitter);
  }
  
  void loadFeatureData()
  {
    for (auto &e : command->feature->getEntries())
      appendEntry(e);
  }
  
  void slcAdded(const msg::Message &mIn)
  {
    if
    (
      view->isHidden()
      || !isGlobalBoundarySelection
      || (mIn.getSLC().type != slc::Type::Edge)
    )
      return;
      
    stopGlobalBoundarySelection();
    tableList->getListWidget()->clearSelection();
    appendDefault(mIn.getSLC());
  }
  
  void glue()
  {
    connect(prmModel, &tbl::Model::dataChanged, view, &Chamfer::modelChanged);
    connect(prmView, &tbl::View::openingPersistent, view, &Chamfer::closeAllPersistent);
    connect(prmView, &tbl::View::openingPersistent, [this](){this->stopGlobalBoundarySelection();});
    
    connect(tableList->getListWidget(), &QListWidget::itemSelectionChanged, view, &Chamfer::entrySelectionChanged);
    
    connect(addSymmetric, &QAction::triggered, view, &Chamfer::appendSymmetricSlot);
    connect(addTwoDistances, &QAction::triggered, view, &Chamfer::appendTwoDistancesSlot);
    connect(addDistanceAngle, &QAction::triggered, view, &Chamfer::appendDistanceAngleSlot);
    connect(remove, &QAction::triggered, view, &Chamfer::removeSlot);
    
    view->sift->insert
    (
      msg::Response | msg::Post | msg::Selection | msg::Add
      , std::bind(&Stow::slcAdded, this, std::placeholders::_1)
    );
    
    prmView->installEventFilter(view);
    tableList->getListWidget()->installEventFilter(view);
  }
  
  void modeChanged()
  {
    int mode = command->feature->getParameter(ftr::Chamfer::PrmTags::mode)->getInt();
    
    switch (mode)
    {
      case 0: //classic
      {
        addSymmetric->setEnabled(true);
        addTwoDistances->setEnabled(true);
        addDistanceAngle->setEnabled(true);
        break;
      }
      case 1: //throat
      {
        addSymmetric->setEnabled(true);
        addTwoDistances->setEnabled(false);
        addDistanceAngle->setEnabled(false);
        break;
      }
      case 2: //throat penetration
      {
        addSymmetric->setEnabled(false);
        addTwoDistances->setEnabled(true);
        addDistanceAngle->setEnabled(false);
        break;
      }
      default:
      {
        assert(0); //unknown chamfer mode.
        break;
      }
    }
    
    /* the feature changes the styles to match the mode. That should happen before
     * this. Now we should just have to update visibility of active widgets and
     * rename the strings in list view.
     */
    tableList->updateHideInactive();
    updateListNames();
  }
  
  /* different modes only accept certain styles. so we call here to append
   * a style that is the default for the current mode.
   */
  void appendDefault(const slc::Message &mIn)
  {
    
    auto setEdge = [&]()
    {
      auto *tm = tableList->getSelectedModel();
      if (tm)
        tm->mySetData(tm->getParameter(tm->index(1,0)), {mIn});
    };
    
    int currentMode = command->feature->getParameter(ftr::Chamfer::PrmTags::mode)->getInt();
    switch (currentMode)
    {
      case 0: //classic -> fall through
      case 1: //throat
      {
        appendEntry(command->feature->addSymmetric());
        tableList->setSelected();
        setEdge();
        auto *tv = tableList->getSelectedView();
        if (tv)
          QTimer::singleShot(0, [tv](){tv->openPersistent(tv->model()->index(1, 1));});
        break;
      }
      case 2: //throat penetration
      {
        appendEntry(command->feature->addTwoDistances());
        tableList->setSelected();
        setEdge();
        auto *tv = tableList->getSelectedView();
        if (tv)
          tv->openPersistentEditor(tv->model()->index(2, 1));
        break;
      }
      default: //error
      {
        assert(0); // unknown chamfer mode
        break;
      }
    }
  }
  
  void appendEntry(ftr::Chamfer::Entry &eIn)
  {
    auto tempPrms = eIn.getParameters();
    int index = tableList->add(tempPrms.front()->adaptToQString(), tempPrms);
    auto *pModel = tableList->getPrmModel(index); assert(pModel);
    auto *pView = tableList->getPrmView(index); assert(pView);
    {
      tbl::SelectionCue cue;
      cue.singleSelection = false;
      cue.mask = slc::EdgesBoth;
      cue.statusPrompt = tr("Select Edge For Chamfer");
      cue.accrueEnabled = false;
      pModel->setCue(tempPrms.at(1), cue);
    }
    {
      tbl::SelectionCue cue;
      cue.singleSelection = false;
      cue.mask = slc::FacesBoth;
      cue.statusPrompt = tr("Select Reference Face");
      cue.accrueEnabled = false;
      pModel->setCue(tempPrms.at(2), cue);
    }
    
    connect(pModel, &tbl::Model::dataChanged, view, &Chamfer::entryModelChanged);
    connect(pView, &tbl::View::openingPersistent, view, &Chamfer::closeAllPersistent);
    connect(pView, &tbl::View::openingPersistent, [this](){this->stopGlobalBoundarySelection();});
    pView->installEventFilter(view);
  }
  
  void removeEntry()
  {
    int ri = tableList->remove();
    if (ri == -1)
      return;
    command->feature->removeEntry(ri);
  }
  
  void updateListNames()
  {
    auto *lw = tableList->getListWidget();
    for (int index = 0; index < tableList->count(); ++index)
    {
      auto *m = tableList->getPrmModel(index);
      auto *prm = m->getParameter(m->index(0, 1));
      lw->item(index)->setText(prm->adaptToQString());
    }
  }
  
  void setSelection()
  {
    cmd::Chamfer::SelectionData data;
    for (int index = 0; index < tableList->count(); ++index)
    {
      auto *model = tableList->getPrmModel(index); assert(model);
      const auto &ePicks = model->getMessages(model->index(1, 0));
      const auto &fPicks = model->getMessages(model->index(2, 0));
      data.emplace_back(ePicks, fPicks);
    }
    command->setSelectionData(data);
  }
  
  void goUpdate()
  {
    command->localUpdate();
    goGlobalBoundarySelection();
    //not sure why I have to delay the following call.
    QTimer::singleShot(0, [this](){this->view->entrySelectionChanged();});
  }
  
  void goGlobalBoundarySelection()
  {
    if (!isGlobalBoundarySelection)
    {
      isGlobalBoundarySelection = true;
      view->goMaskDefault(); // note: we don't remember any selection mask changes the user makes.
      view->node->sendBlocked(msg::buildStatusMessage(command->getStatusMessage()));
      view->goSelectionToolbar();
    }
  }
  
  void stopGlobalBoundarySelection()
  {
    if (isGlobalBoundarySelection)
    {
      isGlobalBoundarySelection = false;
      view->node->sendBlocked(msg::buildSelectionMask(slc::None));
    }
  }
};

Chamfer::Chamfer(cmd::Chamfer *cIn)
: Base("cmv::Chamfer")
, stow(new Stow(cIn, this))
{
  maskDefault = slc::EdgesBoth;
  stow->goGlobalBoundarySelection();
}

Chamfer::~Chamfer() = default;

bool Chamfer::eventFilter(QObject *watched, QEvent *event)
{
  //installed on child widgets so we can dismiss persistent editors
  if(event->type() == QEvent::FocusIn)
  {
    closeAllPersistent();
    stow->goGlobalBoundarySelection();
  }
  
  return QObject::eventFilter(watched, event);
}

void Chamfer::appendSymmetricSlot()
{
  stow->appendEntry(stow->command->feature->addSymmetric());
  stow->tableList->setSelected();
}

void Chamfer::appendTwoDistancesSlot()
{
  stow->appendEntry(stow->command->feature->addTwoDistances());
  stow->tableList->setSelected();
}

void Chamfer::appendDistanceAngleSlot()
{
  stow->appendEntry(stow->command->feature->addDistanceAngle());
  stow->tableList->setSelected();
}

void Chamfer::removeSlot()
{
  stow->removeEntry();
  stow->setSelection();
  stow->goUpdate();
}

void Chamfer::closeAllPersistent()
{
  stow->prmView->closePersistent();
  stow->tableList->closePersistent();
}

void Chamfer::modelChanged(const QModelIndex &index, const QModelIndex&)
{
  if (!index.isValid())
    return;
  
  /* we only have the mode parameter in the main parameter array,
   * so changing that is the only way we get here
   */
  stow->modeChanged();
  stow->goUpdate();
}

void Chamfer::entryModelChanged(const QModelIndex &index, const QModelIndex&)
{
  if (!index.isValid())
    return;
  
  const auto *eModel = dynamic_cast<const tbl::Model*>(index.model()); assert(eModel);
  auto tag = eModel->getParameter(index)->getTag();
  if (tag == ftr::Chamfer::PrmTags::edgePicks || tag == ftr::Chamfer::PrmTags::facePicks)
    stow->setSelection();
  if (tag == ftr::Chamfer::PrmTags::style)
  {
    stow->updateListNames();
    stow->tableList->updateHideInactive();
    stow->setSelection();
  }
  stow->goUpdate();
}

void Chamfer::entrySelectionChanged()
{
  /* We are able to highlight the entry geometry without turning
   * off the global selection because of the use of 'sendBlocked'.
   * It stops the selection messages from coming in.
   */
  closeAllPersistent();
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  for (const auto &m : stow->tableList->getSelectedMessages())
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Add, m));
}
