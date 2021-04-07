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
#include <QStandardItemModel>
#include <QListWidget>
#include <QTableWidget>
#include <QTreeView>
#include <QHeaderView>
#include <QComboBox>
#include <QAction>
#include <QLabel>
#include <QLineEdit>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDragMoveEvent>
#include <QButtonGroup>
#include <QTimer>

#include <TopoDS.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "annex/annseershape.h"
#include "message/msgmessage.h"
#include "message/msgnode.h"
#include "dialogs/dlgselectionbutton.h"
#include "dialogs/dlgselectionlist.h"
#include "dialogs/dlgselectionwidget.h"
#include "commandview/cmvparameterwidgets.h"
#include "parameter/prmparameter.h"
#include "expressions/exprmanager.h"
#include "tools/featuretools.h"
#include "tools/idtools.h"
#include "dialogs/dlgsplitterdecorated.h"
#include "feature/ftrblend.h"
#include "command/cmdblend.h"
#include "commandview/cmvblend.h"

using boost::uuids::uuid;

namespace
{
  class ConstantPage : public QWidget
  {
  public:
    cmv::Blend *view; //to connect signals and slots
    cmv::ParameterTable *parameterTable = nullptr;
    QStackedWidget *stacked = nullptr;
    std::vector<dlg::SelectionWidget*> selectionWidgets;
    QAction *addParameterAction = nullptr;
    QAction *removeParameterAction = nullptr;
    
    explicit ConstantPage(cmv::Blend *viewIn)
    : QWidget(viewIn)
    , view(viewIn)
    {
      buildGui();
    }
    
    const std::vector<dlg::SelectionWidget*>& getSelectionWidgets(){return selectionWidgets;}
    
    void buildGui()
    {
      QVBoxLayout *layout = new QVBoxLayout();
      this->setLayout(layout);
      
      parameterTable = new cmv::ParameterTable(view);
      layout->addWidget(parameterTable);
      addParameterAction = new QAction(tr("Add Radius"), parameterTable);
      parameterTable->addAction(addParameterAction);
      connect(addParameterAction, &QAction::triggered, view, &cmv::Blend::addConstantBlend);
      removeParameterAction = new QAction(tr("Remove"), parameterTable);
      parameterTable->addAction(removeParameterAction);
      connect(removeParameterAction, &QAction::triggered, view, &cmv::Blend::removeConstantBlend);
      parameterTable->setContextMenuPolicy(Qt::ActionsContextMenu);
      connect(parameterTable, &cmv::ParameterTable::selectionHasChanged, this, &ConstantPage::selectionChanged);
      connect(parameterTable, &cmv::ParameterTable::prmValueChanged, view, &cmv::Blend::parameterChanged);
      
      stacked = new QStackedWidget(view);
      layout->addWidget(stacked);
    }
    
    void buildSelectionWidget()
    {
      dlg::SelectionWidgetCue cue;
      cue.name = tr("Edges");
      cue.singleSelection = false;
      cue.showAccrueColumn = false;
      cue.accrueDefault = slc::Accrue::Tangent;
      cue.mask = slc::EdgesEnabled | slc::EdgesSelectable;
      cue.statusPrompt = tr("Select Edges For Blend");
      dlg::SelectionWidget *csw = new dlg::SelectionWidget(this, {cue});
      csw->activate(0); //only 1 button per widget so just activate it.
      selectionWidgets.push_back(csw);
      stacked->addWidget(csw);
      connect(csw->getButton(0), &dlg::SelectionButton::dirty, view, &cmv::Blend::constantSelectionDirty);
    }
    
    void init(const std::vector<ftr::Blend::Constant> &bsIn, const ftr::UpdatePayload &plIn)
    {
      tls::Resolver resolver(plIn);
      for (const auto &b : bsIn)
      {
        auto *cpp = b.radius.get(); //current parameter pointer
        parameterTable->addParameter(cpp);
        
        buildSelectionWidget();
        dlg::SelectionWidget *csw = selectionWidgets.back();
        
        slc::Messages edgeAccumulator;
        for (const auto &p : b.picks)
        {
          if (!resolver.resolve(p))
            continue;
          auto jfc = resolver.convertToMessages();
          edgeAccumulator.insert(edgeAccumulator.end(), jfc.begin(), jfc.end());
        }
        csw->initializeButton(0, edgeAccumulator);
      }
    }
    
