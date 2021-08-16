/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2021 Thomas S. Anderson blobfish.at.gmx.com
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

// #include <cassert>
// #include <boost/optional/optional.hpp>

#include <QSettings>
#include <QHeaderView>
#include <QMouseEvent>
#include <QTimer>
#include <QPainter>
#include <QMimeData>
#include <QComboBox>
#include <QLineEdit>
#include <QStyledItemDelegate>

#include "application/appapplication.h"
#include "application/appmessage.h"
#include "project/prjproject.h"
#include "expressions/exprmanager.h"
#include "feature/ftrbase.h"
#include "selection/slcmessage.h"
#include "message/msgmessage.h"
#include "commandview/cmvtablewidgets.h"
#include "commandview/cmvselectioncue.h"
#include "commandview/cmvselection.h"
#include "parameter/prmvariant.h"
#include "parameter/prmparameter.h"
#include "tools/featuretools.h"
#include "commandview/cmvtable.h"

namespace
{
    /*! @brief Delegate for cmv::tbl::View
   * 
   * @details <beginRant> JFC
   * Doing selection is a real PIA. Once you want to create/display
   * an editor and let the user interact with any other widget, the
   * entire Qt MVC facade evaporates and you are left to dig through the 10
   * degree Qt inheritance hierarchy with each tier's implementation
   * buried beneath a pimpl idiom. Fucking Please! I would call
   * it a leaky abstraction, if it wasn't a god damn tsunami <endRant>
   */
  class TableDelegate : public QStyledItemDelegate
  {
    mutable cmv::edt::View *slcEditor = nullptr;
    
  public:
    explicit TableDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
    {}
    
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
      //dont use QFontMetric::leading. It is fucking negative 1. WTF
      
      QModelIndex i = index;
      
      //csys strings are really long and eat up the table for no benefit.
      //so we cheat here and just pass in the index for the name column
      //and let the table clip the long string. Also let's not fill
      //up the screen with disabled picks...or disabled anything.
      if (i.column() == 1)
      {
        const auto *m = static_cast<const cmv::tbl::Model*>(index.model());
        prm::Parameter *prm = m->getParameter(index);
        if (prm->getValueType() == typeid(osg::Matrixd) || !prm->isActive())
          i = m->index(i.row(), 0);
      }
      
      QSize textSize = QStyledItemDelegate::sizeHint(option, i);
      return textSize;
    }
    
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
      const auto *m = static_cast<const cmv::tbl::Model*>(index.model());
      prm::Parameter *prm = m->getParameter(index);
      if (prm->isConstant())
        QStyledItemDelegate::paint(painter, option, index);
      else
      {
        if (option.state & QStyle::State_Selected)
          painter->fillRect(option.rect, option.palette.highlight());
        auto icon = QIcon(":/resources/images/linkIcon.svg");
        QIcon::Mode mode = (option.state & QStyle::State_Enabled) ? QIcon::Normal : QIcon::Disabled;
        icon.paint(painter, option.rect, Qt::AlignCenter, mode);
      }
    }
    
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex &index) const override
    {
      auto *view = dynamic_cast<cmv::tbl::View*>(this->parent());
      assert(view);
      const auto *model = dynamic_cast<const cmv::tbl::Model*>(index.model());
      assert(model);
      
      QWidget *out = nullptr;
      
      prm::Parameter *p = model->getParameter(index);
      if (p->getValueType() == typeid(ftr::Picks))
      {
        auto *edtModel = new cmv::edt::Model(parent, p, model->getCue(p));
        edtModel->setMessages(model->getMessages(index));
        assert(!slcEditor);
        slcEditor = new cmv::edt::View(parent, edtModel);
        out = slcEditor;
        //set toolbar to selection tab
        app::Message am;
        am.toolbar = 0;
        app::instance()->messageSlot(msg::Message(msg::Request | msg::Toolbar | msg::Show, am));
      }
      else
      {
        auto *editWidget = cmv::tbl::buildWidget(parent, p);
//         connect(editWidget, &cmv::ParameterBase::prmValueChanged, view, &cmv::tbl::View::prmValueChanged);
        out = editWidget;
      }
      
      assert(out);
      return out;
    }

    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex&) const override
    {
      editor->setGeometry(option.rect);
    }
    
    void setEditorData(QWidget *w, const QModelIndex&) const override
    {
      if (slcEditor && slcEditor == w)
      {
//         std::cout << "installing on editor widget: " << w->metaObject()->className() << std::endl;
        auto *view = dynamic_cast<cmv::tbl::View*>(this->parent());
        assert(view);
        auto ff = new cmv::edt::FocusFilter(w);
        w->installEventFilter(ff);
      }
    }

    void setModelData(QWidget *widget, QAbstractItemModel* model, const QModelIndex& index) const override
    {
      if (!index.isValid())
        return;
      auto *myModel = dynamic_cast<cmv::tbl::Model*>(model);
      assert(myModel);
      myModel->mySetData(index, widget);
    }
    
    void commitSlcEditor()
    {
      if (slcEditor)
      {
        commitData(slcEditor);
        slcEditor = nullptr;
      }
    }
    
    void rejectSlcEditor()
    {
      slcEditor = nullptr;
    }
  };
  
  int getId(const QString &stringIn)
  {
    int idOut = -1;
    if (stringIn.startsWith("ExpressionId;"))
    {
      QStringList split = stringIn.split(";");
      if (split.size() == 2)
        idOut = split.at(1).toInt();
    }
    return idOut;
  }
  
