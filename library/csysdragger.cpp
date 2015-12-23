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

#include <assert.h>

#include <osgManipulator/Constraint>
#include <osgUtil/SmoothingVisitor>

#include "geometrylibrary.h"
#include <nodemaskdefs.h>
#include "csysdragger.h"

using namespace osg;
using namespace osgManipulator;
using namespace lbr;

CSysDragger::CSysDragger()
{
  this->setNodeMask(NodeMaskDef::csys);
  
  autoTransform = new osg::AutoTransform();
  autoTransform->setAutoScaleToScreen(true);
  autoTransform->setPosition(osg::Vec3d(0.0, 0.0, 0.0));
  this->addChild(autoTransform);
  
  //why do I need this matrix transform??????
  screenScale = 200.0;
  matrixTransform = new osg::MatrixTransform();
  matrixTransform->setMatrix(osg::Matrixd::scale(osg::Vec3d(screenScale, screenScale, screenScale)));
  autoTransform->addChild(matrixTransform);
  
  draggerSwitch = new Switch();
  matrixTransform->addChild(draggerSwitch);
  
  xTranslate = new Translate1DDragger(osg::Vec3(0.0,0.0,0.0), osg::Vec3(0.0,0.0,1.0));
  draggerSwitch->addChild(xTranslate.get());
  addDragger(xTranslate.get());

  yTranslate = new Translate1DDragger(osg::Vec3(0.0,0.0,0.0), osg::Vec3(0.0,0.0,1.0));
  draggerSwitch->addChild(yTranslate.get());
  addDragger(yTranslate.get());

  zTranslate = new Translate1DDragger(osg::Vec3(0.0,0.0,0.0), osg::Vec3(0.0,0.0,1.0));
  draggerSwitch->addChild(zTranslate.get());
  addDragger(zTranslate.get());
  
  xRotate = new RotateCircularDragger();
  draggerSwitch->addChild(xRotate.get());
  addDragger(xRotate.get());
  
  yRotate = new RotateCircularDragger();
  draggerSwitch->addChild(yRotate.get());
  addDragger(yRotate.get());
  
  zRotate = new RotateCircularDragger();
  draggerSwitch->addChild(zRotate.get());
  addDragger(zRotate.get());
  
  originGeode = new osg::Geode(); //so we can add color above sphere
  draggerSwitch->addChild(originGeode.get());
  
  iconSwitch = new osg::Switch();
  draggerSwitch->addChild(iconSwitch.get());

  translateConstraint = new GridConstraint(*this, Vec3(0.0, 0.0, 0.0), Vec3(0.5, 0.5, 0.5));
  xTranslate->addConstraint(translateConstraint.get());
  yTranslate->addConstraint(translateConstraint.get());
  zTranslate->addConstraint(translateConstraint.get());
  
  rotateConstraint = new AngleConstraint(*this, Vec3(1.0, 0.0, 0.0), osg::DegreesToRadians(15.0));
  xRotate->addConstraint(rotateConstraint.get());
  yRotate->addConstraint(rotateConstraint.get());
  zRotate->addConstraint(rotateConstraint.get());
  
  this->getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);

  setParentDragger(getParentDragger());
}

CSysDragger::~CSysDragger()
{
}

void CSysDragger::setupDefaultGeometry()
{
  this->getOrCreateStateSet()->setAttribute(new osg::LineWidth(3.0));
  
  assert(lbr::Manager::getManager().isLinked(lbr::csys::SphereTag));
  originGeode->addDrawable(lbr::Manager::getManager().getGeometry(lbr::csys::SphereTag));
  setMaterialColor(osg::Vec4(0.7, 0.5, 0.8, 1.0), *originGeode);
  
  setupDefaultTranslation();
  setupDefaultRotation();
  setupIcons();
}

void CSysDragger::setupDefaultRotation()
{
  //by building the geometry here instead of call setupdefault,
  //the axes can share the geometry, instead of each creating it's own duplicate.
  
  assert(lbr::Manager::getManager().isLinked(lbr::csys::SphereTag));
  assert(lbr::Manager::getManager().isLinked(lbr::csys::RotationLineTag));
  assert(lbr::Manager::getManager().isLinked(lbr::csys::RotationTorusTag));
  osg::Geometry *sphere = lbr::Manager::getManager().getGeometry(lbr::csys::SphereTag);
  osg::Geometry *rotateLine = lbr::Manager::getManager().getGeometry(lbr::csys::RotationLineTag);
  osg::Geometry *rotateTorus = lbr::Manager::getManager().getGeometry(lbr::csys::RotationTorusTag);
  
  //move sphere off of origin.
  osg::MatrixTransform *transform = new osg::MatrixTransform();
  osg::Vec3d sphereLocation(1.0, 1.0, 0.0);
  sphereLocation.normalize();
  sphereLocation *= 0.75;
  transform->setMatrix(osg::Matrixd::translate(sphereLocation));
  transform->addChild(sphere);
  
  xRotate->addChild(rotateLine);
  xRotate->addChild(rotateTorus);
  xRotate->addChild(transform);
  yRotate->addChild(rotateLine);
  yRotate->addChild(rotateTorus);
  yRotate->addChild(transform);
  zRotate->addChild(rotateLine);
  zRotate->addChild(rotateTorus);
  zRotate->addChild(transform);
  
  xRotate->setMatrix(Matrixd::rotate(osg::Quat(M_PI_2, Vec3d(0.0, -1.0, 0.0))));
  yRotate->setMatrix(Matrixd::rotate(osg::Quat(M_PI_2, Vec3d(1.0, 0.0, 0.0))));
  
  // Send different colors for each dragger.
  xRotate->setColor(osg::Vec4(1.0f,0.0f,0.0f,1.0f));
  yRotate->setColor(osg::Vec4(0.0f,1.0f,0.0f,1.0f));
  zRotate->setColor(osg::Vec4(0.0f,0.0f,1.0f,1.0f));
}

