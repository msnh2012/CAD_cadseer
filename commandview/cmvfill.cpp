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
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QListWidget>
#include <QAction>

#include "application/appapplication.h"
#include "application/appmessage.h"
#include "project/prjproject.h"
#include "message/msgmessage.h"
#include "message/msgnode.h"
#include "message/msgsift.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "dialogs/dlgsplitterdecorated.h"
#include "commandview/cmvselectioncue.h"
#include "commandview/cmvtable.h"
#include "commandview/cmvtablelist.h"
#include "tools/featuretools.h"
#include "feature/ftrfill.h"
#include "command/cmdfill.h"
#include "commandview/cmvfill.h"

using boost::uuids::uuid;

using namespace cmv;

struct Fill::Stow
{
  cmd::Fill *command;
  cmv::Fill *view;
  prm::Parameters parameters;
  dlg::SplitterDecorated *mainSplitter = nullptr;
  dlg::SplitterDecorated *boundarySplitter = nullptr;
  cmv::tbl::Model *prmModel = nullptr;
  cmv::tbl::View *prmView = nullptr;
  TableList *tableList = nullptr;
  QAction *addBoundaryAction = nullptr;
  QAction *removeBoundaryAction = nullptr;
  bool isGlobalBoundarySelection = true;
  
  Stow(cmd::Fill *cIn, cmv::Fill *vIn)
  : command(cIn)
  , view(vIn)
  {
    //we can't use all parameters because they will contain the parameters of the boundaries
    //that we want to handle separately.
    parameters.push_back(command->feature->getParameter(ftr::Fill::PrmTags::initialPick));
    parameters.push_back(command->feature->getParameter(ftr::Fill::PrmTags::internalPicks));
    buildGui();
    loadFeatureData();
    glue();
    boundarySplitter->restoreSettings(QString::fromUtf8("Fill::BoundarySplitter"));
    mainSplitter->restoreSettings(QString::fromUtf8("Fill::MainSplitter"));
    tableList->setSelectedDelayed();
  }
  
  void buildGui()
  {
    QVBoxLayout *mainLayout = new QVBoxLayout();
    view->setLayout(mainLayout);
    clearContentMargins(view);
    view->setSizePolicy(view->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);
    
    mainSplitter = new dlg::SplitterDecorated(view);
    mainSplitter->setOrientation(Qt::Vertical);
    mainLayout->addWidget(mainSplitter);
    
    prmModel = new tbl::Model(view, command->feature, parameters);
    prmView = new tbl::View(view, prmModel, true);
    {
      tbl::SelectionCue cue;
      cue.singleSelection = true;
      cue.mask = slc::FacesBoth;
      cue.statusPrompt = tr("Select Initial Face");
      cue.accrueEnabled = false;
      prmModel->setCue(parameters.at(0), cue);
    }
    {
      tbl::SelectionCue cue;
      cue.singleSelection = false;
      cue.mask = slc::EdgesBoth | slc::AllPointsEnabled;
      cue.statusPrompt = tr("Select Internal Constraints");
      cue.accrueEnabled = false;
      prmModel->setCue(parameters.at(1), cue);
    }
    mainSplitter->addWidget(prmView);
    
    boundarySplitter = new dlg::SplitterDecorated(view);
    boundarySplitter->setOrientation(Qt::Horizontal);
    mainSplitter->addWidget(boundarySplitter);
    
    tableList = new TableList(view, command->feature);
    tableList->restoreSettings("cmv::Fill::TableList");
    
    addBoundaryAction = new QAction(tr("Add Boundary"), tableList->getListWidget());
    removeBoundaryAction = new QAction(tr("Remove Boundary"), tableList->getListWidget());
    tableList->getListWidget()->addActions({addBoundaryAction, removeBoundaryAction});
    boundarySplitter->addWidget(tableList);
  }
  
  int addBoundary(ftr::Fill::Boundary &bIn)
  {
    prm::Parameters prms = {&bIn.edgePick, &bIn.facePick, &bIn.continuity};
    int i = tableList->add(tr("Boundary"), prms);
    connect(tableList->getPrmModel(i), &tbl::Model::dataChanged, view, &Fill::boundaryModelChanged);
    connect(tableList->getPrmView(i), &tbl::View::openingPersistent, view, &Fill::openingPersistentEditor);
    {
      tbl::SelectionCue cue;
      cue.singleSelection = true;
      cue.mask = slc::EdgesBoth;
      cue.statusPrompt = tr("Select Edge For Boundary");
      cue.accrueEnabled = false;
      tableList->getPrmModel(i)->setCue(prms.at(0), cue);
    }
    {
      tbl::SelectionCue cue;
      cue.singleSelection = true;
      cue.mask = slc::FacesBoth;
      cue.statusPrompt = tr("Select Face For Constraint");
      cue.accrueEnabled = false;
      tableList->getPrmModel(i)->setCue(prms.at(1), cue);
    }
    return i;
  }
  
  void loadFeatureData()
  {
    for (auto &b : command->feature->getBoundaries())
      addBoundary(b);
  }
  
