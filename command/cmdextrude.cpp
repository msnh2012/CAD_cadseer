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

#include "tools/featuretools.h"
#include "tools/occtools.h"
#include "message/msgnode.h"
#include "project/prjproject.h"
#include "selection/slceventhandler.h"
#include "annex/annseershape.h"
#include "feature/ftrinputtype.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvextrude.h"
#include "feature/ftrextrude.h"
#include "command/cmdextrude.h"

using boost::uuids::uuid;

using namespace cmd;

Extrude::Extrude()
: Base()
, leafManager()
{
  auto extrude = std::make_shared<ftr::Extrude>();
  project->addFeature(extrude);
  feature = extrude.get();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
}

Extrude::Extrude(ftr::Base *fIn)
: Base()
, leafManager(fIn)
{
  feature = dynamic_cast<ftr::Extrude*>(fIn);
  assert(feature);
  firstRun = false; //bypass go() in activate.
  viewBase = std::make_unique<cmv::Extrude>(this);
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

Extrude::~Extrude() {}

std::string Extrude::getStatusMessage()
{
  return QObject::tr("Select feature or geometry for extrude feature").toStdString();
}

void Extrude::activate()
{
  isActive = true;
  leafManager.rewind();
  if (firstRun)
  {
    firstRun = false;
    go();
  }
  if (viewBase)
  {
    cmv::Message vm(viewBase.get(), viewBase->getPaneWidth());
    msg::Message out(msg::Mask(msg::Request | msg::Command | msg::View | msg::Show), vm);
    node->sendBlocked(out);
  }
  else
    sendDone();
}

void Extrude::deactivate()
{
  isActive = false;
  if (viewBase)
  {
    msg::Message out(msg::Mask(msg::Request | msg::Command | msg::View | msg::Hide));
    node->sendBlocked(out);
  }
  leafManager.fastForward();
}

void Extrude::localUpdate()
{
  feature->updateModel(project->getPayload(feature->getId()));
  feature->updateVisual();
  feature->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
}

void Extrude::go()
{
  /* We can have more than 1 selection for profile geometry and 
   * we can use geometry for extrude direction. So there is no good way
   * to glean the purpose of the pre-selections. So for pre-selection,
   * we are going to force the first selection as the profile geometry
   * and any further selections as the direction. Note with a commandview
   * we can have more than one selection for the profile geometry.
   */
  
  const slc::Containers &containers = eventHandler->getSelections();
  if (!containers.empty())
  {
    slc::Messages profiles;
    const ftr::Base *pfIn = project->findFeature(containers.front().featureId);
    if (pfIn->hasAnnex(ann::Type::SeerShape))
      profiles.push_back(slc::EventHandler::containerToMessage(containers.front()));
    
    slc::Messages directions;
    if (containers.size() == 2)
    {
      if (containers.back().featureType == ftr::Type::DatumAxis || containers.back().featureType == ftr::Type::DatumPlane)
        directions.push_back(slc::EventHandler::containerToMessage(containers.back()));
      else if (!containers.back().isPointType())
      {
        const ftr::Base *dfIn = project->findFeature(containers.back().featureId);
        if (dfIn->hasAnnex(ann::Type::SeerShape))
        {
          TopoDS_Shape ds;
          const ann::SeerShape &ss = dfIn->getAnnex<ann::SeerShape>();
          if (containers.back().shapeId.is_nil())
          {
            auto ncc = ss.useGetNonCompoundChildren();
            if (!ncc.empty())
              ds = ncc.front();
          }
          else
            ds = ss.getOCCTShape(containers.back().shapeId);
          auto result = occt::gleanAxis(ds);
          if (result.second)
            directions.push_back(slc::EventHandler::containerToMessage(containers.back()));
        }
      }
    }
    else if (containers.size() == 3 && containers.at(1).isPointType() && containers.at(2).isPointType())
    {
      directions.push_back(slc::EventHandler::containerToMessage(containers.at(1)));
      directions.push_back(slc::EventHandler::containerToMessage(containers.at(2)));
    }
    
    if (!profiles.empty())
    {
      if (directions.empty())
      {
        setToAxisInfer(profiles);
        node->sendBlocked(msg::buildStatusMessage("Extrude added with inferred axis", 2.0));
        node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
        return;
      }
      if (directions.size() == 1 || directions.size() == 2)
      {
        setToAxisPicks(profiles, directions);
        node->sendBlocked(msg::buildStatusMessage("Extrude added with picked axis", 2.0));
        node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
        return;
      }
    }
  }
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  viewBase = std::make_unique<cmv::Extrude>(this);
}

ftr::Picks Extrude::connect(const std::vector<slc::Message> &msIn, const std::string &prefix)
{
  ftr::Picks out;
  for (const auto &mIn : msIn)
  {
    const ftr::Base *f = project->findFeature(mIn.featureId);
    ftr::Pick pick = tls::convertToPick(mIn, *f, project->getShapeHistory());
    pick.tag = ftr::InputType::createIndexedTag(prefix, out.size());
    out.push_back(pick);
    project->connectInsert(f->getId(), feature->getId(), {pick.tag});
  };
  return out;
}

void Extrude::setToAxisInfer(const std::vector<slc::Message> &profileMessages)
{
  project->clearAllInputs(feature->getId());
  feature->setPicks(connect(profileMessages, ftr::InputType::target));
  feature->setDirectionType(ftr::Extrude::DirectionType::Infer);
}

void Extrude::setToAxisPicks(const std::vector<slc::Message> &profileMessages, const std::vector<slc::Message> &axisMessages)
{
  project->clearAllInputs(feature->getId());
  feature->setPicks(connect(profileMessages, ftr::InputType::target));
  feature->setAxisPicks(connect(axisMessages, ftr::Extrude::axisName));
  feature->setDirectionType(ftr::Extrude::DirectionType::Picks);
}

void Extrude::setToAxisParameter(const std::vector<slc::Message> &profileMessages)
{
  project->clearAllInputs(feature->getId());
  feature->setPicks(connect(profileMessages, ftr::InputType::target));
  feature->setDirectionType(ftr::Extrude::DirectionType::Parameter);
}
