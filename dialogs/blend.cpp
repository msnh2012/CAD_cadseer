/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#include <limits>

#include <QLabel>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QStackedWidget>
#include <QComboBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QDialogButtonBox>
#include <QCloseEvent>
#include <QAction>
#include <QSettings>
#include <QTimer>
#include <QDragMoveEvent>
#include <QMessageBox>
#include <QDebug>
#include <QMimeData>
#include <QComboBox>
#include <QGridLayout>
#include <QAbstractItemModel>
#include <QTreeView>
#include <QStandardItemModel>

#include <boost/optional.hpp>

#include <TopoDS.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>

#include <osg/Switch>
#include <osg/Vec3d>

#include <tools/idtools.h>
#include <tools/featuretools.h>
#include <application/application.h>
#include <application/mainwindow.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <project/project.h>
#include <expressions/manager.h>
#include <expressions/stringtranslator.h>
#include <message/node.h>
#include <message/sift.h>
#include <viewer/message.h>
#include <selection/message.h>
#include <annex/seershape.h>
#include <parameter/parameter.h>
#include <feature/blend.h>
#include <dialogs/splitterdecorated.h>
#include <dialogs/widgetgeometry.h>
#include <dialogs/expressionedit.h>
#include <dialogs/blend.h>

using boost::uuids::uuid;

namespace dlg
{
  class SimpleBlendItem : public QStandardItem
  {
  public:
    SimpleBlendItem() = delete;
    SimpleBlendItem(std::shared_ptr<prm::Parameter> rIn) :  //new blend definition
      QStandardItem()
      , radius(rIn)
    {
      update();
    }
    void update()
    {
      setToolTip(QString::number(static_cast<double>(*radius), 'f', 12));
      
      if (radius->isConstant())
      {
        setText(QString::number(static_cast<double>(*radius), 'f', 3));
        setIcon(QIcon());
        setData(false, Qt::UserRole);
      }
      else
      {
        setText(QString::fromStdString(expressionName));
        setIcon(QIcon(":resources/images/linkIcon.svg"));
        setData(true, Qt::UserRole);
      }
    }
    
    std::shared_ptr<prm::Parameter> radius;
    boost::uuids::uuid expressionId;
    std::string expressionName;
  };
  
  class PickItem : public QStandardItem
  {
  public:
    PickItem() = delete;
    PickItem(const ftr::Pick &pIn) :
      QStandardItem(), pick(pIn)
    {
      setText(QString::fromStdString(gu::idToShortString(pick.id)));
      setToolTip(QString::fromStdString(gu::idToString(pick.id)));
    }
    ftr::Pick pick;
  };
  
  class VariableWidget : public QWidget
  {
  public:
    
    class ListItem : public QListWidgetItem
    {
    public:
      ListItem() = delete;
      ListItem(const ftr::Pick &pIn) : QListWidgetItem(), pick(pIn){}
      ftr::Pick pick;
    };
    
    class RadiusItem : public QTableWidgetItem
    {
    public:
      RadiusItem() = delete;
      RadiusItem(std::shared_ptr<prm::Parameter> pIn) :
        QTableWidgetItem(QTableWidgetItem::UserType),
        parameter(pIn)
      {
        update();
      }
      void update()
      {
        setData(Qt::ToolTipRole, QString::number(static_cast<double>(*parameter), 'f', 12));
        
        if (parameter->isConstant())
        {
          setData(Qt::DisplayRole, QString::number(static_cast<double>(*parameter), 'f', 3));
          setData(Qt::DecorationRole, QIcon());
          setData(Qt::UserRole, false);
        }
        else
        {
          setData(Qt::DisplayRole, QString::fromStdString(expressionName));
          setData(Qt::DecorationRole, QIcon(":resources/images/linkIcon.svg"));
          setData(Qt::UserRole, true);
        }
      }
      std::shared_ptr<prm::Parameter> parameter;
      boost::uuids::uuid expressionId;
      std::string expressionName;
    };
    
    class IdItem : public QTableWidgetItem
    {
    public:
      IdItem(const ftr::Pick &pIn) : QTableWidgetItem(QTableWidgetItem::UserType + 1), pick(pIn)
      {
        setText(QString::fromStdString(gu::idToShortString(pick.id)));
        setToolTip(QString::fromStdString(gu::idToString(pick.id)));
        this->setFlags(this->flags() & ~Qt::ItemIsEditable & ~Qt::ItemIsDropEnabled);
      }
      ftr::Pick pick;
    };
    
    VariableWidget(QWidget *parent = 0) : QWidget(parent){buildGui();}
    virtual ~VariableWidget() override{}
    
    ftr::VariableBlend getBlend()
    {
      ftr::VariableBlend out;
      //fill in.
      return out;
    }
    
    ListItem* addContour(const ftr::Pick &pIn)
    {
      //scan and make sure we are not adding a duplicate.
      for (int r = 0; r < contourList->count(); ++r)
      {
        ListItem *c = dynamic_cast<ListItem*>(contourList->item(r));
        assert(c);
        if (!c->pick.resolvedOverlap(pIn).empty())
          return nullptr;
      }
      
      ListItem *ni = new ListItem(pIn);
      ni->setText(QString::fromStdString(gu::idToShortString(pIn.id)));
      ni->setToolTip(QString::fromStdString(gu::idToString(pIn.id)));
      contourList->addItem(ni);
      return ni;
    }
    
    bool addConstraint(std::shared_ptr<prm::Parameter> pIn, const ftr::Pick &pickIn)
    {
      //scan and make sure we are not adding a duplicate.
      for (int r = 0; r < constraintTable->rowCount(); ++r)
      {
        IdItem *ii = dynamic_cast<IdItem*>(constraintTable->item(r, 1));
        assert(ii);
        
        if 
        (
          //neither are parameter types, just call resolveOverlap.
          (!ii->pick.isParameterType())
          && (!pickIn.isParameterType())
          && (!ii->pick.resolvedOverlap(pickIn).empty())
        )
          return false;
        if
        (
          //both are parameter types.
          (ii->pick.isParameterType())
          && (pickIn.isParameterType())
          && (!ii->pick.resolvedOverlap(pickIn).empty())
          && (ii->pick.isParameterEqual(pickIn))
        )
          return false;
      }
      
      RadiusItem *ri = new RadiusItem(pIn);
      IdItem *ii = new IdItem(pickIn);
      int row = constraintTable->rowCount();
      constraintTable->insertRow(row);
      constraintTable->setItem(row, 0, ri);
      constraintTable->setItem(row, 1, ii);
      
      return true;
    }
    
    std::vector<ListItem*> getContourItems()
    {
      std::vector<ListItem*> out;
      for (int r = 0; r < contourList->count(); ++r)
      {
        ListItem *li = dynamic_cast<ListItem*>(contourList->item(r));
        assert(li);
        out.push_back(li);
      }
      return out;
    }
    
