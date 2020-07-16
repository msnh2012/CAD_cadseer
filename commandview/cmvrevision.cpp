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
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QAction>
#include <QCloseEvent>
#include <QDialogButtonBox>
#include <QDialog>
#include <QMessageBox>
#include <QScrollBar>

#include "application/appapplication.h"
#include "application/appmainwindow.h"
#include "project/prjproject.h"
#include "project/prjgitmanager.h"
#include "project/prjmessage.h"
#include "message/msgmessage.h"
#include "message/msgnode.h"
#include "dialogs/dlgsplitterdecorated.h"
#include "dialogs/dlgcommitwidget.h"
#include "dialogs/dlgtagwidget.h"
#include "command/cmdrevision.h"
#include "commandview/cmvrevision.h"

using boost::uuids::uuid;

namespace cmv
{
  struct UndoPage::Data
  {
    std::vector<git2::Commit> commits;
  };
  
  struct AdvancedPage::Data
  {
    std::vector<git2::Tag> tags;
    git2::Commit currentHead;
    QIcon headIcon;
  };
}

using namespace cmv;

UndoPage::UndoPage(QWidget *parent) :
QWidget(parent),
data(new UndoPage::Data())
{
  buildGui();
  fillInCommitList();
  if (!data->commits.empty())
  {
    commitList->setCurrentRow(0);
    commitList->item(0)->setSelected(true);
  }
}

UndoPage::~UndoPage(){}

void UndoPage::buildGui()
{
  QHBoxLayout *mainLayout = new QHBoxLayout();
  this->setLayout(mainLayout);
  
  commitList = new QListWidget(this);
  commitList->setSelectionMode(QAbstractItemView::SingleSelection);
  commitList->setContextMenuPolicy(Qt::ActionsContextMenu);
  QAction *resetAction = new QAction(tr("Reset"), this);
  commitList->addAction(resetAction);
  commitList->setWhatsThis(tr("A list of commits to current branch. List is in descending order. Top is the latest"));
  mainLayout->addWidget(commitList);
  
  commitWidget = new dlg::CommitWidget(this);
  mainLayout->addWidget(commitWidget);
  
  connect(commitList, &QListWidget::currentRowChanged, this, &UndoPage::commitRowChangedSlot);
  connect(resetAction, &QAction::triggered, this, &UndoPage::resetActionSlot);
}

