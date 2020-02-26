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

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "message/msgsift.h"
#include "commandview/cmvbase.h"

using namespace cmv;

/*! @brief construct base with name
 *  @note name should be unique.
 */
Base::Base(const QString &nIn)
: QWidget(nullptr)
, node(std::make_unique<msg::Node>())
, sift(std::make_unique<msg::Sift>())
, project(app::instance()->getProject())
, name(nIn)
{
  node->connect(msg::hub());
  node->setHandler(std::bind(&msg::Sift::receive, sift.get(), std::placeholders::_1));
  sift->name = name.toStdString();
  
  QSettings &settings = app::instance()->getUserSettings();
  settings.beginGroup(name);
  paneWidth = settings.value("paneWidth", 100).toInt();
  settings.endGroup();
}

Base::~Base() = default;

//called from command so it can filter on active.
void Base::setPaneWidth(int nw) 
{
  paneWidth = nw;
  QSettings &settings = app::instance()->getUserSettings();
  settings.beginGroup(name);
  settings.setValue("paneWidth", paneWidth);
  settings.endGroup();
}
