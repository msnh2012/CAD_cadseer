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
#include <QListWidget>
#include <QAction>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QTimer>

#include <TopoDS.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "annex/annseershape.h"
#include "message/msgmessage.h"
#include "message/msgnode.h"
#include "message/msgsift.h"
#include "commandview/cmvselectioncue.h"
#include "commandview/cmvtable.h"
#include "commandview/cmvtablelist.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "tools/featuretools.h"
#include "dialogs/dlgsplitterdecorated.h"
#include "feature/ftrblend.h"
#include "command/cmdblend.h"
#include "commandview/cmvblend.h"

using boost::uuids::uuid;

namespace
{
  TopoDS_Edge getEdge(const slc::Message &pm)
  {
    auto *inputFeature = app::instance()->getProject()->findFeature(pm.featureId);
    assert(inputFeature);
    const auto &iss = inputFeature->getAnnex<ann::SeerShape>();
    assert(iss.hasId(pm.shapeId));
    return TopoDS::Edge(iss.getOCCTShape(pm.shapeId));
  }
  
  class VariablePage : public QWidget
  {
  public:
    cmv::Blend *view;
    ftr::Blend::Feature &feature;
    cmv::tbl::Model *mainModel = nullptr; //just has contour picks
    cmv::tbl::View *mainView = nullptr;
    dlg::SplitterDecorated *vSplitter;
    cmv::TableList *entryList = nullptr;
    QAction *removeEntry = nullptr;
    
    VariablePage(cmv::Blend *viewIn, ftr::Blend::Feature &fIn)
    : QWidget(viewIn)
    , view(viewIn)
    , feature(fIn)
    {
      buildGui();
      loadFeatureData();
      glue();
    }
    
    void buildGui()
    {
      QVBoxLayout *layout = new QVBoxLayout();
      setLayout(layout);
      view->clearContentMargins(this);
      
      prm::Parameters mainPrms = {&feature.getVariable().contourPicks};
      mainModel = new cmv::tbl::Model(view, &feature, mainPrms);
      mainView = new cmv::tbl::View(view, mainModel, true);
      cmv::tbl::SelectionCue cue;
      cue.singleSelection = false;
      cue.mask = slc::EdgesBoth;
      cue.statusPrompt = tr("Select Edges For Variable Blend");
      cue.forceTangentAccrue = true;
      mainModel->setCue(mainPrms.at(0), cue);
      
      entryList = new cmv::TableList(view, &feature);
      entryList->restoreSettings("cmv::Blend::entryList");
      removeEntry = new QAction(tr("Remove"), entryList);
      entryList->getListWidget()->addAction(removeEntry);
      
      vSplitter = new dlg::SplitterDecorated(this);
      vSplitter->setOrientation(Qt::Vertical);
      vSplitter->setChildrenCollapsible(false);
      vSplitter->addWidget(mainView);
      vSplitter->addWidget(entryList);
      vSplitter->restoreSettings("cmv::Blend::vSplitter");
      layout->addWidget(vSplitter);
    }
    
    void loadFeatureData()
    {
      entryList->clear();
      for (auto &e : feature.getVariable().entries)
        appendEntry(e);
      //reconcile?
    }
    
    cmv::tbl::Model* appendEntry(ftr::Blend::Entry &eIn)
    {
      auto tempPrms = eIn.getParameters();
      int index = entryList->add(tempPrms.front()->getName(), tempPrms);
      auto *pModel = entryList->getPrmModel(index); assert(pModel);
      auto *pView = entryList->getPrmView(index); assert(pView);
      cmv::tbl::SelectionCue cue;
      cue.singleSelection = true;
      cue.mask = slc::EndPointsBoth | slc::NearestPointsEnabled | slc::MidPointsEnabled; //edges some day.
      cue.statusPrompt = tr("Select Point");
      cue.accrueEnabled = false;
      pModel->setCue(tempPrms.at(0), cue);
      
      connect(pModel, &cmv::tbl::Model::dataChanged, view, &cmv::Blend::variableEntriesChanged);
      connect(pView, &cmv::tbl::View::openingPersistent, view, &cmv::Blend::closeAllPersistent);
      pView->installEventFilter(view);
      
      return pModel;
    }
    
