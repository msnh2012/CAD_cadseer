/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2019 Thomas S. Anderson blobfish.at.gmx.com
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

#include <boost/filesystem/path.hpp>

#include <QFileDialog>

#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "application/appmainwindow.h"
#include "application/appapplication.h"
#include "project/prjproject.h"
#include "viewer/vwrwidget.h"
#include "message/msgnode.h"
#include "feature/ftrimageplane.h"
#include "command/cmdimageplane.h"

using namespace cmd;

ImagePlane::ImagePlane() : Base() {}

ImagePlane::~ImagePlane() {}

std::string ImagePlane::getStatusMessage()
{
  return QObject::tr("ImagePlane feature").toStdString();
}

void ImagePlane::activate()
{
  isActive = true;
  go();
  sendDone();
}

void ImagePlane::deactivate()
{
  isActive = false;
}

void ImagePlane::go()
{
  //I started to build a list of acceptable image formats supported by osg, so
  //I could pass into the file dialog. It became more than I wanted, so we will
  //just accept any file in the dialog and let osg fail to load the image.
  
  QStringList fileNames = QFileDialog::getOpenFileNames
  (
    app::instance()->getMainWindow(),
    QObject::tr("Open Files"),
    QString::fromStdString(prf::manager().rootPtr->project().lastDirectory().get()),
    QObject::tr("Image (*.*)")
  );
  if (fileNames.isEmpty())
      return;
  
  boost::filesystem::path prefPath = fileNames.at(0).toStdString();
  prf::manager().rootPtr->project().lastDirectory() = prefPath.remove_filename().string();
  prf::manager().saveConfig();
  
  for (const auto &fn : fileNames)
  {
    //temp
    boost::filesystem::path p = fn.toStdString();
    
    auto f = std::make_shared<ftr::ImagePlane>();
    std::string result = f->setImage(p);
    if (!result.empty())
    {
      std::string failMessage = "ImagePlane failed for: " + p.filename().string();
      node->sendBlocked(msg::buildStatusMessage(failMessage, 5.0));
      continue;
    }
    project->addFeature(f);
    f->setName(QString::fromStdString(p.filename().string()));
    f->setScale(viewer->getDiagonalLength() / 5.0);
    
    std::string successMessage = "ImagePlane created for: " + p.filename().string();
    node->sendBlocked(msg::buildStatusMessage(successMessage, 5.0));
  }
}
