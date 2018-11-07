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

#include <cassert>

#include <osg/Billboard>
#include <osg/Switch>
#include <osg/Material>
#include <osg/Geometry>
#include <osg/PositionAttitudeTransform>
#include <osg/AutoTransform>
#include <osg/LOD>
#include <osg/ComputeBoundsVisitor>

#include "preferences/preferencesXML.h"
#include "preferences/manager.h"
#include "library/childnamevisitor.h"
#include "library/geometrylibrary.h"
#include "library/circlebuilderlod.h"
#include "library/angulardimension.h"

using namespace lbr;

static const char *rootTransformName = "lbr::AngularDimension";
static const char *rootSwitchName = "lbr::AngularDimension::Switch";
static const char *extensionLine1Name = "lbr::AngularDimension::Extension1";
static const char *extensionLine2Name = "lbr::AngularDimension::Extension2";
static const char *dimensionArc1Name = "lbr::AngularDimension::Arc1";
static const char *dimensionArc2Name = "lbr::AngularDimension::Arc2";
static const char *arrowTransform1Name = "lbr::AngularDimension::arrowTransform1";
static const char *arrowTransform2Name = "lbr::AngularDimension::arrowTransform2";
static const char *arrowShapeName = "lbr::AngularDimension::arrowShape";

/*! @brief Get rotation angle between to vectors.
 * @param v1 Start vector. Will be normalized.
 * @param v2 Finish vector. Will be normalized.
 * @return Angle in radians. [0, 2PI]
 * @note these vectors are expected to be 2d vectors.
 * z() component is zero.
 */
static double vectorAngle(osg::Vec3d v1, osg::Vec3d v2)
{
  v1.normalize();
  v2.normalize();
  double angle;
  osg::Vec3d cross = v1 ^ v2;
  double dot = v1 * v2;
  if (std::fabs(cross.z()) < std::numeric_limits<float>::epsilon())
  {
    if (dot > 0.0)
      angle = osg::PI * 2.0;
    else
      angle = osg::PI;
  }
  else
  {
    angle = std::acos(dot);
    if (cross.z() < 0.0)
      angle = (osg::PI * 2.0) - angle;
  }
  return angle;
}

AngularDimensionCallback::AngularDimensionCallback(const char *toi):
osg::NodeCallback()
, textObjectName(toi)
{
}

AngularDimensionCallback::AngularDimensionCallback(const std::string &toi):
osg::NodeCallback()
, textObjectName(toi)
{
}

void AngularDimensionCallback::setRangeMask1(const RangeMask &rmIn)
{
  if (!(rangeMask1 == rmIn))
  {
    rangeMask1 = rmIn;
    dirty = true;
  }
}

void AngularDimensionCallback::setRangeMask2(const RangeMask &rmIn)
{
  if (!(rangeMask2 == rmIn))
  {
    rangeMask2 = rmIn;
    dirty = true;
  }
}

void AngularDimensionCallback::setAngleDegrees(double aIn)
{
  angle = osg::DegreesToRadians(aIn);
  dirty = true;
}

void AngularDimensionCallback::setAngleRadians(double aIn)
{
  angle = aIn;
  dirty = true;
}

//! @brief set new location for text in local space. Only for update.
void AngularDimensionCallback::setTextLocation(const osg::Vec3d &nl)
{
  assert(textObject);
  textObject->setMatrix(osg::Matrixd::translate(nl));
  //shouldn't need to set dirty, as bounding box of text should trigger update.
}

/*! @brief set the extension factor
 * 
 * @details this controls where the extension line stops 
 * in relation to the distance between text object and 
 * origin. the default is 1.1 to extend 10% beyond
 * 
 */
void AngularDimensionCallback::setExtensionFactor(double ef)
{
  extensionFactor = ef;
  dirty = true;
}

void AngularDimensionCallback::operator()(osg::Node *node, osg::NodeVisitor *visitor)
{
  if (!isValid)
  {
    rootTransform = dynamic_cast<osg::MatrixTransform*>(node);
    assert(rootTransform); if (!rootTransform) return;
    init();
  }
  if (isValid.get())
  {
    osg::ComputeBoundsVisitor bv;
    textObject->accept(bv);
    osg::BoundingBox cb = bv.getBoundingBox();
    if (cb.valid())
    {
      if
      (
        (!cachedTextBound.valid()) ||
        (cachedTextBound != cb)
      )
        dirty = true;
      
      if (dirty)
      {
        cachedTextBound = cb;
        updateLayout();
        dirty = false;
      }
    }
  }
  
  traverse(node, visitor);
}

