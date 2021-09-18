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

#include <limits>

#include <QSettings>
#include <QVBoxLayout>
#include <QListWidget>
#include <QStringListModel>
#include <QListView>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QDragLeaveEvent>

#include "application/appapplication.h"
#include "dialogs/dlgsplitterdecorated.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "commandview/cmvselectioncue.h"
#include "commandview/cmvtable.h"
#include "tools/idtools.h"
#include "feature/ftrstrip.h"
#include "command/cmdstrip.h"
#include "commandview/cmvstrip.h"

namespace
{
  //JFC. lets build a hierarchy of classes. Eyeroll.
  class BaseModel : public QAbstractListModel
  {
  protected:
    ftr::Strip::Stations stations;
  public:
    BaseModel(QObject *p, const ftr::Strip::Stations &sIn)
    : QAbstractListModel(p)
    , stations(sIn)
    {}
    
    int rowCount(const QModelIndex& = QModelIndex()) const override
    {
      return stations.size();
    }
    
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
      if (!index.isValid())
        return QVariant();
      if (role == Qt::DisplayRole)
        return ftr::Strip::getStationString(stations.at(index.row()));
      if (role == Qt::UserRole)
        return static_cast<int>(stations.at(index.row()));
      return QVariant();
    }
    
    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
      //works for both because we disable dropping on the master view
      if (!index.isValid())
        return Qt::ItemIsDropEnabled; //this allows dropping on view!
      return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
    }
    
    Qt::DropActions supportedDropActions() const override
    {
      //works for both because we disable dropping on the master view
      return Qt::CopyAction;
    }
    
    QStringList mimeTypes() const override
    {
      QStringList types;
      types << "text/plain";
      return types;
    }
    
    const ftr::Strip::Stations& getStations(){return stations;}
  };
  
  class MasterModel : public BaseModel
  {
  public:
    MasterModel(QObject *p, const ftr::Strip::Stations &sIn)
    : BaseModel(p, sIn)
    {}
    QMimeData* mimeData(const QModelIndexList &indexes) const override
    {
      QMimeData *mimeData = new QMimeData();
      QString out;
      if (!indexes.isEmpty())
      {
        //-1 indicates to station list that drag source was the master list
        out.setNum(-1);
        out += ";";
        //second is the index into the master station list.
        out += QString::number(data(indexes.front(), Qt::UserRole).toInt());
      }
      mimeData->setText(out);
      return mimeData;
    }
  };
  
  std::optional<std::pair<int, int>> decodeMime(const QMimeData *dataIn, std::size_t stationSize)
  {
    /*
     * pair is <station list index, master list index>
     * This will verify that the 'station list index' is in valid range. Note -1 is valid.
     * This will verify the master list index is in valid range.
     */
    
    if (!dataIn || !dataIn->hasText())
      return std::nullopt;
    
    QStringList strings = dataIn->text().split(";");
    if (strings.size() != 2)
      return std::nullopt;
    bool result;
    int stationIndex = strings.front().toInt(&result);
    if (!result || stationIndex < -1 || stationIndex >= static_cast<int>(stationSize)) //-1 index is a drag from master list.
      return std::nullopt;
    int masterIndex = strings.back().toInt(&result);
    if (!result)
      return std::nullopt;
    if (masterIndex < 0 || masterIndex >= ftr::Strip::getAllStationStrings().size())
      return std::nullopt;
    
    
    return std::make_pair(stationIndex, masterIndex);
  }
  
  class StationModel : public BaseModel
  {
  public:
    using Callback = std::function<void()>;
    
    StationModel(QObject *p, const ftr::Strip::Stations &sIn, Callback cb)
    : BaseModel(p, sIn)
    , callback(cb)
    {}
    
    void mySignal(){}
    
    bool setData(const QModelIndex & index, const QVariant & value, int role) override
    {
      if (!index.isValid() || role != Qt::EditRole)
        return false;
      
      stations.at(index.row()) = static_cast<ftr::Strip::Station>(value.toInt());
      emit dataChanged(index, index, {role});
      return true;
    }
    
    bool insertRows(int row, int count, const QModelIndex&) override
    {
      beginInsertRows(QModelIndex(), row, row + count - 1);
      stations.insert(stations.begin() + row, count, ftr::Strip::Blank);
      endInsertRows();
      return true;
    }
    
    bool removeRows(int row, int count, const QModelIndex&) override
    {
      beginRemoveRows(QModelIndex(), row, row + count - 1);
      stations.erase(stations.begin() + row, stations.begin() + row + count);
      endRemoveRows();
      return true;
    }
    
    QMimeData* mimeData(const QModelIndexList &indexes) const override
    {
      QMimeData *mimeData = new QMimeData();
      QString out;
      if (!indexes.isEmpty())
      {
        //first is the index in the the station list
        out.setNum(indexes.front().row());
        out += ";";
        //second is the index into the master station list.
        out += QString::number(data(indexes.front(), Qt::UserRole).toInt());
      }
      mimeData->setText(out);
      return mimeData;
    }
    
    bool dropMimeData(const QMimeData * data, Qt::DropAction action, int row, int, const QModelIndex&) override
    {
      if (!data || !data->hasText() || action != Qt::CopyAction)
        return false;
      
      //default to end if we don't have a row.
      if (row < 0 || row > static_cast<int>(stations.size()))
        row = stations.size();
      
      auto oMimeData = decodeMime(data, stations.size());
      if (!oMimeData)
        return false;
      
      if (row == oMimeData->first) //don't drag onto self.
        return false;
      
      auto insertNewRow = [&]() -> bool
      {
        if (insertRow(row))
          return setData(index(row, 0), oMimeData->second, Qt::EditRole);
        return false;
      };
      
      if (oMimeData->first == -1 && insertNewRow()) //dragged from master list.
      {
        callback();
        return true;
      }
      
      if (oMimeData->first > row)
      {
        //remove then insert
        if (removeRow(oMimeData->first) && insertNewRow())
        {
          callback();
          return true;
        }
        return false;
      }
      else
      {
        //insert then remove
        if (insertNewRow() && removeRow(oMimeData->first))
        {
          callback();
          return true;
        }
        return false;
      }
      return false;
    }
    
    void trashed(int stationIndex)
    {
      if (stationIndex < 0 || stationIndex >= static_cast<int>(stations.size()))
        return;
      removeRow(stationIndex);
      callback();
    }
  private:
    Callback callback;
  };
}