    QListWidget *contourList;
    QTableWidget *constraintTable;
    
  private:
    void buildGui()
    {
      contourList = new QListWidget(this);
      contourList->setContextMenuPolicy(Qt::ActionsContextMenu);
      constraintTable = new QTableWidget(this);
      constraintTable->setColumnCount(2);
      constraintTable->verticalHeader()->hide();
      constraintTable->horizontalHeader()->setStretchLastSection(true);
      constraintTable->setSelectionMode(QAbstractItemView::SingleSelection);
      constraintTable->setContextMenuPolicy(Qt::ActionsContextMenu);
      QStringList labels;
      labels << tr("Radius") << tr("Id");
      constraintTable->setHorizontalHeaderLabels(labels);
      constraintTable->setDragDropMode(QAbstractItemView::DropOnly);
      constraintTable->setAcceptDrops(true);
      constraintTable->viewport()->setAcceptDrops(true);
      constraintTable->setDropIndicatorShown(true);
      
      dlg::SplitterDecorated *splitter = new dlg::SplitterDecorated(this);
      splitter->setOrientation(Qt::Horizontal);
      splitter->addWidget(contourList);
      splitter->addWidget(constraintTable);
      splitter->setCollapsible(0, false);
      splitter->setCollapsible(1, false);
      splitter->restoreSettings("dlg::BlendVariable");
      
      QHBoxLayout *mainLayout = new QHBoxLayout();
      mainLayout->setContentsMargins(0, 0, 0, 0);
      mainLayout->addWidget(splitter);
      
      this->setContentsMargins(0, 0, 0, 0);
      this->setLayout(mainLayout);
    }
  };
}

static std::vector<uuid> getContourIds(const ann::SeerShape &ssIn, const uuid &edgeIdIn)
{
  std::vector<uuid> out;
  assert(ssIn.hasId(edgeIdIn)); //why did you set me up!
  
  const TopoDS_Edge& e = TopoDS::Edge(ssIn.findShape(edgeIdIn));
  assert(!e.IsNull());
  
  BRepFilletAPI_MakeFillet blendMaker(ssIn.getRootOCCTShape());
  blendMaker.Add(e);
  if (blendMaker.NbContours() != 1)
    return out; //invalid edge, return empty results.
  for (int index = 1; index <= blendMaker.NbEdges(1); ++index)
  {
    assert(ssIn.hasShape(blendMaker.Edge(1, index)));
    out.push_back(ssIn.findId(blendMaker.Edge(1, index)));
  }
  
  return out;
}

using namespace dlg;

Blend::Blend(QWidget *parent) : QDialog(parent)
{
  init();
  isEditDialog = false;
  
  blendSmart = std::make_shared<ftr::Blend>();
  assert(blendSmart);
  blend = blendSmart.get();
  assert(blend);
  //parent will be assigned upon first edge pick.
  
  //defaulting to a constant blend.
  cEnsureOne();
  QTimer::singleShot(0, this, &Blend::cSelectFirstRadius);
}

Blend::Blend(ftr::Blend *editBlendIn, QWidget *parent) : QDialog(parent), blend(editBlendIn)
{
  assert(blend);
  
  init();
  isEditDialog = true;
  
  //smart pointer to remain inValid in edit 'mode'.
  prj::Project *project = app::instance()->getProject();
  assert(project);
  
  //what if the established feature doesn't have parent??
  ftr::UpdatePayload::UpdateMap editMap = project->getParentMap(blend->getId());
  assert(editMap.size() == 1);
  auto it = editMap.find(ftr::InputType::target);
  assert(it != editMap.end());
  blendParent = it->second;
  assert(blendParent->hasAnnex(ann::Type::SeerShape));
  
  shapeCombo->setCurrentIndex(static_cast<int>(blend->getShape()));
  
  if (!blend->getSimpleBlends().empty())
  {
    if (!blend->getVariableBlend().picks.empty())
      std::cout << "ERROR: blend feature has both simple and variable blends in: " << BOOST_CURRENT_FUNCTION << std::endl;
    for (const auto &sb : blend->getSimpleBlends())
    {
      //here we need to clone the parameter so we can change it and user hit cancel.
      SimpleBlendItem *nsbi = new SimpleBlendItem(std::make_shared<prm::Parameter>(*sb.radius));
      if (!nsbi->radius->isConstant())
      {
        const expr::Manager &eManager = app::instance()->getProject()->getManager();
        assert(eManager.hasParameterLink(nsbi->radius->getId()));
        nsbi->expressionId = eManager.getFormulaLink(nsbi->radius->getId());
        nsbi->expressionName = eManager.getFormulaName(nsbi->expressionId);
        nsbi->update();
      }
      cModel->appendRow(nsbi);
      for (const auto &p : sb.picks)
      {
        const ann::SeerShape &sShape = blendParent->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
        PickItem *np = new PickItem(p);
        std::vector<tls::Resolved> resolved = tls::resolvePicks(blendParent, p, app::instance()->getProject()->getShapeHistory());
        for (const auto &r : resolved)
        {
          auto ids = getContourIds(sShape, r.resultId);
          std::copy(ids.begin(), ids.end(), std::back_inserter(np->pick.highlightIds));
        }
        QList<QStandardItem*> items;
        items << new QStandardItem() << np;
        nsbi->appendRow(items);
      }
    }
    QTimer::singleShot(0, this, &Blend::cSelectFirstRadius);
  }
  else //variable blends.
  {
    const ann::SeerShape &sShape = blendParent->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
    const ftr::VariableBlend &vBlend = blend->getVariableBlend();
    for (auto np : vBlend.picks)
    {
      np.resolvedIds.clear();
      np.highlightIds.clear();
      std::vector<tls::Resolved> resolved = tls::resolvePicks(blendParent, np, app::instance()->getProject()->getShapeHistory());
      for (const auto &r : resolved)
      {
        np.resolvedIds.push_back(r.resultId);
        auto ids = getContourIds(sShape, r.resultId);
        std::copy(ids.begin(), ids.end(), std::back_inserter(np.highlightIds));
      }
      vWidget->addContour(np);
    }
    for (const auto &e : vBlend.entries)
    {
      //make an update for radius item.
      ftr::Pick np(e.pick);
      np.resolvedIds.clear();
      np.highlightIds.clear();
      std::vector<tls::Resolved> resolved = tls::resolvePicks(blendParent, np, app::instance()->getProject()->getShapeHistory());
      for (const auto &r : resolved)
      {
        np.resolvedIds.push_back(r.resultId);
        np.highlightIds.push_back(r.resultId);
      }
      
      if (vWidget->addConstraint(std::make_shared<prm::Parameter>(*e.radius), np))
      {
        auto *ct = vWidget->constraintTable;
        auto *ri = dynamic_cast<VariableWidget::RadiusItem*>(ct->item(ct->rowCount() - 1, 0));
        assert(ri);
        if (!ri->parameter->isConstant())
        {
          const expr::Manager &eManager = app::instance()->getProject()->getManager();
          assert(eManager.hasParameterLink(ri->parameter->getId()));
          ri->expressionId = eManager.getFormulaLink(ri->parameter->getId());
          ri->expressionName = eManager.getFormulaName(ri->expressionId);
          ri->update();
        }
      }
      else
        std::cout << "ERROR: failed to add constraint in: " << BOOST_CURRENT_FUNCTION << std::endl;
    }
    QTimer::singleShot(0, [this]() {this->typeCombo->setCurrentIndex(1);});
    QTimer::singleShot(0, this, &Blend::vSelectFirstContour);
  }
  
  leafChildren = project->getLeafChildren(blendParent->getId());
  project->setCurrentLeaf(blendParent->getId());
  
  overlayWasOn = blend->isVisibleOverlay();
  node->sendBlocked(msg::buildHideOverlay(blend->getId()));
}

