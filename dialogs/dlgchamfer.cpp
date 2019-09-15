/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2019 Thomas S. Anderson blobfish.at.gmx.com
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
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QAction>
#include <QStackedWidget>
#include <QTimer>

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "annex/annseershape.h"
#include "message/msgmessage.h"
#include "message/msgnode.h"
#include "dialogs/dlgselectionbutton.h"
#include "dialogs/dlgselectionlist.h"
#include "dialogs/dlgselectionwidget.h"
#include "dialogs/dlgparameterwidget.h"
#include "parameter/prmparameter.h"
#include "tools/featuretools.h"
#include "tools/idtools.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrchamfer.h"
#include "dialogs/dlgchamfer.h"

using boost::uuids::uuid;

using namespace dlg;
using Cue = ::ftr::Chamfer::Cue;
using Entry = ::ftr::Chamfer::Cue::Entry;

namespace dlg
{
  class ChamferWidgetBase : public QWidget
  {
  public:
    ChamferWidgetBase(const Entry& eIn, QWidget *parent)
    : QWidget(parent)
    , entry(eIn, true)
    {}
    
    void activate(int index)
    {
      selectionWidget->activate(index);
    }
    
    SelectionWidget *selectionWidget = nullptr;
    ParameterWidget *parameterWidget = nullptr;
    Entry entry;
  };
  
  class SymmetricWidget : public ChamferWidgetBase
  {
  public:
    SymmetricWidget(const Entry& eIn, QWidget *parent)
    : ChamferWidgetBase(eIn, parent)
    {
      assert(eIn.style == ::ftr::Chamfer::Style::Symmetric);
      QVBoxLayout *layout = new QVBoxLayout();
      layout->setContentsMargins(0, 0, 0, 0);
      this->setLayout(layout);
      
      std::vector<SelectionWidgetCue> cues;
      SelectionWidgetCue cue;
      cue.name = tr("Edges");
      cue.singleSelection = false;
      cue.showAccrueColumn = false;
      cue.accrueDefault = slc::Accrue::Tangent;
      cue.mask = slc::EdgesEnabled | slc::EdgesSelectable;
      cue.statusPrompt = tr("Select Edges For Symmetric Chamfer");
      cues.push_back(cue);
      selectionWidget = new SelectionWidget(this, cues);
      layout->addWidget(selectionWidget);
      
      parameterWidget = new ParameterWidget(this, std::vector<prm::Parameter*>(1, entry.parameter1.get()));
      layout->addWidget(parameterWidget);
    }
  };

  class TwoDistanceWidget : public ChamferWidgetBase
  {
  public:
    TwoDistanceWidget(const Entry& eIn, QWidget *parent)
    : ChamferWidgetBase(eIn, parent)
    {
      assert(eIn.style == ::ftr::Chamfer::Style::TwoDistances);
      QVBoxLayout *layout = new QVBoxLayout();
      layout->setContentsMargins(0, 0, 0, 0);
      this->setLayout(layout);
      
      std::vector<SelectionWidgetCue> cues;
      SelectionWidgetCue cue;
      
      cue.name = tr("Edge");
      cue.singleSelection = true;
      cue.showAccrueColumn = false;
      cue.accrueDefault = slc::Accrue::Tangent;
      cue.mask = slc::EdgesEnabled | slc::EdgesSelectable;
      cue.statusPrompt = tr("Select Edge For Two Distances Chamfer");
      cues.push_back(cue);
      
      cue.name = tr("Face");
      cue.mask = slc::FacesEnabled | slc::FacesSelectable;
      cue.statusPrompt = tr("Select Face For Two Distances Chamfer");
      cue.accrueDefault = slc::Accrue::None;
      cues.push_back(cue);
      
      selectionWidget = new SelectionWidget(this, cues);
      layout->addWidget(selectionWidget);
      
      std::vector<prm::Parameter*> prms;
      prms.push_back(entry.parameter1.get());
      prms.push_back(entry.parameter2.get());
      parameterWidget = new ParameterWidget(this, prms);
      layout->addWidget(parameterWidget);
    }
  };
  
