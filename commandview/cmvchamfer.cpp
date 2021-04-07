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
#include "dialogs/dlgselectionbutton.h"
#include "dialogs/dlgselectionwidget.h"
#include "commandview/cmvparameterwidgets.h"
#include "parameter/prmparameter.h"
#include "tools/featuretools.h"
#include "feature/ftrchamfer.h"
#include "command/cmdchamfer.h"
#include "commandview/cmvchamfer.h"

using boost::uuids::uuid;

namespace
{
  class ChamferWidgetBase : public cmv::ParameterBase
  {
  public:
    ChamferWidgetBase(QWidget *parent)
    : ParameterBase(parent)
    {}
    
    void activate(int index)
    {
      selectionWidget->activate(index);
    }
    
    dlg::SelectionWidget *selectionWidget = nullptr;
    cmv::ParameterWidget *parameterWidget = nullptr;
  };
  
  class SymmetricWidget : public ChamferWidgetBase
  {
  public:
    SymmetricWidget(const ftr::Chamfer::Entry& eIn, QWidget *parent)
    : ChamferWidgetBase(parent)
    {
      assert(eIn.style == ::ftr::Chamfer::Style::Symmetric);
      QVBoxLayout *layout = new QVBoxLayout();
      layout->setContentsMargins(0, 0, 0, 0);
      this->setLayout(layout);
      
      parameterWidget = new cmv::ParameterWidget(this, {eIn.parameter1.get()});
      parameterWidget->setSizePolicy(parameterWidget->sizePolicy().horizontalPolicy(), QSizePolicy::Maximum);
      layout->addWidget(parameterWidget);
      connect(parameterWidget, &ParameterBase::prmValueChanged, this, &ParameterBase::prmValueChanged);
      
      std::vector<dlg::SelectionWidgetCue> cues;
      dlg::SelectionWidgetCue cue;
      cue.name = tr("Edges");
      cue.singleSelection = false;
      cue.showAccrueColumn = false;
      cue.accrueDefault = slc::Accrue::Tangent;
      cue.mask = slc::EdgesEnabled | slc::EdgesSelectable;
      cue.statusPrompt = tr("Select Edges For Symmetric Chamfer");
      cues.push_back(cue);
      selectionWidget = new dlg::SelectionWidget(this, cues);
      layout->addWidget(selectionWidget);
    }
  };

  class TwoDistanceWidget : public ChamferWidgetBase
  {
  public:
    TwoDistanceWidget(const ftr::Chamfer::Entry& eIn, QWidget *parent)
    : ChamferWidgetBase(parent)
    {
      assert(eIn.style == ::ftr::Chamfer::Style::TwoDistances);
      QVBoxLayout *layout = new QVBoxLayout();
      layout->setContentsMargins(0, 0, 0, 0);
      this->setLayout(layout);
      
      std::vector<prm::Parameter*> prms;
      prms.push_back(eIn.parameter1.get());
      prms.push_back(eIn.parameter2.get());
      parameterWidget = new cmv::ParameterWidget(this, prms);
      parameterWidget->setSizePolicy(parameterWidget->sizePolicy().horizontalPolicy(), QSizePolicy::Maximum);
      layout->addWidget(parameterWidget);
      connect(parameterWidget, &ParameterBase::prmValueChanged, this, &ParameterBase::prmValueChanged);
      
      std::vector<dlg::SelectionWidgetCue> cues;
      dlg::SelectionWidgetCue cue;
      
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
      
      selectionWidget = new dlg::SelectionWidget(this, cues);
      layout->addWidget(selectionWidget);
    }
  };
  
  class DistanceAngleWidget : public ChamferWidgetBase
  {
  public:
    DistanceAngleWidget(const ftr::Chamfer::Entry& eIn, QWidget *parent)
    : ChamferWidgetBase(parent)
    {
      assert(eIn.style == ::ftr::Chamfer::Style::DistanceAngle);
      QVBoxLayout *layout = new QVBoxLayout();
      layout->setContentsMargins(0, 0, 0, 0);
      this->setLayout(layout);
      
      std::vector<prm::Parameter*> prms;
      prms.push_back(eIn.parameter1.get());
      prms.push_back(eIn.parameter2.get());
      parameterWidget = new cmv::ParameterWidget(this, prms);
      parameterWidget->setSizePolicy(parameterWidget->sizePolicy().horizontalPolicy(), QSizePolicy::Maximum);
      layout->addWidget(parameterWidget);
      connect(parameterWidget, &ParameterBase::prmValueChanged, this, &ParameterBase::prmValueChanged);
      
      std::vector<dlg::SelectionWidgetCue> cues;
      dlg::SelectionWidgetCue cue;
      
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
      
      selectionWidget = new dlg::SelectionWidget(this, cues);
      layout->addWidget(selectionWidget);
    }
  };
}

using namespace cmv;

struct Chamfer::Stow
{
  cmd::Chamfer *command;
  cmv::Chamfer *view;
  