void Blend::init()
{
  node = std::make_unique<msg::Node>();
  node->connect(msg::hub());
  sift = std::make_unique<msg::Sift>();
  sift->name = "dlg::Blend";
  node->setHandler(std::bind(&msg::Sift::receive, sift.get(), std::placeholders::_1));
  setupDispatcher();
  
  buildGui();
  
  WidgetGeometry *filter = new WidgetGeometry(this, "dlg::Blend");
  this->installEventFilter(filter);
//   
  QSettings &settings = app::instance()->getUserSettings();
  settings.beginGroup("dlg::Blend");
  settings.beginGroup("ConstantTree");
  cView->header()->restoreState(settings.value("header").toByteArray());
  settings.endGroup();
  settings.beginGroup("VariableTable");
  vWidget->constraintTable->horizontalHeader()->restoreState(settings.value("header").toByteArray());
  settings.endGroup();
  settings.endGroup();
}

Blend::~Blend()
{
  QSettings &settings = app::instance()->getUserSettings();
  settings.beginGroup("dlg::Blend");
  settings.beginGroup("ConstantTree");
  settings.setValue("header", cView->header()->saveState());
  settings.endGroup();
  settings.beginGroup("VariableTable");
  settings.setValue("header", vWidget->constraintTable->horizontalHeader()->saveState());
  settings.endGroup();
  settings.endGroup();
}

void Blend::reject()
{
  isAccepted = false;
  finishDialog();
  QDialog::reject();
}

void Blend::accept()
{
  isAccepted = true;
  finishDialog();
  QDialog::accept();
  
  if (prf::manager().rootPtr->dragger().triggerUpdateOnFinish())
    app::instance()->queuedMessage(msg::Message(msg::Request | msg::Project | msg::Update));
}

void Blend::finishDialog()
{
  prj::Project *project = static_cast<app::Application *>(qApp)->getProject();
  
  //blendSmart and blendParent might be invalid dependent on state during close.
  if (!isEditDialog && isAccepted && blendSmart && blendParent) // = creation dialog that got accepted.
  {
    assert(blend);
    assert(blendParent);
    assert(blendParent->hasAnnex(ann::Type::SeerShape));
    
    updateBlendFeature();
    
    project->addFeature(blendSmart);
    project->connectInsert(blendParent->getId(), blendSmart->getId(), ftr::InputType{ftr::InputType::target});
    
    node->sendBlocked(msg::buildHideThreeD(blendParent->getId()));
    node->sendBlocked(msg::buildHideOverlay(blendParent->getId()));
    
    blend->setColor(blendParent->getColor());
  }
  
  if (isEditDialog && isAccepted)
  {
    //blendsmart is not used during edit so it is invalid and not needed.
    assert(blend);
    assert(blendParent);
    
    updateBlendFeature();
    blend->setModelDirty();
  }
  
  if (isEditDialog)
  {
    for (const auto &id : leafChildren)
      project->setCurrentLeaf(id);
    
    if (overlayWasOn)
      node->sendBlocked(msg::buildShowOverlay(blend->getId()));
  }
  
  node->send(msg::Mask(msg::Request | msg::Command | msg::Done));
}

void Blend::updateBlendFeature()
{
  expr::Manager &eManager = app::instance()->getProject()->getManager();
  
  /* we need to remove any expression links before we clear the blends.
   * their copies will be added if exist
   */
  for (const auto &sb : blend->getSimpleBlends())
  {
    if (eManager.hasParameterLink(sb.radius->getId()))
      eManager.removeParameterLink(sb.radius->getId());
  }
  for (const auto &e : blend->getVariableBlend().entries)
  {
    if (eManager.hasParameterLink(e.radius->getId()))
      eManager.removeParameterLink(e.radius->getId());
  }
  
  blend->setShape(static_cast<ftr::Blend::Shape>(shapeCombo->currentIndex()));
    
  if (typeCombo->currentIndex() == 0) // simple/constant blend
  {
    blend->clearBlends();
    for (int r = 0; r < cModel->rowCount(); ++r)
    {
      ftr::SimpleBlend sBlend;
      SimpleBlendItem *sbi = dynamic_cast<SimpleBlendItem*>(cModel->itemFromIndex(cModel->index(r, 0)));
      assert(sbi);
      sBlend.radius = sbi->radius;
      if (!sbi->hasChildren()) // no picks
        continue;
      for (int r2 = 0; r2 < sbi->rowCount(); ++r2)
      {
        PickItem *pi = dynamic_cast<PickItem*>(sbi->child(r2, 1));
        assert(pi);
        sBlend.picks.push_back(pi->pick);
      }
      //either way we going to remove existing expression link if it exists.
      if (eManager.hasParameterLink(sbi->radius->getId()))
        eManager.removeParameterLink(sbi->radius->getId());
      if (!sbi->radius->isConstant())
      {
        assert(eManager.hasFormula(sbi->expressionId));
        eManager.addLink(sbi->radius->getId(), sbi->expressionId);
      }
      blend->addSimpleBlend(sBlend);
    }
  }
  if (typeCombo->currentIndex() == 1) //variable blend
  {
    blend->clearBlends();
    ftr::VariableBlend vBlend;
    for (auto ci : vWidget->getContourItems())
      vBlend.picks.push_back(ci->pick);
    for (int r = 0; r < vWidget->constraintTable->rowCount(); ++r)
    {
      ftr::VariableEntry e;
      
      VariableWidget::RadiusItem *ri = dynamic_cast<VariableWidget::RadiusItem*>(vWidget->constraintTable->item(r, 0));
      assert(ri);
      e.radius = ri->parameter;
      //either way we going to remove existing expression link if it exists.
      if (eManager.hasParameterLink(e.radius->getId()))
        eManager.removeParameterLink(e.radius->getId());
      if (!e.radius->isConstant())
      {
        assert(eManager.hasFormula(ri->expressionId));
        eManager.addLink(e.radius->getId(), ri->expressionId);
      }
      
      VariableWidget::IdItem *ii = dynamic_cast<VariableWidget::IdItem*>(vWidget->constraintTable->item(r, 1));
      assert(ii);
      e.pick = ii->pick;
      
      vBlend.entries.push_back(e);
    }
    blend->addVariableBlend(vBlend);
  }
}