//see cmvtablewidgets.cpp for editor types per parameter type.
  struct WidgetAssignVisitor
  {
    mutable QString message;
    
    WidgetAssignVisitor() = delete;
    WidgetAssignVisitor(const QWidget *eIn, prm::Parameter* prmIn, cmv::tbl::Model *mIn)
    : editor(eIn)
    , parameter(prmIn)
    , myModel(mIn)
    {assert(editor); assert(parameter);}
    
    bool operator()(double) const
    {
      return goExpression(editor);
    }
    
    bool operator()(int) const
    {
      if (parameter->isEnumeration())
      {
        const QComboBox *comboBox = dynamic_cast<const QComboBox*>(editor);
        assert(comboBox);
        return parameter->setValue(comboBox->currentIndex());
      }
      
      return goExpression(editor);
    }
    
    bool operator()(bool) const
    {
      const QComboBox *comboBox = dynamic_cast<const QComboBox*>(editor);
      assert(comboBox);
      return parameter->setValue(comboBox->currentIndex() == 0);
    }
    
    bool operator()(const std::string&) const
    {
      const QLineEdit *lineEdit = dynamic_cast<const QLineEdit*>(editor);
      assert(lineEdit);
      return parameter->setValue(lineEdit->text().toStdString());
    }
    
    bool operator()(const boost::filesystem::path&) const
    {
      const QLineEdit *lineEdit = dynamic_cast<const QLineEdit*>(editor);
      assert(lineEdit);
      return parameter->setValue(boost::filesystem::path(lineEdit->text().toStdString()));
    }
    
    bool operator()(const osg::Vec3d&) const
    {
      return goExpression(editor);
    }
    
    bool operator()(const osg::Quat&) const
    {
      return goExpression(editor);
    }
    
    bool operator()(const osg::Matrixd&) const
    {
      return goExpression(editor);
    }
    
    bool operator()(const ftr::Picks&) const
    {
      const auto *myWidget = dynamic_cast<const cmv::edt::View*>(editor);
      if (myWidget)
      {
        auto *edtModel = dynamic_cast<cmv::edt::Model*>(myWidget->model());
        assert(edtModel);
        myModel->setMessages(parameter, edtModel->getMessages());
        myModel->setCue(parameter, edtModel->getCue());
        return true;
      };
      return false;
    }
  private:
    const QWidget *editor;
    prm::Parameter* parameter;
    cmv::tbl::Model *myModel;
    
    bool goExpression(const QWidget *w) const
    {
      const auto *ee = dynamic_cast<const cmv::tbl::ExpressionEdit*>(w);
      assert(ee);
      
      if (!parameter->isConstant())
        return false;

      expr::Manager localManager;
      std::string formula("temp = ");
      formula += ee->text().toStdString();
      auto result = localManager.parseString(formula);
      if (result.isAllGood())
      {
        auto oId = localManager.getExpressionId(result.expressionName);
        assert(oId);
        auto result = localManager.assignParameter(parameter, *oId);
        switch (result)
        {
          case expr::Amity::Incompatible:
            message = QObject::tr("Incompatible Types");
            return false;
          case expr::Amity::InvalidValue:
            message = QObject::tr("Invalid Value");
            return false;
          case expr::Amity::Equal:
            message = QObject::tr("Same Value");
            return false;
          case expr::Amity::Mutate:
            message = QObject::tr("Value Changed");
            return true;
        }
      }
      
      message = QObject::tr("Parsing failed");
      return false;
    }
  };
}