    void addConstantBlend(const ftr::Blend::Constant &sbIn)
    {
      buildSelectionWidget();
      parameterTable->addParameter(sbIn.radius.get());
    }
    
    void removeConstantBlend(int index)
    {
      parameterTable->removeParameter(index);
      
      auto it = selectionWidgets.begin() + index;
      selectionWidgets.erase(it);
      delete *it;
      
      if (!selectionWidgets.empty())
        parameterTable->setCurrentSelectedIndex(0);
    }
    
    int getCurrentSelectionIndex()
    {
      return parameterTable->getCurrentSelectedIndex();
    }
    
    void selectionChanged()
    {
      int index = parameterTable->getCurrentSelectedIndex();
      if (index >= 0 && index < static_cast<int>(selectionWidgets.size()))
      {
        stacked->setEnabled(true);
        stacked->setCurrentIndex(index);
      }
      else
      {
        stacked->setEnabled(false);
      }
    }
    
    std::vector<slc::Messages> getAllSelections()
    {
      std::vector<slc::Messages> out;
      for (const auto *w : selectionWidgets)
      {
        const auto &msgs = w->getButton(0)->getMessages();
        out.push_back(msgs);
      }
      return out;
    }
    
    void clear() //removes all selections and parameters.
    {
      while (!selectionWidgets.empty())
        removeConstantBlend(0);
    }
  };

  class PointPage : public QWidget
  {
  public:
    cmv::Blend *view = nullptr;
    QButtonGroup *group = nullptr;
    dlg::SelectionWidget *pointSelections = nullptr;
    QStackedWidget *parameterStacked = nullptr;
    std::vector<std::vector<std::shared_ptr<prm::Parameter>>> parameters;
    
    PointPage(cmv::Blend *parent, QButtonGroup *groupIn)
    : QWidget(parent)
    , view(parent)
    , group(groupIn)
    {
      buildGui();
    }
    
    void buildGui()
    {
      setContentsMargins(0, 0, 0, 0);
      QVBoxLayout *layout = new QVBoxLayout();
      layout->setContentsMargins(0, 0, 0, 0);
      setLayout(layout);
      setSizePolicy(sizePolicy().horizontalPolicy(), QSizePolicy::Maximum);
      
      dlg::SelectionWidgetCue cue;
      std::vector<dlg::SelectionWidgetCue> cues;
      
      cue.name = tr("Points");
      cue.singleSelection = false;
      cue.showAccrueColumn = false;
      cue.accrueDefault = slc::Accrue::Tangent;
      cue.mask = slc::None | slc::PointsEnabled | slc::PointsSelectable  | slc::EndPointsEnabled  | slc::EndPointsSelectable | slc::NearestPointsEnabled | slc::MidPointsEnabled;
      cue.statusPrompt = tr("Select Point to assign radius");
      cues.push_back(cue);
      pointSelections = new dlg::SelectionWidget(this, cues, group);
      connect(pointSelections->getView(0), &dlg::SelectionView::dirty, this, &PointPage::pointSelectionChanged);
      layout->addWidget(pointSelections);
      
      parameterStacked = new QStackedWidget(this);
      parameterStacked->setEnabled(false);
      parameterStacked->setSizePolicy(parameterStacked->sizePolicy().horizontalPolicy(), QSizePolicy::Maximum);
      layout->addWidget(parameterStacked);
    }
    
    void addSelection(const slc::Message &mIn)
    {
      pointSelections->getButton(0)->addMessage(mIn);
    }
    
    void addSelection(const slc::Messages &msIn)
    {
      for (const auto &m : msIn)
        addSelection(m);
    }
    
    int addParameterPage(const std::vector<std::shared_ptr<prm::Parameter>> &paras)
    {
      parameters.push_back(paras);
      auto *pt = new cmv::ParameterTable(this);
      for (auto p : parameters.back())
        pt->addParameter(p.get());
      parameterStacked->addWidget(pt);
      connect(pt, &cmv::ParameterBase::prmValueChanged, view, &cmv::Blend::parameterChanged);
      return parameterStacked->count() - 1;
    }
    