void CSysDragger::setupDefaultTranslation()
{
  //by building the geometry here instead of call setupdefault,
  //the axes can share the geometry, instead of each creating it's own duplicate.
  
  assert(lbr::Manager::getManager().isLinked(lbr::csys::TranslationLineTag));
  assert(lbr::Manager::getManager().isLinked(lbr::csys::TranslationCylinderTag));
  assert(lbr::Manager::getManager().isLinked(lbr::csys::TranslationConeTag));
  osg::Geometry *translateLine = lbr::Manager::getManager().getGeometry(lbr::csys::TranslationLineTag);
  osg::Geometry *translateCylinder = lbr::Manager::getManager().getGeometry(lbr::csys::TranslationCylinderTag);
  osg::Geometry *translateCone = lbr::Manager::getManager().getGeometry(lbr::csys::TranslationConeTag);

  xTranslate->addChild(translateLine);
  yTranslate->addChild(translateLine);
  zTranslate->addChild(translateLine);

  xTranslate->addChild(translateCone);
  yTranslate->addChild(translateCone);
  zTranslate->addChild(translateCone);

  xTranslate->addChild(translateCylinder);
  yTranslate->addChild(translateCylinder);
  zTranslate->addChild(translateCylinder);

  //Rotate axes to correct position.
  osg::Quat xRotation;
  xRotation.makeRotate(osg::Vec3(0.0f, 0.0f, 1.0f), osg::Vec3(1.0f, 0.0f, 0.0f));
  xTranslate->setMatrix(osg::Matrix(xRotation));

  osg::Quat yRotation;
  yRotation.makeRotate(osg::Vec3(0.0f, 0.0f, 1.0f), osg::Vec3(0.0f, 1.0f, 0.0f));
  yTranslate->setMatrix(osg::Matrix(yRotation));

  // Send different colors for each dragger.
  xTranslate->setColor(osg::Vec4(1.0f,0.0f,0.0f,1.0f));
  yTranslate->setColor(osg::Vec4(0.0f,1.0f,0.0f,1.0f));
  zTranslate->setColor(osg::Vec4(0.0f,0.0f,1.0f,1.0f));
}

void CSysDragger::setupIcons()
{
  osg::Geometry *link = lbr::Manager::getManager().getGeometry(lbr::csys::IconLinkTag);
  link->setName("LinkIcon");
  osg::Geometry *unlink = lbr::Manager::getManager().getGeometry(lbr::csys::IconUnlinkTag);
  unlink->setName("UnlinkIcon");
  
  osg::AutoTransform *linkTransform = new osg::AutoTransform();
  linkTransform->setAutoRotateMode(osg::AutoTransform::ROTATE_TO_SCREEN);
  linkTransform->setScale(osg::Vec3d(0.25, 0.25, 0.25));
  linkTransform->setPosition(osg::Vec3d(0.25, 0.25, 0.25));
  linkTransform->addChild(link);
  iconSwitch->addChild(linkTransform);
  
  osg::AutoTransform *unlinkTransform = new osg::AutoTransform();
  unlinkTransform->setAutoRotateMode(osg::AutoTransform::ROTATE_TO_SCREEN);
  unlinkTransform->setScale(osg::Vec3d(0.25, 0.25, 0.25));
  unlinkTransform->setPosition(osg::Vec3d(0.25, 0.25, 0.25));
  unlinkTransform->addChild(unlink);
  iconSwitch->addChild(unlinkTransform);
  
  setLink();
}

void CSysDragger::setScreenScale(float scaleIn)
{
  if (screenScale == scaleIn)
    return;
  screenScale = scaleIn;
  matrixTransform->setMatrix(osg::Matrixd::scale(osg::Vec3d(screenScale, screenScale, screenScale)));
}

void CSysDragger::setTranslationIncrement(double incrementIn)
{
  translateConstraint->setSpacing(osg::Vec3d(incrementIn, incrementIn, incrementIn));
}

void CSysDragger::setRotationIncrement(double incrementIn)
{
  rotateConstraint->setAngle(osg::DegreesToRadians(incrementIn));
}

void CSysDragger::show(CSysDragger::SwitchIndexes index)
{
  draggerSwitch->setValue(static_cast<unsigned int>(index), true);
}

void CSysDragger::hide(CSysDragger::SwitchIndexes index)
{
  draggerSwitch->setValue(static_cast<unsigned int>(index), false);
}

void CSysDragger::linkToMatrix(MatrixTransform *matrixIn)
{
  if (matrixLinked)
    addTransformUpdating(matrixIn);
  matrixLinks.push_back(matrixIn);
}

void CSysDragger::unlinkToMatrix(MatrixTransform *matrixIn)
{
  auto it = std::find(matrixLinks.begin(), matrixLinks.end(), matrixIn);
  if (it == matrixLinks.end())
    return;
  if (matrixLinked)
    removeTransformUpdating(matrixIn);
  matrixLinks.erase(it);
}

void CSysDragger::setLink()
{
  iconSwitch->setValue(1, false);
  iconSwitch->setValue(0, true);
  matrixLinked = true;
  for (auto transform : matrixLinks)
    addTransformUpdating(transform);
}

void CSysDragger::setUnlink()
{
  iconSwitch->setValue(0, false);
  iconSwitch->setValue(1, true);
  matrixLinked = false;
  for (auto transform : matrixLinks)
    removeTransformUpdating(transform);
}