void UndoPage::fillInCommitList()
{
  prj::Project *p = app::instance()->getProject();
  assert(p);
  prj::GitManager &gm = p->getGitManager();
  data->commits = gm.getCommitsHeadToNamed("main");
  for (const auto &c : data->commits)
  {
    std::string idString = c.oid().format();
    commitList->addItem(QString::fromStdString(idString.substr(0, 12)));
  }
  
  commitList->setFixedWidth(commitList->sizeHintForColumn(0) + 2 * commitList->frameWidth() + commitList->verticalScrollBar()->sizeHint().width());
  commitList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void UndoPage::commitRowChangedSlot(int r)
{
  assert(static_cast<std::size_t>(r) < data->commits.size());
  commitWidget->setCommit(data->commits.at(r));
}

void UndoPage::resetActionSlot()
{
  int r = commitList->currentRow();
  assert(static_cast<std::size_t>(r) < data->commits.size());
  
  app::Application *application = app::instance();
  std::string pdir = application->getProject()->getSaveDirectory().string();
  app::instance()->messageSlot(msg::Mask(msg::Request | msg::Close | msg::Project));
  
  /* keep in mind the project is closed, so the git manager in the project is gone.
   * when the project opens, the project will build a git manager. So here we have the potential
   * of having 2 git managers alive at the same time. Going to ensure that is not the case
   * with a unique ptr.
   */
  std::unique_ptr<prj::GitManager> localManager(new prj::GitManager());
  localManager->open(pdir);
  localManager->resetHard(data->commits.at(r).oid().format());
  localManager.release();
  
  prj::Message pMessage;
  pMessage.directory = pdir;
  app::instance()->messageSlot(msg::Message(msg::Mask(msg::Request | msg::Open | msg::Project), pMessage));
  
  app::instance()->queuedMessage(msg::Message(msg::Mask(msg::Request | msg::Command | msg::Done)));
}

AdvancedPage::AdvancedPage(QWidget *parent) :
QWidget(parent),
data(new AdvancedPage::Data())
{
  data->headIcon = QIcon(":/resources/images/trafficGreen.svg");
  
  buildGui();
  init();
}

AdvancedPage::~AdvancedPage(){}

void AdvancedPage::init()
{
  tagList->clear();
  tagWidget->clear();
  data->tags.clear();
  
  setCurrentHead();
  fillInTagList();
  if (!data->tags.empty())
  {
    tagList->setCurrentRow(0);
    tagList->item(0)->setSelected(true);
    tagList->setFixedWidth(tagList->sizeHintForColumn(0) + 2 * tagList->frameWidth() + tagList->verticalScrollBar()->sizeHint().width());
  }
}

void AdvancedPage::buildGui()
{
  QLabel *revisionLabel = new QLabel(tr("Revisions:"), this);
  QHBoxLayout *rhl = new QHBoxLayout();
  rhl->addWidget(revisionLabel);
  
  tagList = new QListWidget(this);
  tagList->setSelectionMode(QAbstractItemView::SingleSelection);
  tagList->setContextMenuPolicy(Qt::ActionsContextMenu);
  tagList->setWhatsThis
  (
    tr
    (
      "List of project revisions. "
      "Traffic signal labels a revision that is current. "
      "Can not have identical revisions"
    )
  );
  
  QVBoxLayout *revisionLayout = new QVBoxLayout();
  revisionLayout->setContentsMargins(0, 0, 0, 0);
  revisionLayout->addLayout(rhl);
  revisionLayout->addWidget(tagList);
  
  tagWidget = new dlg::TagWidget(this);
  
  QHBoxLayout *mainLayout = new QHBoxLayout();
  this->setLayout(mainLayout);
  mainLayout->addLayout(revisionLayout);
  mainLayout->addWidget(tagWidget);
  
  createTagAction = new QAction(tr("Create New Revision"), this);
  tagList->addAction(createTagAction);
  connect(createTagAction, &QAction::triggered, this, &AdvancedPage::createTagSlot);
  
  checkoutTagAction = new QAction(tr("Checkout Revision"), this);
  tagList->addAction(checkoutTagAction);
  connect(checkoutTagAction, &QAction::triggered, this, &AdvancedPage::checkoutTagSlot);
  
  destroyTagAction = new QAction(tr("Destroy Selected Revision"), this);
  tagList->addAction(destroyTagAction);
  connect(destroyTagAction, &QAction::triggered, this, &AdvancedPage::destroyTagSlot);
  
  connect(tagList, &QListWidget::currentRowChanged, this, &AdvancedPage::tagRowChangedSlot);
}

void AdvancedPage::tagRowChangedSlot(int r)
{
  if (r == -1)
  {
    tagWidget->clear();
    destroyTagAction->setDisabled(true);
    checkoutTagAction->setDisabled(true);
    return;
  }
  assert(static_cast<std::size_t>(r) < data->tags.size());
  tagWidget->setTag(data->tags.at(r));
  if (data->tags.at(r).targetOid() == data->currentHead.oid())
    checkoutTagAction->setDisabled(true);
  else
    checkoutTagAction->setEnabled(true);
  destroyTagAction->setEnabled(true);
}

void AdvancedPage::createTagSlot()
{
  //hack together a dialog.
  std::unique_ptr<QDialog> ntDialog(new QDialog(this));
  ntDialog->setWindowTitle(tr("Create New Revision"));
  QVBoxLayout *mainLayout = new QVBoxLayout();
  
  QLabel *nameLabel = new QLabel(tr("Name:"), ntDialog.get());
  QLineEdit *nameEdit = new QLineEdit(ntDialog.get());
  nameEdit->setWhatsThis(tr("Name For The New Revision"));
  nameEdit->setPlaceholderText(tr("Name For The New Revision"));
  QHBoxLayout *nameLayout = new QHBoxLayout();
  nameLayout->addWidget(nameLabel);
  nameLayout->addWidget(nameEdit);
  mainLayout->addLayout(nameLayout);
  
  QTextEdit *messageEdit = new QTextEdit(ntDialog.get());
  messageEdit->setWhatsThis(tr("Message For The New Revision"));
  messageEdit->setPlaceholderText(tr("Message For The New Revision"));
  mainLayout->addWidget(messageEdit);
  
  QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, ntDialog.get());
  mainLayout->addWidget(bb);
  connect(bb, &QDialogButtonBox::accepted, ntDialog.get(), &QDialog::accept);
  connect(bb, &QDialogButtonBox::rejected, ntDialog.get(), &QDialog::reject);
  
  ntDialog->setLayout(mainLayout);
  
  if (ntDialog->exec() != QDialog::Accepted)
    return;
  
  //make sure we don't already have a tag with that name
  std::string name = nameEdit->text().toStdString();
  for (const auto &t : data->tags)
  {
    if (t.name() == name)
    {
      QMessageBox::critical(ntDialog.get(), tr("Error"), tr("Name already exists"));
      return;
    }
  }
  
  prj::Project *p = app::instance()->getProject();
  assert(p);
  prj::GitManager &gm = p->getGitManager();
  gm.createTag(name, messageEdit->toPlainText().toStdString());
  init();
}