void AngularDimensionCallback::init()
{
  isValid = false;
  
  ChildNameVisitor rsv(rootSwitchName);
  rootTransform->accept(rsv);
  rootSwitch = rsv.castResult<osg::Switch>();
  if (!rootSwitch) return;
  
  ChildNameVisitor tov(textObjectName.c_str());
  rootSwitch->accept(tov);
  textObject = tov.castResult<osg::MatrixTransform>();
  if (!textObject) return;
  
  ChildNameVisitor eatv(extensionLine1Name);
  rootSwitch->accept(eatv);
  extensionLine1 = eatv.castResult<osg::Geometry>();
  if (!extensionLine1) return;
  
  ChildNameVisitor eatv2(extensionLine2Name);
  rootSwitch->accept(eatv2);
  extensionLine2 = eatv2.castResult<osg::Geometry>();
  if (!extensionLine2) return;
  
  ChildNameVisitor eav(dimensionArc1Name);
  rootSwitch->accept(eav);
  dimensionArc1 = eav.castResult<osg::PositionAttitudeTransform>();
  if (!dimensionArc1) return;
  
  ChildNameVisitor eav2(dimensionArc2Name);
  rootSwitch->accept(eav2);
  dimensionArc2 = eav2.castResult<osg::PositionAttitudeTransform>();
  if (!dimensionArc2) return;
  
  ChildNameVisitor dlv(arrowTransform1Name);
  rootSwitch->accept(dlv);
  arrowTransform1 = dlv.castResult<osg::AutoTransform>();
  if (!arrowTransform1) return;
  
  ChildNameVisitor dlv2(arrowTransform2Name);
  rootSwitch->accept(dlv2);
  arrowTransform2 = dlv2.castResult<osg::AutoTransform>();
  if (!arrowTransform2) return;
  
  isValid = true;
}

void AngularDimensionCallback::updateLayout()
{
  assert(textObject.valid());
  double textLength = cachedTextBound.center().length();
  if (textLength < std::numeric_limits<float>::epsilon())
    return;
  updateExtensionLines(textLength);
  
  double textAngle = vectorAngle(osg::Vec3d(1.0, 0.0, 0.0), cachedTextBound.center());
  if (textAngle >= 0.0 && textAngle <= angle)
  {
    //text center is inside the extension lines.
    osg::ComputeBoundsVisitor bv;
    arrowTransform1->accept(bv);
    osg::BoundingBox arrow1Bound = bv.getBoundingBox();
    
    bv.reset();
    arrowTransform2->accept(bv);
    osg::BoundingBox arrow2Bound = bv.getBoundingBox();
    
    //we want spheres for intersection test
    osg::BoundingSphered textSphere(cachedTextBound);
    osg::BoundingSphered arrow1Sphere(arrow1Bound);
    osg::BoundingSphered arrow2Sphere(arrow2Bound);
    if (textSphere.intersects(arrow1Sphere) || textSphere.intersects(arrow2Sphere))
    {
      placeArrowsOutside(textLength);
      updateDimensionArcsOutside(textLength, arrow1Bound.radius() * 4.0);
    }
    else
    {
      placeArrowsInside(textLength);
      osg::Vec3d distance1 = osg::Vec3d(textLength, 0.0, 0.0) - cachedTextBound.center();
      osg::Vec3d distance2 = osg::Quat(angle, osg::Vec3d(0.0, 0.0, 1.0)) * osg::Vec3d(textLength, 0.0, 0.0) - cachedTextBound.center();
      updateDimensionArcsInside(textLength, distance1.length() - cachedTextBound.radius(), distance2.length() - cachedTextBound.radius());
    }
  }
  else
  {
    placeArrowsInside(textLength);
    updateDimensionArcInsideSolid(textLength, textAngle);
  }
}