  void glue()
  {
    connect(prmModel, &tbl::Model::dataChanged, view, &Fill::modelChanged);
    connect (addBoundaryAction, &QAction::triggered, view, &Fill::boundaryAdded);
    connect (removeBoundaryAction, &QAction::triggered, view, &Fill::boundaryRemoved);
    
    connect (tableList->getListWidget(), &QListWidget::itemSelectionChanged, view, &Fill::boundarySelectionChanged);
    connect (prmView, &tbl::View::openingPersistent, view, &Fill::openingPersistentEditor);
    
    view->sift->insert
    (
      msg::Response | msg::Post | msg::Selection | msg::Add
      , std::bind(&Stow::slcAdded, this, std::placeholders::_1)
    );
    
    prmView->installEventFilter(view);
    tableList->getListWidget()->installEventFilter(view);
  }
  
  void goGlobalBoundarySelection()
  {
    isGlobalBoundarySelection = true;
    view->goMaskDefault();
    view->goSelectionToolbar();
    view->node->sendBlocked(msg::buildStatusMessage(command->getStatusMessage()));
  }
  
  void stopGlobalBoundarySelection()
  {
    isGlobalBoundarySelection = false;
    view->node->sendBlocked(msg::buildSelectionMask(slc::None));
  }
  
  void slcAdded(const msg::Message &mIn)
  {
    if
    (
      view->isHidden()
      || !isGlobalBoundarySelection
      || (mIn.getSLC().type != slc::Type::Face && mIn.getSLC().type != slc::Type::Edge)
    )
      return;
    
    auto &b = command->feature->addBoundary();
    int i = addBoundary(b);
    tableList->setSelected(i);
    auto *tblModel =  tableList->getPrmModel(i); assert(tblModel);
    if (mIn.getSLC().type == slc::Type::Face)
      tblModel->mySetData(&b.facePick, {mIn.getSLC()});
    else
      tblModel->mySetData(&b.edgePick, {mIn.getSLC()});
  }
  
  void goUpdate()
  {
    cmd::Fill::Data data;
    //initial face and internal constraints.
    data.emplace_back(prmModel->getMessages(parameters.at(0)), prmModel->getMessages(parameters.at(1)));
    for (int bIndex = 0; bIndex < tableList->count(); ++bIndex)
    {
      auto *tblModel = tableList->getPrmModel(bIndex); assert(tblModel);
      data.emplace_back(tblModel->getMessages(tblModel->index(0, 0)), tblModel->getMessages(tblModel->index(1, 0)));
    }
    command->setSelections(data);
    command->localUpdate();
    goGlobalBoundarySelection();
    tableList->reselectDelayed();
  }
};

Fill::Fill(cmd::Fill *cIn)
: Base("cmv::Fill")
, stow(new Stow(cIn, this))
{
  maskDefault = slc::FacesEnabled | slc::EdgesBoth;
  stow->goGlobalBoundarySelection();
}

Fill::~Fill() = default;

bool Fill::eventFilter(QObject *watched, QEvent *event)
{
  //installed on child widgets so we can dismiss persistent editors
  if(event->type() == QEvent::FocusIn)
  {
    stow->prmView->closePersistent();
    stow->tableList->closePersistent();
    stow->goGlobalBoundarySelection();
  }
  
  return QObject::eventFilter(watched, event);
}

void Fill::boundaryAdded()
{
  stow->addBoundary(stow->command->feature->addBoundary());
  //no update, because a new empty boundary shouldn't change the feature.
}

void Fill::boundaryRemoved()
{
  int bIndex = stow->tableList->remove();
  if (bIndex == -1)
    return;
  stow->command->feature->removeBoundary(bIndex);
  stow->goUpdate();
}

void Fill::boundarySelectionChanged()
{
  //close any persistent editors for initial and internal
  stow->prmView->closePersistent();
  stow->tableList->closePersistent();
  stow->goGlobalBoundarySelection();
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  for (const auto &m : stow->tableList->getSelectedMessages())
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Add, m));
}

void Fill::modelChanged(const QModelIndex &index, const QModelIndex&)
{
  if (!index.isValid())
    return;
  stow->goUpdate();
  node->sendBlocked(msg::buildStatusMessage(stow->command->getStatusMessage()));
}

void Fill::boundaryModelChanged(const QModelIndex &index, const QModelIndex&)
{
  if (!index.isValid())
    return;
  if (index.row() == 2) //continuity, simple update, no pick changes.
  {
    stow->command->localUpdate();
    stow->tableList->reselectDelayed();
    return;
  }
  stow->goUpdate();
  /* pick parameters don't get updated via the model, so we have to do the update
   * then we can updateHideInactive.
   */
  if (index.row() == 1) //faces, show or hide continuity parameter.
    stow->tableList->updateHideInactive();
}

void Fill::openingPersistentEditor()
{
  //close any open editors
  stow->prmView->closePersistent();
  stow->tableList->closePersistent();
  stow->stopGlobalBoundarySelection();
}
