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

#include <boost/variant.hpp>

#include <tools/idtools.h>
#include <project/project.h>
#include <message/node.h>
#include <message/sift.h>
#include <selection/eventhandler.h>
#include <command/isolate.h>

using namespace cmd;

Isolate::Isolate() : Base(), id(gu::createNilId()), mask(msg::ThreeD | msg::Overlay)
{
  sift = std::make_unique<msg::Sift>();
  sift->name = "cmd::Isolate";
  node->setHandler(std::bind(&msg::Sift::receive, sift.get(), std::placeholders::_1));
  
  setupDispatcher();
  shouldUpdate = false;
}

Isolate::~Isolate() {}


class VwrMessageVisitor : public boost::static_visitor<vwr::Message>
{
public:
  vwr::Message operator()(const prj::Message&) const {return vwr::Message();}
  vwr::Message operator()(const slc::Message&) const {return vwr::Message();}
  vwr::Message operator()(const app::Message&) const {return vwr::Message();}
  vwr::Message operator()(const vwr::Message &mIn) const {return mIn;}
  vwr::Message operator()(const ftr::Message&) const {return vwr::Message();}
  vwr::Message operator()(const lod::Message&) const {return vwr::Message();}
};
void Isolate::setFromMessage(const msg::Message &mIn)
{
  mask = mIn.mask;
  
  vwr::Message vm = boost::apply_visitor(VwrMessageVisitor(), mIn.payload);
  id = vm.featureId;
}

std::string Isolate::getStatusMessage()
{
  return QObject::tr("Select features for isolate").toStdString();
}

void Isolate::activate()
{
  isActive = true;
  go();
}

void Isolate::deactivate()
{
  isActive = false;
}

void Isolate::go()
{
  if (id.is_nil()) //try to get feature id from selection.
  {
    const slc::Containers &containers = eventHandler->getSelections();
    for (const auto &container : containers)
    {
      if (container.selectionType != slc::Type::Object)
        continue;
      id = container.featureId;
      break; //only works on the first selected object.
    }
  }
  if (id.is_nil())
  {
    node->send(msg::buildSelectionMask(slc::ObjectsEnabled | slc::ObjectsSelectable));
    return;
  }
  
  for (const auto &lid : project->getAllFeatureIds())
  {
    if (id == lid)
    {
      if ((mask & msg::ThreeD).any() && project->isFeatureLeaf(lid))
        node->sendBlocked(msg::buildShowThreeD(lid));
      if ((mask & msg::Overlay).any())
        node->sendBlocked(msg::buildShowOverlay(lid));
    }
    else
    {
      if ((mask & msg::ThreeD).any() && project->isFeatureLeaf(lid))
        node->sendBlocked(msg::buildHideThreeD(lid));
      if ((mask & msg::Overlay).any())
        node->sendBlocked(msg::buildHideOverlay(lid));
    }
  }
  node->sendBlocked(msg::Message(msg::Request | msg::View | msg::Fit));
  sendDone();
}

void Isolate::setupDispatcher()
{
  sift->insert
  (
    msg::Response | msg::Post | msg::Selection | msg::Add
    , std::bind(&Isolate::selectionAdditionDispatched, this, std::placeholders::_1)
  );
}

void Isolate::selectionAdditionDispatched(const msg::Message&)
{
  if (!isActive)
    return;
  
  go();
}