void AngularDimensionCallback::updateExtensionLines(double textLength)
{
  auto subUpdate = [&](RangeMask &rm, osg::Geometry *el, osg::Quat rotation)
  {
    //default to invalid range
    osg::Vec3d firstPoint(0.0, 0.0, 0.0); //default to
    osg::Vec3d secondPoint(textLength * extensionFactor, 0.0, 0.0);
    rootSwitch->setChildValue(el, true);
    if (rm.isValid())
    {
      if (rm.isIn(textLength))
      {
        rootSwitch->setChildValue(el, false);
      }
      else if (rm.isBelow(textLength))
      {
        //extension factor was setup to go longer.
        firstPoint = osg::Vec3d(textLength * (2.0 - extensionFactor), 0.0, 0.0);
        secondPoint = osg::Vec3d(rm.start, 0.0, 0.0);
      }
      else if (rm.isAbove(textLength))
      {
        firstPoint = osg::Vec3d(rm.finish, 0.0, 0.0);
        secondPoint = osg::Vec3d(textLength * extensionFactor, 0.0, 0.0);
      }
    }
    
    firstPoint = rotation * firstPoint;
    secondPoint = rotation * secondPoint;
    
    osg::Vec3Array *verts = dynamic_cast<osg::Vec3Array*>(el->getVertexArray());
    assert(verts);
    assert(verts->size() == 2);
    (*verts)[0] = firstPoint;
    (*verts)[1] = secondPoint;
    verts->dirty();
    el->dirtyBound();
  };
  
  subUpdate(rangeMask1, extensionLine1.get(), osg::Quat());
  subUpdate(rangeMask2, extensionLine2.get(), osg::Quat(angle, osg::Vec3d(0.0, 0.0, 1.0)));
}

void AngularDimensionCallback::placeArrowsInside(double textLength)
{
  arrowTransform1->setRotation(osg::Quat(osg::PI_2, osg::Vec3d(0.0, 0.0, -1.0)));
  arrowTransform1->setPosition(osg::Vec3d(textLength, 0.0, 0.0));
  
  arrowTransform2->setRotation(osg::Quat(osg::PI_2 + angle, osg::Vec3d(0.0, 0.0, 1.0)));
  osg::Quat r2 = osg::Quat(angle, osg::Vec3d(0.0, 0.0, 1.0));
  osg::Vec3d p2 = osg::Vec3d(textLength, 0.0, 0.0);
  p2 = r2 * p2;
  arrowTransform2->setPosition(p2);
}

void AngularDimensionCallback::placeArrowsOutside(double textLength)
{
  arrowTransform1->setRotation(osg::Quat(osg::PI_2, osg::Vec3d(0.0, 0.0, 1.0)));
  arrowTransform1->setPosition(osg::Vec3d(textLength, 0.0, 0.0));
  
  arrowTransform2->setRotation(osg::Quat(-osg::PI_2 + angle, osg::Vec3d(0.0, 0.0, 1.0)));
  osg::Quat r2 = osg::Quat(angle, osg::Vec3d(0.0, 0.0, 1.0));
  osg::Vec3d p2 = osg::Vec3d(textLength, 0.0, 0.0);
  p2 = r2 * p2;
  arrowTransform2->setPosition(p2);
}

void AngularDimensionCallback::updateDimensionArcsOutside(double textLength, double arcLength)
{
  double dimArcAngle = std::asin(arcLength / 2.0 / textLength) * 2.0;
  if (std::isnan(dimArcAngle) || dimArcAngle <= std::numeric_limits<float>::epsilon())
    return;
  assert(dimArcAngle > 0.0);
  
  dimensionArc1->removeChildren(0, dimensionArc1->getNumChildren());
  CircleBuilderLOD b1(textLength, dimArcAngle);
  dimensionArc1->addChild(b1);
  dimensionArc1->setAttitude(osg::Quat(dimArcAngle, osg::Vec3d(0.0, 0.0, -1.0)));
  
  dimensionArc2->removeChildren(0, dimensionArc2->getNumChildren());
  CircleBuilderLOD b2(textLength, dimArcAngle);
  dimensionArc2->addChild(b2);
  dimensionArc2->setAttitude(osg::Quat(angle, osg::Vec3d(0.0, 0.0, 1.0)));
  
  rootSwitch->setChildValue(dimensionArc2, true);
}

void AngularDimensionCallback::updateDimensionArcsInside(double textLength, double arcLength1, double arcLength2)
{
  double dimArcAngle1 = std::asin(arcLength1 / 2.0 / textLength) * 2.0;
  if (std::isnan(dimArcAngle1) || dimArcAngle1 <= std::numeric_limits<float>::epsilon())
    return;
  assert(dimArcAngle1 > 0.0);
  dimensionArc1->removeChildren(0, dimensionArc1->getNumChildren());
  CircleBuilderLOD b1(textLength, dimArcAngle1);
  dimensionArc1->addChild(b1);
  dimensionArc1->setAttitude(osg::Quat());
  
  double dimArcAngle2 = std::asin(arcLength2 / 2.0 / textLength) * 2.0;
  if (std::isnan(dimArcAngle2) || dimArcAngle2 <= std::numeric_limits<float>::epsilon())
    return;
  assert(dimArcAngle2 > 0.0);
  dimensionArc2->removeChildren(0, dimensionArc2->getNumChildren());
  CircleBuilderLOD b2(textLength, dimArcAngle2);
  dimensionArc2->addChild(b2);
  dimensionArc2->setAttitude(osg::Quat(angle - dimArcAngle2, osg::Vec3d(0.0, 0.0, 1.0)));
  
  rootSwitch->setChildValue(dimensionArc2, true);
}

