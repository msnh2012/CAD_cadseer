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
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "parameter/prmparameter.h"
#include "commandview/cmvselectioncue.h"
#include "commandview/cmvtable.h"
#include "tools/featuretools.h"
#include "parameter/prmparameter.h"
#include "dialogs/dlgnetgenparameters.h"
#include "dialogs/dlgocctparameters.h"
#include "feature/ftrsurfacemesh.h"
#include "command/cmdsurfacemesh.h"
#include "commandview/cmvsurfacemesh.h"

using boost::uuids::uuid;

using namespace cmv;

struct SurfaceMesh::Stow
{
  cmd::SurfaceMesh *command;
  cmv::SurfaceMesh *view;
  ftr::SurfaceMesh::Feature *feature;
  prm::Parameters parameters;
  tbl::Model *prmModel = nullptr;
  tbl::View *prmView = nullptr;
  
  QStackedWidget *parameterStacked = nullptr;
  dlg::OCCTParameters *occtPage = nullptr;
  dlg::NetgenParameters *netgenPage = nullptr;
  QWidget *gmshPage = nullptr;
  
  Stow(cmd::SurfaceMesh *cIn, cmv::SurfaceMesh *vIn)
  : command(cIn)
  , view(vIn)
  , feature(cIn->feature)
  {
    parameters = feature->getParameters();
    buildGui();
    syncStacked();
    glue();
  }
  
  void buildGui()
  {
    QVBoxLayout *mainLayout = new QVBoxLayout();
    view->setLayout(mainLayout);
    clearContentMargins(view);
    view->setSizePolicy(view->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);
    
    prmModel = new tbl::Model(view, command->feature, parameters);
    tbl::SelectionCue cue;
    cue.singleSelection = true;
    cue.mask = slc::ObjectsBoth;
    cue.statusPrompt = tr("Select Feature To Mesh");
    cue.accrueEnabled = false;
    prmModel->setCue(parameters.at(2), cue);
    prmView = new tbl::View(view, prmModel, true);
    mainLayout->addWidget(prmView);
    
    parameterStacked = new QStackedWidget(view);
    parameterStacked->setObjectName(QString::fromUtf8("parameterStacked"));
    
    QWidget *inertPage = new QWidget(parameterStacked); //empty
    inertPage->setObjectName(QString::fromUtf8("inertPage"));
    parameterStacked->addWidget(inertPage);
    
    occtPage = new dlg::OCCTParameters(parameterStacked);
    occtPage->setObjectName(QString::fromUtf8("occtPage"));
    clearContentMargins(occtPage);
    parameterStacked->addWidget(occtPage);
    
#ifdef NETGEN_PRESENT
    netgenPage = new dlg::NetgenParameters(parameterStacked);
    netgenPage->setObjectName(QString::fromUtf8("netgenPage"));
    clearContentMargins(netgenPage);
    parameterStacked->addWidget(netgenPage);
#else
    parameterStacked->addWidget(new QLabel(tr("Netgen Not Present"), parameterStacked));
#endif
    
#ifdef GMSH_PRESENT
    gmshPage = new QWidget(parameterStacked); //TODO
    gmshPage->setObjectName(QString::fromUtf8("gmshPage"));
    clearContentMargins(gmshPage);
    parameterStacked->addWidget(gmshPage);
#else
    parameterStacked->addWidget(new QLabel(tr("Gmsh Not Present"), parameterStacked));
#endif
    
    mainLayout->addWidget(parameterStacked);
  }
  
  void syncStacked()
  {
    int t = feature->getParameter(ftr::SurfaceMesh::Tags::MeshType)->getInt();
    assert(t >= 0 && t < parameterStacked->count());
    parameterStacked->setCurrentIndex(t);
  }
  
  void glue()
  {
    connect(prmModel, &tbl::Model::dataChanged, view, &SurfaceMesh::modelChanged);
    connect(prmView, &tbl::View::openingPersistent, [this](){this->goPersistent();});
    connect(prmView, &tbl::View::closingPersistent, [this](){this->stopPersistent();});
    connect(occtPage, &dlg::OCCTParameters::dirty, view, &SurfaceMesh::occtValueChanged);
#ifdef NETGEN_PRESENT
    connect(netgenPage, &dlg::NetgenParameters::dirty, view, &SurfaceMesh::netgenValueChanged);
#endif
    //TODO Gmsh
  }
  
  void goPersistent()
  {
    parameterStacked->setEnabled(false);
  }
  
  void stopPersistent()
  {
    parameterStacked->setEnabled(true);
  }
};

SurfaceMesh::SurfaceMesh(cmd::SurfaceMesh *cIn)
: Base("cmv::SurfaceMesh")
, stow(new Stow(cIn, this))
{}

SurfaceMesh::~SurfaceMesh() = default;

void SurfaceMesh::occtValueChanged()
{
  stow->feature->setOcctParameters(stow->occtPage->getParameters());
  stow->command->localUpdate();
  node->sendBlocked(msg::buildStatusMessage(stow->command->getStatusMessage()));
}

void SurfaceMesh::netgenValueChanged()
{
  stow->feature->setNetgenParameters(stow->netgenPage->getParameters());
  stow->command->localUpdate();
  node->sendBlocked(msg::buildStatusMessage(stow->command->getStatusMessage()));
}

void SurfaceMesh::modelChanged(const QModelIndex &index, const QModelIndex&)
{
  if (!index.isValid())
    return;
  
  prm::Parameter *changed = stow->parameters.at(index.row());
  if (changed->getTag() == ftr::SurfaceMesh::Tags::Source)
    stow->command->setSelection(stow->prmModel->getMessages(changed));
  else if (changed->getTag() == ftr::SurfaceMesh::Tags::MeshType)
  {
    stow->command->setSelection(slc::Messages());
    stow->prmView->updateHideInactive();
    stow->syncStacked();
  }
  stow->command->localUpdate();
  node->sendBlocked(msg::buildStatusMessage(stow->command->getStatusMessage()));
}