void Blend::addToSelection(const boost::uuids::uuid &shapeIdIn)
{
  const ann::SeerShape &parentShape = blendParent->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
  assert(parentShape.hasId(shapeIdIn));
  slc::Type sType = slc::convert(parentShape.getOCCTShape(shapeIdIn).ShapeType());
  assert(sType == slc::Type::Edge || sType == slc::Type::Face);
  
  msg::Message freshMMessage(msg::Request | msg::Selection | msg::Add);
  slc::Message freshSMessage;
  freshSMessage.type = sType;
  freshSMessage.featureId = blendParent->getId();
  freshSMessage.featureType = blendParent->getType();
  freshSMessage.shapeId = shapeIdIn;
  freshMMessage.payload = freshSMessage;
  node->sendBlocked(freshMMessage);
}

void Blend::setupDispatcher()
{
  sift->insert
  (
    msg::Response | msg::Post | msg::Selection | msg::Add
    , std::bind(&Blend::selectionAdditionDispatched, this, std::placeholders::_1)
  );
}

void Blend::selectionAdditionDispatched(const msg::Message &messageIn)
{
  const slc::Message &sMessage = boost::get<slc::Message>(messageIn.payload);
  if (!blendParent)
  {
    prj::Project *project = app::instance()->getProject();
    assert(project);
    blendParent = project->findFeature(sMessage.featureId);
    assert(blendParent);
    if (!blendParent->hasAnnex(ann::Type::SeerShape))
    {
      node->send(msg::buildStatusMessage("Invalid object to blend", 2.0));
      node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
      return;
    }
  }
  
  if (blendParent->getId() != sMessage.featureId)
  {
    node->send(msg::buildStatusMessage("Can't tie blend feature to multiple bodies", 2.0));
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return;
  }
  
  const ann::SeerShape &parentShape = blendParent->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
  
  auto buildEdgePick = [&]() -> ftr::Pick
  {
    assert(parentShape.hasId(sMessage.shapeId));
    assert(sMessage.type == slc::Type::Edge);
    
    ftr::Pick out = tls::convertToPick(sMessage, parentShape);
    out.shapeHistory = app::instance()->getProject()->getShapeHistory().createDevolveHistory(sMessage.shapeId);
    out.highlightIds = getContourIds(parentShape, sMessage.shapeId);
    
    return out;
  };
  
  auto buildVertexPick = [&]() -> ftr::Pick
  {
    assert(parentShape.hasId(sMessage.shapeId));
    assert(slc::isPointType(sMessage.type));
    
    ftr::Pick out = tls::convertToPick(sMessage, parentShape);
    out.shapeHistory = app::instance()->getProject()->getShapeHistory().createDevolveHistory(out.id);
    
    return out;
  };
 
  if (typeCombo->currentIndex() == 0) //constant
  {
    auto *sm = cView->selectionModel();
    //we should have ensured that we have something selected before here.
    assert(sm->hasSelection());
    QModelIndex s = cView->selectionModel()->selectedRows(0).front();
    assert(s.isValid());
    SimpleBlendItem *sbi = dynamic_cast<SimpleBlendItem*>(cModel->itemFromIndex(s));
    assert(sbi);
    ftr::Pick pick = buildEdgePick();
    if (pick.highlightIds.empty()) //means invalid edge to blend
    {
      node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
      node->sendBlocked(msg::buildStatusMessage("Invalid edge for blend", 2.0));
      return;
    }
    for (const auto &id : pick.highlightIds)
      addToSelection(id);
//     sbi->setChild(sbi->rowCount(), 1, new PickItem(pick)); //causes headerview to grow. bug?
    QList<QStandardItem *> freshItems;
    freshItems << new QStandardItem() << new PickItem(pick);
    sbi->appendRow(freshItems);
    if (!cView->isExpanded(s))
      cView->expand(s);
  }
  else if (typeCombo->currentIndex() == 1) //variable
  {
    if (sMessage.type == slc::Type::Edge)
    {
      VariableWidget::ListItem *ni = vWidget->addContour(buildEdgePick());
      if (!ni)
      {
        node->sendBlocked(msg::buildStatusMessage("Failed to add contour edge", 2.0));
        app::instance()->queuedMessage(msg::Message(msg::Request | msg::Selection | msg::Remove, sMessage));
      }
      QTimer::singleShot(0, [this, ni]() {this->vWidget->contourList->setCurrentItem(ni);});
      
      //add first and last vertex.
      BRepFilletAPI_MakeFillet blendMaker(parentShape.getRootOCCTShape());
      assert(parentShape.hasId(ni->pick.id));
      blendMaker.Add(TopoDS::Edge(parentShape.getOCCTShape(ni->pick.id)));
      if (blendMaker.NbContours() != 1)
      {
        std::cout << "WARNING: unexpected number of contours in: " << BOOST_CURRENT_FUNCTION << std::endl;
        return;
      }
      TopoDS_Vertex fv = blendMaker.FirstVertex(1);
      assert(parentShape.hasShape(fv));
      uuid fId = parentShape.findId(fv);
      TopoDS_Vertex lv = blendMaker.LastVertex(1);
      assert(parentShape.hasShape(lv));
      uuid lId = parentShape.findId(lv);
      
      auto constructPick = [&](const uuid &idIn) -> ftr::Pick
      {
        ftr::Pick out(idIn, 0.0, 0.0);
        out.selectionType = slc::Type::EndPoint;
        out.shapeHistory = app::instance()->getProject()->getShapeHistory().createDevolveHistory(idIn);
        out.highlightIds.push_back(idIn);
        out.resolvedIds.push_back(idIn);
        return out;
      };
      
      vWidget->addConstraint(ftr::Blend::buildRadiusParameter(), constructPick(fId));
      vWidget->addConstraint(ftr::Blend::buildRadiusParameter(), constructPick(lId));
    }
    else //point, just vertices right now.
    {
      ftr::Pick vp = buildVertexPick();
      if (vp.id.is_nil())
      {
        node->sendBlocked(msg::buildStatusMessage("Invalid vertex selection", 2.0));
        app::instance()->queuedMessage(msg::Message(msg::Request | msg::Selection | msg::Remove, sMessage));
        return;
      }
      
      auto isVertexValid = [&]() -> bool
      {
        for (auto ci : vWidget->getContourItems())
        {
          BRepFilletAPI_MakeFillet blendMaker(parentShape.getRootOCCTShape());
          int contourIndex = 1; //contour index
          for (const auto rid : ci->pick.resolvedIds)
          {
            assert(parentShape.hasId(rid));
            blendMaker.Add(TopoDS::Edge(parentShape.getOCCTShape(rid)));
            if (blendMaker.NbContours() != contourIndex)
            {
              std::cout << "WARNING: failed to add contour to test vertex. in: " << BOOST_CURRENT_FUNCTION << std::endl;
              continue;
            }
            if (vp.selectionType == slc::Type::StartPoint || vp.selectionType == slc::Type::EndPoint)
            {
              double p = blendMaker.RelativeAbscissa(contourIndex, TopoDS::Vertex(parentShape.getOCCTShape(vp.id)));
              if (!(p < 0.0))
                return true;
            }
            else
            {
              //here we have some point and just make sure referring edge is on contour.
              for (int ei = 1; ei < blendMaker.NbEdges(contourIndex) + 1; ++ei)
              {
                if (parentShape.findId(blendMaker.Edge(contourIndex, ei)) == vp.resolvedIds.front())
                  return true;
              }
            }
            contourIndex++;
          }
        }
        return false;
      };
      
      if (isVertexValid())
      {
        if (!vWidget->addConstraint(ftr::Blend::buildRadiusParameter(), vp))
          node->sendBlocked(msg::buildStatusMessage("Point rejected", 2.0));
        else
          node->sendBlocked(msg::buildStatusMessage("Point added", 2.0));
      }
      else
      {
        node->sendBlocked(msg::buildStatusMessage("Point not on contours", 2.0));
      }
      app::instance()->queuedMessage(msg::Message(msg::Request | msg::Selection | msg::Remove, sMessage));
    }
  }
}