using boost::uuids::uuid;

using namespace cmv::tbl;

struct Model::Stow
{
  Model &model;
  ftr::Base *feature = nullptr;
  prm::Parameters parameters;
  prm::Observer observer;
  std::map<const prm::Parameter*, SelectionCue> cueMap;
  std::map<const prm::Parameter*, slc::Messages> messageMap;
  
  Stow(Model& mIn, ftr::Base *featureIn, const prm::Parameters &psIn)
  : model(mIn)
  , feature(featureIn)
  , parameters(psIn)
  {
    assert(feature); //don't set me up with a null feature.
    observer.valueHandler = std::bind(&Stow::valueChanged, this);
    observer.constantHandler = std::bind(&Stow::constantChanged, this);
    observer.activeHandler = std::bind(&Stow::activeChanged, this);
    
    for (auto *p : parameters)
    {
      p->connect(observer);
      
      if (p->getValueType() != typeid(ftr::Picks))
        continue;
      //cache selections
      ftr::UpdatePayload up = app::instance()->getProject()->getPayload(feature->getId());
      tls::Resolver resolver(up);
      slc::Messages out;
      for (const auto &p : p->getPicks())
      {
        if (resolver.resolve(p))
        {
          auto msgs = resolver.convertToMessages();
          out.insert(out.end(), msgs.begin(), msgs.end());
        }
        //no warning to user? to terminal?
      }
      messageMap.insert(std::make_pair(p, out));
    }
  }
  
  void valueChanged()
  {
    //heavy handed approach for external changes
    model.beginResetModel();
    model.endResetModel();
  }
  
  void constantChanged()
  {
    //heavy handed approach for external changes
    model.beginResetModel();
    model.endResetModel();
  }
  
  void activeChanged()
  {
    //heavy handed approach for external changes
    model.beginResetModel();
    model.endResetModel();
  }
};

Model::Model(QObject *parent, ftr::Base *fIn)
: Model(parent, fIn, fIn->getParameters())
{}

Model::Model(QObject *parent, ftr::Base *fIn, const prm::Parameters &psIn)
: QAbstractTableModel(parent)
, stow(std::make_unique<Stow>(*this, fIn, psIn))
{}

Model::~Model() = default;

int Model::rowCount(const QModelIndex&) const
{
  //passed index is always invalid for table model, so don't test
  return static_cast<int>(stow->parameters.size());
}

int Model::columnCount(const QModelIndex&) const
{
  //passed index is always invalid for table model, so don't test
  return 2;
}

