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
  QListWidget *boundaryList = nullptr;
  QStackedWidget *boundaryStack = nullptr;
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
    boundaryStack->setEnabled(false); //controlled by list selection.
    goGlobalBoundarySelection();
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
      cue.mask = slc::EdgesBoth | slc::PointsBoth | slc::AllPointsEnabled;
      cue.statusPrompt = tr("Select Internal Constraints");
      cue.accrueEnabled = false;
      prmModel->setCue(parameters.at(1), cue);
    }
    mainSplitter->addWidget(prmView);
    
    boundarySplitter = new dlg::SplitterDecorated(view);
    boundarySplitter->setOrientation(Qt::Horizontal);
    mainSplitter->addWidget(boundarySplitter);
    
    boundaryList = new QListWidget(view);
    boundaryList->setSelectionMode(QAbstractItemView::SingleSelection);
    addBoundaryAction = new QAction(tr("Add Boundary"), boundaryList);
    removeBoundaryAction = new QAction(tr("Remove Boundary"), boundaryList);
    boundaryList->addActions({addBoundaryAction, removeBoundaryAction});
    boundaryList->setContextMenuPolicy(Qt::ActionsContextMenu);
    boundarySplitter->addWidget(boundaryList);
    
    boundaryStack = new QStackedWidget(view);
    boundaryStack->setSizePolicy(boundaryStack->sizePolicy().horizontalPolicy(), QSizePolicy::Maximum);
    boundarySplitter->addWidget(boundaryStack);
  }
  
  void addBoundary(ftr::Fill::Boundary &bIn)
  {
    prm::Parameters prms = {&bIn.edgePick, &bIn.facePick, &bIn.continuity};
    auto *tblModel = new tbl::Model(view, command->feature, prms);
    auto *tblView = new tbl::View(view, tblModel, true);
    connect(tblModel, &tbl::Model::dataChanged, view, &Fill::boundaryModelChanged);
    connect(tblView, &tbl::View::openingPersistent, view, &Fill::openingPersistentEditor);
    boundaryList->addItem(tr("Boundary"));
    boundaryStack->addWidget(tblView);
    {
      tbl::SelectionCue cue;
      cue.singleSelection = true;
      cue.mask = slc::EdgesBoth;
      cue.statusPrompt = tr("Select Edge For Boundary");
      cue.accrueEnabled = false;
      tblModel->setCue(prms.at(0), cue);
    }
    {
      tbl::SelectionCue cue;
      cue.singleSelection = true;
      cue.mask = slc::FacesBoth;
      cue.statusPrompt = tr("Select Face For Constraint");
      cue.accrueEnabled = false;
      tblModel->setCue(prms.at(1), cue);
    }
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
    
    connect (boundaryList->selectionModel(), &QItemSelectionModel::selectionChanged, view, &Fill::boundarySelectionChanged);
    connect (prmView, &tbl::View::openingPersistent, view, &Fill::openingPersistentEditor);
    
    view->sift->insert
    (
      msg::Response | msg::Post | msg::Selection | msg::Add
      , std::bind(&Stow::slcAdded, this, std::placeholders::_1)
    );
  }
  
  void goGlobalBoundarySelection()
  {
    isGlobalBoundarySelection = true;
    app::instance()->queuedMessage(msg::buildSelectionMask(slc::FacesEnabled | slc::EdgesBoth));
    // note: we don't remember any selection mask changes the user makes.
    //set toolbar to selection tab
    app::Message am;
    am.toolbar = 0;
    app::instance()->messageSlot(msg::Message(msg::Request | msg::Toolbar | msg::Show, am));
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
    addBoundary(b);
    auto *tblView = dynamic_cast<tbl::View*>(boundaryStack->widget(boundaryStack->count() - 1));
    assert(tblView);
    auto *tblModel =  dynamic_cast<tbl::Model*>(tblView->model());
    assert(tblModel);
    
    if (mIn.getSLC().type == slc::Type::Face)
      tblModel->setMessages(&b.facePick, {mIn.getSLC()});
    else
      tblModel->setMessages(&b.edgePick, {mIn.getSLC()});
    auto li = boundaryList->model()->index(boundaryList->count() - 1, 0);
    boundaryList->selectionModel()->select(li, QItemSelectionModel::ClearAndSelect);
  }
  
  void goUpdate()
  {
    cmd::Fill::Data data;
    //initial face and internal constraints.
    data.emplace_back(prmModel->getMessages(parameters.at(0)), prmModel->getMessages(parameters.at(1)));
    for (int bIndex = 0; bIndex < boundaryStack->count(); ++bIndex)
    {
      auto *tblView = dynamic_cast<tbl::View*>(boundaryStack->widget(bIndex)); assert(tblView);
      auto *tblModel = dynamic_cast<tbl::Model*>(tblView->model()); assert(tblModel);
      data.emplace_back(tblModel->getMessages(tblModel->index(0, 0)), tblModel->getMessages(tblModel->index(1, 0)));
    }
    command->setSelections(data);
    command->localUpdate();
    goGlobalBoundarySelection();
  }
};

Fill::Fill(cmd::Fill *cIn)
: Base("cmv::Fill")
, stow(new Stow(cIn, this))
{}

Fill::~Fill() = default;

void Fill::boundaryAdded()
{
  stow->addBoundary(stow->command->feature->addBoundary());
  //no update, because a new empty boundary shouldn't change the feature.
}

void Fill::boundaryRemoved()
{
  auto rows = stow->boundaryList->selectionModel()->selectedRows();
  if (rows.empty())
    return;
  
  int bIndex = rows.front().row();
  assert(bIndex >= 0 && bIndex < static_cast<int>(stow->command->feature->getBoundaries().size()));
  
  stow->boundaryList->clearSelection();
  delete stow->boundaryList->item(bIndex);
  auto *w = stow->boundaryStack->widget(bIndex);
  stow->boundaryStack->removeWidget(w);
  w->deleteLater();
  
  stow->command->feature->removeBoundary(bIndex);
  
  stow->goUpdate();
}

void Fill::boundarySelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
  //close any persistent editors for initial and internal
  stow->prmView->closePersistent();
  
  //close any persistent editors on deselected.
  for (const auto &i : deselected.indexes())
  {
    auto *t = dynamic_cast<tbl::View*>(stow->boundaryStack->widget(i.row()));
    assert(t);
    t->closePersistent();
  }
  
  if (selected.indexes().isEmpty())
  {
    stow->boundaryStack->setEnabled(false);
    stow->goGlobalBoundarySelection();
    return;
  }
  stow->boundaryStack->setEnabled(true);
  stow->boundaryStack->setCurrentIndex(selected.indexes().front().row());
  stow->goGlobalBoundarySelection();
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
    return;
  }
  stow->goUpdate();
  /* pick parameters don't get updated via the model, so we have to do the update
   * then we can updateHideInactive.
   */
  if (index.row() == 1) //faces, show or hide continuity parameter.
  {
    auto *tblView = dynamic_cast<tbl::View*>(stow->boundaryStack->currentWidget());
    assert(tblView);
    tblView->updateHideInactive();
  }
}

void Fill::openingPersistentEditor()
{
  //close any open editors
  stow->prmView->closePersistent();
  
  for (int i = 0; i < stow->boundaryStack->count(); ++i)
  {
    auto *t = dynamic_cast<tbl::View*>(stow->boundaryStack->widget(i));
    assert(t);
    t->closePersistent();
  }
  
  stow->stopGlobalBoundarySelection();
}