    void removeParameterPage(int index)
    {
      assert(index >= 0 && index < parameterStacked->count());
      assert(index < static_cast<int>(parameters.size()));
      delete parameterStacked->widget(index);
      parameters.erase(parameters.begin() + index);
      assert(static_cast<int>(parameters.size()) == parameterStacked->count());
    }
    
    void pointSelectionChanged() //called when the selection inside qt view changes
    {
      int pointIndex = pointSelections->getView(0)->getSelectedIndex();
      assert(pointIndex < parameterStacked->count());
      if (pointIndex < 0)
      {
        parameterStacked->setEnabled(false);
        return;
      }
      parameterStacked->setEnabled(true);
      parameterStacked->setCurrentIndex(pointIndex);
    }
    
    void clear()
    {
      while (parameterStacked->widget(0))
        delete parameterStacked->widget(0);
      pointSelections->getButton(0)->clear();
      parameters.clear();
    }
  };

  class VariablePage : public QWidget
  {
  public:
    cmv::Blend *view; //to connect signals and slots
    dlg::SelectionWidget *edgeSelections = nullptr;
    PointPage *pointPage = nullptr;
    QButtonGroup *buttonGroup = nullptr;
    QPushButton *dummyButton = nullptr; //hidden button to check to uncheck all other buttons.
    
    explicit VariablePage(cmv::Blend *viewIn)
    : QWidget(viewIn)
    , view(viewIn)
    {
      buildGui();
      glue();
    }
    
    void buildGui()
    {
      setContentsMargins(0, 0, 0, 0);
      QVBoxLayout *layout = new QVBoxLayout();
      layout->setContentsMargins(0, 0, 0, 0);
      setLayout(layout);
      setSizePolicy(sizePolicy().horizontalPolicy(), QSizePolicy::Maximum);
      
      buttonGroup = new QButtonGroup(this);
      dummyButton = new QPushButton(this);
      dummyButton->setCheckable(true);
      dummyButton->setVisible(false);
      buttonGroup->addButton(dummyButton);
      
      dlg::SelectionWidgetCue cue;
      
      cue.name = tr("Edges");
      cue.singleSelection = false;
      cue.showAccrueColumn = false;
      cue.accrueDefault = slc::Accrue::Tangent;
      cue.mask = slc::EdgesEnabled | slc::EdgesSelectable;
      cue.statusPrompt = tr("Select Edge For Variable Blend");
      edgeSelections = new dlg::SelectionWidget(this, {cue}, buttonGroup);
      edgeSelections->setSizePolicy(edgeSelections->sizePolicy().horizontalPolicy(), QSizePolicy::Maximum);
      layout->addWidget(edgeSelections);
      
      pointPage = new PointPage(view, buttonGroup);
      pointPage->setEnabled(false);
      layout->addWidget(pointPage);
      
      layout->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    }
    
    void init(const ftr::Blend::Variable &vbIn, const ftr::UpdatePayload &plIn)
    {
      tls::Resolver resolver(plIn);
      
      slc::Messages edgeMessages;
      for (const auto &p : vbIn.picks)
      {
        if (!resolver.resolve(p))
          continue;
        slc::Messages temp = resolver.convertToMessages();
        edgeMessages.insert(edgeMessages.end(), temp.begin(), temp.end());
      }
      edgeSelections->initializeButton(0, edgeMessages);
      edgeSelections->activate(0);
      
      slc::Messages pointMessages;
      for (const auto &entry : vbIn.entries)
      {
        if (!resolver.resolve(entry.pick))
          continue; //warning?
        slc::Messages temp = resolver.convertToMessages();
        if (temp.empty())
          continue; //warning?
        pointMessages.push_back(temp.front());
        
        std::vector<std::shared_ptr<prm::Parameter>> params;
        params.push_back(entry.radius);
        if (temp.front().type != slc::Type::StartPoint && temp.front().type != slc::Type::EndPoint)
        {
          params.push_back(entry.position); //unused for vertices but build anyway.
        }
        pointPage->addParameterPage(params);
      }
      pointPage->pointSelections->initializeButton(0, pointMessages);
    }
    
