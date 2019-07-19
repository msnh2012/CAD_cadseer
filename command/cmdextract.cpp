/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#include <TopoDS_Face.hxx>
#include <TopoDS.hxx>

#include "project/prjproject.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "annex/annseershape.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrextract.h"
#include "tools/featuretools.h"
#include "command/cmdextract.h"

using namespace cmd;
using boost::uuids::uuid;

Extract::Extract() : Base() {}

Extract::~Extract() {}

std::string Extract::getStatusMessage()
{
  return QObject::tr("Select geometry to extract").toStdString();
}

void Extract::activate()
{
  isActive = true;
  
  /* first time the command is activated we will check for a valid preselection.
   * if there is one then we will just build a simple blend feature and call
   * the parameter dialog. Otherwise we will call the blend dialog.
   */
  if (firstRun)
  {
    firstRun = false;
    go();
  }
  
  sendDone();
}

void Extract::deactivate()
{
  isActive = false;
}

void Extract::go()
{
  bool created = false;
  const slc::Containers &containers = eventHandler->getSelections();
  for (const auto &container : containers)
  {
    ftr::Base *baseFeature = project->findFeature(container.featureId);
    assert(baseFeature);
    if (!baseFeature->hasAnnex(ann::Type::SeerShape))
      continue;
    const ann::SeerShape &targetSeerShape = baseFeature->getAnnex<ann::SeerShape>();
    ftr::Pick pick = tls::convertToPick(container, targetSeerShape, project->getShapeHistory());
    pick.tag = ftr::InputType::target;
    if (container.selectionType == slc::Type::Face)
    {
      //for now we just assume face equals tangent accrue.
      TopoDS_Face face = TopoDS::Face(targetSeerShape.getOCCTShape(container.shapeId));  
      pick.accrueType = ftr::AccrueType::Tangent;
      
      std::shared_ptr<ftr::Extract> extract(new ftr::Extract());
      ftr::Extract::AccruePick ap;
      ap.picks = ftr::Picks({pick});
      ap.parameter = ftr::Extract::buildAngleParameter(10.0);
      extract->sync(ftr::Extract::AccruePicks({ap}));
      
      project->addFeature(extract);
      project->connect(baseFeature->getId(), extract->getId(), {pick.tag});
      created = true;
      
      node->sendBlocked(msg::buildHideThreeD(baseFeature->getId()));
      node->sendBlocked(msg::buildHideOverlay(baseFeature->getId()));
      
      extract->setColor(baseFeature->getColor());
    }
    else
    {
      std::shared_ptr<ftr::Extract> extract(new ftr::Extract());
      extract->sync({pick});
      project->addFeature(extract);
      project->connect(baseFeature->getId(), extract->getId(), {pick.tag});
      created = true;
      
      node->sendBlocked(msg::buildHideThreeD(baseFeature->getId()));
      node->sendBlocked(msg::buildHideOverlay(baseFeature->getId()));
      
      extract->setColor(baseFeature->getColor());
    }
  }
  if (!created)
  {
    shouldUpdate = false;
    node->send(msg::buildStatusMessage("Extract: Invalid Preselection", 2.0));
  }
  else
    node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
}
