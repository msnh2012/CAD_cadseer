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

#include <boost/optional.hpp>

#include "tools/featuretools.h"
#include "tools/occtools.h"
#include "message/msgnode.h"
#include "project/prjproject.h"
#include "selection/slceventhandler.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "annex/annseershape.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrrevolve.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvrevolve.h"
#include "command/cmdrevolve.h"

using boost::uuids::uuid;
using namespace cmd;

Revolve::Revolve()
: Base()
, leafManager()
{
  feature = new ftr::Revolve::Feature();
  project->addFeature(std::unique_ptr<ftr::Revolve::Feature>(feature));
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  isEdit = false;
  isFirstRun = true;
}

Revolve::Revolve(ftr::Base *fIn)
: Base()
, leafManager(fIn)
{
  feature = dynamic_cast<ftr::Revolve::Feature*>(fIn);
  assert(feature);
  viewBase = std::make_unique<cmv::Revolve>(this);
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  isEdit = true;
  isFirstRun = false;
}

Revolve::~Revolve() {}

std::string Revolve::getStatusMessage()
{
  return QObject::tr("Select feature or geometry for revolve feature").toStdString();
}

void Revolve::activate()
{
  isActive = true;
  leafManager.rewind();
  if (isFirstRun.get())
  {
    isFirstRun = false;
    go();
  }
  if (viewBase)
  {
    feature->setEditing();
    cmv::Message vm(viewBase.get(), viewBase->getPaneWidth());
    msg::Message out(msg::Mask(msg::Request | msg::Command | msg::View | msg::Show), vm);
    node->sendBlocked(out);
  }
  else
    sendDone();
}

void Revolve::deactivate()
{
  if (viewBase)
  {
    feature->setNotEditing();
    msg::Message out(msg::Mask(msg::Request | msg::Command | msg::View | msg::Hide));
    node->sendBlocked(out);
  }
  leafManager.fastForward(); //do after the view is hidden
  if (!isEdit.get())
  {
    node->sendBlocked(msg::buildShowThreeD(feature->getId()));
    node->sendBlocked(msg::buildShowOverlay(feature->getId()));
  }
  isActive = false;
}

void Revolve::localUpdate()
{
  assert(isActive);
  feature->updateModel(project->getPayload(feature->getId()));
  feature->updateVisual();
  feature->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
}

ftr::Picks Revolve::connect(const slc::Messages &msIn, std::string_view prefix)
{
  ftr::Picks out;
  for (const auto &mIn : msIn)
  {
    const ftr::Base *f = project->findFeature(mIn.featureId);
    ftr::Pick pick = tls::convertToPick(mIn, *f, project->getShapeHistory());
    pick.tag = indexTag(prefix, out.size());
    out.push_back(pick);
    project->connectInsert(f->getId(), feature->getId(), {pick.tag});
  };
  return out;
}

void Revolve::setToAxisPicks(const slc::Messages &profileMessages, const slc::Messages &axisMessages)
{
  assert(isActive);
  project->clearAllInputs(feature->getId());
  feature->getParameter(ftr::Revolve::PrmTags::axisType)->setValue(0);
  feature->getParameter(ftr::Revolve::PrmTags::profilePicks)->setValue(connect(profileMessages, ftr::Revolve::PrmTags::profilePicks));
  feature->getParameter(ftr::Revolve::PrmTags::axisPicks)->setValue(connect(axisMessages, ftr::Revolve::PrmTags::axisPicks));
}


void Revolve::setToAxisParameter(const slc::Messages &profileMessages)
{
  assert(isActive);
  project->clearAllInputs(feature->getId());
  feature->getParameter(ftr::Revolve::PrmTags::axisType)->setValue(1);
  feature->getParameter(ftr::Revolve::PrmTags::profilePicks)->setValue(connect(profileMessages, ftr::Revolve::PrmTags::profilePicks));
}

void Revolve::go()
{
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
      if (containers.back().featureType == ftr::Type::DatumAxis)
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
        setToAxisParameter(profiles);
        node->sendBlocked(msg::buildStatusMessage("Revolve added with parameter axis", 2.0));
        node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
        return;
      }
      if (directions.size() == 1 || directions.size() == 2)
      {
        setToAxisPicks(profiles, directions);
        node->sendBlocked(msg::buildStatusMessage("Revolve added with picked axis", 2.0));
        node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
        return;
      }
    }
  }
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  viewBase = std::make_unique<cmv::Revolve>(this);
}
