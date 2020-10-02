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
// #include "application/appmainwindow.h"
// #include "application/appapplication.h"
// #include "viewer/vwrwidget.h"
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

/* JFC. Maybe we just enforce for strict grouping from the interface.
 * This is a lot of shit trying to associate edges and faces
 */
void Fill::setSelections(const Datas &datas)
{
  assert(isActive);
  
  project->clearAllInputs(feature->getId());
  feature->clearEntries();
  
  if (datas.empty())
    return;
  
  //group together all messages belonging to the same feature.
  //this is a start to organize edges and faces.
  std::vector<Datas> grouped;
  auto findGroupIt = [&](const Data &mIn) -> decltype(grouped)::iterator
  {
    for (auto it = grouped.begin(); it != grouped.end(); ++it)
    {
      //there is always one. because we don't add an empty vector
      if (std::get<0>(it->front()).featureId == std::get<0>(mIn).featureId)
        return it;
    }
    return grouped.end();
  };
  
  for (const auto &d : datas)
  {
    if (!isValidSelection(std::get<0>(d)))
    {
      node->sendBlocked(msg::buildStatusMessage(QObject::tr("No or invalid seershape").toStdString(), 2.0));
      continue;
    }
    auto it = findGroupIt(d);
    if (it == grouped.end())
      grouped.push_back({d});
    else
      it->push_back(d);
  }
  
  //ok now we should have faces and edges belonging to the same feature in grouped.
  //We have to make sure each groups edge belongs to the groups face
  //also what if there are more than 2 in a group because multiple boundaries
  //are on same feature. A single face might have 2 boundary edges, think of
  //a trimmed cylinder. So we can't remove faces as they are discovered from an edge
  int edgeCount = 0;
  int faceCount = 0;
  for (const auto &group : grouped)
  {
    ftr::Base *f = project->findFeature(std::get<0>(group.front()).featureId);
    const ann::SeerShape &ss = f->getAnnex<ann::SeerShape>();
    
    Datas justEdges;
    Datas justFaces;
    std::vector<Datas> edgeFaces;
    for (const auto &t : group)
    {
      const auto &cm = std::get<0>(t);
      
      auto lookForFace = [&]() -> std::optional<Data>
      {
        for (const auto &lff : group)
        {
          if (std::get<0>(lff).type != slc::Type::Face)
            continue;
          if (ss.useIsEdgeOfFace(cm.shapeId, std::get<0>(lff).shapeId))
            return lff;
        }
        return std::nullopt;
      };
      
      auto lookForEdge = [&]() -> std::optional<Data>
      {
        for (const auto &lff : group)
        {
          if (std::get<0>(lff).type != slc::Type::Edge)
            continue;
          if (ss.useIsEdgeOfFace(std::get<0>(lff).shapeId, cm.shapeId))
            return lff;
        }
        return std::nullopt;
      };
      
      if (cm.type == slc::Type::Edge)
      {
        auto oFace = lookForFace();
        if (oFace)
        {
          Datas temp;
          temp.push_back(t); //edge
          temp.push_back(*oFace); //face
          edgeFaces.push_back(temp);
        }
        else
          justEdges.push_back(t);
        continue;
      }
      if (cm.type == slc::Type::Face)
      {
        auto oEdge = lookForEdge();
        if (oEdge)
          continue; //let the search of edge to face add this to 'edgeFaces'
        justFaces.push_back(t);

      }
    }
    
    //now we should have the datas split into 3 groups.
    for (const auto &je : justEdges)
    {
      ftr::Fill::Entry entry;
      entry.edgePick = tls::convertToPick(std::get<0>(je), ss, project->getShapeHistory());
      entry.continuity = std::get<1>(je); //single edges should always be C0.
      entry.createLabel();
      entry.boundary = std::get<2>(je);
      entry.edgePick->tag = ftr::InputType::createIndexedTag(ftr::Fill::edgeTag, edgeCount++);
      project->connect(std::get<0>(je).featureId, feature->getId(), {entry.edgePick->tag});
      feature->addEntry(entry);
    }
    for (const auto &jf : justFaces)
    {
      ftr::Fill::Entry entry;
      entry.facePick = tls::convertToPick(std::get<0>(jf), ss, project->getShapeHistory());
      entry.continuity = std::get<1>(jf);
      entry.createLabel();
      entry.boundary = true; //single face with no edge is always boundary.
      entry.facePick->tag = ftr::InputType::createIndexedTag(ftr::Fill::faceTag, faceCount++);
      project->connect(std::get<0>(jf).featureId, feature->getId(), {entry.facePick->tag});
      feature->addEntry(entry);
    }
    for (const auto &ef : edgeFaces)
    {
      assert(ef.size() == 2);
      if (ef.size() != 2)
        continue;
      ftr::Fill::Entry entry;
      entry.edgePick = tls::convertToPick(std::get<0>(ef.front()), ss, project->getShapeHistory());
      entry.facePick = tls::convertToPick(std::get<0>(ef.back()), ss, project->getShapeHistory());
      entry.continuity = std::get<1>(ef.back()); //use continuity parameter from face
      entry.createLabel();
      entry.boundary = std::get<2>(ef.front()), //use boundary setting from edge.
      entry.edgePick->tag = ftr::InputType::createIndexedTag(ftr::Fill::edgeTag, edgeCount++);
      entry.facePick->tag = ftr::InputType::createIndexedTag(ftr::Fill::faceTag, faceCount++);
      project->connect(std::get<0>(ef.front()).featureId, feature->getId(), {entry.edgePick->tag});
      project->connect(std::get<0>(ef.back()).featureId, feature->getId(), {entry.facePick->tag});
      feature->addEntry(entry);
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
  const slc::Containers &cs = eventHandler->getSelections();
  
  std::vector<slc::Message> tm; //target messages
  Datas datas;
  for (const auto &c : cs)
  {
    auto m = slc::EventHandler::containerToMessage(c);
    auto p = ftr::Fill::buildContinuityParameter();
    if (m.type == slc::Type::Edge)
      p->setValue(0);
    else if (m.type == slc::Type::Face)
      p->setValue(1);
    datas.push_back({m, p, true});
  }
  
  if (!datas.empty())
  {
    setSelections(datas);
    node->sendBlocked(msg::buildStatusMessage("Fill Added", 2.0));
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return;
  }
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  node->sendBlocked(msg::buildShowThreeD(feature->getId()));
  node->sendBlocked(msg::buildShowOverlay(feature->getId()));
  viewBase = std::make_unique<cmv::Fill>(this);
}