    void glue()
    {
      connect(mainModel, &cmv::tbl::Model::dataChanged, view, &cmv::Blend::variableContoursChanged);
      connect(mainView, &cmv::tbl::View::openingPersistent, view, &cmv::Blend::closeAllPersistent);
      connect(entryList->getListWidget(), &QListWidget::itemSelectionChanged, view, &cmv::Blend::entriesSelectionChanged);
      connect(removeEntry, &QAction::triggered, view, &cmv::Blend::removeEntry);
      mainView->installEventFilter(view);
      entryList->getListWidget()->installEventFilter(view);
    }
    
    void closePersistent()
    {
      mainView->closePersistent();
      entryList->closePersistent();
    }
    
    void clear()
    {
      //do we need to clear message for picks?
      entryList->clear();
    }
    
    std::vector<slc::Messages> getSelections()
    {
      std::vector<slc::Messages> out;
      out.push_back(mainModel->getMessages(mainModel->index(0, 1)));
      for (int i = 0; i < entryList->count(); ++i)
        out.push_back(entryList->getMessages(i));
      return out;
    }
    
    slc::Messages getContourEnds(const slc::Message &em)
    {
      assert(em.type == slc::Type::Edge);
      prj::Project *project = app::instance()->getProject(); assert(project);
      ftr::Base *fIn = project->findFeature(em.featureId); assert(fIn);
      assert(fIn->hasAnnex(ann::Type::SeerShape));
      const ann::SeerShape &ssIn = fIn->getAnnex<ann::SeerShape>();
      
      const TopoDS_Shape &rootShape = ssIn.getRootOCCTShape();
      BRepFilletAPI_MakeFillet blendMaker(rootShape);
      blendMaker.Add(TopoDS::Edge(ssIn.getOCCTShape(em.shapeId)));
      
      //using set to ensure unique entries in case of periodic spine.
      std::set<uuid> ids;
      ids.insert(ssIn.findId(blendMaker.FirstVertex(1)));
      ids.insert(ssIn.findId(blendMaker.LastVertex(1)));
      
      slc::Messages out;
      for (const auto &id : ids)
      {
        ftr::Pick dummy;
        dummy.selectionType = slc::Type::StartPoint;
        dummy.shapeHistory = project->getShapeHistory().createDevolveHistory(id);
        auto convertedMessages = tls::convertToMessage(dummy, fIn);
        if (convertedMessages.empty())
          continue;
        out.push_back(convertedMessages.front()); //what if more than one?
      }
      return out;
    }
    
    TopoDS_Vertex getVertex(const slc::Message &mIn)
    {
      assert(mIn.type == slc::Type::StartPoint || mIn.type == slc::Type::EndPoint);
      
      prj::Project *project = app::instance()->getProject(); assert(project);
      ftr::Base *fIn = project->findFeature(mIn.featureId); assert(fIn);
      assert(fIn->hasAnnex(ann::Type::SeerShape));
      const ann::SeerShape &ssIn = fIn->getAnnex<ann::SeerShape>();
      assert(ssIn.hasId(mIn.shapeId));
      
      if (mIn.type == slc::Type::StartPoint)
        return TopoDS::Vertex(ssIn.getOCCTShape(ssIn.useGetStartVertex(mIn.shapeId)));
      else if (mIn.type == slc::Type::EndPoint)
        return TopoDS::Vertex(ssIn.getOCCTShape(ssIn.useGetEndVertex(mIn.shapeId)));
      
      return TopoDS_Vertex();
    }
    
    occt::VertexVector getAllVertices()
    {
      occt::VertexVector out;
      
      for (int index = 0; index < entryList->count(); ++index)
      {
        auto msgs = entryList->getMessages(index);
        if (msgs.empty())
          continue;
        //should be single selection.
        if (msgs.front().type == slc::Type::StartPoint || msgs.front().type == slc::Type::EndPoint)
          out.push_back(getVertex(msgs.front()));
      }
      
      return out;
    }
    
    //check whether we already have this selection.
    bool isUnique(const slc::Message &mIn)
    {
      for (int index = 0; index < entryList->count(); ++index)
      {
        auto msgs = entryList->getMessages(index);
        if (msgs.empty())
          continue;
        //should be single selection.
        if (msgs.front() == mIn)
          return false;
      }
      //the same vertex can be specified from different endpoint selections.
      if (mIn.type == slc::Type::StartPoint || mIn.type == slc::Type::EndPoint)
      {
        auto vert = getVertex(mIn);
        for (const auto &v : getAllVertices())
        {
          if (vert.IsSame(v))
            return false;
        }
      }
      return true;
    }
    
