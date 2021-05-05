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
#include <QLabel>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "dialogs/dlgselectionbutton.h"
#include "dialogs/dlgselectionwidget.h"
#include "feature/ftrinputtype.h"
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
  
  QVBoxLayout *verticalLayout = nullptr;
  QHBoxLayout *horizontalLayout = nullptr;
  QLabel *mesherLabel = nullptr;
  QComboBox *mesherCombo = nullptr;
  QSpacerItem *horizontalSpacer = nullptr;
  QStackedWidget *parameterStacked = nullptr;
  dlg::OCCTParameters *occtPage = nullptr;
  QVBoxLayout *occtPageLayout = nullptr;
  dlg::NetgenParameters *netgenPage = nullptr;
  QVBoxLayout *netgenPageLayout = nullptr;
  QWidget *gmshPage = nullptr;
  QVBoxLayout *gmshPageLayout = nullptr;
  dlg::SelectionWidget *selectionWidget = nullptr;
  
  Stow(cmd::SurfaceMesh *cIn, cmv::SurfaceMesh *vIn)
  : command(cIn)
  , view(vIn)
  , feature(cIn->feature)
  {
    buildGui();
    
    QSettings &settings = app::instance()->getUserSettings();
    settings.beginGroup("cmv::SurfaceMesh");
    //load settings
    settings.endGroup();
    
    loadFeatureData();
    glue();
    selectionWidget->activate(0);
  }
  
  void buildGui()
  {
    verticalLayout = new QVBoxLayout(view);
    verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
    view->setLayout(verticalLayout);
    
    mesherLabel = new QLabel(tr("Mesher:"), view);
    mesherLabel->setObjectName(QString::fromUtf8("mesherLabel"));
    mesherCombo = new QComboBox(view);
    mesherCombo->addItem(tr("Inert"));
    mesherCombo->addItem(tr("OCCT"));
    mesherCombo->addItem(tr("Netgen"));
    mesherCombo->addItem(tr("GMSH"));
    
    mesherCombo->setObjectName(QString::fromUtf8("mesherCombo"));
    QHBoxLayout *comboHLayout = new QHBoxLayout();
    comboHLayout->addWidget(mesherLabel);
    comboHLayout->addWidget(mesherCombo);
    comboHLayout->addStretch();
    QVBoxLayout *comboVLayout = new QVBoxLayout();
    comboVLayout->addLayout(comboHLayout);
    
    horizontalLayout = new QHBoxLayout();
    horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
    horizontalLayout->addLayout(comboVLayout);
    horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    horizontalLayout->addItem(horizontalSpacer);
    
    dlg::SelectionWidgetCue cue;
    cue.name.clear();
    cue.singleSelection = true;
    cue.showAccrueColumn = false;
    cue.mask = slc::ObjectsEnabled | slc::ObjectsSelectable;
    cue.statusPrompt = tr("Select Feature To Mesh");
    selectionWidget = new dlg::SelectionWidget(view, std::vector<dlg::SelectionWidgetCue>(1, cue));
    horizontalLayout->addWidget(selectionWidget);
    verticalLayout->addLayout(horizontalLayout);
    
    parameterStacked = new QStackedWidget(view);
    parameterStacked->setObjectName(QString::fromUtf8("parameterStacked"));
    QWidget *inertPage = new QWidget(parameterStacked); //empty
    inertPage->setObjectName(QString::fromUtf8("inertPage"));
    parameterStacked->addWidget(inertPage);
    occtPage = new dlg::OCCTParameters(parameterStacked);
    occtPage->setObjectName(QString::fromUtf8("occtPage"));
    parameterStacked->addWidget(occtPage);
    netgenPage = new dlg::NetgenParameters(parameterStacked);
    netgenPage->setObjectName(QString::fromUtf8("netgenPage"));
    parameterStacked->addWidget(netgenPage);
    gmshPage = new QWidget(parameterStacked); //TODO
    gmshPage->setObjectName(QString::fromUtf8("gmshPage"));
    parameterStacked->addWidget(gmshPage);
    verticalLayout->addWidget(parameterStacked);
  }
  
  void loadFeatureData()
  {
    auto setSelection = [&]()
    {
      const auto &picks = feature->getParameter(ftr::SurfaceMesh::Tags::Source)->getPicks();
      if (!picks.empty())
      {
        auto pl = view->project->getPayload(feature->getId());
        tls::Resolver resolver(pl);
        if (resolver.resolve(picks.front()))
          selectionWidget->getButton(0)->setMessages(resolver.convertToMessages());
      }
    };
    
    auto ct = static_cast<ftr::SurfaceMesh::MeshType>(feature->getParameter(ftr::SurfaceMesh::Tags::MeshType)->getInt());
    if (ct == ftr::SurfaceMesh::Inert)
    {
      mesherCombo->setCurrentIndex(0);
      parameterStacked->setCurrentIndex(0);
      selectionWidget->hide();
    }
    else if (ct == ftr::SurfaceMesh::Occt)
    {
      occtPage->setOCCT(feature->getOcctParameters());
      mesherCombo->setCurrentIndex(1);
      parameterStacked->setCurrentIndex(1);
      setSelection();
    }
    else if (ct == ftr::SurfaceMesh::MeshType::Netgen)
    {
#ifdef NETGEN_PRESENT
      netgenPage->setParameters(feature->getNetgenParameters());
      mesherCombo->setCurrentIndex(2);
      parameterStacked->setCurrentIndex(2);
      setSelection();
#else
      QTimer::singleShot(0, [this](){mesherCombo->setCurrentIndex(1);});
      view->node->send(msg::buildStatusMessage(tr("Netgen Not Present, Switching to OCCT").toStdString(), 2.0));
#endif
    }
    else if (ct == ftr::SurfaceMesh::Gmsh)
    {
#ifdef GMSH_PRESENT
      //TODO fill in data for gmsh
      feature->getType(); //dummy statement
#else
      QTimer::singleShot(0, [this](){mesherCombo->setCurrentIndex(1);});
      view->node->send(msg::buildStatusMessage(tr("Gmsh Not Present, Switching to OCCT").toStdString(), 2.0));
#endif
    }
  }
  
  void glue()
  {
    QObject::connect(mesherCombo, SIGNAL(currentIndexChanged(int)), view, SLOT(typeChanged(int)));
    QObject::connect(selectionWidget->getButton(0), SIGNAL(dirty()), view, SLOT(selectionChanged()));
    QObject::connect(occtPage, &dlg::OCCTParameters::dirty, view, &SurfaceMesh::occtValueChanged);
    QObject::connect(netgenPage, &dlg::NetgenParameters::dirty, view, &SurfaceMesh::netgenValueChanged);
  }
};

