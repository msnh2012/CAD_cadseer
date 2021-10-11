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

#include <cassert>

#include <QSettings>
#include <QGridLayout>
#include <QButtonGroup>
#include <QLabel>
#include <QComboBox>
#include <QListWidget>
#include <QAbstractListModel>
#include <QListView>
#include <QTimer>
#include <QAction>
#include <QCheckBox>
#include <QStackedWidget>

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "annex/annlawfunction.h"
#include "message/msgmessage.h"
#include "message/msgnode.h"
#include "message/msgsift.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "commandview/cmvselectioncue.h"
#include "commandview/cmvtable.h"
#include "dialogs/dlgsplitterdecorated.h"
#include "tools/featuretools.h"
#include "tools/idtools.h"
#include "law/lwfcue.h"
#include "commandview/cmvlawfunctionwidget.h"
#include "feature/ftrsweep.h"
#include "command/cmdsweep.h"
#include "commandview/cmvsweep.h"

using boost::uuids::uuid;

namespace
{
  class ProfilesModel : public QAbstractListModel
  {
  protected:
    ftr::Sweep::Feature &feature;
  public:
    ProfilesModel(QObject *p, ftr::Sweep::Feature &fIn)
    : QAbstractListModel(p)
    , feature(fIn)
    {}
    
    int rowCount(const QModelIndex& = QModelIndex()) const override
    {
      return feature.getProfiles().size();
    }
    
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
      if (!index.isValid())
        return QVariant();
      if (role == Qt::DisplayRole)
        return tr("Profile");
      return QVariant();
    }
    
    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
      //works for both because we disable dropping on the master view
      if (!index.isValid())
        return Qt::NoItemFlags;
      return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }
    
    /* JFC Qt! These functions take the start and a count.
     * but the begin* functions take a start and a finish.
     * Fail!
     * 
     */
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override
    {
      if (row + count - 1 >= static_cast<int>(feature.getProfiles().size()) || row < 0 || count < 1)
        return false;
      beginRemoveRows(parent, row, row + count -1);
      feature.removeProfile(row);
      endRemoveRows();
      return true;
    }
    
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override
    {
      if (row < 0 || row > static_cast<int>(feature.getProfiles().size()) || count < 1)
        return false;
      beginInsertRows(parent, row, row + count - 1);
      feature.addProfile(); //ignore count we are just adding one at a time.
      endInsertRows();
      return true;
    }
  };
}

using namespace cmv;

struct Sweep::Stow
{
  cmd::Sweep *command = nullptr;
  cmv::Sweep *view = nullptr;
  
  ProfilesModel *profilesModel = nullptr;
  QListView *profilesList = nullptr;
  QStackedWidget *profilesStack = nullptr;
  QAction *addProfileAction = nullptr;
  QAction *removeProfileAction = nullptr;
  
  prm::Parameters parameters;
  cmv::tbl::Model *prmModel = nullptr;
  cmv::tbl::View *prmView = nullptr;
  prm::Observer useLawObserver;
  
  dlg::SplitterDecorated *profileSplitter = nullptr;
  dlg::SplitterDecorated *topSplitter = nullptr;
  LawFunctionWidget *lawWidget;
  bool isGlobalBoundarySelection = false;
  
  Stow(cmd::Sweep *cIn, cmv::Sweep *vIn)
  : command(cIn)
  , view(vIn)
  {
    parameters = command->feature->getParameters();
    buildGui();
    
    profileSplitter->restoreSettings("cmv::Sweep::profileSplitter");
    topSplitter->restoreSettings("cmv::Sweep::topSplitter");
    
    loadFeatureData();
    glue();
  }
  