    //makes sure we don't have duplicate vertices.
    //and make sure all points belong to a spine.
    //are going to enforce the spine have vertex ends defined.
    //if not we can't automatically add endpoints because we don't know which is new or old.
    //maybe when we add from global selection we add endpoints but not when we edit from table?
    void reconcile()
    {
      slc::Messages contoursToRemove; //contours must be on same feature/body
      std::vector<int> entriesToRemove;
      const auto cslcs = mainModel->getMessages(mainModel->index(0, 1));
      
      if (cslcs.empty())
      {
        //remove all entries
        for (int i = 0; i < entryList->count(); ++i)
          entriesToRemove.push_back(i);
      }
      else
      {
        prj::Project *project = app::instance()->getProject(); assert(project);
        ftr::Base *fIn = project->findFeature(cslcs.front().featureId); assert(fIn);
        assert(fIn->hasAnnex(ann::Type::SeerShape));
        const ann::SeerShape &ssIn = fIn->getAnnex<ann::SeerShape>();
        const TopoDS_Shape &rootShape = ssIn.getRootOCCTShape();
        
        BRepFilletAPI_MakeFillet blendMaker(rootShape);
        for (const auto &c : cslcs)
        {
          if (c.featureId != cslcs.front().featureId)
          {
            contoursToRemove.push_back(c);
            continue;
          }
          blendMaker.Add(TopoDS::Edge(ssIn.getOCCTShape(c.shapeId)));
        }
        
        occt::VertexVector vertices;
        auto haveVertex = [&](const TopoDS_Vertex &vIn) -> bool
        {
          for (const auto &cv : vertices)
          {
            if (cv.IsSame(vIn))
              return true;
          }
          vertices.push_back(vIn);
          return false;
        };
        for (int index = 0; index < entryList->count(); ++index)
        {
          const auto &entrySelections = entryList->getMessages(index);
          if (entrySelections.empty()) //only 0 or 1 selection.
            continue;
          const auto &s = entrySelections.front();
          
          auto isValidVertex = [&](const TopoDS_Vertex &vIn) -> bool
          {
            if (haveVertex(vIn))
              return false;
            for (int ci = 1; ci <= blendMaker.NbContours(); ++ci)
            {
              double p = blendMaker.RelativeAbscissa(ci, vIn);
              if (!(p < 0.0))
                return true;
            }
            return false;
          };
          
          if (s.type == slc::Type::StartPoint || s.type == slc::Type::EndPoint)
          {
            if (!isValidVertex(getVertex(s)))
              entriesToRemove.push_back(index);
          }
          else if (s.type == slc::Type::MidPoint || s.type == slc::Type::NearestPoint)
          {
            if (blendMaker.Contour(TopoDS::Edge(ssIn.getOCCTShape(s.shapeId))) == 0)
              entriesToRemove.push_back(index);
          }
        }
      }
      
      if (!contoursToRemove.empty())
      {
        slc::Messages currentContours = cslcs;
        for (const auto &ctr : contoursToRemove)
          slc::remove(currentContours, ctr);
        mainModel->setMessages(mainModel->getParameter(mainModel->index(0, 1)), currentContours);
      }
      
      if (!entriesToRemove.empty())
      {
        //reverse order so as not to screw up indexes by removing.
        std::reverse(entriesToRemove.begin(), entriesToRemove.end());
        for (int ei : entriesToRemove)
        {
          entryList->remove(ei);
          feature.removeVariableEntry(ei);
        }
      }
    }
    
    void addEntry(const slc::Message &pm) //point message
    {
      assert
      (
        pm.type == slc::Type::StartPoint
        || pm.type == slc::Type::EndPoint
        || pm.type == slc::Type::MidPoint
        || pm.type == slc::Type::NearestPoint
      );
      
      if (isUnique(pm))
      {
        auto &ftrEntry = feature.addVariableEntry();
        auto *freshModel = appendEntry(ftrEntry);
        freshModel->setMessages(&ftrEntry.entryPick, {pm});
        if (pm.type == slc::Type::NearestPoint)
        {
          //we want to set the position parameter to the selected point.
          double param = ftr::Pick::parameter(getEdge(pm), pm.pointLocation);
          ftrEntry.position.setValue(param);
        }
        else if (pm.type == slc::Type::MidPoint)
          ftrEntry.position.setValue(0.5);
      }
    }
    