  QComboBox *modeCombo = nullptr;
  QListWidget *styleList = nullptr;
  QAction *addSymmetric = nullptr;
  QAction *addTwoDistances = nullptr;
  QAction *addDistanceAngle = nullptr;
  QAction *separator = nullptr;
  QAction *remove = nullptr;
  QStackedWidget *stackedWidget = nullptr;
  
  Stow(cmd::Chamfer *cIn, cmv::Chamfer *vIn)
  : command(cIn)
  , view(vIn)
  {
    buildGui();
    
    QSettings &settings = app::instance()->getUserSettings();
    settings.beginGroup("cmv::Chamfer");
    //load settings
    settings.endGroup();
    
    loadFeatureData();
    glue();
    
    if (styleList->count() > 0)
    {
      assert(styleList->count() == stackedWidget->count());
      QTimer::singleShot(0, view, &Chamfer::selectFirstStyleSlot);
//       styleList->item(0)->setSelected(true);
//       styleList->setCurrentRow(0, QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent);
//       ChamferWidgetBase *w = dynamic_cast<ChamferWidgetBase*>(stackedWidget->widget(0));
//       assert(w);
//       w->activate(0);
    }
    else
    {
      //create a default type of symmetric
      QTimer::singleShot(0, view, &Chamfer::appendSymmetricSlot);
    }
  }
  
  void buildGui()
  {
    modeCombo = new QComboBox(view);
    modeCombo->addItem(tr("Classic"));
    modeCombo->addItem(tr("Throat"));
    modeCombo->addItem(tr("Penetration"));
    
    styleList = new QListWidget(view);
    styleList->setSizePolicy(styleList->sizePolicy().horizontalPolicy(), QSizePolicy::Maximum);
    styleList->setSelectionMode(QAbstractItemView::SingleSelection);
    styleList->setSelectionBehavior(QAbstractItemView::SelectRows);
    styleList->setContextMenuPolicy(Qt::ActionsContextMenu);
    addSymmetric = new QAction(tr("Add Symmetric"), view);
    addTwoDistances = new QAction(tr("Add Two Distances"), view);
    addDistanceAngle = new QAction(tr("Add Distance Angle"), view);
    separator = new QAction(view);
    separator->setSeparator(true);
    remove = new QAction(tr("Remove"), view);
    styleList->addAction(addSymmetric);
    styleList->addAction(addTwoDistances);
    styleList->addAction(addDistanceAngle);
    styleList->addAction(separator);
    styleList->addAction(remove);
    
    stackedWidget = new QStackedWidget(view);
    stackedWidget->setSizePolicy(stackedWidget->sizePolicy().horizontalPolicy(), QSizePolicy::Maximum);
    stackedWidget->setDisabled(true);
    
    auto *layout = new QVBoxLayout();
    view->setLayout(layout);
    layout->addWidget(modeCombo);
    layout->addWidget(styleList);
    layout->addWidget(stackedWidget);
    layout->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
  }
  
  void loadFeatureData()
  {
    modeCombo->setCurrentIndex(static_cast<int>(command->feature->getMode()));
    for (const auto &e : command->feature->getEntries())
    {
      ChamferWidgetBase *widget = appendEntry(e);
      const auto &pl = view->project->getPayload(command->feature->getId());
      tls::Resolver resolver(pl);
      slc::Messages edgeAccumulator;
      for (const auto &p : e.edgePicks)
      {
        if (!resolver.resolve(p))
          continue;
        auto jfc = resolver.convertToMessages();
        edgeAccumulator.insert(edgeAccumulator.end(), jfc.begin(), jfc.end());
      }
      widget->selectionWidget->initializeButton(0, edgeAccumulator);
      if (e.style == ::ftr::Chamfer::Style::Symmetric)
        continue;
      slc::Messages faceAccumulator;
      for (const auto &p : e.facePicks)
      {
        if (!resolver.resolve(p))
          continue;
        auto jfc = resolver.convertToMessages();
        faceAccumulator.insert(faceAccumulator.end(), jfc.begin(), jfc.end());
      }
      widget->selectionWidget->initializeButton(1, faceAccumulator);
    }
  }
  
  void glue()
  {
    connect(modeCombo, SIGNAL(currentIndexChanged(int)), view, SLOT(modeChangedSlot(int)));
    connect(addSymmetric, &QAction::triggered, view, &Chamfer::appendSymmetricSlot);
    connect(addTwoDistances, &QAction::triggered, view, &Chamfer::appendTwoDistancesSlot);
    connect(addDistanceAngle, &QAction::triggered, view, &Chamfer::appendDistanceAngleSlot);
    connect(remove, &QAction::triggered, view, &Chamfer::removeSlot);
    connect(styleList, &QListWidget::itemSelectionChanged, view, &Chamfer::listSelectionChangedSlot);
    
    for (int index = 0; index < stackedWidget->count(); ++index)
    {
      auto *w = dynamic_cast<ChamferWidgetBase*>(stackedWidget->widget(index));
      assert(w);
      connect(w->selectionWidget->getButton(0), &dlg::SelectionButton::dirty, view, &Chamfer::selectionChangedSlot);
      if (w->selectionWidget->getButtonCount() > 1)
        connect(w->selectionWidget->getButton(1), &dlg::SelectionButton::dirty, view, &Chamfer::selectionChangedSlot);
    }
  }
  