void Blend::cUnlink()
{
  //actual linking/unlinking happen in accepted dialog
  QModelIndexList ss = cView->selectionModel()->selectedIndexes();
  if (ss.isEmpty() || (!ss.front().isValid()))
  {
    node->sendBlocked(msg::buildStatusMessage("Nothing selected to unlink.", 2.0));
    return;
  }
  
  QModelIndex root = ss.front();
  if (root.parent() != QModelIndex())
    root = root.parent();
  assert(root.parent() == QModelIndex());

  SimpleBlendItem *sbi = dynamic_cast<SimpleBlendItem*>(cModel->itemFromIndex(root));
  assert(sbi);
  if (!sbi->radius->isConstant())
  {
    sbi->radius->setConstant(true);
    sbi->update();
  }
  else
    node->sendBlocked(msg::buildStatusMessage("Not linked.", 2.0));
}

void Blend::cLink(QModelIndex index, const QString &idIn)
{
  //actual linking/unlinking happen in accepted dialog
  uuid expressionId = gu::stringToId(idIn.toStdString());
  assert(!expressionId.is_nil());
  expr::Manager &eManager = app::instance()->getProject()->getManager();
  assert(eManager.hasFormula(expressionId));
  assert(eManager.getFormulaValueType(expressionId) == expr::ValueType::Scalar);
  
  if (!(boost::get<double>(eManager.getFormulaValue(expressionId)) > 0.0))
  {
    node->send(msg::buildStatusMessage("No negative radei", 2.0));
    return;
  }
  
  SimpleBlendItem *sbi = dynamic_cast<SimpleBlendItem*>(cModel->itemFromIndex(index));
  assert(sbi);
  sbi->radius->setConstant(false);
  sbi->expressionId = expressionId;
  sbi->expressionName = eManager.getFormulaName(expressionId);
  sbi->radius->setValue(boost::get<double>(eManager.getFormulaValue(expressionId)));
  sbi->update();
}

