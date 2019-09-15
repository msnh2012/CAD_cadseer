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
  QButtonGroup *buttonGroup = nullptr;
  QStackedWidget *stackedWidget = nullptr;
  std::vector<SelectionModel*> selectionModels;
};

SelectionWidget::SelectionWidget(QWidget *parent, const std::vector<SelectionWidgetCue> &cuesIn)
: QWidget(parent)
, stow(std::make_unique<SelectionWidget::Stow>())
{
  buildGui(cuesIn);
}

SelectionWidget::~SelectionWidget() = default;

int SelectionWidget::getButtonCount() const
{
  return stow->buttonGroup->buttons().size();
}

SelectionButton* SelectionWidget::getButton(int index) const
{
  assert(index >= 0 && index < stow->buttonGroup->buttons().size());
  return static_cast<SelectionButton*>(stow->buttonGroup->button(index));
}

const slc::Messages& SelectionWidget::getMessages(int index) const
{
  assert(index >= 0 && index < stow->buttonGroup->buttons().size());
  return static_cast<SelectionButton*>(stow->buttonGroup->button(index))->getMessages();
}

void SelectionWidget::buildGui(const std::vector<SelectionWidgetCue> &cuesIn)
{
  QHBoxLayout *mainLayout = new QHBoxLayout();
  mainLayout->setContentsMargins(0, 0, 0, 0);
  
  //labels and buttons
  QPixmap pmap = QPixmap(":resources/images/cursor.svg").scaled(32, 32, Qt::KeepAspectRatio);
  QGridLayout *gridLayout = new QGridLayout();
  stow->buttonGroup = new QButtonGroup(this);
  connect(stow->buttonGroup, SIGNAL(buttonToggled(QAbstractButton*, bool)), this, SLOT(buttonToggled(QAbstractButton*, bool)));
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
    stow->buttonGroup->addButton(sb, index);
    connect(sb, &SelectionButton::advance, this, &SelectionWidget::advance);
    index++;
  }
  auto *buttonLayout = new QVBoxLayout();
  buttonLayout->addLayout(gridLayout);
  buttonLayout->addStretch();
  mainLayout->addLayout(buttonLayout);
  
  //stackedWidgets
  AccrueDelegate *delegate = new AccrueDelegate(this);
  stow->stackedWidget = new QStackedWidget(this);
  for (int index = 0; index < static_cast<int>(cuesIn.size()); ++index)
  {
    SelectionModel *sm = new SelectionModel(this, static_cast<SelectionButton*>(stow->buttonGroup->button(index)));
    connect(sm, &SelectionModel::accrueChanged, this, &SelectionWidget::accrueChanged);
    SelectionView *sv = new SelectionView(this);
    sv->setModel(sm);
    sv->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch); //has to be done after the model is set.
    sv->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    sv->setItemDelegateForColumn(1, delegate);
    if (!cuesIn.at(index).showAccrueColumn)
      sv->hideColumn(1);
    stow->stackedWidget->addWidget(sv);
    stow->selectionModels.push_back(sm);
  }
  mainLayout->addWidget(stow->stackedWidget);
  
  this->setLayout(mainLayout);
}

/*! @brief move to the next button.
 * 
 * @details does nothing if the last button is currently pressed.
 */
void SelectionWidget::advance()
{
  int cb = stow->buttonGroup->checkedId();
  if (cb < 0)
    return;
  if (cb >= stow->buttonGroup->buttons().size() - 1)
    return;
  cb++;
  stow->buttonGroup->button(cb)->setChecked(true);
}

void SelectionWidget::buttonToggled(QAbstractButton *bIn, bool bState)
{
  SelectionButton *sButton = qobject_cast<SelectionButton*>(bIn);
  if (!sButton)
    return;
  
  int bi = stow->buttonGroup->id(bIn);
  assert (bi >= 0 && bi < stow->stackedWidget->count());
  if (bState)
    stow->stackedWidget->setCurrentIndex(bi);
}

void SelectionWidget::activate(int index)
{
  if (index < 0 || index >= stow->buttonGroup->buttons().size())
    return;
  QTimer::singleShot(0, stow->buttonGroup->button(index), SLOT(setFocus()));
  QTimer::singleShot(0, stow->buttonGroup->button(index), &QPushButton::click);
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