QVariant Model::data(const QModelIndex &index, int role) const
{
  if (!index.isValid())
    return QVariant();
  
  int row = index.row();
  assert(row >= 0 && row < static_cast<int>(stow->parameters.size()));
  const prm::Parameter *prm = stow->parameters.at(row);
  
  switch (index.column())
  {
    case 0:
      switch (role)
      {
        case Qt::DisplayRole:
          return prm->getName();
      }
      break;
    case 1:
      switch (role)
      {
        case Qt::DisplayRole:
          return prm->adaptToQString();
        case Qt::ToolTipRole:
          if (prm->isConstant())
            if (prm->isExpressionLinkable())
              return tr("Double Click To Edit Or Drop Expression To Link");
            else
              return tr("Double Click To Edit");
          else
            return tr("Double Click To Break Expression Link");
      }
      break;
  }
  return QVariant();
}

Qt::ItemFlags Model::flags(const QModelIndex &index) const
{
  if (!index.isValid())
    return Qt::NoItemFlags;
  
  int row = index.row();
  assert(row >= 0 && row < static_cast<int>(stow->parameters.size()));
  prm::Parameter *prm = stow->parameters.at(row);
  
  if (prm->isActive())
  {
    switch (index.column())
    {
      case 0:
//         return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        break;
      case 1:
      {
        Qt::ItemFlags out = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        if (prm->isConstant())
          out |= Qt::ItemIsEditable;
        if (prm->isExpressionLinkable())
          out |= Qt::ItemIsDropEnabled;
        return out;
      }
    }
  }
  
  return Qt::NoItemFlags;
}

bool Model::mySetData(const QModelIndex &index, const QWidget *widget)
{
  prm::ObserverBlocker blocker(stow->observer);
  
  bool result = false;
  WidgetAssignVisitor v(widget, getParameter(index), this);
  if (std::visit(v, getParameter(index)->getStow().variant))
  {
    dataChanged(index, index, {Qt::EditRole});
    result = true;
  }
  app::instance()->messageSlot(msg::buildStatusMessage(v.message.toStdString(), 2.0));
  return result;
}

void Model::setMessages(prm::Parameter *prmIn, const slc::Messages &msgsIn)
{
  auto it = stow->messageMap.find(prmIn);
  assert(it != stow->messageMap.end());
  it->second = msgsIn;
  auto index = createIndex(std::distance(stow->messageMap.begin(), it), 0);
  dataChanged(index, index, {Qt::EditRole});
}

prm::Parameter* Model::getParameter(const QModelIndex &index) const
{
  if (!index.isValid())
    return nullptr;
  
  int row = index.row();
  assert(row >= 0 && row < static_cast<int>(stow->parameters.size()));
  return stow->parameters.at(row);
}

QModelIndex Model::addParameter(prm::Parameter *prm)
{
  beginInsertRows(QModelIndex(), stow->parameters.size(), stow->parameters.size());
  stow->parameters.push_back(prm);
  endInsertRows();
  return index(stow->parameters.size(), 0);
}

void Model::removeParameter(const QModelIndex &index)
{
  beginRemoveRows(QModelIndex(), index.row(), index.row());
  stow->parameters.erase(stow->parameters.begin() + index.row());
  endRemoveRows();
}

void Model::removeParameter(const prm::Parameter *prm)
{
  auto it = std::find(stow->parameters.begin(), stow->parameters.end(), prm);
  if (it != stow->parameters.end())
  {
    int row = std::distance(stow->parameters.begin(), it);
    beginRemoveRows(QModelIndex(), row, row);
    stow->parameters.erase(it);
    endRemoveRows();
  }
}

const slc::Messages& Model::getMessages(const QModelIndex &index) const
{
  static const slc::Messages dummy;
  if (!index.isValid())
    return dummy;
  
  return getMessages(stow->parameters.at(index.row()));
}

const slc::Messages& Model::getMessages(const prm::Parameter *prmIn) const
{
  static const slc::Messages dummy;
  auto it = stow->messageMap.find(prmIn);
  if (it == stow->messageMap.end())
    return dummy;
  return it->second;
}