void Blend::buildGui()
{
  typeCombo = new QComboBox(this);
  typeCombo->addItem(tr("Constant"));
  typeCombo->addItem(tr("Variable"));
  shapeCombo = new QComboBox(this);
  shapeCombo->addItem(tr("Rational"));
  shapeCombo->addItem(tr("QuasiAngular"));
  shapeCombo->addItem(tr("Polynomial"));
  QLabel *typeLabel = new QLabel(tr("Type:"), this);
  QLabel *shapeLabel = new QLabel(tr("Shape:"), this);
  QGridLayout *cgl = new QGridLayout(); //combo grid layout
  cgl->addWidget(typeLabel, 0, 0, Qt::AlignRight); 
  cgl->addWidget(typeCombo, 0, 1, Qt::AlignLeft);
  cgl->addWidget(shapeLabel, 1, 0, Qt::AlignRight); 
  cgl->addWidget(shapeCombo, 1, 1, Qt::AlignLeft);
  QHBoxLayout *chl = new QHBoxLayout(); //combo horizontal layout.
  chl->addLayout(cgl);
  chl->addStretch();
  
  cModel = new QStandardItemModel(this);
  QStringList headers;
  headers << tr("Radius") << tr("Pick");
  cModel->setHorizontalHeaderLabels(headers); //this sets the column count to 2.
  cView = new QTreeView(this);
  cView->setSelectionBehavior(QAbstractItemView::SelectRows);
  cView->setSelectionMode(QAbstractItemView::SingleSelection);
  cView->setContextMenuPolicy(Qt::ActionsContextMenu);
  cView->header()->setStretchLastSection(true);
  cView->setDragDropMode(QAbstractItemView::DropOnly);
  cView->setAcceptDrops(true);
  cView->viewport()->setAcceptDrops(true);
  cView->setDropIndicatorShown(true);
  
  CDropFilter *cf = new CDropFilter(this);
  cView->installEventFilter(cf);
  connect(cf, &CDropFilter::requestLinkSignal, this, &Blend::cLink);
  
  CDelegate *cd = new CDelegate(cView);
  cView->setItemDelegateForColumn(0, cd);
  
  QAction *caa = new QAction(tr("Add Radius"), cView); //constant add action
  cView->addAction(caa);
  QAction *cra = new QAction(tr("Remove"), cView); //constant remove action
  cView->addAction(cra);
  QAction *cua = new QAction(tr("Unlink"), cView); //unlink parameter with expression.
  cView->addAction(cua);
  cView->setModel(cModel);
  
  vWidget = new VariableWidget(this);
  QAction *vrc = new QAction(tr("Remove Constraint"), vWidget->constraintTable);
  vWidget->constraintTable->addAction(vrc);
  QAction *vua = new QAction(tr("Unlink"), vWidget->constraintTable);
  vWidget->constraintTable->addAction(vua);
  QAction *vrn = new QAction(tr("Remove Contour"), vWidget->contourList);
  vWidget->contourList->addAction(vrn);
  
  VDropFilter *vf = new VDropFilter(this);
  vWidget->constraintTable->installEventFilter(vf);
  connect(vf, &VDropFilter::requestLinkSignal, this, &Blend::vLink);
  
  VDelegate *vd = new VDelegate(vWidget->constraintTable);
  vWidget->constraintTable->setItemDelegateForColumn(0, vd);
  
  stacked = new QStackedWidget(this);
  stacked->addWidget(cView);
  stacked->addWidget(vWidget);
  
  QDialogButtonBox *buttons = new QDialogButtonBox
    (QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
  QHBoxLayout *buttonLayout = new QHBoxLayout();
  buttonLayout->addStretch();
  buttonLayout->addWidget(buttons);
  connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
  
  QVBoxLayout *mainLayout = new QVBoxLayout();
  mainLayout->addLayout(chl);
  mainLayout->addWidget(stacked);
  mainLayout->addLayout(buttonLayout);
  this->setLayout(mainLayout);
  
  //currentIndexChanged is overloaded so new connect syntax gets ugly. Just use the old syntax.
  connect(typeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(typeComboChangedSlot(int)));
  connect(cView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &Blend::cSelectionChanged);
  connect(cra, &QAction::triggered, this, &Blend::cRemove);
  connect(caa, &QAction::triggered, this, &Blend::cAddRadius);
  connect(cua, &QAction::triggered, this, &Blend::cUnlink);
  connect(vWidget->contourList, &QListWidget::currentRowChanged, this, &Blend::vContourSelectionChanged);
  connect(vWidget->constraintTable, &QTableWidget::currentItemChanged, this, &Blend::vConstraintSelectionChanged);
  connect(vrc, &QAction::triggered, this, &Blend::vConstraintRemove);
  connect(vrn, &QAction::triggered, this, &Blend::vContourRemove);
  connect(vua, &QAction::triggered, this, &Blend::vUnlink);
  connect(cd, &CDelegate::requestUnlinkSignal, this, &Blend::cUnlink);
  connect(vd, &VDelegate::requestUnlinkSignal, this, &Blend::vUnlink);
  
  cView->setFocus();
}

void Blend::typeComboChangedSlot(int index)
{
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  stacked->setCurrentIndex(index);
  if (index == 0)
  {
    cSelectFirstRadius();
  }
  else if (index == 1)
  {
    node->sendBlocked
    (
      msg::buildSelectionMask
      (
        slc::None
        | slc::EdgesEnabled
        | slc::EdgesSelectable
        | slc::PointsEnabled
        | slc::PointsSelectable
        | slc::EndPointsEnabled
        | slc::NearestPointsEnabled
        | slc::MidPointsEnabled
      )
    );
  }
}

void Blend::cEnsureOne()
{
  if (cModel->rowCount() == 0)
  {
    std::shared_ptr<prm::Parameter> np = ftr::Blend::buildRadiusParameter();
    cModel->appendRow(new SimpleBlendItem(np));
  }
}

void Blend::cSelectFirstRadius()
{
  assert(cModel->rowCount() != 0);
  
  auto *sm = cView->selectionModel();
  QModelIndex fe = cModel->index(0, 0, QModelIndex());
  assert(fe.isValid());
  sm->select(fe, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows | QItemSelectionModel::Clear);
}

void Blend::cSelectionChanged(const QItemSelection &selected, const QItemSelection &/*deselected*/)
{
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  QModelIndexList ss = selected.indexes();
  if (ss.isEmpty() || (!ss.front().isValid()))
  {
    node->sendBlocked(msg::buildSelectionMask(slc::None));
    return;
  }
  if (ss.front().parent() == QModelIndex())
  {
    //we have a root/blend item.
    node->sendBlocked(msg::buildSelectionMask(slc::None | slc::EdgesEnabled | slc::EdgesSelectable));
    node->sendBlocked(msg::buildStatusMessage("Select edges to blend."));
    
    cView->collapseAll();
    cView->expand(ss.front());
    
    SimpleBlendItem *sbi = dynamic_cast<SimpleBlendItem*>(cModel->itemFromIndex(ss.front()));
    assert(sbi);
    for (int i = 0; i < sbi->rowCount(); ++i)
    {
      PickItem *pi = dynamic_cast<PickItem*>(sbi->child(i, 1));
      assert(pi);
      if (pi->pick.highlightIds.empty())
        node->sendBlocked(msg::buildStatusMessage("No edges found from pick", 2.0));
      for (const auto &tid : pi->pick.highlightIds)
        addToSelection(tid);
    }
  }
  else
  {
    //have a pick item.
    node->sendBlocked(msg::buildSelectionMask(slc::None));
    node->sendBlocked(msg::buildStatusMessage("Select radius in dialog to add more edges."));
    
    assert(ss.size() == 2);
    PickItem *pi = dynamic_cast<PickItem*>(cModel->itemFromIndex(ss.at(1)));
    assert(pi);
    for (const auto &tid : pi->pick.highlightIds)
      addToSelection(tid);
  }
}

void Blend::cRemove()
{
  QModelIndexList ss = cView->selectionModel()->selectedIndexes();
  if (ss.isEmpty() || (!ss.front().isValid()))
  {
    node->sendBlocked(msg::buildStatusMessage("Nothing selected to remove.", 2.0));
    return;
  }
  
  if (ss.front().parent() == QModelIndex())
  {
    //we have a root/blend item.
    SimpleBlendItem *sbi = dynamic_cast<SimpleBlendItem*>(cModel->itemFromIndex(ss.front()));
    assert(sbi);
    if (cModel->rowCount() == 1) //dont remove last entry
    {
      while (sbi->rowCount() > 0)
      {
        sbi->removeRow(0);
        node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
        QTimer::singleShot(0, this, &Blend::cSelectFirstRadius);
      }
    }
    else
    {
      cModel->removeRow(ss.front().row());
    }
  }
  else
  {
    //have a pick item.
    assert(ss.size() == 2);
    PickItem *pi = dynamic_cast<PickItem*>(cModel->itemFromIndex(ss.at(1)));
    assert(pi);
    QModelIndex parentIndex = ss.front().parent();
    assert(parentIndex.isValid());
    cModel->itemFromIndex(parentIndex)->removeRow(ss.front().row());
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    auto *sm = cView->selectionModel();
    QTimer::singleShot
    (
      0,
      [sm, parentIndex]()
      {
        sm->select(parentIndex, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows | QItemSelectionModel::Clear);
      }
    );
  }
}

void Blend::cAddRadius()
{
  cView->collapseAll();
  
  std::shared_ptr<prm::Parameter> np = ftr::Blend::buildRadiusParameter();
  cModel->appendRow(new SimpleBlendItem(np));
  //no need to expand a new item.
  
  auto *sm = cView->selectionModel();
  QModelIndex le = cModel->index(cModel->rowCount() - 1, 0, QModelIndex());
  assert(le.isValid());
  QTimer::singleShot
  (
    0,
    [sm, le]()
    {
      sm->select(le, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows | QItemSelectionModel::Clear);
    }
  );
}

void Blend::vHighlightContour(int row)
{
  if ((row < 0) || (row >= vWidget->contourList->count()))
    return;
  
  auto item = dynamic_cast<VariableWidget::ListItem*>(vWidget->contourList->item(row));
  assert(item);
  for (const auto &tid : item->pick.highlightIds)
    addToSelection(tid);
}

void Blend::vHighlightConstraint(int row)
{
  auto *ct = vWidget->constraintTable;
  if ((row < 0) || (row >= ct->rowCount()))
    return;
  VariableWidget::IdItem *ii = dynamic_cast<VariableWidget::IdItem*>(ct->item(row, 1));
  assert(ii);
  
  slc::Message m = tls::convertToMessage(ii->pick, blendParent);
  if (m.shapeId.is_nil())
    return;
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Add, m));
}

