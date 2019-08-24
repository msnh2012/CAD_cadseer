/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2015  Thomas S. Anderson blobfish.at.gmx.com
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

#include <sstream>

#include <boost/current_function.hpp>

#include <QAction>

#include <osg/Node>

#include "application/appapplication.h"
#include "modelviz/mdvnodemaskdefs.h"
#include "selection/slcdefinitions.h"
#include "message/msgnode.h"
#include "message/msgsift.h"
#include "selection/slcmessage.h"
#include "selection/slcmanager.h"

using namespace slc;

Manager::Manager(QObject *parent) :
  QObject(parent), selectionMask(slc::AllEnabled)
{
  node = std::make_unique<msg::Node>();
  node->connect(msg::hub());
  sift = std::make_unique<msg::Sift>();
  sift->name = "slc::Manager";
  node->setHandler(std::bind(&msg::Sift::receive, sift.get(), std::placeholders::_1));
  setupDispatcher();
  app::instance()->queuedMessage(msg::buildSelectionMask(slc::AllEnabled));
}

Manager::~Manager() {}

void Manager::triggeredObjects(bool objectStateIn)
{
  if (objectStateIn)
    selectionMask |= slc::ObjectsSelectable;
  else
    selectionMask &= ~slc::ObjectsSelectable;
  sendUpdatedMask();
}

void Manager::triggeredFeatures(bool featureStateIn)
{
  if (featureStateIn)
    selectionMask |= slc::FeaturesSelectable;
  else
    selectionMask &= ~slc::FeaturesSelectable;
  sendUpdatedMask();
}

void Manager::triggeredSolids(bool solidStateIn)
{
  if (solidStateIn)
    selectionMask |= slc::SolidsSelectable;
  else
    selectionMask &= ~slc::SolidsSelectable;
  sendUpdatedMask();
}

void Manager::triggeredShells(bool shellStateIn)
{
  if (shellStateIn)
    selectionMask |= slc::ShellsSelectable;
  else
    selectionMask &= ~slc::ShellsSelectable;
  sendUpdatedMask();
}

void Manager::triggeredFaces(bool faceStateIn)
{
  if (faceStateIn)
    selectionMask |= slc::FacesSelectable;
  else
    selectionMask &= ~slc::FacesSelectable;
  sendUpdatedMask();
}

void Manager::triggeredWires(bool wireStateIn)
{
  if (wireStateIn)
    selectionMask |= slc::WiresSelectable;
  else
    selectionMask &= ~slc::WiresSelectable;
  sendUpdatedMask();
}

void Manager::triggeredEdges(bool edgeStateIn)
{
  if (edgeStateIn)
    selectionMask |= slc::EdgesSelectable;
  else
    selectionMask &= ~slc::EdgesSelectable;
  sendUpdatedMask();
}

void Manager::triggeredVertices(bool vertexStateIn)
{
  if (vertexStateIn)
  {
    selectionMask |= slc::PointsSelectable;
    selectionMask |= slc::EndPointsEnabled;
    selectionMask |= slc::MidPointsEnabled;
    selectionMask |= slc::CenterPointsEnabled;
    selectionMask |= slc::QuadrantPointsEnabled;
    selectionMask |= slc::NearestPointsEnabled;
  }
  else
  {
    selectionMask &= ~slc::PointsSelectable;
    selectionMask &= ~slc::EndPointsEnabled;
    selectionMask &= ~slc::MidPointsEnabled;
    selectionMask &= ~slc::CenterPointsEnabled;
    selectionMask &= ~slc::QuadrantPointsEnabled;
    selectionMask &= ~slc::NearestPointsEnabled;
  }
  updateToolbar();
  sendUpdatedMask();
}

void Manager::triggeredEndPoints(bool endPointStateIn)
{
  if (endPointStateIn)
    selectionMask |= slc::EndPointsSelectable;
  else
    selectionMask &= ~slc::EndPointsSelectable;
  sendUpdatedMask();
}

void Manager::triggeredMidPoints(bool midPointStateIn)
{
  if (midPointStateIn)
    selectionMask |= slc::MidPointsSelectable;
  else
    selectionMask &= ~slc::MidPointsSelectable;
  sendUpdatedMask();
}

void Manager::triggeredCenterPoints(bool centerPointStateIn)
{
  if (centerPointStateIn)
    selectionMask |= slc::CenterPointsSelectable;
  else
    selectionMask &= ~slc::CenterPointsSelectable;
  sendUpdatedMask();
}