void Model::setCue(prm::Parameter *prm, const SelectionCue &cueIn)
{
  stow->cueMap.insert_or_assign(prm, cueIn);
}

void Model::setCue(const QModelIndex &index, const SelectionCue &cue)
{
  if (!index.isValid())
    return;
  setCue(getParameter(index), cue);
}

const SelectionCue& Model::getCue(const prm::Parameter *prmIn) const
{
  auto it = stow->cueMap.find(prmIn);
  if (it != stow->cueMap.end())
    return it->second;
  
  //no warning to user? to terminal?
  static const SelectionCue dummy;
  return dummy;
}

QStringList Model::mimeTypes() const
{
  QStringList types;
  types << "text/plain";
  return types;
}

View::View(QWidget *parent, Model *theModel, bool stateIn)
: QTableView(parent)
, hideInactive(stateIn)
{
  setSizePolicy(sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);
  horizontalHeader()->setStretchLastSection(true); //?
  verticalHeader()->setVisible(false);
  horizontalHeader()->setVisible(false);
  setSelectionBehavior(QAbstractItemView::SelectItems);
  setSelectionMode(QAbstractItemView::SingleSelection);
  setDragDropMode(QAbstractItemView::DropOnly);
  setAcceptDrops(true);
  setDropIndicatorShown(true);
  setModel(theModel); //has to be done before connect.
  connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &View::selectionHasChanged);
  connect(theModel, &QAbstractItemModel::modelReset, this, &View::updateHideInactive);
  setItemDelegateForColumn(1, new TableDelegate(this));
  QTimer::singleShot(0, this, &View::resizeRowsToContents);
  QTimer::singleShot(0, this, &View::updateHideInactive);
}

View::~View() = default;

void View::mouseReleaseEvent(QMouseEvent *event)
{
  if (event->button() == Qt::LeftButton)
  {
    //just trying to close any persistent editors.
    //if the user picks on a valid model index that is selectable then
    //the framework will call the selectionChanged handler and the editor
    //will be closed. So the following lets the user pick in all
    //the other areas of the view in order to dismiss the persistent editor.
    auto index = indexAt(event->pos());
    if (!index.isValid() || (!(model()->flags(index) & Qt::ItemIsSelectable)))
      selectionModel()->clear();
  }
    
  QTableView::mouseReleaseEvent(event);
}

void View::mouseDoubleClickEvent(QMouseEvent *event)
{
  QModelIndex index = indexAt(event->pos());
  if (index.isValid() && index.column() == 1)
  {
    prm::Parameter *prm = static_cast<Model*>(model())->getParameter(index);
    if (!prm->isConstant())
    {
      //double click a link expression unlinks it.
      auto &em = app::instance()->getProject()->getManager();
      em.removeLink(prm->getId());
//       return; //without return we go right into editing.
    }
    if (prm->getValueType() == typeid(ftr::Picks))
    {
      //pad the height for picks so we have room to grow and
      //user isn't dealing with tiny scroll when editing
      
      double lowScale = 1.1;
      double highScale = 4.0;
      
      int pickCount = static_cast<int>(prm->getPicks().size());
      double scale = 1.0;
      if (pickCount == 0)
        scale = highScale;
      else if (pickCount > 10)
        scale = lowScale;
      else
      {
        // x is per item factor
        // y is multiply pixel factor.
        osg::Vec2d linear = (osg::Vec2d(10, lowScale) - osg::Vec2d(0, highScale));
        linear.normalize();
        scale = (linear * static_cast<double>(pickCount)).y() + highScale;
      }
      
      int height = rowHeight(index.row()) * scale;
      height = std::max(10, height);
      height = std::min(height, static_cast<int>(size().height() * 0.8)); //max at 80% of main table.
      
      setRowHeight(index.row(), height);
      openingPersistent();
      openPersistentEditor(index);
      app::instance()->installEventFilter(this);
      return;
    }
  }
  QTableView::mouseDoubleClickEvent(event);
}