using boost::uuids::uuid;
using namespace cmv;

TrashCan::TrashCan(QWidget *parent)
: QLabel(parent)
{
  setMinimumSize(64, 64);
  setFrameStyle(QFrame::Sunken | QFrame::StyledPanel);
  setAlignment(Qt::AlignCenter);
  setAcceptDrops(true);
  setAutoFillBackground(true);
  
  setPixmap(QPixmap(":resources/images/editRemove.svg").scaled(64, 64, Qt::KeepAspectRatio));
}
void TrashCan::dragEnterEvent(QDragEnterEvent *event)
{
  if (!isDataOk(event->mimeData()))
    return;
  setBackgroundRole(QPalette::Highlight);
  event->acceptProposedAction();
}
void TrashCan::dragMoveEvent(QDragMoveEvent *event)
{
  if (!isDataOk(event->mimeData()))
    return;
  event->acceptProposedAction();
}
void TrashCan::dragLeaveEvent(QDragLeaveEvent *event)
{
  setBackgroundRole(QPalette::Window);
  event->accept();
}
void TrashCan::dropEvent(QDropEvent *event)
{
  setBackgroundRole(QPalette::Window);
  if (!isDataOk(event->mimeData()))
    return;
  event->acceptProposedAction();
  droppedIndex(stationIndex);
}

bool cmv::TrashCan::isDataOk(const QMimeData *dataIn)
{
  assert(dataIn);
  
  //have to fake station list max.
  auto oMimeData = decodeMime(dataIn, std::numeric_limits<int>::max());
  if (!oMimeData)
    return false;
  if (oMimeData->first == -1) //can't delete drag from master list.
    return false;
  stationIndex = oMimeData->first;
  return true;
}

struct Strip::Stow
{
  cmd::Strip *command;
  cmv::Strip *view;
  prm::Parameters parameters;
  cmv::tbl::Model *prmModel = nullptr;
  cmv::tbl::View *prmView = nullptr;
  QListView *stationsList = nullptr;
  StationModel *stationModel = nullptr;
  dlg::SplitterDecorated *mainSplitter = nullptr;
  TrashCan *trash = nullptr;
  