    void glue()
    {
      connect(edgeSelections->getView(0), &dlg::SelectionView::dirty, this, &VariablePage::edgeSelectionChanged);
      connect(edgeSelections->getButton(0), &dlg::SelectionButton::toggled, this, &VariablePage::edgeButtonToggled);
      connect(edgeSelections->getButton(0), &dlg::SelectionButton::selectionAdded, view, &cmv::Blend::variableEdgeAdded);
      connect(edgeSelections->getButton(0), &dlg::SelectionButton::selectionRemoved, view, &cmv::Blend::variableEdgeRemoved);
      connect(pointPage->pointSelections->getButton(0), &dlg::SelectionButton::selectionAdded, view, &cmv::Blend::variablePointAdded);
      connect(pointPage->pointSelections->getButton(0), &dlg::SelectionButton::selectionRemoved, view, &cmv::Blend::variablePointRemoved);
    }
    
    void uncheckAll()
    {
      dummyButton->setChecked(true);
    }
    
    void edgeSelectionChanged() //called when the selection inside qt view changes
    {
      int edgeIndex = edgeSelections->getView(0)->getSelectedIndex();
      if (edgeIndex < 0)
      {
        pointPage->setEnabled(false);
        return;
      }
      pointPage->setEnabled(true);
      //we could activate the button on the point page but then the user
      //can selected the individual edges for a visual of what he wants
      //to edit. We will only auto active on a new contour being added.
      pointPage->parameterStacked->setEnabled(false);
    }
    
    void edgeButtonToggled(bool state)
    {
      if (state)
        pointPage->setEnabled(false);
    }
    
    void clear()
    {
      edgeSelections->getButton(0)->clear();
      pointPage->setEnabled(false);
      pointPage->clear();
    }
  };
}

using namespace cmv;

struct Blend::Stow
{
  cmd::Blend *command;
  cmv::Blend *view;
  
  QComboBox *typeCombo;
  QComboBox *shapeCombo;
  ConstantPage *cPage;
  VariablePage *vPage;
  QStackedWidget *stacked;
//   QStandardItemModel *cModel;
//   QTreeView *cView;
//   VariableWidget *vWidget;
  
  Stow(cmd::Blend *cIn, cmv::Blend *vIn)
  : command(cIn)
  , view(vIn)
  {
    buildGui();
    
    QSettings &settings = app::instance()->getUserSettings();
    settings.beginGroup("cmv::Blend");
    //load settings
    settings.endGroup();
    
    loadFeatureData();
    glue();
//     selectionWidget->activate(0);
  }
  
  void buildGui()
  {
    QVBoxLayout *mainLayout = new QVBoxLayout();
    view->setLayout(mainLayout);
    
    typeCombo = new QComboBox(view);
    typeCombo->addItem(tr("Constant"));
    typeCombo->addItem(tr("Variable"));
    shapeCombo = new QComboBox(view);
    shapeCombo->addItem(tr("Rational"));
    shapeCombo->addItem(tr("QuasiAngular"));
    shapeCombo->addItem(tr("Polynomial"));
    QLabel *typeLabel = new QLabel(tr("Type:"), view);
    QLabel *shapeLabel = new QLabel(tr("Shape:"), view);
    QHBoxLayout *chl = new QHBoxLayout(); //combo horizontal layout.
    chl->addWidget(typeLabel);
    chl->addWidget(typeCombo);
    chl->addStretch();
    chl->addWidget(shapeLabel);
    chl->addWidget(shapeCombo);
    mainLayout->addLayout(chl);
    
    stacked = new QStackedWidget(view);
    stacked->setSizePolicy(stacked->sizePolicy().horizontalPolicy(), QSizePolicy::Maximum);
    
    cPage = new ConstantPage(view);
    stacked->addWidget(cPage);
    
    vPage = new VariablePage(view);
    stacked->addWidget(vPage);
    
    mainLayout->addWidget(stacked);
    mainLayout->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
  }
  
  void loadFeatureData()
  {
    if (command->feature->getBlendType() == ftr::Blend::BlendType::Constant)
    {
      typeCombo->setCurrentIndex(0);
      stacked->setCurrentIndex(0);
      
      //build a default blend if there is none.
      if (command->feature->getConstantBlends().empty())
        command->feature->addConstantBlend(ftr::Blend::Constant());
      
      cPage->init(command->feature->getConstantBlends(), view->project->getPayload(command->feature->getId()));
    }
    else 
    {
      typeCombo->setCurrentIndex(1);
      stacked->setCurrentIndex(1);
      if (command->feature->getVariableBlend())
        vPage->init(*command->feature->getVariableBlend(), view->project->getPayload(command->feature->getId()));
    }
  }
  
