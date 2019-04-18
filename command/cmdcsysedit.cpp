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

#include <cassert>

#include <boost/signals2/shared_connection_block.hpp>

#include "globalutilities.h"
#include "library/lbrcsysdragger.h"
#include "tools/occtools.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "message/msgsift.h"
#include "selection/slcmessage.h"
#include "parameter/prmparameter.h"
#include "feature/ftrbase.h"
#include "annex/annseershape.h"
#include "annex/anncsysdragger.h"
#include "command/cmdcsysedit.h"

using namespace cmd;

CSysEdit::CSysEdit() : Base()
{
  sift = std::make_unique<msg::Sift>();
  sift->name = "cmd::CSysEdit";
  node->setHandler(std::bind(&msg::Sift::receive, sift.get(), std::placeholders::_1));
  
  setupDispatcher();
}

CSysEdit::~CSysEdit()
{

}

std::string CSysEdit::getStatusMessage()
{
  if (type == Type::Origin)
    return "select point for new origin";
  if (type == Type::Vector)
  {
    std::string draggerName = translateDragger->getName();
    std::string axisLabel = "none";
    if (draggerName == "xTranslate")
      axisLabel = "X";
    else if (draggerName == "yTranslate")
      axisLabel = "Y";
    else if (draggerName == "zTranslate")
      axisLabel = "Z";
    
    std::ostringstream stream;
    stream << "Select 2 points or object for new " << 
      axisLabel << " vector direction";
      
    return stream.str();
  }
  return "unknown condition";
}