  class DistanceAngleWidget : public ChamferWidgetBase
  {
  public:
    DistanceAngleWidget(const Entry& eIn, QWidget *parent)
    : ChamferWidgetBase(eIn, parent)
    {
      assert(eIn.style == ::ftr::Chamfer::Style::DistanceAngle);
      QVBoxLayout *layout = new QVBoxLayout();
      layout->setContentsMargins(0, 0, 0, 0);
      this->setLayout(layout);
      
      std::vector<SelectionWidgetCue> cues;
      SelectionWidgetCue cue;
      
      cue.name = tr("Edge");
      cue.singleSelection = true;
      cue.showAccrueColumn = false;
      cue.accrueDefault = slc::Accrue::Tangent;
      cue.mask = slc::EdgesEnabled | slc::EdgesSelectable;
      cue.statusPrompt = tr("Select Edge For Distance Angle Chamfer");
      cues.push_back(cue);
      
      cue.name = tr("Face");
      cue.mask = slc::FacesEnabled | slc::FacesSelectable;
      cue.statusPrompt = tr("Select Face For Distance Angle Chamfer");
      cue.accrueDefault = slc::Accrue::None;
      cues.push_back(cue);
      
      selectionWidget = new SelectionWidget(this, cues);
      layout->addWidget(selectionWidget);
      
      std::vector<prm::Parameter*> prms;
      prms.push_back(entry.parameter1.get());
      prms.push_back(entry.parameter2.get());
      parameterWidget = new ParameterWidget(this, prms);
      layout->addWidget(parameterWidget);
    }
  };

  struct Chamfer::Stow
  {
    Stow(ftr::Chamfer *fIn)
    : feature(fIn)
    {}
    
    ftr::Chamfer *feature;
    QComboBox *modeCombo = nullptr;
    QListWidget *styleList = nullptr;
    QAction *addSymmetric = nullptr;
    QAction *addTwoDistances = nullptr;
    QAction *addDistanceAngle = nullptr;
    QAction *separator = nullptr;
    QAction *remove = nullptr;
    QStackedWidget *stackedWidget = nullptr;
    
    void clearStackedWidget()
    {
      while (stackedWidget->count() > 0)
        stackedWidget->removeWidget(stackedWidget->widget(0));
    }
    
    void modeChanged()
    {
      clearStackedWidget();
      styleList->clear();
      //remove all actions
      for (auto *a : styleList->actions())
        styleList->removeAction(a);
      if (modeCombo->currentIndex() == 0)
      {
        styleList->addAction(addSymmetric);
        styleList->addAction(addTwoDistances);
        styleList->addAction(addDistanceAngle);
      }
      else if (modeCombo->currentIndex() == 1)
      {
        styleList->addAction(addSymmetric);
      }
      else if (modeCombo->currentIndex() == 2)
      {
        styleList->addAction(addTwoDistances);
      }
      else
        return;
      styleList->addAction(separator);
      styleList->addAction(remove);
    }
    
    ChamferWidgetBase* appendEntry(const Entry &eIn, QWidget *parent)
    {
      if (eIn.style == ::ftr::Chamfer::Style::Symmetric)
      {
        styleList->addItem(tr("Symmetric"));
        SymmetricWidget *sw = new SymmetricWidget(eIn, parent);
        stackedWidget->addWidget(sw);
        return sw;
      }
      if (eIn.style == ::ftr::Chamfer::Style::TwoDistances)
      {
        styleList->addItem(tr("Two Distances"));
        TwoDistanceWidget *tdw = new TwoDistanceWidget(eIn, parent);
        stackedWidget->addWidget(tdw);
        return tdw;
      }
      if (eIn.style == ::ftr::Chamfer::Style::DistanceAngle)
      {
        styleList->addItem(tr("Angle Distance"));
        DistanceAngleWidget *daw = new DistanceAngleWidget(eIn, parent);
        stackedWidget->addWidget(daw);
        return daw;
      }
      assert(0); //unrecognized entry type.
      return nullptr;
    }
    
    void removeEntry()
    {
      QList<QListWidgetItem *> items = styleList->selectedItems();
      if (items.empty())
        return;
      assert(items.size() == 1);
      if (items.size() != 1)
        return;
      int index = styleList->row(items.front());
      delete styleList->takeItem(index);
      stackedWidget->removeWidget(stackedWidget->widget(index));
    }
  };
}

Chamfer::Chamfer(ftr::Chamfer *fIn, QWidget *pIn, bool isEditIn)
: Base(fIn, pIn, isEditIn)
, stow(new Stow(fIn))
{
  init();
}

Chamfer::~Chamfer() = default;

void Chamfer::init()
{
  buildGui();
  
  QSettings &settings = app::instance()->getUserSettings();
  settings.beginGroup("dlg::Chamfer");

  settings.endGroup();
  
  loadFeatureData();
  if (isEditDialog && stow->stackedWidget->count() > 0)
  {
    ChamferWidgetBase *w = dynamic_cast<ChamferWidgetBase*>(stow->stackedWidget->widget(0));
    assert(w);
    w->activate(0);
  }
  else
  {
    //create a default type of symmetric
    QTimer::singleShot(0, this, &Chamfer::appendSymmetricSlot);
  }
}