    void addContour(const slc::Message &em) //edge message
    {
      auto picks = mainModel->getMessages(&feature.getVariable().contourPicks);
      picks.push_back(em);
      mainModel->setMessages(&feature.getVariable().contourPicks, picks);
      auto endMsgs = getContourEnds(em);
      for (const auto &msg : endMsgs)
        addEntry(msg);
      view->variableContoursChanged(mainModel->index(0, 1), QModelIndex());
      entryList->setSelectedDelayed();
    }
  };
}

using namespace cmv;

struct Blend::Stow
{
  cmd::Blend *command;
  cmv::Blend *view;
  
  prm::Parameters parameters;
  dlg::SplitterDecorated *mainSplitter = nullptr;
  tbl::Model *prmModel = nullptr;
  tbl::View *prmView = nullptr;
  TableList *constantsList = nullptr;
  QAction *removeConstant = nullptr;
  VariablePage *vPage = nullptr;
  QStackedWidget *stacked = nullptr;
  bool isGlobalSelection = false;
  
  Stow(cmd::Blend *cIn, cmv::Blend *vIn)
  : command(cIn)
  , view(vIn)
  {
    parameters = command->feature->getParameters();
    buildGui();
    
    loadFeatureData();
    glue();
  }
  
  void buildGui()
  {
    QVBoxLayout *mainLayout = new QVBoxLayout();
    view->setLayout(mainLayout);
    clearContentMargins(view);
    view->setSizePolicy(view->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);
    
    prmModel = new tbl::Model(view, command->feature, parameters);
    prmView = new tbl::View(view, prmModel, true);
    
    stacked = new QStackedWidget(view);
    stacked->setSizePolicy(stacked->sizePolicy().horizontalPolicy(), QSizePolicy::Maximum);
    
    constantsList = new TableList(view, command->feature);
    constantsList->restoreSettings("cmv::Blend::contactList");
    stacked->addWidget(constantsList);
    
    removeConstant = new QAction(tr("Remove"), constantsList->getListWidget());
    constantsList->getListWidget()->addAction(removeConstant);
    
    vPage = new VariablePage(view, *command->feature);
    stacked->addWidget(vPage);
    
    mainSplitter = new dlg::SplitterDecorated(view);
    mainSplitter->setOrientation(Qt::Vertical);
    mainSplitter->setChildrenCollapsible(false);
    mainSplitter->addWidget(prmView);
    mainSplitter->addWidget(stacked);
    mainSplitter->restoreSettings("cmv::Blend::mainSplitter");
    mainLayout->addWidget(mainSplitter);
  }
  
  void slcAdded(const msg::Message &mIn)
  {
    if (view->isHidden() || !isGlobalSelection)
      return;
    
    auto sm = mIn.getSLC();
    stopGlobalSelection();
    view->node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    if (sm.type == slc::Type::Edge) //all edge selections are tangent.
      sm.accrue = slc::Accrue::Tangent;

    int type = command->feature->getParameter(ftr::Blend::PrmTags::blendType)->getInt();
    if (type == 0) //constant
    {
      auto &c = command->feature->addConstant();
      int index = addConstant(c);
      constantsList->getPrmModel(index)->mySetData(&c.contourPicks, {sm}); //triggers update
      constantsList->setSelected(index);
      auto *v = constantsList->getPrmView(index);
      if (v)
        QTimer::singleShot(0, [v](){v->openPersistent(v->model()->index(0, 1));});
    }
    else if (type == 1) //variable
    {
      if (sm.type == slc::Type::Edge)
        vPage->addContour(sm);
      else
      {
        vPage->addEntry(sm);
        int last = vPage->entryList->count() - 1;
        if (last != -1)
        {
          // see Blend::variableEntriesChanged as to why we to set the selection
          //of the entry list before call variableEntriesChanged.
          vPage->entryList->setSelected(last);
          auto *eModel = vPage->entryList->getPrmModel(last); assert(eModel);
          view->variableEntriesChanged(eModel->index(0, 1), QModelIndex());
          vPage->entryList->setSelectedDelayed();
        }
      }
    }
    else
      assert(0); //unknown blend type.
  }
  
