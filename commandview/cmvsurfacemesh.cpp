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
    
    //because we have preprocessor conditionals,
    //the current index of comboBox is not static
    //from one build to the next. so we add userdata
    //to the combo box item to test for.
    mesherLabel = new QLabel(tr("Mesher:"), view);
    mesherLabel->setObjectName(QString::fromUtf8("mesherLabel"));
    mesherCombo = new QComboBox(view);
    mesherCombo->addItem(tr("Inert"), QVariant(0));
    mesherCombo->addItem(tr("OCCT"), QVariant(1));
#ifdef NETGEN_PRESENT
    mesherCombo->addItem(tr("Netgen"), QVariant(2));
#endif
#ifdef GMSH_PRESENT
    mesherCombo->addItem(tr("GMSH"), QVariant(3));
#endif
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
      auto pl = view->project->getPayload(command->feature->getId());
      auto features = pl.getFeatures(ftr::InputType::target);
      if (features.empty())
      {
        int index = mesherCombo->findData(0);
        assert(index >= 0);
        mesherCombo->setCurrentIndex(index);
        return;
      }
      slc::Message proto;
      proto.type = slc::Type::Object;
      proto.featureType = features.front()->getType();
      proto.featureId = features.front()->getId();
      selectionWidget->getButton(0)->setMessages(proto);
    };
    if (command->feature->getMeshType() == ftr::SurfaceMesh::MeshType::inert)
    {
      int index = mesherCombo->findData(0);
      assert(index >= 0);
      mesherCombo->setCurrentIndex(index);
      parameterStacked->setCurrentIndex(index);
      selectionWidget->hide();
    }
    else if (command->feature->getMeshType() == ftr::SurfaceMesh::MeshType::occt)
    {
      occtPage->setOCCT(command->feature->getOcctParameters());
      int index = mesherCombo->findData(1);
      assert(index >= 0);
      mesherCombo->setCurrentIndex(index);
      parameterStacked->setCurrentIndex(index);
      setSelection();
    }
    else if (command->feature->getMeshType() == ftr::SurfaceMesh::MeshType::netgen)
    {
#ifdef NETGEN_PRESENT
      netgenPage->setParameters(command->feature->getNetgenParameters());
      int index = mesherCombo->findData(2);
      assert(index >= 0);
      mesherCombo->setCurrentIndex(index);
      parameterStacked->setCurrentIndex(index);
      setSelection();
#else
      int index = mesherCombo->findData(1);
      assert(index >= 0);
      QTimer::singleShot(0, [this, index](){mesherCombo->setCurrentIndex(index);});
      view->node->send(msg::buildStatusMessage(tr("Netgen Not Present, Switching to OCCT").toStdString(), 2.0));
#endif
    }
    else if (command->feature->getMeshType() == ftr::SurfaceMesh::MeshType::gmsh)
    {
#ifdef GMSH_PRESENT
      //fill in data for gmsh
      command->feature->getMeshType(); //dummy statement
#else
      int index = mesherCombo->findData(1);
      assert(index >= 0);
      QTimer::singleShot(0, [this, index](){mesherCombo->setCurrentIndex(index);});
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

void SurfaceMesh::typeChanged(int)
{
  QVariant v = stow->mesherCombo->currentData();
  assert(v.isValid());
  int userData = v.toInt();
  assert(userData >= 0 && userData <= 3);
  stow->parameterStacked->setCurrentIndex(userData);
  slc::Message tm;
  const auto &msgs = stow->selectionWidget->getMessages(0);
  if (!msgs.empty())
    tm = msgs.front();
  stow->command->setSelection(tm);
  
  if (userData == 0)
  {
    stow->selectionWidget->getButton(0)->setMessagesQuietly(std::vector<slc::Message>()); //clear any selection
    stow->command->feature->setMeshType(ftr::SurfaceMesh::MeshType::inert);
    stow->command->setSelection(slc::Message());
    stow->selectionWidget->hide();
  }
  else if (userData == 1)
  {
    stow->command->feature->setOcctParameters(stow->occtPage->getParameters());
    stow->command->setSelection(tm);
    stow->selectionWidget->show();
  }
  else if (userData == 2)
  {
    stow->command->feature->setNetgenParameters(stow->netgenPage->getParameters());
    stow->command->setSelection(tm);
    stow->selectionWidget->show();
  }
  else if (userData == 3)
  {
    stow->selectionWidget->show();
    //TODO set gmsh parameters.
  }
  
  stow->command->localUpdate();
}

void SurfaceMesh::selectionChanged()
{
  project->clearAllInputs(stow->command->feature->getId());
  
  const auto &msgs = stow->selectionWidget->getMessages(0);
  if (!msgs.empty())
  {
    stow->command->setSelection(msgs.front());
    stow->command->localUpdate();
  }
}

void SurfaceMesh::occtValueChanged()
{
  stow->command->feature->setOcctParameters(stow->occtPage->getParameters());
  stow->command->localUpdate();
}

void SurfaceMesh::netgenValueChanged()
{
  stow->command->feature->setNetgenParameters(stow->netgenPage->getParameters());
  stow->command->localUpdate();
}