  void modeChanged()
  {
    while (stackedWidget->count() > 0)
    {
      auto *w = stackedWidget->widget(0);
      stackedWidget->removeWidget(w);
      delete w;
    }
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
    command->setMode(modeCombo->currentIndex());
  }
  
  ChamferWidgetBase* appendEntry(const ftr::Chamfer::Entry &eIn)
  {
    if (eIn.style == ::ftr::Chamfer::Style::Symmetric)
    {
      styleList->addItem(tr("Symmetric"));
      SymmetricWidget *sw = new SymmetricWidget(eIn, stackedWidget);
      stackedWidget->addWidget(sw);
      connect(sw, &ParameterBase::prmValueChanged, view, &Chamfer::parameterChanged);
      return sw;
    }
    if (eIn.style == ::ftr::Chamfer::Style::TwoDistances)
    {
      styleList->addItem(tr("Two Distances"));
      TwoDistanceWidget *tdw = new TwoDistanceWidget(eIn, stackedWidget);
      stackedWidget->addWidget(tdw);
      connect(tdw, &ParameterBase::prmValueChanged, view, &Chamfer::parameterChanged);
      return tdw;
    }
    if (eIn.style == ::ftr::Chamfer::Style::DistanceAngle)
    {
      styleList->addItem(tr("Angle Distance"));
      DistanceAngleWidget *daw = new DistanceAngleWidget(eIn, stackedWidget);
      stackedWidget->addWidget(daw);
      connect(daw, &ParameterBase::prmValueChanged, view, &Chamfer::parameterChanged);
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
    auto *w = stackedWidget->widget(index);
    stackedWidget->removeWidget(w);
    delete w;
    command->feature->removeEntry(index);
  }
  
  void setSelection()
  {
    cmd::Chamfer::SelectionData data;
    slc::Messages empty;
    for (int index = 0; index < stackedWidget->count(); ++index)
    {
      auto *w = dynamic_cast<ChamferWidgetBase*>(stackedWidget->widget(index));
      assert(w);
      if (w->selectionWidget->getButtonCount() == 1)
      {
        auto sels = std::make_tuple(w->selectionWidget->getButton(0)->getMessages(), empty);
        data.push_back(sels);
      }
      else
      {
        auto sels = std::make_tuple(w->selectionWidget->getButton(0)->getMessages(), w->selectionWidget->getButton(1)->getMessages());
        data.push_back(sels);
      }
    }
    command->setSelectionData(data);
  }
};

Chamfer::Chamfer(cmd::Chamfer *cIn)
: Base("cmv::Chamfer")
, stow(new Stow(cIn, this))
{}

Chamfer::~Chamfer() = default;

void Chamfer::modeChangedSlot(int)
{
  stow->modeChanged();
  stow->command->localUpdate();
}

void Chamfer::appendSymmetricSlot()
{
  ChamferWidgetBase* widget = stow->appendEntry(stow->command->feature->getEntry(stow->command->feature->addSymmetric()));
  connect(widget->selectionWidget->getButton(0), &dlg::SelectionButton::dirty, this, &Chamfer::selectionChangedSlot);
  stow->styleList->setCurrentRow(stow->styleList->count() - 1, QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent);
  widget->activate(0);
}

void Chamfer::appendTwoDistancesSlot()
{
  ChamferWidgetBase* widget = stow->appendEntry(stow->command->feature->getEntry(stow->command->feature->addTwoDistances()));
  connect(widget->selectionWidget->getButton(0), &dlg::SelectionButton::dirty, this, &Chamfer::selectionChangedSlot);
  connect(widget->selectionWidget->getButton(1), &dlg::SelectionButton::dirty, this, &Chamfer::selectionChangedSlot);
  stow->styleList->setCurrentRow(stow->styleList->count() - 1, QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent);
  widget->activate(0);
}

void Chamfer::appendDistanceAngleSlot()
{
  ChamferWidgetBase* widget = stow->appendEntry(stow->command->feature->getEntry(stow->command->feature->addDistanceAngle()));
  connect(widget->selectionWidget->getButton(0), &dlg::SelectionButton::dirty, this, &Chamfer::selectionChangedSlot);
  connect(widget->selectionWidget->getButton(1), &dlg::SelectionButton::dirty, this, &Chamfer::selectionChangedSlot);
  stow->styleList->setCurrentRow(stow->styleList->count() - 1, QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent);
  widget->activate(0);
}

void Chamfer::removeSlot()
{
  stow->removeEntry();
  stow->command->localUpdate();
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
  
  ChamferWidgetBase* widget = dynamic_cast<ChamferWidgetBase*>(stow->stackedWidget->widget(index));
  assert(widget);
  widget->activate(0);
}

void Chamfer::selectionChangedSlot()
{
  stow->setSelection();
  stow->command->localUpdate();
}

void Chamfer::selectFirstStyleSlot()
{
  stow->styleList->setCurrentRow(0, QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent);
}

void Chamfer::parameterChanged()
{
  stow->command->localUpdate();
}