void CSysEdit::setupDispatcher()
{
  sift->insert
  (
    {
      std::make_pair
      (
        msg::Response | msg::Post | msg::Selection | msg::Add
        , std::bind(&CSysEdit::selectionAdditionDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Pre | msg::Selection | msg::Remove
        , std::bind(&CSysEdit::selectionSubtractionDispatched, this, std::placeholders::_1)
      )
    }
  );
}


void CSysEdit::selectionAdditionDispatched(const msg::Message &messageIn)
{
  if (!isActive)
    return;
  
  slc::Message sMessage = messageIn.getSLC();
  
  if (!slc::has(messages, sMessage))
    slc::add(messages, sMessage);
  
  analyzeSelections();
}

void CSysEdit::selectionSubtractionDispatched(const msg::Message &messageIn)
{
  if (!isActive)
    return;
  
  slc::Message sMessage = messageIn.getSLC();
  
  if (slc::has(messages, sMessage))
    slc::remove(messages, sMessage);
  
  analyzeSelections();
}

void CSysEdit::analyzeSelections()
{
  if (type == Type::Vector)
  {
    if (messages.size() == 1)
    {
      ftr::Base *feature = dynamic_cast<ftr::Base*>(project->findFeature(messages.at(0).featureId));
      assert(feature);
      if (feature->hasAnnex(ann::Type::SeerShape))
      {
        const ann::SeerShape &seerShape = feature->getAnnex<ann::SeerShape>();
        assert(!seerShape.isNull());
        if (seerShape.isNull())
        {
          std::cerr << "seershape is null in CSysEdit::analyzeSelections" << std::endl;
          //remove selection?
          return;
        }
        
        gp_Pnt point(gu::toOcc(messages.at(0).pointLocation).XYZ());
        if
        (
          (messages.at(0).type == slc::Type::Edge)
          || (messages.at(0).type == slc::Type::Face)
        )
        {
          gp_Vec vector;
          bool results;
          std::tie(vector, results) = occt::gleanVector(seerShape.getOCCTShape(messages.at(0).shapeId), point);
          if (!results)
          {
            node->sendBlocked(msg::buildStatusMessage("Couldn't get vector from shape"));
            //remove selection?
            return;
          }
          osg::Vec3d direction = gu::toOsg(vector);
          if (direction.isNaN())
          {
            node->sendBlocked(msg::buildStatusMessage("Direction is invalid"));
            //remove selection?
            return;
          }
          
          updateToVector(direction);
          sendDone();
          return;
        }
      }
    }
    else if ((messages.size() == 2))
    {
      if
      (
        (slc::isPointType(messages.at(0).type)) &&
        (slc::isPointType(messages.at(1).type))
      )
      {
        //osg vec3d are filled will values from occ geometry so should be accurate.
        
        //head to tail pick.
        osg::Vec3d toVector = messages.at(0).pointLocation - messages.at(1).pointLocation;
        if (toVector.isNaN())
          return;
        toVector.normalize();
        
        updateToVector(toVector);
        
        sendDone();
        return;
      }
    }
  }
  else if((type == Type::Origin) && (messages.size() == 1))
  {
    boost::uuids::uuid id = gu::getId(csysDragger);
    if (id.is_nil() || (!csysDragger->isLinked())) //current system
    {
      osg::Matrixd fm = csysDragger->getMatrix();
      fm.setTrans(messages.at(0).pointLocation);
      csysDragger->setMatrix(fm);
      sendDone();
      return;
    }

    ftr::Base *feature = dynamic_cast<ftr::Base*>(project->findFeature(id));
    assert(feature);
    if (feature->hasAnnex(ann::Type::CSysDragger))
    {
      osg::Vec3d move = messages.at(0).pointLocation - csysDragger->getMatrix().getTrans();
      ann::CSysDragger &da = feature->getAnnex<ann::CSysDragger>(ann::Type::CSysDragger);
      osg::Matrixd ft = static_cast<osg::Matrixd>(*(da.parameter));
      ft.setTrans(ft.getTrans() + move);
      da.setCSys(ft); //this should update the dragger also
    }
      
    sendDone();
    return;
  }
}

void CSysEdit::updateToVector(const osg::Vec3d& toVector)
{
  osg::Vec3d freshX, freshY, freshZ;
  osg::Matrixd originalMatrix = csysDragger->getMatrix();
  if (translateDragger->getName() == "xTranslate")
  {
    if
    (
      (std::fabs(toVector * gu::getYVector(originalMatrix))) <
      (std::fabs(toVector * gu::getZVector(originalMatrix)))
    )
    {
      freshZ = toVector ^ gu::getYVector(originalMatrix);
      freshZ.normalize();
      freshY = freshZ ^ toVector;
      freshY.normalize();
    }
    else
    {
      freshY = gu::getZVector(originalMatrix) ^ toVector;
      freshY.normalize();
      freshZ = toVector ^ freshY;
      freshZ.normalize();
    }
    freshX = toVector;
  }
  else if (translateDragger->getName() == "yTranslate")
  {
    if
    (
      (std::fabs(toVector * gu::getXVector(originalMatrix))) <
      (std::fabs(toVector * gu::getZVector(originalMatrix)))
    )
    {
      freshZ = gu::getXVector(originalMatrix) ^ toVector;
      freshZ.normalize();
      freshX = toVector ^ freshZ;
      freshX.normalize();
    }
    else
    {
      freshX = toVector ^ gu::getZVector(originalMatrix);
      freshX.normalize();
      freshZ = freshX ^ toVector;
      freshZ.normalize();
    }
    freshY = toVector;
  }
  else if (translateDragger->getName() == "zTranslate")
  {
    if
    (
      (std::fabs(toVector * gu::getXVector(originalMatrix))) <
      (std::fabs(toVector * gu::getYVector(originalMatrix)))
    )
    {
      freshY = toVector ^ gu::getXVector(originalMatrix);
      freshY.normalize();
      freshX = freshY ^ toVector;
      freshX.normalize();
    }
    else
    {
      freshX = gu::getYVector(originalMatrix) ^ toVector;
      freshX.normalize();
      freshY = toVector ^ freshX;
      freshY.normalize();
    }
    freshZ = toVector;
  }
  
  osg::Matrixd fm = originalMatrix;
  fm(0,0) = freshX.x(); fm(0,1) = freshX.y(); fm(0,2) = freshX.z();
  fm(1,0) = freshY.x(); fm(1,1) = freshY.y(); fm(1,2) = freshY.z();
  fm(2,0) = freshZ.x(); fm(2,1) = freshZ.y(); fm(2,2) = freshZ.z();
  
  boost::uuids::uuid id = gu::getId(csysDragger);
  if (id.is_nil() || (!csysDragger->isLinked())) //current system
  {
    csysDragger->setMatrix(fm);
    return;
  }

  ftr::Base *feature = dynamic_cast<ftr::Base*>(project->findFeature(id));
  assert(feature);
  if (feature->hasAnnex(ann::Type::CSysDragger))
  {
    osg::Matrixd diffMatrix = osg::Matrixd::inverse(originalMatrix) * fm;
    ann::CSysDragger &da = feature->getAnnex<ann::CSysDragger>(ann::Type::CSysDragger);
    da.setCSys(static_cast<osg::Matrixd>(*(da.parameter)) * diffMatrix); //should move dragger also.
  }
}

void CSysEdit::activate()
{
  if ((type == Type::Vector) && translateDragger)
    osgManipulator::setMaterialColor(translateDragger->getPickColor(), *translateDragger);
  if (type == Type::Origin)
    csysDragger->highlightOrigin();
  
  auto block = node->createBlocker();
  for (const auto &sMessage : messages)
  {
    msg::Message messageOut(msg::Message(msg::Request | msg::Selection | msg::Add, sMessage));
    node->send(messageOut);
  }
  isActive = true;
}

void CSysEdit::deactivate()
{
  if ((type == Type::Vector) && translateDragger)
    osgManipulator::setMaterialColor(translateDragger->getColor(), *translateDragger);
  if (type == Type::Origin)
    csysDragger->unHighlightOrigin();
  
  isActive = false;
}