void AdvancedPage::destroyTagSlot()
{
  QList<QListWidgetItem*> items = tagList->selectedItems();
  if (items.isEmpty())
    return;
  int row = tagList->row(items.front());
  assert(row >= 0 && static_cast<std::size_t>(row) < data->tags.size());
  if (row < 0 || static_cast<std::size_t>(row) > data->tags.size())
    return;
  prj::Project *p = app::instance()->getProject();
  assert(p);
  prj::GitManager &gm = p->getGitManager();
  gm.destroyTag(data->tags.at(static_cast<std::size_t>(row)).name());
  init();
}

void AdvancedPage::checkoutTagSlot()
{
  // we disable the checkout action when selected tag equals the current head
  // so we should be good to go here.
  
  QList<QListWidgetItem*> items = tagList->selectedItems();
  if (items.isEmpty())
    return;
  int row = tagList->row(items.front());
  assert(row >= 0 && static_cast<std::size_t>(row) < data->tags.size());
  if (row < 0 || static_cast<std::size_t>(row) > data->tags.size())
    return;
  
  //lets warn user if current head is not tagged.
  bool currentTagged = false;
  for (const auto &t : data->tags)
  {
    if (t.targetOid() == data->currentHead.oid())
      currentTagged = true;
  }
  if (!currentTagged)
  {
    if
    (
      QMessageBox::question
      (
        this,
        tr("Warning"),
        tr("No revision at current state. Work will be lost. Continue?")
      ) != QMessageBox::Yes
    )
    return;
  }
  
  /* 1) close the project
   * 2) update the git repo.
   * 3) reopen the project
   * see UndoPage::resetActionSlot() for why we dynamically allocate a new git manager.
   */
  app::Application *application = app::instance();
  std::string pdir = application->getProject()->getSaveDirectory().string();
  app::instance()->messageSlot(msg::Mask(msg::Request | msg::Close | msg::Project));
  
  std::unique_ptr<prj::GitManager> localManager(new prj::GitManager());
  localManager->open(pdir);
  localManager->checkoutTag(data->tags.at(static_cast<std::size_t>(row)));
  localManager.release();
  
  prj::Message pMessage;
  pMessage.directory = pdir;
  app::instance()->messageSlot(msg::Message(msg::Mask(msg::Request | msg::Open | msg::Project), pMessage));
  
  app::instance()->queuedMessage(msg::Message(msg::Mask(msg::Request | msg::Command | msg::Done)));
}

void AdvancedPage::fillInTagList()
{
  createTagAction->setEnabled(true);
  
  prj::Project *p = app::instance()->getProject();
  assert(p);
  prj::GitManager &gm = p->getGitManager();
  data->tags = gm.getTags();
  for (const auto &t : data->tags)
  {
    if (t.targetOid() == data->currentHead.oid())
    {
      new QListWidgetItem(data->headIcon, QString::fromStdString(t.name()), tagList);
      createTagAction->setDisabled(true); //don't create more than 1 tag at any one commit.
    }
    else
      tagList->addItem(QString::fromStdString(t.name()));
  }
}

void AdvancedPage::setCurrentHead()
{
  prj::Project *p = app::instance()->getProject();
  assert(p);
  prj::GitManager &gm = p->getGitManager();
  data->currentHead = gm.getCurrentHead();
}

struct Revision::Stow
{
  cmd::Revision *command;
  cmv::Revision *view;
  QTabWidget *tabWidget;
  
  Stow(cmd::Revision *cIn, cmv::Revision *vIn)
  : command(cIn)
  , view(vIn)
  {
    buildGui();
    
    QSettings &settings = app::instance()->getUserSettings();
    settings.beginGroup("cmv::Revision");
    //load settings
    settings.endGroup();
  }
  
  void buildGui()
  {
    view->setContentsMargins(0, 0, 0, 0);
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setContentsMargins(0, 0, 0, 0);
    view->setLayout(mainLayout);
  
    tabWidget = new QTabWidget(view);
    tabWidget->addTab(new UndoPage(tabWidget), tr("Undo")); //addTab reparents.
    tabWidget->addTab(new AdvancedPage(tabWidget), tr("Advanced"));
    mainLayout->addWidget(tabWidget);
  }
};

Revision::Revision(cmd::Revision *cIn)
: Base("cmv::Revision")
, stow(new Stow(cIn, this))
{}

Revision::~Revision() = default;