void Blend::vUpdate3dSelection()
{
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  vHighlightContour(vWidget->contourList->currentRow());
  vHighlightConstraint(vWidget->constraintTable->currentRow());
}

void Blend::vSelectFirstContour()
{
  vWidget->contourList->setCurrentRow(0, QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent);
}

void Blend::vContourSelectionChanged(int)
{
  QTimer::singleShot(0, this, &Blend::vUpdate3dSelection);
}

void Blend::vConstraintSelectionChanged(QTableWidgetItem*, QTableWidgetItem*)
{
  QTimer::singleShot(0, this, &Blend::vUpdate3dSelection);
}

void Blend::vConstraintRemove()
{
  auto *ct = vWidget->constraintTable;
  QList<QTableWidgetItem *> sItems = ct->selectedItems();
  if (sItems.isEmpty())
    return;
  
  ct->removeRow(sItems.front()->row());
  
  QTimer::singleShot(0, this, &Blend::vUpdate3dSelection);
}

void Blend::vContourRemove()
{
  auto *cl = vWidget->contourList;
  QList<QListWidgetItem*> sItems = cl->selectedItems();
  if (sItems.isEmpty())
    return;
  
  delete sItems.front();
  QTimer::singleShot(0, this, &Blend::vUpdate3dSelection);
}

bool CDropFilter::eventFilter(QObject *obj, QEvent *event)
{
  QTreeView *treeView = dynamic_cast<QTreeView*>(obj);
  assert(treeView);
  
  auto getId = [](const QString &stringIn) -> boost::uuids::uuid
  {
    boost::uuids::uuid idOut = gu::createNilId();
    if (stringIn.startsWith("ExpressionId;"))
    {
      QStringList split = stringIn.split(";");
      if (split.size() == 2)
        idOut = gu::stringToId(split.at(1).toStdString());
    }
    return idOut;
  };
  
  auto getIndex = [&](const QPoint &pointIn) -> QModelIndex
  {
    //Really! the fucking header throws off the position.
    int headerHeight = treeView->header()->height();
    QPoint adjusted(pointIn.x(), pointIn.y() - headerHeight);
    QModelIndex index = treeView->indexAt(adjusted);
    return index; //might be null.
  };
  
  if (event->type() == QEvent::DragEnter)
  {
    QDragEnterEvent *dEvent = dynamic_cast<QDragEnterEvent*>(event);
    if (dEvent->mimeData()->hasText())
    {
      boost::uuids::uuid id = getId(dEvent->mimeData()->text());
      if (!id.is_nil())
        dEvent->acceptProposedAction();
    }
    return true;
  }
  else if (event->type() == QEvent::DragMove)
  {
    QDragMoveEvent *dEvent = dynamic_cast<QDragMoveEvent*>(event);
    assert(dEvent);
    
    dEvent->ignore(); //ignore by default
    QModelIndex index = getIndex(dEvent->pos());
    if
    (
      dEvent->mimeData()->hasText()
      && index.isValid()
      && (treeView->model()->flags(index) & Qt::ItemIsDropEnabled)
      && (!(getId(dEvent->mimeData()->text()).is_nil()))
    )
      dEvent->acceptProposedAction();
      
    return true;
  }
  else if (event->type() == QEvent::Drop)
  {
    QDropEvent *dEvent = dynamic_cast<QDropEvent*>(event);
    assert(dEvent);
    
    dEvent->ignore(); //ignore by default
    uuid expressionId = getId(dEvent->mimeData()->text());
    QModelIndex index = getIndex(dEvent->pos());
    if
    (
      dEvent->mimeData()->hasText()
      && index.isValid()
      && (treeView->model()->flags(index) & Qt::ItemIsDropEnabled)
      && (!(expressionId.is_nil()))
    )
    {
      dEvent->acceptProposedAction();
      Q_EMIT requestLinkSignal(index, QString::fromStdString(gu::idToString(expressionId)));
    }
    
    return true;
  }
  else
    return QObject::eventFilter(obj, event);
}

CDelegate::CDelegate(QObject *parent): ExpressionDelegate(parent){}

void CDelegate::setEditorData(QWidget*, const QModelIndex& index) const
{
  assert(eEditor);
  eEditor->lineEdit->setText(index.model()->data(index, Qt::ToolTipRole).toString());
  
  QVariant isLinked = index.model()->data(index, Qt::UserRole);
  if (isLinked.isNull() || (isLinked.toBool() == false))
    isExpressionLinked = false;
  else
    isExpressionLinked = true;
  
  initEditor();
}

void CDelegate::setModelData(QWidget*, QAbstractItemModel* model, const QModelIndex& index) const
{
  assert(eEditor);
  
  if (isExpressionLinked)
    return; //shouldn't need to do anything if the expression is linked.
  
  expr::Manager localManager;
  expr::StringTranslator translator(localManager);
  std::string formula("temp = ");
  formula += eEditor->lineEdit->text().toStdString();
  if (translator.parseString(formula) == expr::StringTranslator::ParseSucceeded)
  {
    localManager.update();
    assert(localManager.getFormulaValueType(translator.getFormulaOutId()) == expr::ValueType::Scalar);
    double value = boost::get<double>(localManager.getFormulaValue(translator.getFormulaOutId()));
    
    QStandardItemModel* sim = dynamic_cast<QStandardItemModel*>(model);
    assert(sim);
    SimpleBlendItem *item = dynamic_cast<SimpleBlendItem*>(sim->itemFromIndex(index));
    assert(item);
    if (!(item->radius->isValidValue(value)))
    {
      app::instance()->messageSlot(msg::buildStatusMessage("ERROR: Invalid Value for radius", 2.0));
      return;
    }
    item->radius->setValue(value);
    item->update();
  }
  else
  {
    app::instance()->messageSlot(msg::buildStatusMessage("ERROR: Couldn't parse", 2.0));
  }
}