SurfaceMesh::SurfaceMesh(cmd::SurfaceMesh *cIn)
: Base("cmv::SurfaceMesh")
, stow(new Stow(cIn, this))
{}

SurfaceMesh::~SurfaceMesh() = default;

void SurfaceMesh::typeChanged(int currentIndex)
{
  stow->parameterStacked->setCurrentIndex(currentIndex);
  auto oldType = stow->feature->getParameter(ftr::SurfaceMesh::Tags::MeshType)->getInt();
  assert(oldType >= 0 && oldType < stow->mesherCombo->count());
  if (oldType == currentIndex)
    return;
  
  slc::Message tm;
  const auto &msgs = stow->selectionWidget->getMessages(0);
  if (!msgs.empty())
    tm = msgs.front();
  stow->command->setSelection(tm);
  
  if (currentIndex == 0)
  {
    stow->selectionWidget->getButton(0)->setMessagesQuietly(std::vector<slc::Message>()); //clear any selection
    stow->feature->getParameter(ftr::SurfaceMesh::Tags::MeshType)->setValue(static_cast<int>(ftr::SurfaceMesh::Inert));
    stow->command->setSelection(slc::Message());
    stow->selectionWidget->hide();
  }
  else if (currentIndex == 1)
  {
    stow->feature->setOcctParameters(stow->occtPage->getParameters());
    stow->command->setSelection(tm);
    stow->selectionWidget->show();
  }
  else if (currentIndex == 2)
  {
#ifdef NETGEN_PRESENT
    stow->feature->setNetgenParameters(stow->netgenPage->getParameters());
    stow->command->setSelection(tm);
    stow->selectionWidget->show();
#else
    node->send(msg::buildStatusMessage(tr("Netgen Not Present, Reverting Type Change").toStdString(), 2.0));
    QTimer::singleShot(0, [this, oldType](){stow->mesherCombo->setCurrentIndex(oldType);});
#endif
  }
  else if (currentIndex == 3)
  {
#ifdef GMSH_PRESENT
    //TODO set gmsh parameters.
    stow->selectionWidget->show();
#else
    node->send(msg::buildStatusMessage(tr("Gmsh Not Present, Reverting Type Change").toStdString(), 2.0));
    QTimer::singleShot(0, [this, oldType](){stow->mesherCombo->setCurrentIndex(oldType);});
#endif
  }
  
  stow->command->localUpdate();
}

void SurfaceMesh::selectionChanged()
{
  project->clearAllInputs(stow->feature->getId());
  
  const auto &msgs = stow->selectionWidget->getMessages(0);
  if (!msgs.empty())
  {
    stow->command->setSelection(msgs.front());
    stow->command->localUpdate();
  }
}

void SurfaceMesh::occtValueChanged()
{
  stow->feature->setOcctParameters(stow->occtPage->getParameters());
  stow->command->localUpdate();
}

void SurfaceMesh::netgenValueChanged()
{
  stow->feature->setNetgenParameters(stow->netgenPage->getParameters());
  stow->command->localUpdate();
}