  void buildGui()
  {
    QVBoxLayout *mainLayout = new QVBoxLayout();
    view->setLayout(mainLayout);
    clearContentMargins(view);
    view->setSizePolicy(view->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);
    
    addProfileAction = new QAction(tr("Add Profile"), profilesList);
    removeProfileAction = new QAction(tr("Remove Profile"), profilesList);
    profilesModel = new ProfilesModel(view, *command->feature);
    profilesList = new QListView(view);
    profilesList->setModel(profilesModel);
    profilesList->addAction(addProfileAction);
    profilesList->addAction(removeProfileAction);
    profilesList->setContextMenuPolicy(Qt::ActionsContextMenu);
    profilesStack = new QStackedWidget(view);
    profilesStack->setDisabled(true);
    profileSplitter = new dlg::SplitterDecorated(view);
    profileSplitter->setOrientation(Qt::Horizontal);
    profileSplitter->setChildrenCollapsible(false);
    profileSplitter->addWidget(profilesList);
    profileSplitter->addWidget(profilesStack);
    
    prmModel = new tbl::Model(view, command->feature, parameters);
    prmView = new tbl::View(view, prmModel, true);
    
    tbl::SelectionCue cue;
    cue.singleSelection = true;
    cue.mask = slc::None;
    cue.statusPrompt = tr("Something");
    cue.accrueEnabled = false;
    
    cue.mask = slc::ObjectsBoth | slc::WiresEnabled | slc::EdgesEnabled;
    cue.statusPrompt = tr("Select One Spine");
    prmModel->setCue(command->feature->getParameter(ftr::Sweep::PrmTags::spine), cue);
    
    cue.statusPrompt = tr("Select One Auxiliary Spine");
    prmModel->setCue(command->feature->getParameter(ftr::Sweep::PrmTags::auxPick), cue);
    
    cue.mask = slc::ShellsEnabled | slc::FacesBoth;
    cue.statusPrompt = tr("Select Support Face");
    prmModel->setCue(command->feature->getParameter(ftr::Sweep::PrmTags::support), cue);
    
    cue.mask = slc::ObjectsBoth
      | slc::FacesEnabled
      | slc::EdgesEnabled
      | slc::EndPointsEnabled
      | slc::MidPointsEnabled
      | slc::CenterPointsEnabled;
    cue.statusPrompt = tr("Select Binormal");
    prmModel->setCue(command->feature->getParameter(ftr::Sweep::PrmTags::biPicks), cue);
    
    lawWidget = new LawFunctionWidget(view);
    lawWidget->setWhatsThis(tr("Modifies The Law"));
    
    topSplitter = new dlg::SplitterDecorated(view);
    topSplitter->setOrientation(Qt::Vertical);
    topSplitter->addWidget(profileSplitter);
    topSplitter->addWidget(prmView);
    topSplitter->addWidget(lawWidget);
    mainLayout->addWidget(topSplitter);
    
    useLawChanged();
  }
  
  void slcAdded(const msg::Message &mIn)
  {
    if
    (
      view->isHidden()
      || !isGlobalBoundarySelection
      || (mIn.getSLC().type != slc::Type::Object && mIn.getSLC().type != slc::Type::Wire && mIn.getSLC().type != slc::Type::Wire)
    )
    return;
    
    closeAllPersistentEditors();
    profilesList->clearSelection();
    if (profilesModel->insertRow(static_cast<int>(command->feature->getProfiles().size())))
    {
      auto &freshProfile(command->feature->getProfiles().back());
      addProfileWidget(freshProfile);
      auto *tblView = dynamic_cast<tbl::View*>(profilesStack->widget(profilesStack->count() - 1)); assert(tblView);
      auto *tblModel =  dynamic_cast<tbl::Model*>(tblView->model()); assert(tblModel);
      tblModel->mySetData(&freshProfile.pick, {mIn.getSLC()});
      
      QTimer::singleShot
      (
        0
        , [&](){profilesList->selectionModel()->select(profilesModel->index(profilesModel->rowCount() - 1, 0), QItemSelectionModel::ClearAndSelect);}
      );
      useLawChanged();
    }
  }
  
  void addProfile()
  {
    closeAllPersistentEditors();
    profilesList->clearSelection();
    if (profilesModel->insertRow(static_cast<int>(command->feature->getProfiles().size())))
    {
      addProfileWidget(command->feature->getProfiles().back());
      QTimer::singleShot
      (
        0
        , [&](){profilesList->selectionModel()->select(profilesModel->index(profilesModel->rowCount() - 1, 0), QItemSelectionModel::ClearAndSelect);}
      );
      useLawChanged();
    }
  }
  
  void removeProfile()
  {
    closeAllPersistentEditors();
    if (profilesList->selectionModel()->hasSelection())
    {
      auto index = profilesList->selectionModel()->selectedIndexes().front();
      profilesList->selectionModel()->clearSelection();
      auto *cw = dynamic_cast<tbl::View*>(profilesStack->widget(index.row()));
      assert(cw);
      profilesModel->removeRow(index.row());
      profilesStack->removeWidget(cw);
      cw->model()->deleteLater();
      cw->deleteLater();
      goPicks();
      command->localUpdate();
      useLawChanged();
    }
  }
  