bool VDropFilter::eventFilter(QObject *obj, QEvent *event)
{
  QTableWidget *table = dynamic_cast<QTableWidget*>(obj);
  assert(table);
  
  auto getId = [](const QString &stringIn) -> boost::uuids::uuid
  {
    boost::uuids::uuid idOut = gu::createNilId();
    if (stringIn.startsWith("ExpressionId;"))
    {
      QStringList split = stringIn.split(";");
      if (split.size() == 2)
        idOut = gu::stringToId(split.at(1).toStdString());
    }
    return idOut;
  };
  
  auto getItem = [&](const QPoint &pointIn) -> QTableWidgetItem*
  {
    //Really! the fucking header throws off the position.
    int headerHeight = table->horizontalHeader()->height();
    QPoint adjusted(pointIn.x(), pointIn.y() - headerHeight);
    return table->itemAt(adjusted);
  };
  
  if (event->type() == QEvent::DragEnter)
  {
    QDragEnterEvent *dEvent = dynamic_cast<QDragEnterEvent*>(event);
    if (dEvent->mimeData()->hasText())
    {
      boost::uuids::uuid id = getId(dEvent->mimeData()->text());
      if (!id.is_nil())
        dEvent->acceptProposedAction();
    }
    return true;
  }
  else if (event->type() == QEvent::DragMove)
  {
    QDragMoveEvent *dEvent = dynamic_cast<QDragMoveEvent*>(event);
    assert(dEvent);
    
    dEvent->ignore(); //ignore by default
    QTableWidgetItem *item = getItem(dEvent->pos());
    if
    (
      dEvent->mimeData()->hasText()
      && item
      && (item->flags() & Qt::ItemIsDropEnabled)
      && (!(getId(dEvent->mimeData()->text()).is_nil()))
    )
      dEvent->acceptProposedAction();
      
    return true;
  }
  else if (event->type() == QEvent::Drop)
  {
    QDropEvent *dEvent = dynamic_cast<QDropEvent*>(event);
    assert(dEvent);
    
    dEvent->ignore(); //ignore by default
    uuid expressionId = getId(dEvent->mimeData()->text());
    QTableWidgetItem *item = getItem(dEvent->pos());
    if
    (
      dEvent->mimeData()->hasText()
      && item
      && (item->flags() & Qt::ItemIsDropEnabled)
      && (!(expressionId.is_nil()))
    )
    {
      dEvent->acceptProposedAction();
      Q_EMIT requestLinkSignal(item, QString::fromStdString(gu::idToString(expressionId)));
    }
    
    return true;
  }
  else
    return QObject::eventFilter(obj, event);
}

VDelegate::VDelegate(QObject *parent): ExpressionDelegate(parent){}

void VDelegate::setEditorData(QWidget*, const QModelIndex& index) const
{
  assert(eEditor);
  eEditor->lineEdit->setText(index.model()->data(index, Qt::ToolTipRole).toString());
  
  QVariant isLinked = index.model()->data(index, Qt::UserRole);
  if (isLinked.isNull() || (isLinked.toBool() == false))
    isExpressionLinked = false;
  else
    isExpressionLinked = true;
  
  initEditor();
}

void VDelegate::setModelData(QWidget*, QAbstractItemModel*, const QModelIndex& index) const
{
  assert(eEditor);
  
  if (isExpressionLinked)
    return; //shouldn't need to do anything if the expression is linked.
  
  expr::Manager localManager;
  expr::StringTranslator translator(localManager);
  std::string formula("temp = ");
  formula += eEditor->lineEdit->text().toStdString();
  if (translator.parseString(formula) == expr::StringTranslator::ParseSucceeded)
  {
    localManager.update();
    assert(localManager.getFormulaValueType(translator.getFormulaOutId()) == expr::ValueType::Scalar);
    double value = boost::get<double>(localManager.getFormulaValue(translator.getFormulaOutId()));
    
    QTableWidget *table = dynamic_cast<QTableWidget*>(this->parent());
    assert(table);
    VariableWidget::RadiusItem *item = dynamic_cast<VariableWidget::RadiusItem*>(table->item(index.row(), index.column()));
    assert(item);
    
    if (!(item->parameter->isValidValue(value)))
    {
      app::instance()->messageSlot(msg::buildStatusMessage("ERROR: Invalid Value for radius", 2.0));
      return;
    }
    item->parameter->setValue(value);
    item->update();
  }
  else
  {
    app::instance()->messageSlot(msg::buildStatusMessage("ERROR: Couldn't parse", 2.0));
  }
}

void Blend::vLink(QTableWidgetItem *itemIn, const QString &idIn)
{
  //actual linking/unlinking happen in accepted dialog
  uuid expressionId = gu::stringToId(idIn.toStdString());
  assert(!expressionId.is_nil());
  expr::Manager &eManager = app::instance()->getProject()->getManager();
  assert(eManager.hasFormula(expressionId));
  assert(eManager.getFormulaValueType(expressionId) == expr::ValueType::Scalar);
  
  double cv = boost::get<double>(eManager.getFormulaValue(expressionId));
  
  VariableWidget::RadiusItem *item = dynamic_cast<VariableWidget::RadiusItem *>(itemIn);
  assert(item);
  if (!item->parameter->isValidValue(cv))
  {
    node->send(msg::buildStatusMessage("Invalid value for radius", 2.0));
    return;
  }
  
  item->parameter->setConstant(false);
  item->expressionId = expressionId;
  item->expressionName = eManager.getFormulaName(expressionId);
  item->parameter->setValue(cv);
  item->update();
}

void Blend::vUnlink()
{
  auto *ct = vWidget->constraintTable;
  QList<QTableWidgetItem *> sItems = ct->selectedItems();
  if (sItems.isEmpty() || (!sItems.front()))
  {
    node->send(msg::buildStatusMessage("Nothing selected to unlink", 2.0));
    return;
  }
  
  VariableWidget::RadiusItem *item = dynamic_cast<VariableWidget::RadiusItem*>(sItems.front());
  assert(item);
  if (item->parameter->isConstant())
  {
    node->sendBlocked(msg::buildStatusMessage("Not linked.", 2.0));
    return;
  }
  
  item->parameter->setConstant(true);
  item->update();
}