void AngularDimensionCallback::updateDimensionArcInsideSolid(double textLength, double textAngle)
{
  dimensionArc1->removeChildren(0, dimensionArc1->getNumChildren());
  
  if (textAngle < (angle + osg::PI))
  {
    CircleBuilderLOD b1(textLength, textAngle);
    dimensionArc1->setAttitude(osg::Quat());
    dimensionArc1->addChild(b1);
  }
  else
  {
    double supplement = osg::PI * 2.0 - textAngle;
    CircleBuilderLOD b1(textLength, supplement + angle);
    dimensionArc1->setAttitude(osg::Quat(supplement, osg::Vec3d(0.0, 0.0, -1.0)));
    dimensionArc1->addChild(b1);
  }
  
  rootSwitch->setChildValue(dimensionArc2, false);
}

osg::MatrixTransform* lbr::buildAngularDimension(osg::MatrixTransform *textObject)
{
  osg::MatrixTransform* rootTransform = new osg::MatrixTransform();
  rootTransform->setName(rootTransformName);
  osg::Switch *rootSwitch = new osg::Switch();
  rootSwitch->setName(rootSwitchName);
  
  auto buildDummyLine = []()
  {
    osg::Geometry *out = new osg::Geometry();
  
    osg::Vec3Array *array = new osg::Vec3Array();
    array->push_back(osg::Vec3(0.0, 0.0, 0.0));
    array->push_back(osg::Vec3(1.0, 0.0, 0.0));
    out->setVertexArray(array);
    
    out->setUseDisplayList(false);
    out->setUseVertexBufferObjects(true);
    
    out->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, 2));
    return out;
  };
  osg::Geometry *extensionLine1 = buildDummyLine();
  extensionLine1->setName(extensionLine1Name);
  osg::Geometry *extensionLine2 = buildDummyLine();
  extensionLine2->setName(extensionLine2Name);
  
  osg::PositionAttitudeTransform *dimensionArc1 = new osg::PositionAttitudeTransform();
  dimensionArc1->setName(dimensionArc1Name);
  osg::PositionAttitudeTransform *dimensionArc2 = new osg::PositionAttitudeTransform();
  dimensionArc2->setName(dimensionArc2Name);
  
  osg::AutoTransform *arrowTransform1 = new osg::AutoTransform();
  arrowTransform1->setName(arrowTransform1Name);
  arrowTransform1->setAutoScaleToScreen(true);
  osg::AutoTransform *arrowTransform2 = new osg::AutoTransform();
  arrowTransform2->setName(arrowTransform2Name);
  arrowTransform2->setAutoScaleToScreen(true);
  osg::PositionAttitudeTransform *arrowShape = new osg::PositionAttitudeTransform();
  arrowShape->setName(arrowShapeName);
  const prf::InteractiveParameter& iPref = prf::manager().rootPtr->interactiveParameter();
  osg::Vec3d scale(iPref.arrowHeight(), iPref.arrowWidth(), 1.0);
  arrowShape->setScale(scale);
  
  assert(Manager::getManager().isLinked(dim::ArrowTag));
  osg::Geometry *arrow = Manager::getManager().getGeometry(dim::ArrowTag);
  
  //build graph. note billboard is not added by default.
  rootTransform->addChild(rootSwitch);
  rootSwitch->addChild(textObject);
  rootSwitch->addChild(extensionLine1);
  rootSwitch->addChild(extensionLine2);
  rootSwitch->addChild(dimensionArc1);
  rootSwitch->addChild(dimensionArc2);
  rootSwitch->addChild(arrowTransform1);
  rootSwitch->addChild(arrowTransform2);
  arrowTransform1->addChild(arrowShape);
  arrowTransform2->addChild(arrowShape);
  arrowShape->addChild(arrow);
  
  osg::Material *material = new osg::Material();
  material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0, 1.0, 0.0, 1.0));
  material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0, 1.0, 0.0, 1.0));
  rootTransform->getOrCreateStateSet()->setAttributeAndModes(material);
  rootTransform->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
  
  return rootTransform;
}