  Stow(cmd::Strip *cIn, cmv::Strip *vIn)
  : command(cIn)
  , view(vIn)
  {
    parameters = command->feature->getParameters();
    buildGui();
    connect(prmModel, &tbl::Model::dataChanged, view, &Strip::modelChanged);
    connect(trash, &TrashCan::droppedIndex, view, &Strip::trashed);
    mainSplitter->restoreSettings("cmv::Strip::mainSplitter");
  }
  
  void buildGui()
  {
    QVBoxLayout *mainLayout = new QVBoxLayout();
    view->setLayout(mainLayout);
    Base::clearContentMargins(view);
    
    mainSplitter = new dlg::SplitterDecorated(view);
    mainSplitter->setOrientation(Qt::Vertical);
    mainSplitter->setChildrenCollapsible(false);
    mainLayout->addWidget(mainSplitter);
    
    prmModel = new tbl::Model(view, command->feature);
    prmView = new tbl::View(view, prmModel, true);
    mainSplitter->addWidget(prmView);
    
    tbl::SelectionCue cue;
    cue.singleSelection = true;
    cue.mask = slc::ObjectsBoth;
    cue.accrueEnabled = false;
    cue.forceTangentAccrue = false;
    
    cue.statusPrompt = tr("Select Object For Part");
    prmModel->setCue(command->feature->getParameter(ftr::Strip::PrmTags::part), cue);
    
    cue.statusPrompt = tr("Select Object For Blank");
    prmModel->setCue(command->feature->getParameter(ftr::Strip::PrmTags::blank), cue);
    
    cue.statusPrompt = tr("Select Object For Nest");
    prmModel->setCue(command->feature->getParameter(ftr::Strip::PrmTags::nest), cue);
    
    mainSplitter->addWidget(buildStationWidget());
  }
  
  QWidget* buildStationWidget()
  {
    QWidget *out = new QWidget(view);
    QVBoxLayout *mainLayout = new QVBoxLayout();
    out->setLayout(mainLayout);
    clearContentMargins(out);
    
    QHBoxLayout *hl = new QHBoxLayout();
    mainLayout->addLayout(hl);
    
    QVBoxLayout *vl = new QVBoxLayout();
    hl->addLayout(vl);
    
    stationModel = new StationModel(out, command->feature->getStations(), std::bind(&Stow::stationsChanged, this));
    stationsList = new QListView(out);
    stationsList->setModel(stationModel);
    stationsList->setDragEnabled(true);
    stationsList->setAcceptDrops(true);
    stationsList->setDropIndicatorShown(true);
    stationsList->setDefaultDropAction(Qt::CopyAction);
    vl->addWidget(stationsList);
    
    trash = new TrashCan(out);
    vl->addWidget(trash);
    
    auto *masterModel = new MasterModel(out, ftr::Strip::getAllStations());
    auto *protoList = new QListView(out);
    protoList->setModel(masterModel);
    protoList->setDragEnabled(true);
    hl->addWidget(protoList);
    
    return out;
  }
  
  void stationsChanged()
  {
    command->feature->setStations(stationModel->getStations());
    command->localUpdate();
  }
};

Strip::Strip(cmd::Strip *cIn)
: Base("cmv::Strip")
, stow(new Stow(cIn, this))
{
  goSelectionToolbar();
  goMaskDefault();
}

Strip::~Strip() = default;

void Strip::modelChanged(const QModelIndex &index, const QModelIndex&)
{
  if (!index.isValid())
    return;
  
  auto tag = stow->parameters.at(index.row())->getTag();
  if (tag == ftr::Strip::PrmTags::part || tag == ftr::Strip::PrmTags::blank || tag == ftr::Strip::PrmTags::nest)
  {
    const auto *f = stow->command->feature;
    const auto &parts = stow->prmModel->getMessages(f->getParameter(ftr::Strip::PrmTags::part));
    const auto &blanks = stow->prmModel->getMessages(f->getParameter(ftr::Strip::PrmTags::blank));
    const auto &nests = stow->prmModel->getMessages(f->getParameter(ftr::Strip::PrmTags::nest));
    stow->command->setSelections(parts, blanks, nests);
    //when user changes selection lets enable auto calc.
    stow->command->feature->getParameter(ftr::Strip::PrmTags::autoCalc)->setValue(true);
  }
  stow->command->localUpdate();
  stow->prmView->updateHideInactive();
  node->sendBlocked(msg::buildStatusMessage(stow->command->getStatusMessage()));
  goMaskDefault();
}

void Strip::trashed(int stationIndex)
{
  stow->stationModel->trashed(stationIndex);
}
