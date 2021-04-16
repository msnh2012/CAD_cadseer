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

#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <limits>

#include <QObject>

#include <osg/Switch>
#include <osg/AutoTransform>

#include "application/appapplication.h"
#include "parameter/prmparameter.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "message/msgmessage.h"
#include "project/serial/generated/prjsrlsptoverlay.h"
#include "library/lbrlineardimension.h"
#include "library/lbrlineardragger.h"
#include "library/lbripgroup.h"

using namespace lbr;

IPGroup::IPGroup()
{
  assert(0);
}

IPGroup::IPGroup(prm::Parameter *parameterIn) :
osg::MatrixTransform(),
mainDim(),
differenceDim(),
overallDim(),
parameter(parameterIn),
value(std::numeric_limits<double>::max()),
prmObserver
(
  std::make_unique<prm::Observer>
  (
    std::bind(&IPGroup::valueHasChanged, this)
    , std::bind(&IPGroup::constantHasChanged, this)
    , std::bind(&IPGroup::activeHasChanged, this)
  )
)
{
  displaySwitch = new osg::Switch();
  this->addChild(displaySwitch.get());
  
  rotation = new osg::AutoTransform();
  displaySwitch->addChild(rotation.get());
  
  dimSwitch = new osg::Switch();
  rotation->addChild(dimSwitch.get());
  
  mainDim = new LinearDimension();
  dimSwitch->addChild(mainDim.get());
  
  differenceDim = new LinearDimension();
  differenceDim->setColor(osg::Vec4d(0.79, 0.13, 0.48, 1.0));
  dimSwitch->addChild(differenceDim.get());
  dimSwitch->setChildValue(differenceDim.get(), false);
  
  overallDim = new LinearDimension();
  overallDim->setColor(osg::Vec4d(0.79, 0.13, 0.48, 1.0));
  dimSwitch->addChild(overallDim.get());
  dimSwitch->setChildValue(overallDim.get(), false);
  
  draggerMatrix = new osg::MatrixTransform();
  draggerMatrix->setMatrix(osg::Matrixd::identity());
  rotation->addChild(draggerMatrix.get());
  
  draggerSwitch = new osg::Switch();
  draggerMatrix->addChild(draggerSwitch.get());
  
  dragger = new LinearDragger();
  dragger->setScreenScale(75.0);
  dragger->setColor(osg::Vec4d(0.79, 0.13, 0.48, 1.0));
  draggerSwitch->addChild(dragger.get());
  
  ipCallback = new IPCallback(dragger.get(), this);
  dragger->addDraggerCallback(ipCallback.get());
  
  refresh();
  parameter->connect(*prmObserver);
}

IPGroup::IPGroup(const IPGroup& copy, const osg::CopyOp& copyOp) : osg::MatrixTransform(copy, copyOp)
{
  assert(0);
}

void IPGroup::setRotationAxis(const osg::Vec3d &axisIn, const osg::Vec3d &normalIn)
{
  rotation->setAutoRotateMode(osg::AutoTransform::ROTATE_TO_AXIS);
  rotation->setAxis(axisIn);
  rotation->setNormal(normalIn);
}

void IPGroup::setMatrixDims(const osg::Matrixd& matrixIn)
{
  mainDim->setMatrix(matrixIn);
  differenceDim->setMatrix(matrixIn);
  overallDim->setMatrix(matrixIn);
}

void IPGroup::setMatrixDragger(const osg::Matrixd& matrixIn)
{
  draggerMatrix->setMatrix(matrixIn);
}

void IPGroup::setDimsFlipped(bool flippedIn)
{
  mainDim->setFlipped(flippedIn);
  differenceDim->setFlipped(flippedIn);
  overallDim->setFlipped(flippedIn);
}

void IPGroup::refresh()
{
  constantHasChanged();
  valueHasChanged();
  activeHasChanged();
}

void IPGroup::noAutoRotateDragger()
{
  rotation->removeChild(draggerMatrix.get());
  displaySwitch->addChild(draggerMatrix.get());
}

osg::BoundingBox IPGroup::maxTextBoundingBox()
{
  osg::BoundingBox box = mainDim->getTextBoundingBox();
  if (differenceDim->getTextBoundingBox().radius() > box.radius())
    box = differenceDim->getTextBoundingBox();
  if (overallDim->getTextBoundingBox().radius() > box.radius())
    box = overallDim->getTextBoundingBox();
  
  return box;
}