  void addProfileWidget(ftr::Sweep::Profile &pIn)
  {
    prm::Parameters prms{&pIn.pick, &pIn.contact, &pIn.correction};
    auto *lModel = new cmv::tbl::Model(profilesStack, command->feature, prms);
    auto *lView = new tbl::View(profilesStack, lModel, true);
    
    tbl::SelectionCue cue;
    cue.singleSelection = true;
    cue.mask = slc::ObjectsBoth | slc::WiresEnabled | slc::EdgesEnabled;
    cue.statusPrompt = tr("Select Profile");
    cue.accrueEnabled = false;
    lModel->setCue(&pIn.pick, cue);
    
    QObject::connect(lModel, &tbl::Model::dataChanged, view, &Sweep::profilesChanged);
    QObject::connect(lView, &tbl::View::openingPersistent, [this](){this->closeAllPersistentEditors();});
    QObject::connect(lView, &tbl::View::openingPersistent, [this](){this->stopGlobalBoundarySelection();});
    lView->installEventFilter(view);
    
    profilesStack->addWidget(lView);
  }
  
  void loadFeatureData()
  {
    const ann::LawFunction &lfa = command->feature->getAnnex<ann::LawFunction>(ann::Type::LawFunction);
    lawWidget->setCue(lfa.getCue());
    
    for (auto &p : command->feature->getProfiles())
      addProfileWidget(p);
  }
  
  void useLawChanged()
  {
    const auto *lawPrm = command->feature->getParameter(ftr::Sweep::PrmTags::useLaw);
    auto lawState = lawPrm->isActive() & lawPrm->getBool();
    if (lawState)
      lawWidget->setVisible(true);
    else
      lawWidget->setVisible(false);
    prmView->updateHideInactive();
  }
  
  void glue()
  {
    connect(prmModel, &tbl::Model::dataChanged, view, &Sweep::modelChanged);
    connect(addProfileAction, &QAction::triggered, [this](){this->addProfile();});
    connect(removeProfileAction, &QAction::triggered, [this](){this->removeProfile();});
    connect(profilesList->selectionModel(), &QItemSelectionModel::selectionChanged, view, &Sweep::profileSelectionChanged);
    connect(lawWidget, &LawFunctionWidget::lawIsDirty, view, &Sweep::lawDirtySlot);
    connect(lawWidget, &LawFunctionWidget::valueChanged, view, &Sweep::lawValueChangedSlot);
    auto *lawPrm = command->feature->getParameter(ftr::Sweep::PrmTags::useLaw);
    useLawObserver.valueHandler = std::bind(&Stow::useLawChanged, this);
    lawPrm->connect(useLawObserver);
    connect(prmView, &tbl::View::openingPersistent, [this](){this->closeAllPersistentEditors();});
    connect(prmView, &tbl::View::openingPersistent, [this](){this->stopGlobalBoundarySelection();});
    profilesList->installEventFilter(view);
    prmView->installEventFilter(view);
    
    view->sift->insert
    (
      msg::Response | msg::Post | msg::Selection | msg::Add
      , std::bind(&Stow::slcAdded, this, std::placeholders::_1)
    );
  }
  
  std::vector<slc::Messages> getProfiles()
  {
    std::vector<slc::Messages> out;
    
    for (int index = 0; index < profilesStack->count(); ++index)
    {
      auto *aView = dynamic_cast<tbl::View*>(profilesStack->widget(index)); assert(aView);
      auto *aModel = dynamic_cast<tbl::Model*>(aView->model()); assert(aModel);
      out.push_back(aModel->getMessages(aModel->index(0,0)));
    }
    
    return out;
  }
  
  void closeAllPersistentEditors()
  {
    prmView->closePersistent();
    for (int index = 0; index < profilesStack->count(); ++index)
    {
      auto *aView = dynamic_cast<tbl::View*>(profilesStack->widget(index)); assert(aView);
      aView->closePersistent();
    }
  }
  
