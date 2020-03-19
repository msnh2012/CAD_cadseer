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
  QButtonGroup *buttonGroup = nullptr; //obsolete
  QStackedWidget *stackedWidget = nullptr;
  std::vector<SelectionButton*> selectionButtons;
  std::vector<SelectionModel*> selectionModels;
  
  boost::optional<int> findButtonIndex(const SelectionButton *sbIn)
  {
    auto it = std::find(selectionButtons.begin(), selectionButtons.end(), sbIn);
    if (it != selectionButtons.end())
      return static_cast<int>(std::distance(selectionButtons.begin(), it));
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
  return static_cast<int>(stow->selectionButtons.size());
}

SelectionButton* SelectionWidget::getButton(int index) const
{
  assert(index >= 0 && index < static_cast<int>(stow->selectionButtons.size()));
  return stow->selectionButtons.at(index);
}

const slc::Messages& SelectionWidget::getMessages(int index) const
{
  assert(index >= 0 && index < static_cast<int>(stow->selectionButtons.size()));
  return stow->selectionButtons.at(index)->getMessages();
}

/*! @brief Setup the button and list without triggering signals.
 * 
 * @parameter index of the button.
 * @parameter msIn is the new selection messages for the button.
 */
void SelectionWidget::initializeButton(int index, const slc::Message &mIn)
{
  assert(index >= 0 && index < static_cast<int>(stow->selectionButtons.size()));
  stow->selectionButtons.at(index)->setMessagesQuietly(mIn);
  stow->selectionModels.at(index)->populateList();
}

/*! @brief Setup the button and list without triggering signals.
 * 
 * @parameter index of the button.
 * @parameter msIn is the new selection messages for the button.
 */
void SelectionWidget::initializeButton(int index, const slc::Messages &msIn)
{
  assert(index >= 0 && index < static_cast<int>(stow->selectionButtons.size()));
  stow->selectionButtons.at(index)->setMessagesQuietly(msIn);
  stow->selectionModels.at(index)->populateList();
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
  QPixmap pmap = QPixmap(":resources/images/cursor.svg").scaled(32, 32, Qt::KeepAspectRatio);
  QGridLayout *gridLayout = new QGridLayout();
  int index = 0;
  for (const auto &n : cuesIn)
  {
    QLabel *label = new QLabel(n.name, this);
    QIcon ti(n.icon);
    if (ti.isNull())
      ti = QIcon(pmap);
    SelectionButton *sb = new SelectionButton(ti, QString(), this);
    sb->isSingleSelection = n.singleSelection;
    sb->accrueDefault = n.accrueDefault;
    sb->mask = n.mask;
    sb->statusPrompt = n.statusPrompt;
    gridLayout->addWidget(label, index, 0, Qt::AlignVCenter | Qt::AlignRight);
    gridLayout->addWidget(sb, index, 1, Qt::AlignVCenter | Qt::AlignCenter);
    stow->selectionButtons.push_back(sb);
    stow->buttonGroup->addButton(sb);
    connect(sb, &SelectionButton::advance, this, &SelectionWidget::advance);
    connect(sb, &SelectionButton::toggled, this, &SelectionWidget::buttonToggled);
    index++;
  }
  auto *buttonLayout = new QVBoxLayout();
  buttonLayout->addLayout(gridLayout);
  buttonLayout->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
  nestedLayout->addLayout(buttonLayout);
  
  //stackedWidgets
  int slw = 17 * fontMetrics().averageCharWidth(); //selection width
  AccrueDelegate *delegate = new AccrueDelegate(this);
  stow->stackedWidget = new QStackedWidget(this);
  QSizePolicy adjust(stow->stackedWidget->sizePolicy());
  adjust.setVerticalPolicy(QSizePolicy::Maximum);
  stow->stackedWidget->setSizePolicy(adjust);
  
  for (int index = 0; index < static_cast<int>(cuesIn.size()); ++index)
  {
    SelectionModel *sm = new SelectionModel(this, stow->selectionButtons.at(index));
    connect(sm, &SelectionModel::accrueChanged, this, &SelectionWidget::accrueChanged);
    SelectionView *sv = new SelectionView(this, gridLayout);
    sv->setModel(sm);
    sv->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch); //has to be done after the model is set.
    sv->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    sv->setItemDelegateForColumn(1, delegate);
    if (!cuesIn.at(index).showAccrueColumn)
      sv->hideColumn(1);
    sv->setMinimumWidth(slw);
    stow->stackedWidget->addWidget(sv);
    stow->selectionModels.push_back(sm);
  }
  auto *stackLayout = new QVBoxLayout();
  stackLayout->addWidget(stow->stackedWidget);
  nestedLayout->addLayout(stackLayout);
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
  
  if (oi.get() >= static_cast<int>(stow->selectionButtons.size()) - 1)
  {
    finishedSignal();
    return;
  }
  stow->selectionButtons.at(oi.get() + 1)->setChecked(true);
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
  assert(oi.get() >= 0 && oi.get() < stow->stackedWidget->count());
  if (bState)
    stow->stackedWidget->setCurrentIndex(oi.get());
}

void SelectionWidget::activate(int index)
{
  if (index < 0 || index >= static_cast<int>(stow->selectionButtons.size()))
    return;
  QTimer::singleShot(0, stow->selectionButtons.at(index), SLOT(setFocus()));
  QTimer::singleShot(0, stow->selectionButtons.at(index), &QPushButton::click);
}

void SelectionWidget::setAngle(double aIn)
{
  for (auto *m : stow->selectionModels)
  {
    for (int i = 0; i < m->rowCount(QModelIndex()); ++i)
    {
      m->setData(m->index(i, 0), aIn, Qt::UserRole);
    }
  }
}

void SelectionWidget::reset()
{
  for (auto *b : stow->selectionButtons)
    b->clear();
  activate(0);
}
