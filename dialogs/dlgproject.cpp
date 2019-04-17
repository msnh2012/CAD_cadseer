/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2015  Thomas S. Anderson blobfish.at.gmx.com
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
#include <iostream>
#include <stdexcept>

#include <boost/filesystem.hpp>

#include <QDoubleValidator>
#include <QMessageBox>
#include <QFileDialog>
#include <QSettings>

#include "application/appapplication.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "dialogs/dlgwidgetgeometry.h"
#include "dialogs/dlgproject.h"
#include "ui_dlgproject.h" //in build directory

using namespace boost::filesystem;

using namespace dlg;

Project::Project(QWidget *parent) : QDialog(parent), ui(new Ui::projectDialog)
{
  ui->setupUi(this);
  populateRecentList();
  
  connect(ui->newButton, SIGNAL(clicked()), this, SLOT(goNewSlot()));
  connect(ui->openButton, SIGNAL(clicked()), this, SLOT(goOpenSlot()));
  connect(ui->recentTableWidget, SIGNAL(cellClicked(int,int)), this, SLOT(goRecentSlot(int,int)));
  
  dlg::WidgetGeometry *filter = new dlg::WidgetGeometry(this, "dlg::Project");
  this->installEventFilter(filter);
  
  QSettings &settings = app::instance()->getUserSettings();
  settings.beginGroup("dlg::Project");
  settings.beginGroup("RecentTable");
  ui->recentTableWidget->horizontalHeader()->restoreState(settings.value("header").toByteArray());
  settings.endGroup();
  settings.endGroup();
  
  //test the default project directory.
  path p = prf::manager().rootPtr->project().basePath();
  if (!exists(p))
  {
    ui->newNameEdit->setPlaceholderText(QObject::tr("Default project location doesn't exist"));
    ui->newNameEdit->setDisabled(true);
  }
  else
    ui->newNameEdit->setFocus();
}

Project::~Project()
{
  QSettings &settings = app::instance()->getUserSettings();
  settings.beginGroup("dlg::Project");
  settings.beginGroup("RecentTable");
  settings.setValue("header", ui->recentTableWidget->horizontalHeader()->saveState());
  settings.endGroup();
  settings.endGroup();
  
  delete ui;
}

void Project::populateRecentList()
{
  const auto &recent = prf::manager().rootPtr->project().recentProjects().Entry();
  prf::RecentProjects::EntrySequence reconcile;
  std::size_t row = 0;
  for (const auto &entry : recent)
  {
    path p = entry;
    if (!validateDir(p)) //only valid directories.
      continue;
    reconcile.push_back(entry);
    QString name = QString::fromStdString(p.stem().string());
    QString parentPath = QString::fromStdString(p.string());
    
    ui->recentTableWidget->insertRow(row);
    QTableWidgetItem *nameItem = new QTableWidgetItem(name);
    ui->recentTableWidget->setItem(row, 0, nameItem);
    QTableWidgetItem *pathItem = new QTableWidgetItem(parentPath);
    ui->recentTableWidget->setItem(row, 1, pathItem);
    
    row++;
  }
  
  prf::manager().rootPtr->project().recentProjects().Entry() = reconcile;
  prf::manager().saveConfig();
}

bool Project::validateDir(const path& dir)
{
  if (!exists(dir))
    return false;
  if (!exists(dir / "project.prjt"))
    return false;
  if (!exists(dir / "project.brep"))
    return false;
  if (!exists(dir / ".git"))
    return false;
  
  return true;
}

void Project::goNewSlot()
{
  path basePath = prf::manager().rootPtr->project().basePath();
  QString newNameText = ui->newNameEdit->text();
  if (exists(basePath) && (!newNameText.isEmpty()))
  {
    path newProjectPath = basePath /= newNameText.toStdString();
    if (exists(newProjectPath))
    {
      QMessageBox::critical(this, tr("Error"), tr("Project with name already exists"));
      return;
    }
    if (!create_directory(newProjectPath))
    {
      QMessageBox::critical(this, tr("Error"), tr("Couldn't create directory"));
      return;
    }
    result = Result::New;
    directory = newProjectPath.string();
    addToRecentList();
  }
  else
  {
    //browse dialog.
    path browsePath = basePath;
    if (!exists(browsePath))
      browsePath = prf::manager().rootPtr->project().lastDirectory().get();
    if (!exists(browsePath))
    {
      const char *home = std::getenv("HOME");
      if (home == NULL)
      {
        QMessageBox::critical(this, tr("Error"), tr("REALLY!? no home directory"));
        return;
      }
      
      browsePath = home;
      if (!exists(browsePath))
      {
        QMessageBox::critical(this, tr("Error"), tr("home directory doesn't exist"));
        return;
      }
    }
    
    QString freshDirectory = QFileDialog::getExistingDirectory
    (
      this,
      tr("Browse to new project directory"),
      QString::fromStdString(browsePath.string())
    );
    if (freshDirectory.isEmpty())
      return;
    browsePath = freshDirectory.toStdString();
    if (!is_empty(browsePath))
    {
      QMessageBox::critical(this, tr("Error"), tr("Expecting an empty directory"));
      return;
    }
    prf::manager().rootPtr->project().lastDirectory() = browsePath.string();
    prf::manager().saveConfig();
    result = Result::New;
    directory = freshDirectory.toStdString();
    addToRecentList();
  }
  
  this->accept();
}

void Project::goOpenSlot()
{
  path p = prf::manager().rootPtr->project().basePath();
  if (p.empty() || (!exists(p)))
    p = prf::manager().rootPtr->project().lastDirectory().get();
  QString freshDirectory = QFileDialog::getExistingDirectory
  (
    this,
    tr("Browse to existing project directory"),
    QString::fromStdString(p.string())
  );
  if (freshDirectory.isEmpty())
    return;
  
  directory = freshDirectory.toStdString();
  prf::manager().rootPtr->project().lastDirectory() = directory.string();
  prf::manager().saveConfig();
  
  result = Result::Open;
  addToRecentList();
  
  this->accept();
}

void Project::goRecentSlot(int rowIn, int)
{
  QTableWidgetItem *widget = ui->recentTableWidget->item(rowIn, 1);
  result = Result::Recent;
  directory = widget->text().toStdString();
  addToRecentList();
  
  this->accept();
}

void Project::addToRecentList()
{
  std::string freshEntry = directory.string();
  auto &recent = prf::manager().rootPtr->project().recentProjects().Entry();
  auto it = std::find(recent.begin(), recent.end(), freshEntry);
  if (it != recent.end())
    recent.erase(it);
  recent.insert(recent.begin(), freshEntry);
  prf::manager().saveConfig();
}
