/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2018  Thomas S. Anderson blobfish.at.gmx.com
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

#include <QPainter>

#include "dialogs/dlgabout.h"
#include "ui_dlgabout.h" //in build directory

using namespace dlg;

About::About(QWidget *parent) : QDialog(parent), ui(new Ui::AboutDialog())
{
  ui->setupUi(this);
  
  int iconSize = 64;
  QPixmap temp = QPixmap(":/resources/images/cadSeer.svg").scaled(iconSize, iconSize, Qt::KeepAspectRatio);
  QPixmap out(iconSize, iconSize);
  QPainter painter(&out);
  painter.fillRect(out.rect(), this->palette().color(QPalette::Window));
  painter.drawPixmap(out.rect(), temp, temp.rect());
  painter.end();
  
  ui->iconLabel->setPixmap(out);
}

About::~About()
{
  delete ui;
}
