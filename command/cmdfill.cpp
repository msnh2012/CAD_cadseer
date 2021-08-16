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

#include "tools/featuretools.h"
#include "project/prjproject.h"
#include "annex/annseershape.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "parameter/prmparameter.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrfill.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvfill.h"
#include "command/cmdfill.h"

using boost::uuids::uuid;

using namespace cmd;

Fill::Fill()
: Base("cmd::Fill")
, leafManager()
{
  auto nf = std::make_shared<ftr::Fill::Feature>();
  project->addFeature(nf);
  feature = nf.get();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  isEdit = false;
  isFirstRun = true;
}

Fill::Fill(ftr::Base *fIn)
: Base("cmd::Fill")
, leafManager(fIn)
{
  feature = dynamic_cast<ftr::Fill::Feature*>(fIn);
  assert(feature);
  viewBase = std::make_unique<cmv::Fill>(this);
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  isEdit = true;
  isFirstRun = false;
}

Fill::~Fill() = default;

std::string Fill::getStatusMessage()
{
  return QObject::tr("Select geometry for fill feature").toStdString();
}

void Fill::activate()
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

void Fill::deactivate()
{
  if (viewBase)
  {
    feature->setNotEditing();
    msg::Message out(msg::Mask(msg::Request | msg::Command | msg::View | msg::Hide));
    node->sendBlocked(out);
  }
  leafManager.fastForward();
  if (!isEdit.get())
  {
    node->sendBlocked(msg::buildShowThreeD(feature->getId()));
    node->sendBlocked(msg::buildShowOverlay(feature->getId()));
  }
  isActive = false;
}

bool Fill::isValidSelection(const slc::Message &mIn)
{
  const ftr::Base *lf = project->findFeature(mIn.featureId);
  if (!lf->hasAnnex(ann::Type::SeerShape) || lf->getAnnex<ann::SeerShape>().isNull())
    return false;
  
  auto t = mIn.type;
  if
  (
    (t != slc::Type::Object)
    && (t != slc::Type::Face)
    && (t != slc::Type::Edge)
  )
    return false;
  return true;
}

void Fill::setSelections(const Data &data)
{
  assert(isActive);
  project->clearAllInputs(feature->getId());
  
  auto messageToPick = [&](const slc::Message &mIn) -> ftr::Pick
  {
    const ftr::Base *lf = project->findFeature(mIn.featureId);
    return tls::convertToPick(mIn, *lf, project->getShapeHistory());
  };
  
  //initial pick
  if(std::get<0>(data.front()).empty())
  {
    feature->getParameter(ftr::Fill::PrmTags::initialPick)->setValue(ftr::Picks());
  }
  else
  {
    const auto &m = std::get<0>(data.front()).front();
    ftr::Pick initialPick = messageToPick(m);
    initialPick.tag = indexTag(ftr::Fill::InputTags::initialPick, 0);
    feature->getParameter(ftr::Fill::PrmTags::initialPick)->setValue(initialPick);
    project->connect(m.featureId, feature->getId(), {initialPick.tag});
  }
  
  //do all internal constraints if any.
  ftr::Picks ips;
  for (const auto &m : std::get<1>(data.front()))
  {
    ips.emplace_back(messageToPick(m));
    ips.back().tag = indexTag(ftr::Fill::InputTags::internalPicks, ips.size() - 1);
    project->connect(m.featureId, feature->getId(), {ips.back().tag});
  }
  feature->getParameter(ftr::Fill::PrmTags::internalPicks)->setValue(ips);
  
  //do boundaries.
  //make sure command view in sync with feature. off by 1 because we use first entry in vector
  //for the initial face pick and internal boundary picks.
  assert(data.size() - 1 == feature->getBoundaries().size());
  auto dIt = data.begin() + 1;
  auto bIt = feature->getBoundaries().begin();
  for (; bIt != feature->getBoundaries().end(); ++dIt, ++bIt)
  {
    const auto &es = std::get<0>(*dIt); //edge selections
    if (!es.empty())
    {
      ftr::Pick ep = messageToPick(es.front());
      ep.tag = indexTag(ftr::Fill::InputTags::edgePick, std::distance(feature->getBoundaries().begin(), bIt));
      bIt->edgePick.setValue(ep);
      project->connect(es.front().featureId, feature->getId(), {ep.tag});
    }
    
    const auto &fs = std::get<1>(*dIt); //face selections
    if (!fs.empty())
    {
      ftr::Pick fp = messageToPick(fs.front());
      fp.tag = indexTag(ftr::Fill::InputTags::facePick, std::distance(feature->getBoundaries().begin(), bIt));
      bIt->facePick.setValue(fp);
      project->connect(es.front().featureId, feature->getId(), {fp.tag});
    }
  }
}

void Fill::localUpdate()
{
  assert(isActive);
  feature->updateModel(project->getPayload(feature->getId()));
  feature->updateVisual();
  feature->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
}

void Fill::go()
{
  /* any pre selection is assumed a boundary.
   * this means that if user wants to have a face for continuity constraint
   * or an edge or point for internal constraint.
   * they will need to do so via the command view.
   */
  const slc::Containers &cs = eventHandler->getSelections();
  
  Data data;
  data.emplace_back(); //no initial face or internal constraints.
  for (const auto &c : cs)
  {
    auto m = slc::EventHandler::containerToMessage(c);
    if (m.type == slc::Type::Edge)
    {
      data.emplace_back(std::make_tuple(slc::Messages(1, m), slc::Messages()));
      feature->addBoundary();
    }
    else if (m.type == slc::Type::Face)
    {
      data.emplace_back(std::make_tuple(slc::Messages(), slc::Messages(1, m)));
      feature->addBoundary();
    }
  }
  
  if (data.size() != 1)
  {
    setSelections(data);
    node->sendBlocked(msg::buildStatusMessage("Fill Added", 2.0));
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return;
  }
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  node->sendBlocked(msg::buildShowThreeD(feature->getId()));
  node->sendBlocked(msg::buildShowOverlay(feature->getId()));
  viewBase = std::make_unique<cmv::Fill>(this);
}
