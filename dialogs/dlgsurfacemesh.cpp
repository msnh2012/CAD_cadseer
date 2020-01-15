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
#include <QVariant>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpacerItem>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QPushButton>
#include <QTimer>

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "message/msgmessage.h"
#include "mesh/mshparameters.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrsurfacemesh.h"
#include "selection/slcmessage.h"
#include "dialogs/dlgselectionbutton.h"
#include "dialogs/dlgnetgenparameters.h"
#include "dialogs/dlgocctparameters.h"
#include "dialogs/dlgselectionwidget.h"
#include "dialogs/dlgsurfacemesh.h"

using boost::uuids::uuid;

using namespace dlg;

struct SurfaceMesh::Stow
{
  QVBoxLayout *verticalLayout = nullptr;
  QHBoxLayout *horizontalLayout = nullptr;
  QLabel *mesherLabel = nullptr;
  QComboBox *mesherCombo = nullptr;
  QSpacerItem *horizontalSpacer = nullptr;
  QStackedWidget *parameterStacked = nullptr;
  OCCTParameters *occtPage = nullptr;
  QVBoxLayout *occtPageLayout = nullptr;
  NetgenParameters *netgenPage = nullptr;
  QVBoxLayout *netgenPageLayout = nullptr;
  QWidget *gmshPage = nullptr;
  QVBoxLayout *gmshPageLayout = nullptr;
  QDialogButtonBox *buttonBox = nullptr;
  SelectionWidget *selectionWidget = nullptr;
  
  ftr::SurfaceMesh *feature;
  
  Stow(ftr::SurfaceMesh *fIn, QDialog *dialog)
  : feature(fIn)
  {
    buildGui(dialog);
  }
  
  void buildGui(QDialog *dialog)
  {
    verticalLayout = new QVBoxLayout(dialog);
    verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
    
    //because we have preprocessor conditionals,
    //the current index of comboBox is not static
    //from one build to the next. so we add userdata
    //to the combo box item to test for.
    mesherLabel = new QLabel(tr("Mesher:"), dialog);
    mesherLabel->setObjectName(QString::fromUtf8("mesherLabel"));
    mesherCombo = new QComboBox(dialog);
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
    comboVLayout->addStretch();
    
    horizontalLayout = new QHBoxLayout();
    horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
    horizontalLayout->addLayout(comboVLayout);
    horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    horizontalLayout->addItem(horizontalSpacer);
    
    SelectionWidgetCue cue;
    cue.name.clear();
    cue.singleSelection = true;
    cue.showAccrueColumn = false;
    cue.mask = slc::ObjectsEnabled | slc::ObjectsSelectable;
    cue.statusPrompt = tr("Select Feature To Mesh");
    selectionWidget = new SelectionWidget(dialog, std::vector<SelectionWidgetCue>(1, cue));
    horizontalLayout->addWidget(selectionWidget);
    verticalLayout->addLayout(horizontalLayout);
    
    parameterStacked = new QStackedWidget(dialog);
    parameterStacked->setObjectName(QString::fromUtf8("parameterStacked"));
    QWidget *inertPage = new QWidget(dialog); //empty
    inertPage->setObjectName(QString::fromUtf8("inertPage"));
    parameterStacked->addWidget(inertPage);
    occtPage = new OCCTParameters(dialog);
    occtPage->setObjectName(QString::fromUtf8("occtPage"));
    parameterStacked->addWidget(occtPage);
    netgenPage = new NetgenParameters(dialog);
    netgenPage->setObjectName(QString::fromUtf8("netgenPage"));
    parameterStacked->addWidget(netgenPage);
    gmshPage = new QWidget(dialog); //TODO
    gmshPage->setObjectName(QString::fromUtf8("gmshPage"));
    parameterStacked->addWidget(gmshPage);
    verticalLayout->addWidget(parameterStacked);

    buttonBox = new QDialogButtonBox(dialog);
    buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
    verticalLayout->addWidget(buttonBox);

    QObject::connect(buttonBox, SIGNAL(accepted()), dialog, SLOT(accept()));
    QObject::connect(buttonBox, SIGNAL(rejected()), dialog, SLOT(reject()));
    QObject::connect(mesherCombo, SIGNAL(currentIndexChanged(int)), dialog, SLOT(comboChanged(int)));
  }
};

SurfaceMesh::SurfaceMesh(ftr::SurfaceMesh *fIn, QWidget *pIn, bool isEditIn)
: Base(fIn, pIn, isEditIn)
, stow(new Stow(fIn, this))
{
  init();
}

SurfaceMesh::~SurfaceMesh() = default;

void SurfaceMesh::init()
{
  QSettings &settings = app::instance()->getUserSettings();
  settings.beginGroup("dlg::SurfaceMesh");

  settings.endGroup();
  
  loadFeatureData();
}

