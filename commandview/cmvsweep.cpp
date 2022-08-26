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
#include "tools/idtools.h"
#include "feature/ftrsweep.h"
#include "command/cmdsweep.h"
#include "commandview/cmvsweep.h"

using boost::uuids::uuid;
using namespace cmv;

struct Sweep::Stow
{
  cmd::Sweep *command = nullptr;
  cmv::Sweep *view = nullptr;
  
  TableList *profilesList = nullptr;
  QAction *addProfileAction = nullptr;
  QAction *removeProfileAction = nullptr;
  
  prm::Parameters parameters;
  cmv::tbl::Model *prmModel = nullptr;
  cmv::tbl::View *prmView = nullptr;
  
  dlg::SplitterDecorated *topSplitter = nullptr;
  bool isGlobalBoundarySelection = false;
  
  Stow(cmd::Sweep *cIn, cmv::Sweep *vIn)
  : command(cIn)
  , view(vIn)
  {
    parameters = command->feature->getParameters();
    buildGui();
    loadFeatureData();
    glue();
    topSplitter->restoreSettings("cmv::Sweep::topSplitter");
    
    //start into spine editing if it is empty.
    const auto &msgs = prmModel->getMessages(command->feature->getParameter(ftr::Sweep::PrmTags::spine));
    if (msgs.empty())
      QTimer::singleShot(0, [this](){this->prmView->openPersistent(prmModel->index(0, 1));});
    else
      profilesList->setSelectedDelayed();
  }
  
  void buildGui()
  {
    QVBoxLayout *mainLayout = new QVBoxLayout();
    view->setLayout(mainLayout);
    clearContentMargins(view);
    view->setSizePolicy(view->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);
    
    addProfileAction = new QAction(tr("Add Profile"), view);
    removeProfileAction = new QAction(tr("Remove Profile"), view);
    profilesList = new TableList(view, command->feature);
    profilesList->restoreSettings("cmv::Sweep::TableList");
    profilesList->getListWidget()->addAction(addProfileAction);
    profilesList->getListWidget()->addAction(removeProfileAction);
    
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
    
    topSplitter = new dlg::SplitterDecorated(view);
    topSplitter->setOrientation(Qt::Vertical);
    topSplitter->addWidget(prmView);
    topSplitter->addWidget(profilesList);
    mainLayout->addWidget(topSplitter);
  }
  
  void slcAdded(const msg::Message &mIn)
  {
    if
    (
      view->isHidden()
      || !isGlobalBoundarySelection
      || (mIn.getSLC().type != slc::Type::Object && mIn.getSLC().type != slc::Type::Wire && mIn.getSLC().type != slc::Type::Edge)
    )
    return;
    
    auto &p = addProfile();
    auto *tblModel = profilesList->getSelectedModel();
    tblModel->mySetData(&p.pick, {mIn.getSLC()});
  }
  
  ftr::Sweep::Profile& addProfile()
  {
    closeAllPersistentEditors();
    auto &freshProfile = command->feature->addProfile();
    addProfileWidget(freshProfile);
    profilesList->setSelected();
    return freshProfile;
  }
  
  void removeProfile()
  {
    closeAllPersistentEditors();
    int i = profilesList->remove();
    if (i == -1)
      return;
    command->feature->removeProfile(i);
    goPicks();
    command->localUpdate();
  }
  
  void addProfileWidget(ftr::Sweep::Profile &pIn)
  {
    prm::Parameters prms = pIn.getParameters();
    int i = profilesList->add(tr("Profile"), prms);
    auto *pModel = profilesList->getPrmModel(i); assert(pModel);
    auto *pView = profilesList->getPrmView(i); assert(pView);
    
    tbl::SelectionCue cue;
    cue.singleSelection = true;
    cue.mask = slc::ObjectsBoth | slc::WiresEnabled | slc::EdgesEnabled;
    cue.statusPrompt = tr("Select Profile");
    cue.accrueEnabled = false;
    pModel->setCue(&pIn.pick, cue);
    
    QObject::connect(pModel, &tbl::Model::dataChanged, view, &Sweep::profilesChanged);
    QObject::connect(pView, &tbl::View::openingPersistent, [this](){this->closeAllPersistentEditors();});
    QObject::connect(pView, &tbl::View::openingPersistent, [this](){this->stopGlobalBoundarySelection();});
    pView->installEventFilter(view);
  }
  
  void loadFeatureData()
  {
    for (auto &p : command->feature->getProfiles())
      addProfileWidget(p);
  }
  
  void glue()
  {
    connect(prmModel, &tbl::Model::dataChanged, view, &Sweep::modelChanged);
    connect(addProfileAction, &QAction::triggered, [this](){this->addProfile();});
    connect(removeProfileAction, &QAction::triggered, [this](){this->removeProfile();});
    connect(profilesList->getListWidget(), &QListWidget::itemSelectionChanged, view, &Sweep::profileSelectionChanged);
    connect(prmView, &tbl::View::openingPersistent, [this](){this->closeAllPersistentEditors();});
    connect(prmView, &tbl::View::openingPersistent, [this](){this->stopGlobalBoundarySelection();});
    profilesList->getListWidget()->installEventFilter(view);
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
    for (int index = 0; index < profilesList->count(); ++index)
    {
      auto *aModel = profilesList->getPrmModel(index); assert(aModel);
      out.push_back(aModel->getMessages(aModel->index(0,0)));
    }
    return out;
  }
  
  void closeAllPersistentEditors()
  {
    prmView->closePersistent();
    profilesList->closePersistent();
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
    profilesList->reselectDelayed();
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

void Sweep::profilesChanged(const QModelIndex &index, const QModelIndex&)
{
  if (!index.isValid())
    return;
  
  stow->goPicks();
  stow->goUpdate();
}

void Sweep::profileSelectionChanged()
{
  stow->closeAllPersistentEditors();
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  for (const auto &m : stow->profilesList->getSelectedMessages())
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Add, m));
}

void Sweep::modelChanged(const QModelIndex &index, const QModelIndex&)
{
  if (!index.isValid()) return;
  
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
  stow->profilesList->reselectDelayed();
}