void IPGroup::valueHasChanged()
{
  assert(parameter);
  value = parameter->getDouble();
  mainDim->setSpread(value);
  overallDim->setSpread(value);
  dragger->setMatrix(osg::Matrixd::translate(0.0, 0.0, value));
}

void IPGroup::constantHasChanged()
{
  assert(parameter);
  if (parameter->isConstant())
  {
    draggerShow();
    mainDim->setTextColor(osg::Vec4(0.0, 0.0, 1.0, 1.0));
  }
  else
  {
    draggerHide();
    mainDim->setTextColor(osg::Vec4(0.0, 1.0, 0.0, 1.0));
  }
}

void IPGroup::activeHasChanged()
{
  if (parameter->isActive())
    displaySwitch->setAllChildrenOn();
  else
    displaySwitch->setAllChildrenOff();
}

void IPGroup::draggerShow()
{
  draggerSwitch->setAllChildrenOn();
}

void IPGroup::draggerHide()
{
  draggerSwitch->setAllChildrenOff();
}

prj::srl::spt::IPGroup IPGroup::serialOut() const
{
  auto matrixOut = [](const osg::Matrixd &m) -> prj::srl::spt::Matrixd
  {
    return prj::srl::spt::Matrixd
    (
      m(0,0), m(0,1), m(0,2), m(0,3),
      m(1,0), m(1,1), m(1,2), m(1,3),
      m(2,0), m(2,1), m(2,2), m(2,3),
      m(3,0), m(3,1), m(3,2), m(3,3)
    );
  };
  
  auto vecOut = [](const osg::Vec3d &v) -> prj::srl::spt::Vec3d
  {
    return prj::srl::spt::Vec3d(v.x(), v.y(), v.z());
  };
  
  return prj::srl::spt::IPGroup
  (
    matrixOut(this->getMatrix())
    , matrixOut(mainDim->getMatrix())
    , matrixOut(draggerMatrix->getMatrix())
    , vecOut(rotation->getAxis())
    , vecOut(rotation->getNormal())
    , mainDim->isFlipped()
    , static_cast<bool>(rotation->getNumChildren())
    , draggerSwitch->getChildValue(dragger.get())
    , prj::srl::spt::Color(1.0, 0.0, 0.0, 1.0) //don't currently need.
  );
}

void IPGroup::serialIn(const prj::srl::spt::IPGroup &sgi)
{
  auto matrixIn = [](const prj::srl::spt::Matrixd &mIn) -> osg::Matrixd
  {
    return osg::Matrixd
    (
      mIn.i0j0(), mIn.i0j1(), mIn.i0j2(), mIn.i0j3(),
      mIn.i1j0(), mIn.i1j1(), mIn.i1j2(), mIn.i1j3(),
      mIn.i2j0(), mIn.i2j1(), mIn.i2j2(), mIn.i2j3(),
      mIn.i3j0(), mIn.i3j1(), mIn.i3j2(), mIn.i3j3()
    );
  };
  
  auto vecIn = [](const prj::srl::spt::Vec3d &v) -> osg::Vec3d
  {
    return osg::Vec3d(v.x(), v.y(), v.z());
  };
  
  setMatrix(matrixIn(sgi.matrixRoot()));
  setMatrixDims(matrixIn(sgi.matrixDims()));
  setMatrixDragger(matrixIn(sgi.matrixDragger()));
  setRotationAxis(vecIn(sgi.rotationAxis()), vecIn(sgi.rotationNormal()));
  setDimsFlipped(sgi.dimsFlipped());
  if (!sgi.rotateDragger())
    noAutoRotateDragger();
  if (sgi.visibleDragger())
    draggerShow();
  else
    draggerHide();
  //not using color
  
  valueHasChanged();
  constantHasChanged();
  activeHasChanged();
}

