/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2019  Thomas S. Anderson blobfish.at.gmx.com
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

#include <boost/optional/optional.hpp>

#include <QButtonGroup>
#include <QStackedWidget>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QHeaderView>

#include "dialogs/dlgselectionbutton.h"
#include "dialogs/dlgselectionlist.h"
#include "dialogs/dlgselectionwidget.h"

using namespace dlg;

struct SelectionWidget::Stow
{
  struct Entry
  {
    QLabel *label = nullptr;
    SelectionButton *button = nullptr;
    SelectionModel *model = nullptr;
    SelectionView *view = nullptr;
    bool userHidden = false; // keep separate from qt state
    
    void hide()
    {
      label->hide();
      button->hide();
      view->hide();
      userHidden = true;
    }
    void show()
    {
      label->show();
      button->show();
      view->show();
      userHidden = false;
    }
    bool isHidden()
    {
      return userHidden;
    }
    bool isShown()
    {
      return !userHidden;
    }
  };
  std::vector<Entry> entries;
  bool validEntryIndex(int index)
  {
    if (index >= 0 && index < static_cast<int>(entries.size()))
      return true;
    return false;
  }
  
  QButtonGroup *buttonGroup = nullptr;
  QStackedWidget *stackedWidget = nullptr;
  QGridLayout *gridLayout = nullptr;
  QPixmap pmap = QPixmap(":resources/images/cursor.svg").scaled(32, 32, Qt::KeepAspectRatio);
  int selectionWidth;
  AccrueDelegate *accrueDelegate = nullptr;
  
  boost::optional<int> findButtonIndex(const SelectionButton *sbIn)
  {
    int index = 0;
    for (const auto &e : entries)
    {
      if (e.button == sbIn)
        return index;
      index++;
    }
    return boost::none;
  }
};

SelectionWidget::SelectionWidget(QWidget *parent, const std::vector<SelectionWidgetCue> &cuesIn, QButtonGroup *buttonGroupIn)
: QWidget(parent)
, stow(std::make_unique<SelectionWidget::Stow>())
{
  QSizePolicy adjust(sizePolicy());
  adjust.setVerticalPolicy(QSizePolicy::Maximum);
  setSizePolicy(adjust);
  
  if (buttonGroupIn)
    stow->buttonGroup = buttonGroupIn;
  else
    stow->buttonGroup = new QButtonGroup(this);
  
  buildGui(cuesIn);
}

SelectionWidget::~SelectionWidget() = default;

int SelectionWidget::getButtonCount() const
{
  return static_cast<int>(stow->entries.size());
}

/*! @brief add a button to group of buttons
 * 
 * @parameter index is the index of new button. if invalid added at end.
 * @parameter cueIn is the data describing new button.
 */
void SelectionWidget::addButton(const SelectionWidgetCue &cueIn)
{
  int row = static_cast<int>(stow->entries.size());
  stow->entries.emplace_back();
  stow->entries.back().label = new QLabel(cueIn.name, this);
  QIcon ti(cueIn.icon);
  if (ti.isNull())
    ti = QIcon(stow->pmap);
  stow->entries.back().button = new SelectionButton(ti, QString(), this);
  stow->entries.back().button->isSingleSelection = cueIn.singleSelection;
  stow->entries.back().button->accrueDefault = cueIn.accrueDefault;
  stow->entries.back().button->mask = cueIn.mask;
  stow->entries.back().button->statusPrompt = cueIn.statusPrompt;
  stow->gridLayout->addWidget(stow->entries.back().label, row, 0, Qt::AlignVCenter | Qt::AlignRight);
  stow->gridLayout->addWidget(stow->entries.back().button, row, 1, Qt::AlignVCenter | Qt::AlignCenter);
  stow->buttonGroup->addButton(stow->entries.back().button);
  connect(stow->entries.back().button, &SelectionButton::advance, this, &SelectionWidget::advance);
  connect(stow->entries.back().button, &SelectionButton::toggled, this, &SelectionWidget::buttonToggled);
  
  stow->entries.back().model = new SelectionModel(this, stow->entries.back().button);
  connect(stow->entries.back().model, &SelectionModel::accrueChanged, this, &SelectionWidget::accrueChanged);
  stow->entries.back().view = new SelectionView(this, stow->gridLayout);
  stow->entries.back().view->setModel(stow->entries.back().model);
  stow->entries.back().view->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch); //has to be done after the model is set.
  stow->entries.back().view->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  stow->entries.back().view->setItemDelegateForColumn(1, stow->accrueDelegate);
  if (!cueIn.showAccrueColumn)
    stow->entries.back().view->hideColumn(1);
  stow->entries.back().view->setMinimumWidth(stow->selectionWidth);
  stow->stackedWidget->addWidget(stow->entries.back().view);
}

SelectionButton* SelectionWidget::getButton(int index) const
{
  assert(stow->validEntryIndex(index));
  return stow->entries.at(index).button;
}

