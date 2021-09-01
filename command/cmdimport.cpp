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

#include <STEPControl_Reader.hxx>
#include <IGESControl_Reader.hxx>
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
  if (shouldUpdate)
    node->sendBlocked(msg::Message(msg::Request | msg::View | msg::Fit));
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
    "Supported Formats"
    " (*.brep *.brp"
    " *.step *.stp"
    " *.iges *.igs"
    " *.off"
    " *.ply"
    " *.stl"
    ")"
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
    boost::filesystem::path currentFilePath = fn.toStdString(); //current path.
    
    if (fn.endsWith(QObject::tr(".brep")) || fn.endsWith(QObject::tr(".brp")))
    {
      project->readOCC(currentFilePath.string());
      node->sendBlocked(msg::buildStatusMessage("OCCT Imported", 2.0));
      shouldUpdate = true;
    }
    else if (fn.endsWith(QObject::tr(".step")) || fn.endsWith(QObject::tr(".stp")))
    {
      goStep(currentFilePath);
      shouldUpdate = true;
    }
    else if (fn.endsWith(QObject::tr(".iges")) || fn.endsWith(QObject::tr(".igs")))
    {
      goIges(currentFilePath);
      shouldUpdate = true;
    }
    else if (fn.endsWith(QObject::tr(".off")) || fn.endsWith(QObject::tr(".ply")) || fn.endsWith(QObject::tr(".stl")))
    {
      auto *meshFeature = new ftr::SurfaceMesh::Feature();
      project->addFeature(std::unique_ptr<ftr::SurfaceMesh::Feature>(meshFeature));
      meshFeature->setName(QString::fromStdString(currentFilePath.stem().string()));
      
      std::unique_ptr<ann::SurfaceMesh> mesh = std::make_unique<ann::SurfaceMesh>();
      if (fn.endsWith(QObject::tr(".off")))
        mesh->readOFF(currentFilePath);
      else if (fn.endsWith(QObject::tr(".ply")))
        mesh->readPLY(currentFilePath);
      else if (fn.endsWith(QObject::tr(".stl")))
        mesh->readSTL(currentFilePath);
      meshFeature->setMesh(std::move(mesh));
      
      meshFeature->updateModel(project->getPayload(meshFeature->getId()));
      meshFeature->updateVisual();
      meshFeature->setModelDirty();
      node->sendBlocked(msg::buildStatusMessage("Surface Mesh Imported", 2.0));
      shouldUpdate = true;
    }
  }
  
  node->sendBlocked(msg::Message(msg::Request | msg::View | msg::Fit));
}

void Import::outputShape(const TopoDS_Shape &shapeIn, const std::string &namePrefix)
{
  occt::ShapeVector ncs = occt::getNonCompounds(shapeIn);
  if (ncs.empty())
    return;
  
  auto add = [&](const TopoDS_Shape &sIn, const std::string &nameIn)
  {
    auto *inert = new ftr::Inert::Feature(sIn);
    project->addFeature(std::unique_ptr<ftr::Inert::Feature>(inert));
    inert->setName(QString::fromStdString(nameIn));
    inert->updateModel(project->getPayload(inert->getId()));
    inert->updateVisual();
    inert->setModelDirty();
    shouldUpdate = true;
  };
  
  if (ncs.size() == 1)
  {
    add(ncs.front(), namePrefix);
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
    add(s, name);
  }
}

void Import::goStep(const boost::filesystem::path &filePath)
{
  std::string baseName = filePath.stem().string();
  
  STEPControl_Reader scr; //step control reader.
  if (scr.ReadFile(filePath.string().c_str()) != IFSelect_RetDone)
  {
    std::string em("Failed To Read: ");
    em += filePath.string().c_str();
    node->sendBlocked(msg::buildStatusMessage(em, 2.0));
    return;
  }
  
  //todo check units!
  
  scr.TransferRoots();
  int nos = scr.NbShapes(); //number of shapes.
  
  if (nos < 1)
  {
    std::string em("No Shapes In File: ");
    em += filePath.string().c_str();
    node->sendBlocked(msg::buildStatusMessage(em, 2.0));
    return;
  }
  else if (nos == 1)
  {
    outputShape(scr.Shape(1), baseName);
    return;
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
    outputShape(s, currentName);
    node->sendBlocked(msg::buildStatusMessage("Step Imported", 2.0));
  }
}

void cmd::Import::goIges(const boost::filesystem::path &filePathIn)
{
  std::string baseName = filePathIn.stem().string();
  
  IGESControl_Reader reader;
  reader.SetReadVisible(Standard_True);
  auto readResult = reader.ReadFile(filePathIn.string().c_str());
  if (readResult == IFSelect_RetVoid)
  {
    std::string error = QObject::tr("Nothing To Translate For File: ").toStdString() + filePathIn.string();
    node->sendBlocked(msg::buildStatusMessage(error, 2.0));
    return;
  }
  else if(readResult == IFSelect_RetError)
  {
    std::string error = QObject::tr("Input Data Error For File: ").toStdString() + filePathIn.string();
    node->sendBlocked(msg::buildStatusMessage(error, 2.0));
    return;
  }
  else if(readResult == IFSelect_RetFail)
  {
    std::string error = QObject::tr("Execution Failed Error For File: ").toStdString() + filePathIn.string();
    node->sendBlocked(msg::buildStatusMessage(error, 2.0));
    return;
  }
  else if(readResult == IFSelect_RetStop)
  {
    std::string error = QObject::tr("Execution Stopped For File: ").toStdString() + filePathIn.string();
    node->sendBlocked(msg::buildStatusMessage(error, 2.0));
    return;
  }
  
  reader.TransferRoots();
  
  int nos = reader.NbShapes();
  tls::NameIndexer outer(nos);
  for (int i = 1; i < nos + 1; ++i)
  {
    TopoDS_Shape s = reader.Shape(i);
    if (s.IsNull())
    {
      outer.bump();
      continue;
    }
    std::string currentName = baseName + "_";
    currentName += outer.buildSuffix();
    outputShape(s, currentName);
    node->sendBlocked(msg::buildStatusMessage("Step Imported", 2.0));
  }
}
