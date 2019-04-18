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

#include <BRepTools.hxx>

#include <QFileDialog>

#include "tools/occtools.h"
#include "application/appmainwindow.h"
#include "application/appapplication.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "annex/annseershape.h"
#include "feature/ftrbase.h"
#include "command/cmdocctexport.h"

using namespace cmd;

OCCTExport::OCCTExport() : Base() {}

OCCTExport::~OCCTExport() {}

std::string OCCTExport::getStatusMessage()
{
  return QObject::tr("Select geometry for OCCTExport feature").toStdString();
}

void OCCTExport::activate()
{
  isActive = true;
  go();
  sendDone();
}

void OCCTExport::deactivate()
{
  isActive = false;
}

void OCCTExport::go()
{
  const slc::Containers &cs = eventHandler->getSelections();
  if (cs.empty())
  {
    node->sendBlocked(msg::buildStatusMessage("Incorrect selection for OCCTExport", 2.0));
    shouldUpdate = false;
    return;
  }
  
  QString fileName = QFileDialog::getSaveFileName
  (
    app::instance()->getMainWindow(),
    QObject::tr("Save File"),
    QString::fromStdString(prf::manager().rootPtr->project().lastDirectory().get()),
    QObject::tr("brep (*.brep *.brp)")
  );
  if (fileName.isEmpty())
      return;
  if
  (
    (!fileName.endsWith(QObject::tr(".brep"))) &&
    (!fileName.endsWith(QObject::tr(".brp")))
  )
    fileName += QObject::tr(".brep");
    
  boost::filesystem::path p = fileName.toStdString();
  prf::manager().rootPtr->project().lastDirectory() = p.remove_filename().string();
  prf::manager().saveConfig();
  
  //I guess for multiple picks I will pack everything into a compound.
  occt::ShapeVector out;
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
    
    if (c.selectionType == slc::Type::Object)
      out.push_back(ss.getRootOCCTShape());
    else
    {
      if (!c.shapeId.is_nil())
      {
        assert(ss.hasId(c.shapeId));
        out.push_back(ss.getOCCTShape(c.shapeId));
      }
    }
  }
  
  TopoDS_Compound compoundOut = static_cast<TopoDS_Compound>(occt::ShapeVectorCast(out));
  BRepTools::Write(compoundOut, fileName.toStdString().c_str());
  
  node->sendBlocked(msg::buildStatusMessage("OCCT Exported", 2.0));
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
}