  void glue()
  {
    connect(typeCombo, qOverload<int>(&QComboBox::currentIndexChanged), view, &Blend::typeChanged);
  }
  
  std::vector<ftr::Blend::VariableEntry> buildVariableEntries()
  {
    std::vector<ftr::Blend::VariableEntry> out;
    
    const auto &ems = vPage->edgeSelections->getButton(0)->getMessages();
    if (ems.empty())
      return out;
    auto inputFeatureId = ems.front().featureId;
    assert(view->project->hasFeature(inputFeatureId));
    const auto &inputFeature = *view->project->findFeature(inputFeatureId);
    assert(inputFeature.hasAnnex(ann::Type::SeerShape) && !inputFeature.getAnnex<ann::SeerShape>().isNull());
    
    int edgeIndex = -1;
    assert(vPage->pointPage->parameters.size() == vPage->pointPage->pointSelections->getButton(0)->getMessages().size());
    for (const auto &ps : vPage->pointPage->pointSelections->getButton(0)->getMessages())
    {
      edgeIndex++;
      if (ps.featureId != inputFeatureId)
      {
        view->node->sendBlocked(msg::buildStatusMessage(QObject::tr("Variable entry from another body skipped").toStdString(), 2.0));
        continue;
      }
      ftr::Blend::VariableEntry ce;
      ce.pick = tls::convertToPick(ps, inputFeature, view->project->getShapeHistory());
      ce.radius = vPage->pointPage->parameters.at(edgeIndex).at(0);
      if (vPage->pointPage->parameters.at(edgeIndex).size() > 1)
      {
        ce.position = vPage->pointPage->parameters.at(edgeIndex).at(1);
        ce.pick.u = static_cast<double>(*ce.position);
      }
      else
        ce.position = ftr::Blend::buildPositionParameter();
      out.push_back(ce);
    }
    
    return out;
  }
  
  void goLocalUpdate()
  {
    if (typeCombo->currentIndex() == 1)
    {
      const auto &edgeMsgs = vPage->edgeSelections->getButton(0)->getMessages();
      auto ves = buildVariableEntries();
      command->setVariableSelections(std::make_tuple(edgeMsgs, ves));
    }
    
    command->localUpdate();
  };
};

Blend::Blend(cmd::Blend *cIn)
: Base("cmv::Blend")
, stow(new Stow(cIn, this))
{}

Blend::~Blend() = default;

void Blend::addConstantBlend()
{
  assert(stow->typeCombo->currentIndex() == 0);
  stow->command->feature->addConstantBlend(ftr::Blend::Constant());
  const auto &sb = stow->command->feature->getConstantBlends().back();
  stow->cPage->addConstantBlend(sb);
//   stow->goLocalUpdate(); //shouldn't need to update until something is actually selected
}

void Blend::removeConstantBlend()
{
  assert(stow->typeCombo->currentIndex() == 0);
  int selectedIndex = stow->cPage->getCurrentSelectionIndex();
  if (selectedIndex == -1)
    return;
  assert(selectedIndex >= 0 && selectedIndex < static_cast<int>(stow->command->feature->getConstantBlends().size()));
  stow->command->feature->removeConstantBlend(selectedIndex);
  stow->cPage->removeConstantBlend(selectedIndex);
  stow->goLocalUpdate();
}

void Blend::constantSelectionDirty()
{
  assert(stow->typeCombo->currentIndex() == 0);
  stow->command->setConstantSelections(stow->cPage->getAllSelections());
  stow->goLocalUpdate();
}

void Blend::parameterChanged()
{
  stow->goLocalUpdate();
  
  //when we change a position parameter, the point on screen does not get updated.
  //there is currently no support inside selection button for updating the message.
  //ftr::Pick has a static method to convert edge parameters to osg::Vec3d
}

