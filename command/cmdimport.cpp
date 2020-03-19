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

#include <QFileDialog>

#include <STEPControl_Reader.hxx>
#include <TopoDS_Iterator.hxx>

#include "application/appmainwindow.h"
#include "application/appapplication.h"
#include "project/prjproject.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "message/msgnode.h"
#include "annex/annsurfacemesh.h"
#include "feature/ftrinert.h"
#include "feature/ftrsurfacemesh.h"
#include "tools/occtools.h"
#include "tools/tlsnameindexer.h"
#include "command/cmdimport.h"

using namespace cmd;

Import::Import() : Base() {}

Import::~Import() = default;

std::string Import::getStatusMessage()
{
  return QObject::tr("Select geometry for Import feature").toStdString();
}

void Import::activate()
{
  isActive = true;
  go();
  sendDone();
}

void Import::deactivate()
{
  isActive = false;
}

void Import::go()
{
  shouldUpdate = false;
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  QString supportedFiles = QObject::tr
  (
    "brep (*.brep *.brp)"
    ";;step (*.step *.stp)"
    ";;off (*.off)"
    ";;ply (*.ply)"
    ";;stl (*.stl)"
  );
  
  QStringList fileNames = QFileDialog::getOpenFileNames
  (
    app::instance()->getMainWindow(),
    QObject::tr("Open File"),
    QString::fromStdString(prf::manager().rootPtr->project().lastDirectory().get()),
    supportedFiles
  );
  if (fileNames.isEmpty())
      return;
  
  boost::filesystem::path p = fileNames.at(0).toStdString();
  prf::manager().rootPtr->project().lastDirectory() = p.remove_filename().string();
  prf::manager().saveConfig();
  
  app::WaitCursor wc; //show busy.
  qApp->processEvents();
  assert(project);
  for (const auto &fn : fileNames)
  {
    boost::filesystem::path cp = fn.toStdString(); //current path.
    std::string baseName = cp.stem().string();
    
    if (fn.endsWith(QObject::tr(".brep")) || fn.endsWith(QObject::tr(".brp")))
    {
      project->readOCC(fn.toStdString());
      node->sendBlocked(msg::buildStatusMessage("OCCT Imported", 2.0));
      shouldUpdate = true;
    }
    else if (fn.endsWith(QObject::tr(".step")) || fn.endsWith(QObject::tr(".stp")))
    {
      STEPControl_Reader scr; //step control reader.
      if (scr.ReadFile(fn.toUtf8().constData()) != IFSelect_RetDone)
      {
        std::string em("Failed To Read: ");
        em += fn.toUtf8().constData();
        node->sendBlocked(msg::buildStatusMessage(em, 2.0));
        continue;
      }
      
      //todo check units!
      
      scr.TransferRoots();
      int nos = scr.NbShapes(); //number of shapes.
      
      auto outputShape = [&](int index, std::string namePrefix)
      {
        occt::ShapeVector ncs = occt::getNonCompounds(scr.Shape(index));
        if (ncs.empty())
          return;
        if (ncs.size() == 1)
        {
          std::shared_ptr<ftr::Inert> inert(new ftr::Inert(ncs.front()));
          project->addFeature(inert);
          inert->setName(QString::fromStdString(namePrefix));
          shouldUpdate = true;
          return;
        }
        tls::NameIndexer inner(ncs.size());
        for (const auto &s : ncs)
        {
          if (s.IsNull())
          {
            inner.bump();
            continue;
          }
          std::string name = namePrefix;
          name += "_" + inner.buildSuffix();
          std::shared_ptr<ftr::Inert> inert(new ftr::Inert(s));
          project->addFeature(inert);
          inert->setName(QString::fromStdString(name));
          shouldUpdate = true;
        }
      };
      
      if (nos < 1)
      {
        std::string em("No Shapes In File: ");
        em += fn.toUtf8().constData();
        node->sendBlocked(msg::buildStatusMessage(em, 2.0));
        continue;
      }
      else if (nos == 1)
      {
        outputShape(1, baseName);
        continue;
      }
      tls::NameIndexer outer(nos);
      for (int i = 1; i < nos + 1; ++i)
      {
        TopoDS_Shape s = scr.Shape(i);
        if (s.IsNull())
        {
          outer.bump();
          continue;
        }
        std::string currentName = baseName + "_";
        currentName += outer.buildSuffix();
        outputShape(i, currentName);
        node->sendBlocked(msg::buildStatusMessage("Step Imported", 2.0));
      }
    }
    else if (fn.endsWith(QObject::tr(".off")) || fn.endsWith(QObject::tr(".ply")) || fn.endsWith(QObject::tr(".stl")))
    {
      std::shared_ptr<ftr::SurfaceMesh> meshFeature(new ftr::SurfaceMesh());
      project->addFeature(meshFeature);
      meshFeature->setName(QString::fromStdString(baseName));
      
      std::unique_ptr<ann::SurfaceMesh> mesh = std::make_unique<ann::SurfaceMesh>();
      if (fn.endsWith(QObject::tr(".off")))
        mesh->readOFF(cp);
      else if (fn.endsWith(QObject::tr(".ply")))
        mesh->readPLY(cp);
      else if (fn.endsWith(QObject::tr(".stl")))
        mesh->readSTL(cp);
      meshFeature->setMesh(std::move(mesh));
      
      node->sendBlocked(msg::buildStatusMessage("Surface Mesh Imported", 2.0));
      shouldUpdate = true;
    }
  }
}