  int addConstant(ftr::Blend::Constant &cIn)
  {
    auto prms = cIn.getParameters();
    auto index = constantsList->add(tr("Constant"), prms);
    
    cmv::tbl::SelectionCue cue;
    cue.singleSelection = false;
    cue.mask = slc::EdgesBoth;
    cue.statusPrompt = tr("Select Edges For Constant Blend");
    cue.accrueEnabled = false;
    cue.forceTangentAccrue = true;
    cue.accrueDefault = slc::Accrue::Tangent;
    constantsList->getPrmModel(index)->setCue(&cIn.contourPicks, cue);
    
    connect(constantsList->getPrmModel(index), &tbl::Model::dataChanged, view, &Blend::constantChanged);
    connect(constantsList->getPrmView(index), &tbl::View::openingPersistent, view, &Blend::closeAllPersistent);
    constantsList->getPrmView(index)->installEventFilter(view);
    
    return index;
  }
  
  void loadFeatureData()
  {
    int type = command->feature->getParameter(ftr::Blend::PrmTags::blendType)->getInt();
    switch (type)
    {
      case 0: //constant
      {
        auto &constants = command->feature->getConstants();
        for (auto &c : constants)
          addConstant(c);
        stacked->setCurrentIndex(0);
        constantsList->setSelectedDelayed();
        break;
      }
      case 1: //variable
      {
        vPage->loadFeatureData();
        stacked->setCurrentIndex(1);
        break;
      }
      default:
      {
        assert(0); //unknown blend type
        break;
      }
    }
  }
  
  void glue()
  {
    connect(prmModel, &cmv::tbl::Model::dataChanged, view, &Blend::modelChanged);
    connect(constantsList->getListWidget(), &QListWidget::itemSelectionChanged, view, &Blend::constantsSelectionChanged);
    connect(removeConstant, &QAction::triggered, view, &Blend::removeConstant);
    
    view->sift->insert
    (
      msg::Response | msg::Post | msg::Selection | msg::Add
      , std::bind(&Stow::slcAdded, this, std::placeholders::_1)
    );
    
    prmView->installEventFilter(view);
    constantsList->getListWidget()->installEventFilter(view);
  }
  
  void modeChanged()
  {
    constantsList->clear();
    vPage->clear();
    int type = command->feature->getParameter(ftr::Blend::PrmTags::blendType)->getInt();
    stacked->setCurrentIndex(type);
  }
  
  void setSelections()
  {
    int type = command->feature->getParameter(ftr::Blend::PrmTags::blendType)->getInt();
    switch (type)
    {
      case 0: //constant
      {
        std::vector<slc::Messages> out;
        for (int i = 0; i < constantsList->count(); ++i)
          out.push_back(constantsList->getMessages(i));
        command->setSelections(out);
        break;
      }
      case 1: //variable
      {
        command->setSelections(vPage->getSelections());
        vPage->entryList->updateHideInactive();
        break;
      }
      default:
      {
        assert(0); //unknown blend type
        break;
      }
    }
  }
  
  void goGlobalSelection()
  {
    if (!isGlobalSelection)
    {
      isGlobalSelection = true;
      view->goSelectionToolbar();
      int type = command->feature->getParameter(ftr::Blend::PrmTags::blendType)->getInt();
      if (type == 0) //constant
      {
        view->maskDefault = slc::EdgesBoth;
        view->goMaskDefault();
        view->node->sendBlocked(msg::buildStatusMessage(QObject::tr("Select Edge For Constant Blend").toStdString()));
      }
      else if (type == 1) //variable
      {
        if (command->feature->getVariable().contourPicks.getPicks().empty())
        {
          view->maskDefault = slc::EdgesBoth;
          view->goMaskDefault();
          view->node->sendBlocked(msg::buildStatusMessage(QObject::tr("Select Edge For Variable Blend").toStdString()));
        }
        else
        {
          view->maskDefault = slc::EndPointsBoth | slc::NearestPointsEnabled | slc::MidPointsEnabled;
          view->goMaskDefault();
          view->node->sendBlocked(msg::buildStatusMessage(QObject::tr("Select Point Constraint").toStdString()));
        }
      }
      else
        assert(0); //unknown blend type.
    }
  }
  
  void stopGlobalSelection()
  {
    if (isGlobalSelection)
    {
      isGlobalSelection = false;
      view->node->sendBlocked(msg::buildSelectionMask(slc::None));
    }
  }
  
  void goLocalUpdate()
  {
    command->localUpdate();
    QTimer::singleShot(0, [this](){this->goGlobalSelection();});
  };
};

Blend::Blend(cmd::Blend *cIn)
: Base("cmv::Blend")
, stow(new Stow(cIn, this))
{
  stow->goGlobalSelection();
}