bool IPGroup::processMotion(const osgManipulator::MotionCommand &commandIn)
{
  assert(parameter);
  
  static double lastTranslation;
  
  const osgManipulator::TranslateInLineCommand *tCommand = 
    dynamic_cast<const osgManipulator::TranslateInLineCommand *>(&commandIn);
  assert(tCommand);
  
  std::ostringstream stream;
  if (commandIn.getStage() == osgManipulator::MotionCommand::START)
  {
    originStart = tCommand->getTranslation() * tCommand->getLocalToWorld();
    lastTranslation = 0.0;
    
    //assumes differenceDim and mainDim have same orientation.
    osg::Matrixd temp = mainDim->getMatrix();
    temp.setTrans(osg::Vec3d(0.0, value, 0.0) * temp);
    differenceDim->setMatrix(temp);
    differenceDim->setSpread(0.0);
    
    //set other dimensions properties relative to mains.
    double mainTextDia = mainDim->getTextBoundingBox().radius() * 2.0;
    double extensionOffset = mainDim->getExtensionOffset() * mainDim->getFlippedFactor();
    differenceDim->setSqueeze(mainDim->getSqueeze() + mainTextDia);
    differenceDim->setExtensionOffset(extensionOffset);
    overallDim->setSqueeze(differenceDim->getSqueeze() + mainTextDia);
    overallDim->setExtensionOffset(extensionOffset);
    
    dragger->setIncrement(prf::manager().rootPtr->dragger().linearIncrement());
    
    dimSwitch->setChildValue(differenceDim.get(), true);
    dimSwitch->setChildValue(overallDim.get(), true);
  }
    
  if (commandIn.getStage() == osgManipulator::MotionCommand::MOVE)
  {
    //the constraint on dragger limits the values but still passes all
    //events on through. so we filter them out.
    //have to be in world to calculate the value
    osg::Vec3d tVec = tCommand->getTranslation() * tCommand->getLocalToWorld();
    double diff = (tVec-originStart).length();
    if (std::fabs(diff - lastTranslation) < std::numeric_limits<double>::epsilon())
      return false;
    
    //but have to be local to get sign of value.
    double directionFactor = 1.0;
    if (tCommand->getTranslation().z() < 0.0)
      directionFactor = -1.0;
    double freshOverall = diff * directionFactor + value;
    
    //limit to positive overall size.
    //might make this a custom manipulator constraint.
    if (!parameter->isValidValue(freshOverall))
    {
      osg::Matrixd temp = dragger->getMatrix();
      temp.setTrans(osg::Vec3d(0.0, 0.0, overallDim->getSpread()));
      dragger->setMatrix(temp);
      app::instance()->messageSlot(msg::buildStatusMessage(
        QObject::tr("Value out of range").toStdString()));
      return false;
    }
    lastTranslation = diff;
    
    differenceDim->setSpread(diff * directionFactor);
    overallDim->setSpread(freshOverall);
    
    stream << QObject::tr("Translation Increment: ").toUtf8().data() << std::setprecision(3) << std::fixed <<
      prf::manager().rootPtr->dragger().linearIncrement() << std::endl <<
      QObject::tr("Translation: ").toUtf8().data() << std::setprecision(3) << std::fixed << diff;
  }
  
  if (commandIn.getStage() == osgManipulator::MotionCommand::FINISH)
  {
    dimSwitch->setChildValue(differenceDim.get(), false);
    dimSwitch->setChildValue(overallDim.get(), false);
    parameter->setValue(overallDim->getSpread());
    
    //add git message.
    std::ostringstream gitStream;
    gitStream << QObject::tr("Parameter ").toStdString() << parameter->getName().toStdString() <<
      QObject::tr(" changed to: ").toStdString() << parameter->getDouble();
    app::instance()->messageSlot(msg::buildGitMessage(gitStream.str()));
    
    if (prf::manager().rootPtr->dragger().triggerUpdateOnFinish())
    {
      msg::Message messageOut(msg::Request | msg::Project | msg::Update);
      app::instance()->queuedMessage(messageOut);
    }
  }
  
  app::instance()->messageSlot(msg::buildStatusMessage(stream.str()));
  
  return true;
}

IPCallback::IPCallback(osg::MatrixTransform* t, IPGroup *groupIn):
  DraggerTransformCallback(t, HANDLE_TRANSLATE_IN_LINE), group(groupIn)
{
  assert(group);
}

bool IPCallback::receive(const osgManipulator::TranslateInLineCommand& commandIn)
{
  return group->processMotion(commandIn);
}
