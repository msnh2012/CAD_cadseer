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
#include <QListWidget>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>

#include "feature/ftrbase.h"
#include "selection/slcmessage.h"
#include "commandview/cmvtable.h"
#include "dialogs/dlgsplitterdecorated.h"
#include "commandview/cmvtablelist.h"

using namespace cmv;

struct TableList::Stow
{
  ftr::Base *feature;
  TableList *view;
  QListWidget *list;
  QStackedWidget *stacked;
  dlg::SplitterDecorated *splitter;
  
  Stow(cmv::TableList *vIn, ftr::Base *fIn)
  : feature(fIn)
  , view(vIn)
  {
    buildGui();
    QTimer::singleShot(0, [this](){this->selectionChanged();});
    QObject::connect(list, &QListWidget::itemSelectionChanged, [this](){this->selectionChanged();});
  }
  
  void buildGui()
  {
    QVBoxLayout *mainLayout = new QVBoxLayout();
    view->setLayout(mainLayout);
    view->setContentsMargins(0, 0, 0, 0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    view->setSizePolicy(view->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);
    
    list = new QListWidget(view);
    list->setSelectionMode(QAbstractItemView::SingleSelection);
    list->setSelectionBehavior(QAbstractItemView::SelectRows);
    list->setContextMenuPolicy(Qt::ActionsContextMenu);
    
    stacked = new QStackedWidget(view);
    
    splitter = new dlg::SplitterDecorated(view);
    splitter->setOrientation(Qt::Horizontal);
    splitter->setChildrenCollapsible(false);
    splitter->addWidget(list);
    splitter->addWidget(stacked);
    mainLayout->addWidget(splitter);
  }
  
  void selectionChanged()
  {
    int index = view->getSelectedIndex();
    if (index == -1)
    {
      stacked->setEnabled(false);
      return;
    }
    stacked->setEnabled(true);
    stacked->setCurrentIndex(index);
  }
};

TableList::TableList(QWidget *pIn, ftr::Base* fIn)
: QWidget(pIn)
, stow(std::make_unique<Stow>(this, fIn))
{}

TableList::~TableList()
{
  stow->splitter->saveSettings();
}

int TableList::count() const
{
  assert(stow->list->count() == stow->stacked->count());
  return stow->list->count();
}

int TableList::add(const QString &itemString, prm::Parameters &prmsIn)
{
  return add(itemString, prmsIn, stow->list->count());
}

int TableList::add(const QString &itemString, prm::Parameters &prmsIn, int position)
{
  position = std::clamp(position, 0, stow->list->count());
  stow->list->insertItem(position, itemString);
  
  auto *lModel = new tbl::Model(this, stow->feature, prmsIn);
  auto *lView = new tbl::View(this, lModel, true);
  stow->stacked->insertWidget(position, lView);
  
  assert(stow->list->count() == stow->stacked->count());
  return position;
}

void TableList::remove(int position)
{
  if (position < 0 || position >= stow->list->count())
    return; //do nothing if out of range.

  auto *view = dynamic_cast<tbl::View*>(stow->stacked->widget(position)); assert(view);
  stow->stacked->removeWidget(view);
  view->model()->deleteLater();
  view->deleteLater();
  delete stow->list->item(position);
}

int TableList::remove()
{
  int si = getSelectedIndex();
  if (si == -1)
    return si;
  remove(si);
  return si;
}

void TableList::clear()
{
  while(stow->list->count() > 0)
    remove(0);
}

void TableList::updateHideInactive()
{
  for (int i = 0; i < stow->stacked->count(); ++i)
  {
    auto *v = dynamic_cast<tbl::View*>(stow->stacked->widget(i)); assert(v);
    v->updateHideInactive();
  }
}

void TableList::restoreSettings(const QString& nameIn)
{
  stow->splitter->restoreSettings(nameIn);
}

void TableList::setSelected(int index)
{
  assert(stow->list->count() == stow->stacked->count());
  if (stow->list->count() == 0)
    return;
  if (index == -1)
    index = stow->list->count() - 1;
  std::clamp(index, 0, stow->list->count() - 1);
  stow->list->setCurrentRow(index, QItemSelectionModel::ClearAndSelect);
}

void TableList::setSelectedDelayed(int index)
{
  QTimer::singleShot(0, [this, index](){this->setSelected(index);});
}

void TableList::reselectDelayed()
{
  int index = getSelectedIndex();
  stow->list->clearSelection();
  setSelectedDelayed(index);
}

QListWidget* TableList::getListWidget() const
{
  return stow->list;
}

QStackedWidget* TableList::getStackedWidget() const
{
  return stow->stacked;
}

tbl::Model* TableList::getPrmModel(int position) const
{
  if (position < 0 || position >= stow->stacked->count())
    return nullptr;
  
  auto *m = dynamic_cast<tbl::Model*>(getPrmView(position)->model()); assert(m);
  return m;
}

tbl::View* TableList::getPrmView(int position) const
{
  if (position < 0 || position >= stow->stacked->count())
    return nullptr;
  
  auto *v = dynamic_cast<tbl::View*>(stow->stacked->widget(position)); assert(v);
  return v;
}

slc::Messages TableList::getMessages(int index) const
{
  slc::Messages out;
  
  auto *m = getPrmModel(index);
  if (m)
  {
    for (int r = 0; r < m->rowCount(QModelIndex()); ++r)
    {
      //Model::getMessages ignores non pick parameters.
      const auto &t = m->getMessages(m->index(r, 0));
      out.insert(out.end(), t.begin(), t.end());
    }
  }
  
  return out;
}

int TableList::getSelectedIndex() const
{
  auto selected = stow->list->selectedItems();
  if (selected.empty())
    return -1;
  return stow->list->row(selected.front());
}

tbl::Model* TableList::getSelectedModel() const
{
  return getPrmModel(getSelectedIndex());
}

tbl::View* TableList::getSelectedView() const
{
  return getPrmView(getSelectedIndex());
}

slc::Messages TableList::getSelectedMessages() const
{
  return getMessages(getSelectedIndex());
}

void TableList::closePersistent(bool shouldCommit)
{
  for (int i = 0; i < stow->stacked->count(); ++i)
  {
    auto *v = dynamic_cast<tbl::View*>(stow->stacked->widget(i)); assert(v);
    v->closePersistent(shouldCommit);
  }
}