void Blend::typeChanged(int index)
{
  stow->stacked->setCurrentIndex(index);
  stow->command->setType(index);
  if (index == 0)
  {
    stow->vPage->clear();
    stow->cPage->parameterTable->setCurrentSelectedIndex(index);
  }
  else if (index == 1)
  {
    stow->cPage->clear();
    stow->vPage->edgeSelections->activate(0);
  }
  stow->goLocalUpdate();
}

void Blend::variableEdgeAdded(int index)
{
  const auto &edgeMsgs = stow->vPage->edgeSelections->getButton(0)->getMessages();
  assert(index >= 0 && index < static_cast<int>(edgeMsgs.size()));
  const auto *f = project->findFeature(edgeMsgs.at(index).featureId);
  assert(f);
  assert(f->hasAnnex(ann::Type::SeerShape) && !f->getAnnex<ann::SeerShape>().isNull());
  const auto &ss = f->getAnnex<ann::SeerShape>();
  auto vertIds = ftr::Blend::getSpineEnds(ss, edgeMsgs.at(index).shapeId);
  std::optional<int> pointIndex;
  for (const auto &id : vertIds)
  {
    //getSpineEnds tests for duplicate vertices(periodic).
    //So we don't have any duplicates there but we could be
    //duplicating a vertex from a previous edge selection.
    //dlg::SelectionButton doesn't allow duplicate selections
    //so we will take the size of the messages before, add the message
    //take the size again and compare to determine if we need to add
    //a new parameter page. This will keep the number of point
    //selections in sync with the parameter pages.
    //another goofy thing is how the selection of start/end/vertex points
    //is handled. They are fictitious like other points so we select them
    //from an edge. Going from a vertex to a selection point is cumbersome,
    //so we will build a pick and use feature tools to do this.
    
    ftr::Pick dummy;
    dummy.selectionType = slc::Type::StartPoint;
    dummy.shapeHistory = project->getShapeHistory().createDevolveHistory(id);
    auto convertedMessages = tls::convertToMessage(dummy, f);
    if (convertedMessages.empty())
    {
      node->sendBlocked(msg::buildStatusMessage(QObject::tr("Couldn't convert pick to message in Blend::variableEdgeAdded").toStdString(), 2.0));
      continue;
    }
    
    std::size_t sizeBefore = stow->vPage->pointPage->pointSelections->getButton(0)->getMessages().size();
    stow->vPage->pointPage->addSelection(convertedMessages.front());
    if (sizeBefore == stow->vPage->pointPage->pointSelections->getButton(0)->getMessages().size())
      continue; //selection was filtered out.
    
    std::vector<std::shared_ptr<prm::Parameter>> freshParameters;
    freshParameters.push_back(ftr::Blend::buildRadiusParameter());
    if (convertedMessages.front().type != slc::Type::StartPoint && convertedMessages.front().type != slc::Type::EndPoint)
    {
      freshParameters.push_back(ftr::Blend::buildPositionParameter()); //unused for vertices but build anyway.
    }
    int pi = stow->vPage->pointPage->addParameterPage(freshParameters);
    if (!pointIndex)
      pointIndex = pi;
  }
  stow->goLocalUpdate();
  stow->vPage->edgeSelections->getView(0)->setSelectedIndex(index);
  stow->vPage->pointPage->pointSelections->activate(0);
  if (pointIndex)
  {
    //have to use timer because activate above is queued also.
    int wtf = *pointIndex; //so I can capture by copy.
    QTimer::singleShot(0, [this, wtf](){stow->vPage->pointPage->pointSelections->getView(0)->setSelectedIndex(wtf);});
  }
}

