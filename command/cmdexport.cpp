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

#include <BRepTools.hxx>
#include <TopoDS_Compound.hxx>
#include <STEPControl_Writer.hxx>
#include <APIHeaderSection_MakeHeader.hxx>

#include "application/appmainwindow.h"
#include "application/appapplication.h"
#include "project/prjproject.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "annex/annseershape.h"
#include "annex/annsurfacemesh.h"
#include "mesh/mshmesh.h"
#include "mesh/mshconvert.h"
#include "feature/ftrbase.h"
#include "tools/occtools.h"
#include "command/cmdexport.h"

using namespace cmd;

Export::Export() : Base() {}

Export::~Export() = default;

std::string Export::getStatusMessage()
{
  return QObject::tr("Select Export File").toStdString();
}

void Export::activate()
{
  isActive = true;
  go();
  sendDone();
}

void Export::deactivate()
{
  isActive = false;
}

void Export::go()
{
  shouldUpdate = false;
  
  QString supportedFiles = QObject::tr
  (
    "brep (*.brep *.brp)"
    ";;step (*.step *.stp)"
    ";;off (*.off)"
    ";;ply (*.ply)"
    ";;stl (*.stl)"
//     ";;Scene (*.osgt *.osgx *.osgb *.osg *.ive)"
  );
  
  QString fileName = QFileDialog::getSaveFileName
  (
    app::instance()->getMainWindow(),
    QObject::tr("Save File"),
    QString::fromStdString(prf::manager().rootPtr->project().lastDirectory().get()),
    supportedFiles
  );
  if (fileName.isEmpty())
      return;
  
  boost::filesystem::path p = fileName.toStdString();
  prf::manager().rootPtr->project().lastDirectory() = p.remove_filename().string();
  prf::manager().saveConfig();
  
  app::WaitCursor wc; //show busy.
  qApp->processEvents();
  const slc::Containers &cs = eventHandler->getSelections();
  auto buildCompound = [&]() -> TopoDS_Compound
  {
    occt::ShapeVector out;
    if (cs.empty())
    {
      //build a compound for the entire project.
      for (const auto &id : project->getAllFeatureIds())
      {
        const ftr::Base *f = project->findFeature(id);
        assert(f);
        if (!f || !f->hasAnnex(ann::Type::SeerShape))
          continue;
        const ann::SeerShape &ss = f->getAnnex<ann::SeerShape>();
        if (ss.isNull())
          continue;
        occt::ShapeVector children = ss.useGetNonCompoundChildren();
        std::copy(children.begin(), children.end(), std::back_inserter(out));
      }
    }
    else
    {
      for (const auto &c : cs)
      {
        const ftr::Base *bf = project->findFeature(c.featureId);
        if (!bf || !bf->hasAnnex(ann::Type::SeerShape))
          continue;
        const ann::SeerShape &ss = bf->getAnnex<ann::SeerShape>();
        if (ss.isNull())
          continue;
        if (slc::isPointType(c.selectionType))
          continue;
        
        if (slc::isObjectType(c.selectionType))
        {
          occt::ShapeVector children = ss.useGetNonCompoundChildren();
          std::copy(children.begin(), children.end(), std::back_inserter(out));
        }
        else
        {
          if (!c.shapeId.is_nil())
          {
            assert(ss.hasId(c.shapeId));
            out.push_back(ss.getOCCTShape(c.shapeId));
          }
        }
      }
    }
    
    return static_cast<TopoDS_Compound>(occt::ShapeVectorCast(out));
  };
  
  if (fileName.endsWith(QObject::tr(".brep")) || fileName.endsWith(QObject::tr(".brp")))
  {
    //export occt ascii.
    TopoDS_Compound out = buildCompound();
    if (out.IsNull())
    {
      node->sendBlocked(msg::buildStatusMessage("Invalid Export", 2.0));
      return;
    }
    BRepTools::Write(out, fileName.toStdString().c_str());
    
    node->sendBlocked(msg::buildStatusMessage("OCCT Exported", 2.0));
    node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return;
  }
  
  if (fileName.endsWith(QObject::tr(".step")) || fileName.endsWith(QObject::tr(".stp")))
  {
    TopoDS_Compound out = buildCompound();
    if (out.IsNull())
    {
      node->sendBlocked(msg::buildStatusMessage("Invalid Export", 2.0));
      return;
    }
    
    STEPControl_Writer stepOut;
    if (stepOut.Transfer(out, STEPControl_AsIs) != IFSelect_RetDone)
    {
      node->send(msg::buildStatusMessage("Step Translation Failed", 2.0));
      return;
    }
    
    std::string author = prf::manager().rootPtr->project().gitName();
    APIHeaderSection_MakeHeader header(stepOut.Model());
    header.SetName(new TCollection_HAsciiString(fileName.toUtf8().data()));
    header.SetOriginatingSystem(new TCollection_HAsciiString("CadSeer"));
    header.SetAuthorValue (1, new TCollection_HAsciiString(author.c_str()));
    header.SetOrganizationValue (1, new TCollection_HAsciiString(author.c_str()));
//     header.SetDescriptionValue(1, new TCollection_HAsciiString(feature->getName().toUtf8().data()));
    header.SetAuthorisation(new TCollection_HAsciiString(author.c_str()));
    
    if (stepOut.Write(fileName.toStdString().c_str()) != IFSelect_RetDone)
    {
      node->send(msg::buildStatusMessage("Step Write Failed", 2.0));
      return;
    }
    node->sendBlocked(msg::buildStatusMessage("Step Exported", 2.0));
    node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return;
  }
  
  if (fileName.endsWith(QObject::tr(".off")) || fileName.endsWith(QObject::tr(".ply")) || fileName.endsWith(QObject::tr(".stl")))
  {
    msh::srf::Mesh totalMesh;
    for (const auto &s : cs)
    {
      ftr::Base *feature = project->findFeature(s.featureId);
      if (!feature->hasAnnex(ann::Type::SurfaceMesh))
        continue;
      const ann::SurfaceMesh &sf = feature->getAnnex<ann::SurfaceMesh>(ann::Type::SurfaceMesh);
      totalMesh += sf.getStow().mesh;
      
    }
    if (!totalMesh.is_valid() || totalMesh.is_empty())
    {
      node->sendBlocked(msg::buildStatusMessage("Couldn't export invalid mesh", 2.0));
      return;
    }
    
    ann::SurfaceMesh tempMesh((msh::srf::Stow(totalMesh))); //vexing parse
    if (fileName.endsWith(QObject::tr(".off")))
    {
      if (tempMesh.writeOFF(fileName.toStdString().c_str()))
        node->sendBlocked(msg::buildStatusMessage("off file exported", 2.0));
      else
        node->sendBlocked(msg::buildStatusMessage("off file NOT exported", 2.0));
    }
    else if (fileName.endsWith(QObject::tr(".ply")))
    {
      if (tempMesh.writePLY(fileName.toStdString().c_str()))
        node->sendBlocked(msg::buildStatusMessage("ply file exported", 2.0));
      else
        node->sendBlocked(msg::buildStatusMessage("ply file NOT exported", 2.0));
    }
    else if (fileName.endsWith(QObject::tr(".stl")))
    {
      if (tempMesh.writeSTL(fileName.toStdString().c_str()))
        node->sendBlocked(msg::buildStatusMessage("stl file exported", 2.0));
      else
        node->sendBlocked(msg::buildStatusMessage("stl file NOT exported", 2.0));
    }
    
    node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return;
  }
}