void SurfaceMesh::loadFeatureData()
{
  if (isEditDialog)
  {
    auto setSelection = [&]()
    {
      auto pl = project->getPayload(stow->feature->getId());
      auto features = pl.getFeatures(ftr::InputType::target);
      if (features.empty())
      {
        int index = stow->mesherCombo->findData(0);
        assert(index >= 0);
        stow->mesherCombo->setCurrentIndex(index);
        return;
      }
      slc::Message proto;
      proto.type = slc::Type::Object;
      proto.featureType = features.front()->getType();
      proto.featureId = features.front()->getId();
      stow->selectionWidget->getButton(0)->setMessages(proto);
    };
    
    if (stow->feature->getMeshType() == ftr::SurfaceMesh::MeshType::inert)
    {
      int index = stow->mesherCombo->findData(0);
      assert(index >= 0);
      stow->mesherCombo->setCurrentIndex(index);
      stow->selectionWidget->hide();
    }
    else if (stow->feature->getMeshType() == ftr::SurfaceMesh::MeshType::occt)
    {
      stow->occtPage->setOCCT(stow->feature->getOcctParameters());
      int index = stow->mesherCombo->findData(1);
      assert(index >= 0);
      stow->mesherCombo->setCurrentIndex(index);
      setSelection();
    }
    else if (stow->feature->getMeshType() == ftr::SurfaceMesh::MeshType::netgen)
    {
#ifdef NETGEN_PRESENT
      stow->netgenPage->setParameters(stow->feature->getNetgenParameters());
      int index = stow->mesherCombo->findData(2);
      assert(index >= 0);
      stow->mesherCombo->setCurrentIndex(index);
      setSelection();
#else
      int index = stow->mesherCombo->findData(1);
      assert(index >= 0);
      QTimer::singleShot(0, [this, index](){stow->mesherCombo->setCurrentIndex(index);});
      node->send(msg::buildStatusMessage(tr("Netgen Not Present, Switching to OCCT").toStdString(), 2.0));
#endif
    }
    else if (stow->feature->getMeshType() == ftr::SurfaceMesh::MeshType::gmsh)
    {
#ifdef GMSH_PRESENT
      //fill in data for gmsh
      stow->feature->getMeshType(); //dummy statement
#else
      int index = stow->mesherCombo->findData(1);
      assert(index >= 0);
      QTimer::singleShot(0, [this, index](){stow->mesherCombo->setCurrentIndex(index);});
      node->send(msg::buildStatusMessage(tr("Gmsh Not Present, Switching to OCCT").toStdString(), 2.0));
#endif
    }
  }
  else
  {
    int index = stow->mesherCombo->findData(1);
    assert(index >= 0);
    stow->mesherCombo->setCurrentIndex(index);
  }
  stow->selectionWidget->activate(0);
}

void SurfaceMesh::reject()
{
  isAccepted = false;
  finishDialog();
  QDialog::reject();
}

void SurfaceMesh::accept()
{
  isAccepted = true;
  finishDialog();
  QDialog::accept();
}

void SurfaceMesh::comboChanged(int index)
{
  if (index < 0)
    return;
  //we don't care about index because of the preprocess conditionals. see buildGui.
  QVariant v = stow->mesherCombo->currentData();
  assert(v.isValid());
  int userData = v.toInt();
  assert(userData >= 0 && userData <= 3);
  stow->parameterStacked->setCurrentIndex(userData);
  if (userData == 0)
    stow->selectionWidget->hide();
  else
    stow->selectionWidget->show();
}

void SurfaceMesh::finishDialog()
{
  prj::Project *p = app::instance()->getProject();
  
  auto restoreState = [&]()
  {
    if (isEditDialog)
    {
      for (const auto &id : leafChildren)
        p->setCurrentLeaf(id);
    }
  };
  auto bail = [&]()
  {
    if (!isEditDialog)
      p->removeFeature(stow->feature->getId());
    commandDone();
    restoreState();
  };
  
  if(!isAccepted)
  {
    bail();
    return;
  }
  
  int userData = stow->mesherCombo->currentData().toInt();
  assert(userData >= 0 && userData <= 3);
  project->clearAllInputs(stow->feature->getId()); //add if needed
  auto hookSelection = [&]()
  {
    auto msgs = stow->selectionWidget->getMessages(0);
    if (!msgs.empty())
    {
      project->connectInsert(msgs.front().featureId, stow->feature->getId(), {ftr::InputType::target});
    }
  };
  
  if (userData == 0)
  {
    stow->feature->setMeshType(ftr::SurfaceMesh::MeshType::inert);
  }
  else if (userData == 1)
  {
    stow->feature->setOcctParameters(stow->occtPage->getParameters());
    hookSelection();
  }
  else if (userData == 2)
  {
    stow->feature->setNetgenParameters(stow->netgenPage->getParameters());
    hookSelection();
  }
  else if (userData == 3)
  {
    //TODO set gmsh parameters.
    hookSelection();
  }
  
  queueUpdate();
  commandDone();
  restoreState();
}
