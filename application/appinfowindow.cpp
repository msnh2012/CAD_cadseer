/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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

#include <QHBoxLayout>
#include <QSettings>
#include <QShowEvent>
#include <QResizeEvent>
#include <QCloseEvent>

#include "message/msgmessage.h"
#include "message/msgnode.h"
#include "message/msgsift.h"
#include "application/appmessage.h"
#include "application/appapplication.h"
#include "application/appinfowindow.h"

using namespace app;

InfoWindow::InfoWindow(QWidget* parent) : QTextEdit(parent)
{
  node = std::make_unique<msg::Node>();
  node->connect(msg::hub());
  sift = std::make_unique<msg::Sift>();
  sift->name = "app::InfoWindow";
  node->setHandler(std::bind(&msg::Sift::receive, sift.get(), std::placeholders::_1));
  
  setupDispatcher();
  
  this->setWordWrapMode(QTextOption::NoWrap);
}

InfoWindow::~InfoWindow()
{
}

void InfoWindow::setupDispatcher()
{
  sift->insert
  (
    {
      std::make_pair
      (
        msg::Request | msg::Info | msg::Text
        , std::bind(&InfoWindow::infoTextDispatched, this, std::placeholders::_1)
      )
    }
  );
}

void InfoWindow::infoTextDispatched(const msg::Message &messageIn)
{
  app::Message message = messageIn.getAPP();
  append(message.infoMessage);
}

InfoDialog::InfoDialog(QWidget *parent) : QDialog(parent)
{
  setWindowTitle(tr("Information Window"));
  infoWindow = new InfoWindow(this);
  QHBoxLayout *layout = new QHBoxLayout();
  layout->addWidget(infoWindow);
  this->setLayout(layout);
}

void InfoDialog::showEvent(QShowEvent *event)
{
  restoreSettings();
  QDialog::showEvent(event);
}

void InfoDialog::resizeEvent(QResizeEvent *event)
{
  saveSettings();
  QDialog::resizeEvent(event);
}

void InfoDialog::closeEvent(QCloseEvent *event)
{
  saveSettings();
  QDialog::closeEvent(event);
}

void InfoDialog::restoreSettings()
{
  QSettings &settings = app::instance()->getUserSettings();
  settings.beginGroup("infoDialog");
  restoreGeometry(settings.value("dialogGeometry").toByteArray());
  settings.endGroup();
}

void InfoDialog::saveSettings()
{
  QSettings &settings = app::instance()->getUserSettings();
  settings.beginGroup("infoDialog");
  settings.setValue("dialogGeometry", saveGeometry());
  settings.endGroup();
}