void View::selectionHasChanged(const QItemSelection&, const QItemSelection&)
{
  //Doc says, we won't get here when model is reset.
  closePersistent(true);
}

//this allows the user to hit enter or esc to finish with the
//the selection editor. this behavior matches the normal
//non-persistent editors.
bool View::eventFilter(QObject *watched, QEvent *event)
{
  if (event->type() == QEvent::KeyRelease)
  {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
    if(keyEvent->key() == Qt::Key_Escape)
    {
      closePersistent(false);
      selectionModel()->clear();
      return true;
    }
    else if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)
    {
      closePersistent(true);
      selectionModel()->clear();
      return true;
    }
  }
    
  return QTableView::eventFilter(watched, event);
}

void View::dragMoveEvent(QDragMoveEvent *event)
{
  auto index = indexAt(event->pos());
  if (index.isValid())
  {
    auto *theModel = dynamic_cast<Model*>(model());
    assert(theModel);
    auto *prm = theModel->getParameter(index);
    
    if (event->mimeData()->hasText() && prm->isExpressionLinkable() && prm->isActive())
    {
      QString textIn = event->mimeData()->text();
      int id = getId(textIn);
      if (id >= 0 && app::instance()->getProject()->getManager().canLinkExpression(prm, id))
      {
        event->acceptProposedAction();
        return;
      }
    }
  }
  
  event->setDropAction(Qt::IgnoreAction);
  event->ignore();
}

void View::dropEvent(QDropEvent *event)
{
  auto index = indexAt(event->pos());
  if (!index.isValid())
    return;
  
  auto *theModel = dynamic_cast<Model*>(model());
  assert(theModel);
  auto *prm = theModel->getParameter(index);
  
  auto mo = [&](const std::string &s)
  {
    app::instance()->messageSlot(msg::buildStatusMessage(s, 2.0));
  };
  
  auto reject = [&]()
  {
    event->setDropAction(Qt::IgnoreAction);
    event->ignore();
  };
  
  if (event->mimeData()->hasText() && prm->isExpressionLinkable())
  {
    QString textIn = event->mimeData()->text();
    int id = getId(textIn);
    
    auto linkResults = app::instance()->getProject()->getManager().addLink(prm, id);
    switch (linkResults)
    {
      case expr::Amity::Incompatible:
        mo("Incompatible Types. Link Rejected");
        reject();
        break;
      case expr::Amity::InvalidValue:
        mo("Invalid Value. Link Rejected");
        reject();
        break;
      case expr::Amity::Equal:
        mo("Link Accepted");
        event->acceptProposedAction();
        theModel->dataChanged(index, index);
        break;
      case expr::Amity::Mutate:
        mo("Link Accepted");
        event->acceptProposedAction();
        theModel->dataChanged(index, index);
        break;
    }
  }
}

void View::updateHideInactive()
{
  if (!hideInactive)
    return;
  auto *m = dynamic_cast<Model*>(model()); assert(m);
  for (int r = 0; r < model()->rowCount(); ++r)
  {
    if (m->getParameter(m->index(r, 1))->isActive())
      showRow(r);
    else
      hideRow(r);
  }
}

void View::closePersistent(bool shouldCommit)
{
  auto *td = dynamic_cast<TableDelegate*>(itemDelegateForColumn(1));
  assert(td);
  
  for (int r = 0; r < model()->rowCount(); ++r)
  {
    auto i = model()->index(r, 1);
    if (isPersistentEditorOpen(i))
    {
      if (shouldCommit)
        td->commitSlcEditor();
      else
        td->rejectSlcEditor();
      closePersistentEditor(i);
      app::instance()->removeEventFilter(this);
      app::instance()->messageSlot(msg::Message(msg::Request | msg::Selection | msg::Clear));
      closingPersistent();
      break; //should only be one.
    }
  }
  resizeRowsToContents();
}