Blend::~Blend() = default;

bool Blend::eventFilter(QObject *watched, QEvent *event)
{
  if(event->type() == QEvent::FocusIn)
  {
    closeAllPersistent();
    stow->goGlobalSelection();
  }
  
  return QObject::eventFilter(watched, event);
}

void Blend::addConstant()
{
  assert(stow->command->feature->getParameter(ftr::Blend::PrmTags::blendType)->getInt() == 0);
  auto &c = stow->command->feature->addConstant();
  stow->addConstant(c);
  //adding an empty constant doesn't require a call to update
}

void Blend::removeConstant()
{
  assert(stow->command->feature->getParameter(ftr::Blend::PrmTags::blendType)->getInt() == 0);
  int selectedIndex = stow->constantsList->remove();
  if (selectedIndex == -1)
    return;
  stow->command->feature->removeConstant(selectedIndex);
  stow->constantsList->reselectDelayed();
  stow->setSelections();
  stow->goLocalUpdate();
}

void Blend::closeAllPersistent()
{
  stow->stopGlobalSelection();
  stow->prmView->closePersistent();
  stow->constantsList->closePersistent();
  stow->vPage->closePersistent();
}

void Blend::modelChanged(const QModelIndex &index, const QModelIndex&)
{
  if (!index.isValid())
    return;
  
  auto tag = stow->prmModel->getParameter(index)->getTag();
  if (tag == ftr::Blend::PrmTags::blendType)
    stow->modeChanged();
  stow->setSelections();
  stow->goLocalUpdate();
}

void Blend::constantChanged(const QModelIndex &index, const QModelIndex&)
{
  if (!index.isValid())
    return;
  const auto *m = dynamic_cast<const tbl::Model*>(index.model()); assert(m);
  if (m->getParameter(index)->getTag() == ftr::Blend::PrmTags::contourPicks)
    stow->setSelections();
  stow->goLocalUpdate();
}

void Blend::constantsSelectionChanged()
{
  closeAllPersistent();
  stow->goGlobalSelection();
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  for (const auto &m : stow->constantsList->getSelectedMessages())
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Add, m));
}

void Blend::variableContoursChanged(const QModelIndex &index, const QModelIndex&)
{
  if (!index.isValid())
    return;
  stow->vPage->reconcile();
  stow->setSelections();
  stow->goLocalUpdate();
}

/* ok we want to alter the nearest point selections to reflect the
 * position parameter. Problem is index.model() is const so we can't
 * do that here through that. So either we do a search through the 
 * vpage entrylist to try and find the appropriate model or use the
 * current selected model and require it's selection before changing
 * parameters. We have chosen the second option and if the selection
 * isn't correct then changes will go unnoticed in the GUI.
 */
void Blend::variableEntriesChanged(const QModelIndex &index, const QModelIndex&)
{
  if (!index.isValid())
    return;
  
  auto *lm = stow->vPage->entryList->getSelectedModel();
  if (!lm || lm != index.model()) //something behind the scenes.
    return;
  if (lm->getParameter(index)->getTag() == prm::Tags::Position)
  {
    auto *pickPrm = lm->getParameter(lm->index(0, 1));
    auto msgs = lm->getMessages(pickPrm);
    if (!msgs.empty() && msgs.front().type == slc::Type::NearestPoint)
    {
      auto point = ftr::Pick::point(getEdge(msgs.front()), lm->getParameter(index)->getDouble());
      msgs.front().pointLocation = point;
      lm->setMessages(pickPrm, msgs);
      stow->setSelections(); //setMessages doesn't trigger data changed so we need to set selection
      stow->vPage->entryList->reselectDelayed();
    }
  }
  if (lm->getParameter(index)->getTag() == ftr::Blend::PrmTags::entryPick)
    stow->setSelections();
  stow->goLocalUpdate();
}

void Blend::removeEntry()
{
  int index = stow->vPage->entryList->remove();
  if (index != -1)
  {
    stow->command->feature->removeVariableEntry(index);
    stow->vPage->entryList->reselectDelayed();
    stow->setSelections();
    stow->goLocalUpdate();
  }
}

void Blend::entriesSelectionChanged()
{
  closeAllPersistent();
  stow->goGlobalSelection();
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  for (const auto &m : stow->vPage->entryList->getSelectedMessages())
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Add, m));
}