void Blend::variableEdgeRemoved(int)
{
  const auto &edgeMsgs = stow->vPage->edgeSelections->getButton(0)->getMessages();
  if (edgeMsgs.empty())
  {
    stow->vPage->pointPage->clear();
    stow->goLocalUpdate();
    return;
  }
  
  //we have to reconcile constraint points with edges.
  const auto *iFeature = project->findFeature(edgeMsgs.at(0).featureId);
  assert(iFeature);
  assert(iFeature->hasAnnex(ann::Type::SeerShape) && !iFeature->getAnnex<ann::SeerShape>().isNull());
  const auto &ss = iFeature->getAnnex<ann::SeerShape>();

  BRepFilletAPI_MakeFillet blendMaker(ss.getRootOCCTShape());
  for (const auto &em : edgeMsgs)
  {
    if (em.featureId != iFeature->getId())
    {
      //we should be removing the edge, but I plan on restricting selection to prevent this situation.
      node->sendBlocked(msg::buildStatusMessage(QObject::tr("Skipping edge not on solid").toStdString(), 2.0));
      continue;
    }
    blendMaker.Add(TopoDS::Edge(ss.getOCCTShape(em.shapeId)));
  }

  auto isValidConstraint = [&](const slc::Message &cm) -> bool
  {
    for (int index = 1; index < blendMaker.NbContours() + 1; ++index)
    {
      assert(ss.hasId(cm.shapeId));
      if (cm.type == slc::Type::StartPoint || cm.type == slc::Type::EndPoint)
      {
        TopoDS_Vertex v;
        if (cm.type == slc::Type::StartPoint)
          v = TopoDS::Vertex(ss.getOCCTShape(ss.useGetStartVertex(cm.shapeId)));
        else
          v = TopoDS::Vertex(ss.getOCCTShape(ss.useGetEndVertex(cm.shapeId)));
        double p = blendMaker.RelativeAbscissa(index, v);
        if (!(p < 0.0))
          return true;
      }
      else if (cm.type == slc::Type::MidPoint || cm.type == slc::Type::NearestPoint)
      {
        if (blendMaker.Contour(TopoDS::Edge(ss.getOCCTShape(cm.shapeId))) != 0)
          return true;
      }
    }
    return false;
  };  
  
  std::vector<int> indexesToRemove;
  int index = -1;
  for (const auto &pm : stow->vPage->pointPage->pointSelections->getButton(0)->getMessages())
  {
    index++;
    if (pm.featureId != iFeature->getId())
    {
      indexesToRemove.push_back(index);
      node->sendBlocked(msg::buildStatusMessage(QObject::tr("Removing constraint not on solid").toStdString(), 2.0));
      continue;
    }
    if (!isValidConstraint(pm))
      indexesToRemove.push_back(index);
  }
  std::reverse(indexesToRemove.begin(), indexesToRemove.end());
  
  for (auto itr : indexesToRemove)
  {
    stow->vPage->pointPage->pointSelections->getButton(0)->removeMessage(itr);
    stow->vPage->pointPage->removeParameterPage(itr);
  }
  
  stow->goLocalUpdate();
}

/* We have a fundamental problem. We have 'pick.u' describing position of a point.
 * Then we have the position parameter describing same point. The parameter should
 * be master after it is set from the initial pick. We have no method to keep the
 * visual point synced with the parameter.
 */
void Blend::variablePointAdded(int pointIndex)
{
  std::vector<std::shared_ptr<prm::Parameter>> freshParameters;
  freshParameters.push_back(ftr::Blend::buildRadiusParameter());
  const slc::Message &sm = stow->vPage->pointPage->pointSelections->getButton(0)->getMessages().at(pointIndex);
  if (sm.type != slc::Type::StartPoint && sm.type != slc::Type::EndPoint)
  {
    freshParameters.push_back(ftr::Blend::buildPositionParameter());
    
    //set the parameter to the point.
    assert(project->hasFeature(sm.featureId));
    const ftr::Base &inputFeature = *project->findFeature(sm.featureId);
    assert(inputFeature.hasAnnex(ann::Type::SeerShape));
    const ann::SeerShape &ss = inputFeature.getAnnex<ann::SeerShape>();
    assert(!ss.isNull());
    assert(ss.hasId(sm.shapeId));
    const TopoDS_Shape &es = ss.getOCCTShape(sm.shapeId);
    if (es.ShapeType() == TopAbs_EDGE)
      freshParameters.back()->setValue(ftr::Pick::parameter(TopoDS::Edge(es), sm.pointLocation));
  }
  int index = stow->vPage->pointPage->addParameterPage(freshParameters);
  
  stow->goLocalUpdate();
  
  stow->vPage->pointPage->pointSelections->getView(0)->setSelectedIndex(index);
}

void Blend::variablePointRemoved(int index)
{
  stow->vPage->pointPage->removeParameterPage(index);
  stow->goLocalUpdate();
}