void Chamfer::buildGui()
{
  stow->modeCombo = new QComboBox(this);
  stow->modeCombo->addItem(tr("Classic"));
  stow->modeCombo->addItem(tr("Throat"));
  stow->modeCombo->addItem(tr("Penetration"));
  
  stow->styleList = new QListWidget(this);
  stow->styleList->setSelectionMode(QAbstractItemView::SingleSelection);
  stow->styleList->setContextMenuPolicy(Qt::ActionsContextMenu);
  stow->addSymmetric = new QAction(tr("Add Symmetric"), this);
  stow->addTwoDistances = new QAction(tr("Add Two Distances"), this);
  stow->addDistanceAngle = new QAction(tr("Add Distance Angle"), this);
  stow->separator = new QAction(this);
  stow->separator->setSeparator(true);
  stow->remove = new QAction(tr("Remove"), this);
  stow->styleList->addAction(stow->addSymmetric);
  stow->styleList->addAction(stow->addTwoDistances);
  stow->styleList->addAction(stow->addDistanceAngle);
  stow->styleList->addAction(stow->separator);
  stow->styleList->addAction(stow->remove);
  
  stow->stackedWidget = new QStackedWidget(this);
  stow->stackedWidget->setDisabled(true);
  
  QDialogButtonBox *buttons = new QDialogButtonBox
    (QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
  
  connect(stow->modeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(modeChangedSlot(int)));
  connect(stow->addSymmetric, &QAction::triggered, this, &Chamfer::appendSymmetricSlot);
  connect(stow->addTwoDistances, &QAction::triggered, this, &Chamfer::appendTwoDistancesSlot);
  connect(stow->addDistanceAngle, &QAction::triggered, this, &Chamfer::appendDistanceAngleSlot);
  connect(stow->remove, &QAction::triggered, this, &Chamfer::removeSlot);
  connect(stow->styleList, &QListWidget::itemSelectionChanged, this, &Chamfer::listSelectionChangedSlot);
  connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
  
  auto *modeStyleLayout = new QVBoxLayout();
  modeStyleLayout->addWidget(stow->modeCombo);
  modeStyleLayout->addWidget(stow->styleList);
  
  auto *controlLayout = new QHBoxLayout();
  controlLayout->addLayout(modeStyleLayout);
  controlLayout->addStretch();
  controlLayout->addWidget(stow->stackedWidget);
  
  auto *dButtonLayout = new QHBoxLayout();
  dButtonLayout->addStretch();
  dButtonLayout->addWidget(buttons);
  
  auto *layout = new QVBoxLayout();
  layout->addLayout(controlLayout);
  layout->addLayout(dButtonLayout);
  
  this->setLayout(layout);
}

void Chamfer::loadFeatureData()
{
  if (isEditDialog)
  {

  }
}

void Chamfer::reject()
{
  isAccepted = false;
  finishDialog();
  QDialog::reject();
}

void Chamfer::accept()
{
  isAccepted = true;
  finishDialog();
  QDialog::accept();
}

void Chamfer::modeChangedSlot(int)
{
  stow->modeChanged();
}

void Chamfer::appendSymmetricSlot()
{
  ChamferWidgetBase* widget = stow->appendEntry(::ftr::Chamfer::Cue::Entry::buildDefaultSymmetric(), this);
  stow->styleList->setCurrentRow(stow->styleList->count() - 1, QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent);
  widget->activate(0);
}

void Chamfer::appendTwoDistancesSlot()
{
  ChamferWidgetBase* widget = stow->appendEntry(::ftr::Chamfer::Cue::Entry::buildDefaultTwoDistances(), this);
  stow->styleList->setCurrentRow(stow->styleList->count() - 1, QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent);
  widget->activate(0);
}

void Chamfer::appendDistanceAngleSlot()
{
  ChamferWidgetBase* widget = stow->appendEntry(::ftr::Chamfer::Cue::Entry::buildDefaultDistanceAngle(), this);
  stow->styleList->setCurrentRow(stow->styleList->count() - 1, QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent);
  widget->activate(0);
}

void Chamfer::removeSlot()
{
  stow->removeEntry();
}

void Chamfer::listSelectionChangedSlot()
{
  QList<QListWidgetItem *> items = stow->styleList->selectedItems();
  if (items.empty())
  {
    stow->stackedWidget->setDisabled(true);
    return;
  }
  assert(items.size() == 1);
  if (items.size() != 1)
  {
    stow->stackedWidget->setDisabled(true);
    return;
  }
  stow->stackedWidget->setEnabled(true);
  int index = stow->styleList->row(items.front());
  stow->stackedWidget->setCurrentIndex(index);
}

void Chamfer::finishDialog()
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
  
  
  ::ftr::Chamfer::Cue cue;
  assert(stow->modeCombo->currentIndex() >= 0 && stow->modeCombo->currentIndex() < 3);
  cue.mode = static_cast<::ftr::Chamfer::Mode>(stow->modeCombo->currentIndex());
  ann::SeerShape *ss = nullptr;
  osg::Vec4 color;
  auto goSeerShape = [&](const uuid &idIn) -> bool
  {
    ftr::Base *fb = p->findFeature(idIn);
    if (!fb)
      return false;
    if (!fb->hasAnnex(ann::Type::SeerShape) || fb->getAnnex<ann::SeerShape>().isNull())
      return false;
    ss = &fb->getAnnex<ann::SeerShape>();
    color = fb->getColor();
    return true;
  };
  uuid featureId = gu::createNilId();
  auto goFeatureId = [&](const uuid &idIn) -> bool
  {
    //ensure all selections are to the same feature.
    if (!featureId.is_nil())
    {
      if (featureId == idIn)
        return true;
      return false;
    }
    if (!goSeerShape(idIn))
      return false;
    featureId = idIn;
    return true;
  };
  tls::Connector connector;
  int edgePickCount = 0;
  int facePickCount = 0;
  for (int index = 0; index < stow->stackedWidget->count(); ++index)
  {
    ChamferWidgetBase *b = dynamic_cast<ChamferWidgetBase*>(stow->stackedWidget->widget(index));
    assert(b);
    if (!b)
      continue;
    cue.entries.emplace_back(b->entry);
    if (b->entry.style == ::ftr::Chamfer::Style::Symmetric)
    {
      for (const auto &message : b->selectionWidget->getButton(0)->getMessages())
      {
        if (!goFeatureId(message.featureId))
          continue;
        cue.entries.back().edgePicks.push_back(tls::convertToPick(message, *ss, p->getShapeHistory()));
        cue.entries.back().edgePicks.back().tag = ftr::InputType::createIndexedTag(::ftr::Chamfer::edge, edgePickCount++);
        connector.add(message.featureId, cue.entries.back().edgePicks.back().tag);
      }
    }
    else if (b->entry.style == ::ftr::Chamfer::Style::TwoDistances || b->entry.style == ::ftr::Chamfer::Style::DistanceAngle)
    {
      //single selection
      const auto &ems = b->selectionWidget->getButton(0)->getMessages();
      const auto &fms = b->selectionWidget->getButton(1)->getMessages();
      if (ems.size() == 1 && fms.size() == 1 && goFeatureId(ems.front().featureId) && goFeatureId(fms.front().featureId))
      {
        cue.entries.back().edgePicks.push_back(tls::convertToPick(ems.front(), *ss, p->getShapeHistory()));
        cue.entries.back().edgePicks.back().tag = ftr::InputType::createIndexedTag(::ftr::Chamfer::edge, edgePickCount++);
        connector.add(ems.front().featureId, cue.entries.back().edgePicks.back().tag);
        
        cue.entries.back().facePicks.push_back(tls::convertToPick(fms.front(), *ss, p->getShapeHistory()));
        cue.entries.back().facePicks.back().tag = ftr::InputType::createIndexedTag(::ftr::Chamfer::face, facePickCount++);
        connector.add(fms.front().featureId, cue.entries.back().facePicks.back().tag);
      }
    }
  }
  
  if (featureId.is_nil() || !ss || cue.entries.empty())
  {
    bail();
    node->sendBlocked(msg::buildStatusMessage("Invalid selection", 2.0));
    return;
  }
  
  for (int index = 0; index < stow->stackedWidget->count(); ++index)
  {
    ChamferWidgetBase *b = dynamic_cast<ChamferWidgetBase*>(stow->stackedWidget->widget(index));
    assert(b);
    if (!b)
      continue;
    b->parameterWidget->syncLinks();
  }
  p->clearAllInputs(stow->feature->getId());
  stow->feature->setCue(cue);
  for (const auto &pr : connector.pairs)
    p->connectInsert(pr.first, stow->feature->getId(), {pr.second});
  stow->feature->setColor(color);
  
  node->sendBlocked(msg::buildHideThreeD(featureId));
  node->sendBlocked(msg::buildHideOverlay(featureId));
  
  queueUpdate();
  commandDone();
  restoreState();
}