  void goPicks()
  {
    const auto &spine = prmModel->getMessages(command->feature->getParameter(ftr::Sweep::PrmTags::spine));
    auto profiles = getProfiles();
    int type = command->feature->getParameter(ftr::Sweep::PrmTags::trihedron)->getInt();
    switch (type)
    {
      case 4: //constant bi normal.
      {
        const auto &biPicks = prmModel->getMessages(command->feature->getParameter(ftr::Sweep::PrmTags::biPicks));
        command->setBinormal(spine, profiles, biPicks);
        break;
      }
      case 5: //support
      {
        const auto &support = prmModel->getMessages(command->feature->getParameter(ftr::Sweep::PrmTags::support));
        command->setSupport(spine, profiles, support);
        break;
      }
      case 6: // auxiliary
      {
        const auto &aux = prmModel->getMessages(command->feature->getParameter(ftr::Sweep::PrmTags::auxPick));
        command->setAuxiliary(spine, profiles, aux);
        break;
      }
      default:
      {
        command->setCommon(spine, profiles);
        break;
      }
    }
  }
  
  void goUpdate()
  {
    command->localUpdate();
    prmView->updateHideInactive();
    view->node->sendBlocked(msg::buildStatusMessage(command->getStatusMessage()));
    goGlobalBoundarySelection();
  }
  
  void goGlobalBoundarySelection()
  {
    if (!isGlobalBoundarySelection)
    {
      isGlobalBoundarySelection = true;
      view->goMaskDefault(); // note: we don't remember any selection mask changes the user makes.
      view->node->sendBlocked(msg::buildStatusMessage(QObject::tr("Select New Profiles").toStdString()));
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

Sweep::Sweep(cmd::Sweep *cIn)
: Base("cmv::Sweep")
, stow(new Stow(cIn, this))
{
  maskDefault = slc::ObjectsBoth | slc::WiresEnabled | slc::EdgesEnabled;
  stow->goGlobalBoundarySelection();
}

Sweep::~Sweep() = default;

bool Sweep::eventFilter(QObject *watched, QEvent *event)
{
  //installed on child widgets so we can dismiss persistent editors
  if(event->type() == QEvent::FocusIn)
  {
    stow->closeAllPersistentEditors();
    stow->goGlobalBoundarySelection();
  }
  
  return QObject::eventFilter(watched, event);
}

void Sweep::lawDirtySlot()
{
  stow->command->feature->setLaw(stow->lawWidget->getCue());
  stow->goUpdate();
}

void Sweep::lawValueChangedSlot()
{
  //this is goofy. law cue in law widget is a copy so widget doesn't change
  //parameters in the actual feature. So we have to sync by parameter ids.
  for (const prm::Parameter *wp : stow->lawWidget->getCue().getParameters())
  {
    prm::Parameter *fp = stow->command->feature->getParameter(wp->getId());
    if (!fp)
    {
      std::cout << "WARNING: law widget cue parameter doesn't exist in feature" << std::endl;
      continue;
    }
    (*fp) = (*wp);
  }
  stow->goUpdate();
}


void Sweep::profilesChanged(const QModelIndex &index, const QModelIndex&)
{
  if (!index.isValid())
    return;
  
  stow->goPicks();
  stow->goUpdate();
}

void Sweep::profileSelectionChanged(const QItemSelection &selected, const QItemSelection&)
{
  stow->closeAllPersistentEditors();
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  if (selected.indexes().isEmpty())
  {
    stow->profilesStack->setDisabled(true);
  }
  else
  {
    stow->profilesStack->setEnabled(true);
    int row = selected.indexes().front().row();
    stow->profilesStack->setCurrentIndex(row);
    auto *tView = dynamic_cast<tbl::View*>(stow->profilesStack->widget(row)); assert(tView);
    auto *tModel = dynamic_cast<tbl::Model*>(tView->model()); assert(tModel);
    const auto &msgs = tModel->getMessages(tModel->index(0,0));
    for (const auto &m : msgs)
      node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Add, m));
  }
}

void Sweep::modelChanged(const QModelIndex &index, const QModelIndex&)
{
  if (!index.isValid())
    return;
  
  auto tag = stow->parameters.at(index.row())->getTag(); //tag of changed parameter.
  if
  (
    tag == ftr::Sweep::PrmTags::spine
    //note: profile changes come through the specialized list.
    || tag == ftr::Sweep::PrmTags::support
    || tag == ftr::Sweep::PrmTags::auxPick
    || tag == ftr::Sweep::PrmTags::support
    || tag == ftr::Sweep::PrmTags::biPicks
  )
  {
    stow->goPicks();
  }

  stow->command->localUpdate();
  stow->prmView->updateHideInactive();
  node->sendBlocked(msg::buildStatusMessage(stow->command->getStatusMessage()));
  stow->goGlobalBoundarySelection();
}
