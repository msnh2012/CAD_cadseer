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

#include <functional>

#include <osg/Geometry> //need this for containers.

#include "application/appapplication.h"
#include "application/appmainwindow.h"
#include "project/prjproject.h"
#include "feature/ftrbase.h"
#include "feature/ftrbox.h"
#include "feature/ftrcylinder.h"
#include "feature/ftroblong.h"
#include "feature/ftrtorus.h"
#include "feature/ftrsphere.h"
#include "feature/ftrcone.h"
#include "feature/ftrthread.h"
#include "feature/ftrprism.h"
#include "command/cmdfeaturetosystem.h"
#include "command/cmdsystemtofeature.h"
#include "command/cmdfeaturetodragger.h"
#include "command/cmddraggertofeature.h"
#include "command/cmdcheckgeometry.h"
#include "command/cmdeditcolor.h"
#include "command/cmdfeaturerename.h"
#include "command/cmdblend.h"
#include "command/cmdextract.h"
#include "command/cmdfeaturereposition.h"
#include "command/cmddissolve.h"
#include "command/cmdsystemtoselection.h"
#include "command/cmdsquash.h"
#include "command/cmdstrip.h"
#include "command/cmdnest.h"
#include "command/cmddieset.h"
#include "command/cmdquote.h"
#include "command/cmdisolate.h"
#include "command/cmdmeasurelinear.h"
#include "command/cmdrefine.h"
#include "command/cmdinstancelinear.h"
#include "command/cmdinstancemirror.h"
#include "command/cmdinstancepolar.h"
#include "command/cmdboolean.h"
#include "command/cmdoffset.h"
#include "command/cmdthicken.h"
#include "command/cmdsew.h"
#include "command/cmdtrim.h"
#include "command/cmdrevision.h"
#include "command/cmdremovefaces.h"
#include "command/cmddatumaxis.h"
#include "command/cmddatumplane.h"
#include "command/cmdsketch.h"
#include "command/cmdextrude.h"
#include "command/cmdrevolve.h"
#include "command/cmdsurfacemesh.h"
#include "command/cmdline.h"
#include "command/cmdtransitioncurve.h"
#include "command/cmdruled.h"
#include "command/cmdimageplane.h"
#include "command/cmdsweep.h"
#include "command/cmddraft.h"
#include "command/cmdchamfer.h"
#include "command/cmdhollow.h"
#include "command/cmdimport.h"
#include "command/cmdexport.h"
#include "command/cmdpreferences.h"
#include "command/cmdremove.h"
#include "command/cmdinfo.h"
#include "command/cmddirty.h"
#include "command/cmdfeaturedump.h"
#include "command/cmdshapetrackdump.h"
#include "command/cmdshapegraphdump.h"
#include "command/cmdtest.h"
#include "command/cmddatumsystem.h"
#include "command/cmdsurfaceremesh.h"
#include "command/cmdsurfacemeshfill.h"
#include "command/cmdprimitive.h"
#include "command/cmdmappcurve.h"
#include "command/cmduntrim.h"
#include "command/cmdface.h"
#include "command/cmdfill.h"
#include "command/cmdundercut.h"
#include "message/msgnode.h"
#include "message/msgsift.h"
#include "selection/slcmessage.h"
#include "selection/slccontainer.h"
#include "selection/slcdefinitions.h"
#include "selection/slceventhandler.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "viewer/vwrmessage.h"
#include "viewer/vwrwidget.h"
#include "command/cmdmanager.h"

using namespace cmd;

Manager& cmd::manager()
{
  static Manager localManager;
  return localManager;
}

Manager::Manager()
{
  node = std::make_unique<msg::Node>();
  node->connect(msg::hub());
  sift = std::make_unique<msg::Sift>();
  sift->name = "cmd::Manager";
  node->setHandler(std::bind(&msg::Sift::receive, sift.get(), std::placeholders::_1));
  
  setupDispatcher();
  
  setupEditFunctionMap();
  
  selectionMask = slc::AllEnabled;
  app::instance()->queuedMessage(msg::Mask(msg::Response | msg::Command | msg::Inactive));
}