void Manager::triggeredQuadrantPoints(bool quadrantPointStateIn)
{
  if (quadrantPointStateIn)
    selectionMask |= slc::QuadrantPointsSelectable;
  else
    selectionMask &= ~slc::QuadrantPointsSelectable;
  sendUpdatedMask();
}

void Manager::triggeredNearestPoints(bool nearestPointStateIn)
{
  if (nearestPointStateIn)
    selectionMask |= slc::NearestPointsSelectable;
  else
    selectionMask &= ~slc::NearestPointsSelectable;
  sendUpdatedMask();
}

void Manager::triggeredScreenPoints(bool screenPointStateIn)
{
  if (screenPointStateIn)
    selectionMask |= slc::ScreenPointsSelectable;
  else
    selectionMask &= ~slc::ScreenPointsSelectable;
  sendUpdatedMask();
}

void Manager::setState(Mask stateIn)
{
  selectionMask = stateIn;
  updateToolbar();
  sendUpdatedMask();
}
void Manager::sendUpdatedMask()
{
  msg::Message mOut
  (
    msg::Response | msg::Selection | msg::SetMask
    , slc::Message(selectionMask)
  );
  node->send(mOut);
}

void Manager::updateToolbar()
{
  actionSelectObjects->setEnabled((slc::ObjectsEnabled & selectionMask).any());
  actionSelectObjects->setChecked((slc::ObjectsSelectable & selectionMask).any());
  actionSelectFeatures->setEnabled((slc::FeaturesEnabled & selectionMask).any());
  actionSelectFeatures->setChecked((slc::FeaturesSelectable & selectionMask).any());
  actionSelectSolids->setEnabled((slc::SolidsEnabled & selectionMask).any());
  actionSelectSolids->setChecked((slc::SolidsSelectable & selectionMask).any());
  actionSelectShells->setEnabled((slc::ShellsEnabled & selectionMask).any());
  actionSelectShells->setChecked((slc::ShellsSelectable & selectionMask).any());
  actionSelectFaces->setEnabled((slc::FacesEnabled & selectionMask).any());
  actionSelectFaces->setChecked((slc::FacesSelectable & selectionMask).any());
  actionSelectWires->setEnabled((slc::WiresEnabled & selectionMask).any());
  actionSelectWires->setChecked((slc::WiresSelectable & selectionMask).any());
  actionSelectEdges->setEnabled((slc::EdgesEnabled & selectionMask).any());
  actionSelectEdges->setChecked((slc::EdgesSelectable & selectionMask).any());
  actionSelectVertices->setEnabled((slc::PointsEnabled & selectionMask).any());
  actionSelectVertices->setChecked((slc::PointsSelectable & selectionMask).any());
  actionSelectEndPoints->setEnabled((slc::EndPointsEnabled & selectionMask).any());
  actionSelectEndPoints->setChecked((slc::EndPointsSelectable & selectionMask).any());
  actionSelectMidPoints->setEnabled((slc::MidPointsEnabled & selectionMask).any());
  actionSelectMidPoints->setChecked((slc::MidPointsSelectable & selectionMask).any());
  actionSelectCenterPoints->setEnabled((slc::CenterPointsEnabled & selectionMask).any());
  actionSelectCenterPoints->setChecked((slc::CenterPointsSelectable & selectionMask).any());
  actionSelectQuadrantPoints->setEnabled((slc::QuadrantPointsEnabled & selectionMask).any());
  actionSelectQuadrantPoints->setChecked((slc::QuadrantPointsSelectable & selectionMask).any());
  actionSelectNearestPoints->setEnabled((slc::NearestPointsEnabled & selectionMask).any());
  actionSelectNearestPoints->setChecked((slc::NearestPointsSelectable & selectionMask).any());
  actionSelectScreenPoints->setEnabled((slc::ScreenPointsEnabled & selectionMask).any());
  actionSelectScreenPoints->setChecked((slc::ScreenPointsSelectable & selectionMask).any());
}

void Manager::setupDispatcher()
{
  sift->insert
  (
    msg::Request | msg::Selection | msg::SetMask
    , std::bind(&Manager::requestSelectionMaskDispatched, this, std::placeholders::_1)
  );
}

void Manager::requestSelectionMaskDispatched(const msg::Message &messageIn)
{
  slc::Message message = messageIn.getSLC();
  setState(message.selectionMask);
}