const slc::Messages& SelectionWidget::getMessages(int index) const
{
  assert(stow->validEntryIndex(index));
  return stow->entries.at(index).button->getMessages();
}

/*! @brief Setup the button and list without triggering signals.
 * 
 * @parameter index of the button.
 * @parameter msIn is the new selection messages for the button.
 */
void SelectionWidget::initializeButton(int index, const slc::Message &mIn)
{
  assert(stow->validEntryIndex(index));
  stow->entries.at(index).button->setMessagesQuietly(mIn);
  stow->entries.at(index).model->populateList();
}

/*! @brief Setup the button and list without triggering signals.
 * 
 * @parameter index of the button.
 * @parameter msIn is the new selection messages for the button.
 */
void SelectionWidget::initializeButton(int index, const slc::Messages &msIn)
{
  assert(stow->validEntryIndex(index));
  stow->entries.at(index).button->setMessagesQuietly(msIn);
  stow->entries.at(index).model->populateList();
}

void SelectionWidget::buildGui(const std::vector<SelectionWidgetCue> &cuesIn)
{
  QVBoxLayout *mainLayout = new QVBoxLayout();
  this->setLayout(mainLayout);
  this->setContentsMargins(0, 0, 0, 0);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  QHBoxLayout *nestedLayout = new QHBoxLayout();
  nestedLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->addLayout(nestedLayout);
  mainLayout->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
  
  //labels and buttons
  auto *buttonLayout = new QVBoxLayout();
  stow->gridLayout = new QGridLayout();
  buttonLayout->addLayout(stow->gridLayout);
  buttonLayout->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
  nestedLayout->addLayout(buttonLayout);
  
  //StackedWidget
  stow->selectionWidth = 17 * fontMetrics().averageCharWidth();
  stow->accrueDelegate = new AccrueDelegate(this);
  stow->stackedWidget = new QStackedWidget(this);
  QSizePolicy adjust(stow->stackedWidget->sizePolicy());
  adjust.setVerticalPolicy(QSizePolicy::Maximum);
  stow->stackedWidget->setSizePolicy(adjust);
  auto *stackLayout = new QVBoxLayout();
  stackLayout->addWidget(stow->stackedWidget);
  nestedLayout->addLayout(stackLayout);
  
  for (const auto &n : cuesIn)
    addButton(n);
}

/*! @brief move to the next button.
 * 
 * @details does nothing if the last button is currently pressed.
 */
void SelectionWidget::advance()
{
  SelectionButton *sb = dynamic_cast<SelectionButton *>(sender());
  assert(sb);
  if (!sb)
    return;
  assert(sb->isChecked());
  if(!sb->isChecked())
    return;
  auto oi = stow->findButtonIndex(sb); //optional index
  assert(oi);
  if (!oi)
    return;
  
  SelectionButton *nb = nullptr; // next button
  for (int index = oi.get() + 1; index < static_cast<int>(stow->entries.size()); ++index)
  {
    if (stow->entries.at(index).isShown())
    {
      nb = stow->entries.at(index).button;
      break;
    }
  }
  if (!nb)
  {
    finishedSignal();
    return;
  }
  nb->setChecked(true);
}

void SelectionWidget::buttonToggled(bool bState)
{
  //we don't care about button unchecked
  if (!bState)
    return;
  
  SelectionButton *sb = dynamic_cast<SelectionButton *>(sender());
  assert(sb);
  if (!sb)
    return;
  auto oi = stow->findButtonIndex(sb); //optional index
  assert(oi);
  if (!oi)
    return;
  //shouldn't have to worry about a hidden button being toggled down
  assert(oi.get() >= 0 && oi.get() < stow->stackedWidget->count());
  if (bState)
    stow->stackedWidget->setCurrentIndex(oi.get());
}

void SelectionWidget::activate(int index)
{
  if (!(stow->validEntryIndex(index)))
    return;
  if (stow->entries.at(index).isHidden())
    return;
  QTimer::singleShot(0, stow->entries.at(index).button, SLOT(setFocus()));
  QTimer::singleShot(0, stow->entries.at(index).button, &QPushButton::click);
}

void SelectionWidget::setAngle(double aIn)
{
  for (auto &e : stow->entries)
  {
    for (int i = 0; i < e.model->rowCount(QModelIndex()); ++i)
    {
      e.model->setData(e.model->index(i, 0), aIn, Qt::UserRole);
    }
  }
}

void SelectionWidget::reset()
{
  for (auto &e : stow->entries)
    e.button->clear();
  activate(0);
}

void SelectionWidget::showEntry(int index)
{
  assert(stow->validEntryIndex(index));
  if (!stow->validEntryIndex(index))
    return;
  stow->entries.at(index).show();
}

void SelectionWidget::hideEntry(int index)
{
  assert(stow->validEntryIndex(index));
  if (!stow->validEntryIndex(index))
    return;
  stow->entries.at(index).hide();
}