void Manager::setupDispatcher()
{
  sift->insert
  (
    {
      std::make_pair
      (
        msg::Request | msg::Command | msg::Clear
        , std::bind(&Manager::clearCommandDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Command | msg::Done
        , std::bind(&Manager::doneCommandDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Selection | msg::SetMask
        , std::bind(&Manager::selectionMaskDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::FeatureToSystem
        , std::bind(&Manager::featureToSystemDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::SystemToFeature
        , std::bind(&Manager::systemToFeatureDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::FeatureToDragger
        , std::bind(&Manager::featureToDraggerDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::DraggerToFeature
        , std::bind(&Manager::draggerToFeatureDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::FeatureReposition
        , std::bind(&Manager::featureRepositionDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Feature | msg::Dissolve
        , std::bind(&Manager::featureDissolveDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::SystemToSelection
        , std::bind(&Manager::systemToSelectionDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::CheckGeometry
        , std::bind(&Manager::checkGeometryDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Edit | msg::Feature | msg::Color
        , std::bind(&Manager::editColorDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Edit | msg::Feature | msg::Name
        , std::bind(&Manager::featureRenameDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Blend
        , std::bind(&Manager::constructBlendDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Extract
        , std::bind(&Manager::constructExtractDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Squash
        , std::bind(&Manager::constructSquashDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Strip
        , std::bind(&Manager::constructStripDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Nest
        , std::bind(&Manager::constructNestDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::DieSet
        , std::bind(&Manager::constructDieSetDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Quote
        , std::bind(&Manager::constructQuoteDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Edit | msg::Feature
        , std::bind(&Manager::editFeatureDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::View | msg::ThreeD | msg::Isolate
        , std::bind(&Manager::viewIsolateDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::View | msg::Overlay | msg::Isolate
        , std::bind(&Manager::viewIsolateDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::View | msg::ThreeD | msg::Overlay | msg::Isolate
        , std::bind(&Manager::viewIsolateDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::LinearMeasure
        , std::bind(&Manager::measureLinearDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Refine
        , std::bind(&Manager::constructRefineDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::InstanceLinear
        , std::bind(&Manager::constructInstanceLinearDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::InstanceMirror
        , std::bind(&Manager::constructInstanceMirrorDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::InstancePolar
        , std::bind(&Manager::constructInstancePolarDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Intersect
        , std::bind(&Manager::constructBooleanDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Subtract
        , std::bind(&Manager::constructBooleanDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Union
        , std::bind(&Manager::constructBooleanDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Offset
        , std::bind(&Manager::constructOffsetDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Thicken
        , std::bind(&Manager::constructThickenDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Sew
        , std::bind(&Manager::constructSewDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Trim
        , std::bind(&Manager::constructTrimDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::RemoveFaces
        , std::bind(&Manager::constructRemoveFacesDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Thread
        , std::bind(&Manager::constructThreadDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::DatumAxis
        , std::bind(&Manager::constructDatumAxisDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::DatumPlane
        , std::bind(&Manager::constructDatumPlaneDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Sketch
        , std::bind(&Manager::constructSketchDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Extrude
        , std::bind(&Manager::constructExtrudeDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Revolve
        , std::bind(&Manager::constructRevolveDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::SurfaceMesh
        , std::bind(&Manager::constructSurfaceMeshDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Line
        , std::bind(&Manager::constructLineDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::TransitionCurve
        , std::bind(&Manager::constructTransitionCurveDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Ruled
        , std::bind(&Manager::constructRuledDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::ImagePlane
        , std::bind(&Manager::constructImagePlaneDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Sweep
        , std::bind(&Manager::constructSweepDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Draft
      , std::bind(&Manager::constructDraftDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Chamfer
      , std::bind(&Manager::constructChamferDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Box
      , std::bind(&Manager::constructBoxDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Oblong
      , std::bind(&Manager::constructOblongDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Torus
      , std::bind(&Manager::constructTorusDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Cylinder
      , std::bind(&Manager::constructCylinderDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Sphere
      , std::bind(&Manager::constructSphereDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Cone
      , std::bind(&Manager::constructConeDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Hollow
      , std::bind(&Manager::constructHollowDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::DatumSystem
      , std::bind(&Manager::constructDatumSystemDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::SurfaceReMesh
      , std::bind(&Manager::constructSurfaceReMeshDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::SurfaceMeshFill
      , std::bind(&Manager::constructSurfaceMeshFillDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::MapPCurve
      , std::bind(&Manager::constructMapPCurveDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Untrim
      , std::bind(&Manager::constructUntrimDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Face
      , std::bind(&Manager::constructFaceDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Fill
      , std::bind(&Manager::constructFillDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::Prism
        , std::bind(&Manager::constructPrismDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Construct | msg::UnderCut
        , std::bind(&Manager::constructUnderCutDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Project | msg::Revision | msg::Dialog
        , std::bind(&Manager::revisionDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Import
        , std::bind(&Manager::importDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Export
        , std::bind(&Manager::exportDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Preferences
        , std::bind(&Manager::preferencesDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Remove
        , std::bind(&Manager::removeDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Info
        , std::bind(&Manager::infoDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Feature | msg::Model | msg::Dirty
        , std::bind(&Manager::dirtyDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Feature | msg::Dump
        , std::bind(&Manager::featureDumpDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Shape | msg::Track | msg::Dump
        , std::bind(&Manager::shapeTrackDumpDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Shape | msg::Graph | msg::Dump
        , std::bind(&Manager::shapeGraphDumpDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Test
        , std::bind(&Manager::testDispatched, this, std::placeholders::_1)
      )
    }
  );
}

void Manager::doneCommandDispatched(const msg::Message&)
{
  doneSlot();
}

void Manager::clearCommandDispatched(const msg::Message &)
{
  while(!stack.empty())
    doneSlot();
}

void Manager::addCommand(BasePtr pointerIn)
{
  //preselection will only work if the command stack is empty.
  if (!stack.empty())
  {
    stack.top()->deactivate();
    clearSelection();
  }
  stack.push(pointerIn);
  activateTop();
}

void Manager::doneSlot()
{
  bool shouldCommandUpdate = false;
  //only active command should trigger it is done.
  if (!stack.empty())
  {
    shouldCommandUpdate = stack.top()->getShouldUpdate();
    stack.top()->deactivate();
    stack.pop();
  }
  clearSelection();
  node->send(msg::buildStatusMessage(""));
  
  if (prf::manager().rootPtr->dragger().triggerUpdateOnFinish() && shouldCommandUpdate)
  {
    node->sendBlocked(msg::Mask(msg::Request | msg::Project | msg::Update));
  }
  
  if (!stack.empty())
    activateTop();
  else
  {
    sendCommandMessage("Active command count: 0");
    node->send(msg::buildSelectionMask(selectionMask));
    node->sendBlocked(msg::Mask(msg::Response | msg::Command | msg::Inactive));
  }
}

void Manager::activateTop()
{
  node->sendBlocked(msg::buildStatusMessage(stack.top()->getStatusMessage()));
  
  std::ostringstream stream;
  stream << 
    "Active command count: " << stack.size() << std::endl <<
    "Command: " << stack.top()->getCommandName();
  sendCommandMessage(stream.str());
  
  node->sendBlocked(msg::Mask(msg::Response | msg::Command | msg::Active));
  stack.top()->activate();
}

void Manager::sendCommandMessage(const std::string& messageIn)
{
  vwr::Message statusVMessage;
  statusVMessage.text = messageIn;
  msg::Message statusMessage(msg::Request | msg::Command | msg::Text, statusVMessage);
  node->sendBlocked(statusMessage);
}

void Manager::clearSelection()
{
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

void Manager::selectionMaskDispatched(const msg::Message &messageIn)
{
  if (!stack.empty()) //only when no commands 
    return;
  
  slc::Message sMsg = messageIn.getSLC();
  selectionMask = sMsg.selectionMask;
}

void Manager::featureToSystemDispatched(const msg::Message&)
{
  addCommand(std::make_shared<FeatureToSystem>());
}

void Manager::systemToFeatureDispatched(const msg::Message&)
{
  addCommand(std::make_shared<SystemToFeature>());
}

void Manager::draggerToFeatureDispatched(const msg::Message&)
{
  addCommand(std::make_shared<DraggerToFeature>());
}

void Manager::featureToDraggerDispatched(const msg::Message&)
{
  addCommand(std::make_shared<FeatureToDragger>());
}

void Manager::checkGeometryDispatched(const msg::Message&)
{
  addCommand(std::make_shared<CheckGeometry>());
}

void Manager::editColorDispatched(const msg::Message&)
{
  addCommand(std::make_shared<EditColor>());
}

void Manager::featureRenameDispatched(const msg::Message &mIn)
{
  auto featureRename = std::make_shared<FeatureRename>();
  featureRename->setFromMessage(mIn);
  addCommand(featureRename);
}

void Manager::constructBlendDispatched(const msg::Message&)
{
  addCommand(std::make_shared<Blend>());
}

void Manager::constructExtractDispatched(const msg::Message&)
{
  addCommand(std::make_shared<Extract>());
}

void Manager::constructSquashDispatched(const msg::Message&)
{
  addCommand(std::make_shared<Squash>());
}

void Manager::constructStripDispatched(const msg::Message&)
{
  addCommand(std::make_shared<Strip>());
}

void Manager::constructNestDispatched(const msg::Message&)
{
  addCommand(std::make_shared<Nest>());
}

void Manager::constructDieSetDispatched(const msg::Message&)
{
  addCommand(std::make_shared<DieSet>());
}

void Manager::constructQuoteDispatched(const msg::Message&)
{
  addCommand(std::make_shared<Quote>());
}

void Manager::viewIsolateDispatched(const msg::Message &mIn)
{
  auto i = std::make_shared<Isolate>();
  i->setFromMessage(mIn);
  addCommand(i);
}

void Manager::featureRepositionDispatched(const msg::Message&)
{
  addCommand(std::make_shared<FeatureReposition>());
}

void Manager::featureDissolveDispatched(const msg::Message&)
{
  addCommand(std::make_shared<Dissolve>());
}

void Manager::systemToSelectionDispatched(const msg::Message&)
{
  addCommand(std::make_shared<SystemToSelection>());
}

void Manager::measureLinearDispatched(const msg::Message&)
{
  addCommand(std::make_shared<MeasureLinear>());
}

void Manager::constructRefineDispatched(const msg::Message&)
{
  addCommand(std::make_shared<Refine>());
}

void Manager::constructInstanceLinearDispatched(const msg::Message&)
{
  addCommand(std::make_shared<InstanceLinear>());
}

void Manager::constructInstanceMirrorDispatched(const msg::Message&)
{
  addCommand(std::make_shared<InstanceMirror>());
}

void Manager::constructInstancePolarDispatched(const msg::Message&)
{
  addCommand(std::make_shared<InstancePolar>());
}

void Manager::constructBooleanDispatched(const msg::Message &mIn)
{
  if ((mIn.mask & msg::Intersect) != 0)
    addCommand(std::make_shared<Boolean>(0));
  else if ((mIn.mask & msg::Subtract) != 0)
    addCommand(std::make_shared<Boolean>(1));
  else if ((mIn.mask & msg::Union) != 0)
    addCommand(std::make_shared<Boolean>(2));
}

void Manager::constructOffsetDispatched(const msg::Message&)
{
  addCommand(std::make_shared<Offset>());
}

void Manager::constructThickenDispatched(const msg::Message&)
{
  addCommand(std::make_shared<Thicken>());
}

void Manager::constructSewDispatched(const msg::Message&)
{
  addCommand(std::make_shared<Sew>());
}

void Manager::constructTrimDispatched(const msg::Message&)
{
  addCommand(std::make_shared<Trim>());
}

void Manager::revisionDispatched(const msg::Message&)
{
  addCommand(std::make_shared<Revision>());
}

void Manager::constructRemoveFacesDispatched(const msg::Message&)
{
  addCommand(std::make_shared<RemoveFaces>());
}

void Manager::constructThreadDispatched(const msg::Message&)
{
  auto *nf = new ftr::Thread::Feature();
  app::instance()->getProject()->addFeature(std::unique_ptr<ftr::Thread::Feature>(nf));
  auto c = std::make_shared<Primitive>(nf, false);
  addCommand(c);
}

void Manager::constructDatumAxisDispatched(const msg::Message&)
{
  addCommand(std::make_shared<DatumAxis>());
}

void Manager::constructDatumPlaneDispatched(const msg::Message&)
{
  addCommand(std::make_shared<DatumPlane>());
}

void Manager::constructSketchDispatched(const msg::Message&)
{
  addCommand(std::make_shared<Sketch>());
}

void Manager::constructExtrudeDispatched(const msg::Message&)
{
  addCommand(std::make_shared<Extrude>());
}

void Manager::constructRevolveDispatched(const msg::Message&)
{
  addCommand(std::make_shared<Revolve>());
}

void Manager::constructSurfaceMeshDispatched(const msg::Message&)
{
  addCommand(std::make_shared<SurfaceMesh>());
}

void Manager::constructLineDispatched(const msg::Message&)
{
  addCommand(std::make_shared<Line>());
}

void Manager::constructTransitionCurveDispatched(const msg::Message&)
{
  addCommand(std::make_shared<TransitionCurve>());
}

void Manager::constructRuledDispatched(const msg::Message&)
{
  addCommand(std::make_shared<Ruled>());
}

void Manager::constructImagePlaneDispatched(const msg::Message&)
{
  addCommand(std::make_shared<ImagePlane>());
}

void Manager::constructSweepDispatched(const msg::Message&)
{
  addCommand(std::make_shared<Sweep>());
}

void Manager::constructDraftDispatched(const msg::Message&)
{
  addCommand(std::make_shared<Draft>());
}

void Manager::constructChamferDispatched(const msg::Message&)
{
  addCommand(std::make_shared<Chamfer>());
}

void Manager::constructBoxDispatched(const msg::Message&)
{
  auto *nf = new ftr::Box::Feature();
  app::instance()->getProject()->addFeature(std::unique_ptr<ftr::Box::Feature>(nf));
  auto c = std::make_shared<Primitive>(nf, false);
  addCommand(c);
}

void Manager::constructOblongDispatched(const msg::Message&)
{
  auto *nf = new ftr::Oblong::Feature();
  app::instance()->getProject()->addFeature(std::unique_ptr<ftr::Oblong::Feature>(nf));
  auto c = std::make_shared<Primitive>(nf, false);
  addCommand(c);
}

void Manager::constructTorusDispatched(const msg::Message&)
{
  auto *nf = new ftr::Torus::Feature();
  app::instance()->getProject()->addFeature(std::unique_ptr<ftr::Torus::Feature>(nf));
  auto c = std::make_shared<Primitive>(nf, false);
  addCommand(c);
}

void Manager::constructCylinderDispatched(const msg::Message&)
{
  auto *nf = new ftr::Cylinder::Feature();
  app::instance()->getProject()->addFeature(std::unique_ptr<ftr::Cylinder::Feature>(nf));
  auto c = std::make_shared<Primitive>(nf, false);
  addCommand(c);
}

void Manager::constructSphereDispatched(const msg::Message&)
{
  auto *nf = new ftr::Sphere::Feature();
  app::instance()->getProject()->addFeature(std::unique_ptr<ftr::Sphere::Feature>(nf));
  auto c = std::make_shared<Primitive>(nf, false);
  addCommand(c);
}

void Manager::constructConeDispatched(const msg::Message&)
{
  auto *nf = new ftr::Cone::Feature();
  app::instance()->getProject()->addFeature(std::unique_ptr<ftr::Cone::Feature>(nf));
  auto c = std::make_shared<Primitive>(nf, false);
  addCommand(c);
}

void Manager::constructHollowDispatched(const msg::Message&)
{
  addCommand(std::make_shared<Hollow>());
}

void Manager::constructDatumSystemDispatched(const msg::Message&)
{
  addCommand(std::make_shared<DatumSystem>());
}

void Manager::constructSurfaceReMeshDispatched(const msg::Message&)
{
  addCommand(std::make_shared<SurfaceReMesh>());
}

void Manager::constructSurfaceMeshFillDispatched(const msg::Message&)
{
  addCommand(std::make_shared<SurfaceMeshFill>());
}

void Manager::constructMapPCurveDispatched(const msg::Message&)
{
  addCommand(std::make_shared<MapPCurve>());
}

void Manager::constructUntrimDispatched(const msg::Message&)
{
  addCommand(std::make_shared<Untrim>());
}

void Manager::constructFaceDispatched(const msg::Message&)
{
  addCommand(std::make_shared<Face>());
}

void Manager::constructFillDispatched(const msg::Message&)
{
  addCommand(std::make_shared<Fill>());
}

void Manager::constructPrismDispatched(const msg::Message&)
{
  auto *nf = new ftr::Prism::Feature();
  app::instance()->getProject()->addFeature(std::unique_ptr<ftr::Prism::Feature>(nf));
  auto c = std::make_shared<Primitive>(nf, false);
  addCommand(c);
}

void Manager::constructUnderCutDispatched(const msg::Message&)
{
  addCommand(std::make_shared<UnderCut>());
}

void Manager::importDispatched(const msg::Message&)
{
  addCommand(std::make_shared<Import>());
}

void Manager::exportDispatched(const msg::Message&)
{
  addCommand(std::make_shared<Export>());
}

void Manager::preferencesDispatched(const msg::Message&)
{
  addCommand(std::make_shared<Preferences>());
}

void Manager::removeDispatched(const msg::Message&)
{
  addCommand(std::make_shared<Remove>());
}

void Manager::infoDispatched(const msg::Message&)
{
  addCommand(std::make_shared<Info>());
}

void Manager::dirtyDispatched(const msg::Message&)
{
  addCommand(std::make_shared<Dirty>());
}

void Manager::featureDumpDispatched(const msg::Message&)
{
  addCommand(std::make_shared<FeatureDump>());
}

void Manager::shapeTrackDumpDispatched(const msg::Message&)
{
  addCommand(std::make_shared<ShapeTrackDump>());
}

void Manager::shapeGraphDumpDispatched(const msg::Message&)
{
  addCommand(std::make_shared<ShapeGraphDump>());
}

void Manager::testDispatched(const msg::Message&)
{
  addCommand(std::make_shared<Test>());
}

//editing commands and dispatching.
void Manager::editFeatureDispatched(const msg::Message&)
{
  app::Application *application = app::instance();
  const slc::Containers &selections = application->
    getMainWindow()->getViewer()->getSelectionEventHandler()->getSelections();
    
  //edit feature only works with 1 object pre-selection.
  if (selections.size() != 1)
  {
    node->sendBlocked(msg::buildStatusMessage("Select 1 object prior to edit feature command", 2.0));
    return;
  }
  
  if (selections.front().selectionType != slc::Type::Object)
  {
    node->sendBlocked(msg::buildStatusMessage("Wrong selection type for edit feature command", 2.0));
    return;
  }
  
  ftr::Base *feature = application->getProject()->findFeature(selections.front().featureId);
  
  auto it = editFunctionMap.find(feature->getType());
  if (it == editFunctionMap.end())
  {
    node->sendBlocked(msg::buildStatusMessage("Editing of feature type not implemented", 2.0));
    return;
  }
  
  addCommand(it->second(feature));
}

BasePtr editBlend(ftr::Base *feature)
{
  return std::make_shared<Blend>(feature);
}

BasePtr editStrip(ftr::Base *feature)
{
  return std::make_shared<Strip>(feature);
}

BasePtr editQuote(ftr::Base *feature)
{
  return std::make_shared<Quote>(feature);
}

BasePtr editBoolean(ftr::Base *feature)
{
  return std::make_shared<Boolean>(feature);
}

BasePtr editSketch(ftr::Base *feature)
{
  return std::make_shared<Sketch>(feature);
}

BasePtr editSweep(ftr::Base *feature)
{
  return std::make_shared<Sweep>(feature);
}

BasePtr editDraft(ftr::Base *feature)
{
  return std::make_shared<Draft>(feature);
}

BasePtr editExtract(ftr::Base *feature)
{
  return std::make_shared<Extract>(feature);
}

BasePtr editChamfer(ftr::Base *feature)
{
  return std::make_shared<Chamfer>(feature);
}

BasePtr editPrimitive(ftr::Base *feature)
{
  return std::make_shared<Primitive>(feature, true);
}

BasePtr editHollow(ftr::Base *feature)
{
  return std::make_shared<Hollow>(feature);
}

BasePtr editDatumSystem(ftr::Base *feature)
{
  return std::make_shared<DatumSystem>(feature);
}

BasePtr editSurfaceMesh(ftr::Base *feature)
{
  return std::make_shared<SurfaceMesh>(feature);
}

BasePtr editDatumAxis(ftr::Base *feature)
{
  return std::make_shared<DatumAxis>(feature);
}

BasePtr editDatumPlane(ftr::Base *feature)
{
  return std::make_shared<DatumPlane>(feature);
}

BasePtr editExtrude(ftr::Base *feature)
{
  return std::make_shared<Extrude>(feature);
}

BasePtr editRevolve(ftr::Base *feature)
{
  return std::make_shared<Revolve>(feature);
}

BasePtr editTrim(ftr::Base *feature)
{
  return std::make_shared<Trim>(feature);
}

BasePtr editOffset(ftr::Base *feature)
{
  return std::make_shared<Offset>(feature);
}

BasePtr editRemoveFaces(ftr::Base *feature)
{
  return std::make_shared<RemoveFaces>(feature);
}

BasePtr editRuled(ftr::Base *feature)
{
  return std::make_shared<Ruled>(feature);
}

BasePtr editSew(ftr::Base *feature)
{
  return std::make_shared<Sew>(feature);
}

BasePtr editThicken(ftr::Base *feature)
{
  return std::make_shared<Thicken>(feature);
}

BasePtr editTransitionCurve(ftr::Base *feature)
{
  return std::make_shared<TransitionCurve>(feature);
}

BasePtr editMapPCurve(ftr::Base *feature)
{
  return std::make_shared<MapPCurve>(feature);
}

BasePtr editUntrim(ftr::Base *feature)
{
  return std::make_shared<Untrim>(feature);
}

BasePtr editFace(ftr::Base *feature)
{
  return std::make_shared<Face>(feature);
}

BasePtr editFill(ftr::Base *feature)
{
  return std::make_shared<Fill>(feature);
}

BasePtr editUnderCut(ftr::Base *feature)
{
  return std::make_shared<UnderCut>(feature);
}

void Manager::setupEditFunctionMap()
{
  editFunctionMap.insert(std::make_pair(ftr::Type::Blend, std::bind(editBlend, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::Strip, std::bind(editStrip, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::Quote, std::bind(editQuote, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::Boolean, std::bind(editBoolean, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::Sketch, std::bind(editSketch, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::Sweep, std::bind(editSweep, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::Draft, std::bind(editDraft, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::Extract, std::bind(editExtract, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::Chamfer, std::bind(editChamfer, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::Box, std::bind(editPrimitive, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::Oblong, std::bind(editPrimitive, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::Torus, std::bind(editPrimitive, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::Cylinder, std::bind(editPrimitive, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::Sphere, std::bind(editPrimitive, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::Cone, std::bind(editPrimitive, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::Thread, std::bind(editPrimitive, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::Prism, std::bind(editPrimitive, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::Hollow, std::bind(editHollow, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::DatumSystem, std::bind(editDatumSystem, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::SurfaceMesh, std::bind(editSurfaceMesh, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::DatumAxis, std::bind(editDatumAxis, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::DatumPlane, std::bind(editDatumPlane, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::Extrude, std::bind(editExtrude, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::Revolve, std::bind(editRevolve, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::Trim, std::bind(editTrim, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::Offset, std::bind(editOffset, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::RemoveFaces, std::bind(editRemoveFaces, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::Ruled, std::bind(editRuled, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::Sew, std::bind(editSew, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::Thicken, std::bind(editThicken, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::TransitionCurve, std::bind(editTransitionCurve, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::MapPCurve, std::bind(editMapPCurve, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::Untrim, std::bind(editUntrim, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::Face, std::bind(editFace, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::Fill, std::bind(editFill, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::UnderCut, std::bind(editUnderCut, std::placeholders::_1)));
}
