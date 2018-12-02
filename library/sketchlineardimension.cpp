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

#include <boost/optional/optional.hpp>

#include <osg/Billboard>
#include <osg/Switch>
#include <osg/AutoTransform>
#include <osg/MatrixTransform>
#include <osg/PositionAttitudeTransform>
#include <osg/Material>
#include <osg/Geometry>
#include <osg/ComputeBoundsVisitor>

#include "preferences/preferencesXML.h"
#include "preferences/manager.h"
#include "library/childnamevisitor.h"
#include "library/geometrylibrary.h"
#include "library/sketchlineardimension.h"

static const char *rootTransformName = "lbr::LinearDimension";
static const char *rootSwitchName = "lbr::LinearDimension::Switch";
static const char *extensionLine1Name = "lbr::LinearDimension::Extension1";
static const char *extensionLine2Name = "lbr::LinearDimension::Extension2";
static const char *dimensionLine1Name = "lbr::LinearDimension::Dimension1";
static const char *dimensionLine2Name = "lbr::LinearDimension::Dimension2";
static const char *arrowTransform1Name = "lbr::LinearDimension::arrowTransform1";
static const char *arrowTransform2Name = "lbr::LinearDimension::arrowTransform2";
static const char *arrowShapeName = "lbr::LinearDimension::arrowShape";

using namespace lbr;

//! @brief set the text object name to search for when updating
void LinearDimensionCallback::setTextObjectName(const char *ton)
{
  textObjectName = ton;
  dirty = true;
}

//! @brief set new location for text in local space. Only for update.
void LinearDimensionCallback::setTextLocation(const osg::Vec3d &nl)
{
  assert(textObject);
  textObject->setMatrix(osg::Matrixd::translate(nl));
  //shouldn't need to set dirty, as bounding box of text should trigger update.
}

//! @brief set the value for the dimension
void LinearDimensionCallback::setDistance(double dIn)
{
  distance = dIn;
  dirty = true;
}

//! @brief set the 1st range mask
void LinearDimensionCallback::setRangeMask1(const RangeMask &rmIn)
{
  rangeMask1 = rmIn;
  dirty = true;
}

//! @brief set the 2nd range mask
void LinearDimensionCallback::setRangeMask2(const RangeMask &rmIn)
{
  rangeMask2 = rmIn;
  dirty = true;
}

/*! @brief set the extension factor
 * 
 * @details this controls where the extension line stops 
 * in relation to the distance between text object and 
 * origin. the default is 1.1 to extend 10% beyond
 * 
 */
void LinearDimensionCallback::setExtensionFactor(double ef)
{
  extensionFactor = ef;
  dirty = true;
}

//! @brief get the extension factor
double LinearDimensionCallback::getExtensionFactor() const
{
  return extensionFactor;
}

void LinearDimensionCallback::operator()(osg::Node *node, osg::NodeVisitor *visitor)
{
  if (!isValid)
  {
    rootTransform = dynamic_cast<osg::MatrixTransform*>(node);
    assert(rootTransform);
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

void LinearDimensionCallback::init()
{
  isValid = false;
  
  if (!rootTransform) return;
  
  ChildNameVisitor rsv(rootSwitchName);
  rootTransform->accept(rsv);
  assert(rsv.out); if (!rsv.out) return;
  rootSwitch = rsv.out->asSwitch();
  assert(rootSwitch); if (!rootSwitch) return;
  
  if (!textObjectName) return;
  ChildNameVisitor cnv(textObjectName);
  rootTransform->accept(cnv);
  assert(cnv.out); if (!cnv.out) return;
  textObject = dynamic_cast<osg::MatrixTransform*>(cnv.out);
  assert(textObject); if (!textObject) return;
  
  auto getGeom = [&](const char *elName) -> osg::Geometry*
  {
    ChildNameVisitor visitor(elName);
    rootTransform->accept(visitor);
    assert(visitor.out); if (!visitor.out) return nullptr;
    osg::Geometry *out = visitor.out->asGeometry();
    assert(out); if (!out) return nullptr;
    return out;
  };
  extensionLine1 = getGeom(extensionLine1Name); if (!extensionLine1) return;
  extensionLine2 = getGeom(extensionLine2Name); if (!extensionLine2) return;
  dimensionLine1 = getGeom(dimensionLine1Name); if (!dimensionLine1) return;
  dimensionLine2 = getGeom(dimensionLine2Name); if (!dimensionLine2) return;
  
  auto getArrow = [&](const char *aName) -> osg::AutoTransform*
  {
    ChildNameVisitor visitor(aName);
    rootTransform->accept(visitor);
    assert(visitor.out); if (!visitor.out) return nullptr;
    osg::AutoTransform *out = visitor.out->asTransform()->asAutoTransform();
    assert(out); if (!out) return nullptr;
    return out;
  };
  arrowTransform1 = getArrow(arrowTransform1Name); if (!arrowTransform1) return;
  arrowTransform2 = getArrow(arrowTransform2Name); if (!arrowTransform2) return;
  
  
  isValid = true;
}

void LinearDimensionCallback::updateLayout()
{
  if (std::fabs(distance) < std::numeric_limits<float>::epsilon())
    return;
  osg::Vec3d textPosition = textObject->getMatrix().getTrans();
  if (std::fabs(textPosition.y()) > std::numeric_limits<float>::epsilon())
    updateExtensionLines(textPosition);
  RangeMask testMask(0.0, distance);
  if (testMask.isIn(textPosition.x()))
  {
    //text is inside the extension lines.
    osg::ComputeBoundsVisitor bv;
    extensionLine1->accept(bv);
    osg::BoundingBox el1Bound = bv.getBoundingBox();
    bv.reset();
    extensionLine2->accept(bv);
    osg::BoundingBox el2Bound = bv.getBoundingBox();
    
    //there is no z thickness with these bounding box.
    //so we get no intersection when we should due to
    //rounding errors, so just set thickness for testing
    el1Bound.zMin() = -1.0; el1Bound.zMax() = 1.0;
    el2Bound.zMin() = -1.0; el2Bound.zMax() = 1.0;
    
    if (cachedTextBound.intersects(el1Bound) || cachedTextBound.intersects(el2Bound))
    {
      placeArrowsOutside();
      updateDimensionLinesOutside();
    }
    else
    {
      placeArrowsInside();
      updateDimensionLinesInside();
    }
    rootSwitch->setChildValue(dimensionLine2, true);
  }
  else
  {
    //text is outside extension lines.
    placeArrowsInside();
    updateDimensionLineInsideSolid();
    rootSwitch->setChildValue(dimensionLine2, false);
  }
}

void LinearDimensionCallback::updateExtensionLines(const osg::Vec3d &textPosition)
{
  auto subUpdate = [&](const RangeMask &rm, osg::Geometry *el, double xPosition)
  {
    //default to invalid range
    osg::Vec3d firstPoint(xPosition, 0.0, 0.0);
    osg::Vec3d secondPoint(xPosition, textPosition.y() * extensionFactor, 0.0);
    rootSwitch->setChildValue(el, true);
    if (rm.isValid())
    {
      if (rm.isIn(textPosition.y()))
      {
        rootSwitch->setChildValue(el, false);
      }
      else if (rm.isBelow(textPosition.y()))
      {
        firstPoint = osg::Vec3d(xPosition, textPosition.y() * extensionFactor, 0.0);
        secondPoint = osg::Vec3d(xPosition, rm.start, 0.0);
      }
      else if (rm.isAbove(textPosition.y()))
      {
        firstPoint = osg::Vec3d(xPosition, rm.finish, 0.0);
        secondPoint = osg::Vec3d(xPosition, textPosition.y() * extensionFactor, 0.0);
      }
    }
    
    osg::Vec3Array *verts = dynamic_cast<osg::Vec3Array*>(el->getVertexArray());
    assert(verts);
    assert(verts->size() == 2);
    (*verts)[0] = firstPoint;
    (*verts)[1] = secondPoint;
    verts->dirty();
    el->dirtyBound();
  };
  
  subUpdate(rangeMask1, extensionLine1, 0.0);
  subUpdate(rangeMask2, extensionLine2, distance);
}

void LinearDimensionCallback::placeArrowsInside()
{
  osg::Quat z; //zero rotation
  osg::Quat f(osg::PI, osg::Vec3d(0.0, 0.0, 1.0)); //180 deg rotation.
  
  arrowTransform1->setRotation((distance > 0.0) ? f : z);
  arrowTransform1->setPosition(osg::Vec3d(0.0, cachedTextBound.center().y(), 0.0));
  
  arrowTransform2->setRotation((distance < 0.0) ? f : z);
  arrowTransform2->setPosition(osg::Vec3d(distance, cachedTextBound.center().y(), 0.0));
}

void LinearDimensionCallback::placeArrowsOutside()
{
  osg::Quat z; //zero rotation
  osg::Quat f(osg::PI, osg::Vec3d(0.0, 0.0, 1.0)); //180 deg rotation.
  
  arrowTransform1->setRotation((distance > 0.0) ? z : f);
  arrowTransform1->setPosition(osg::Vec3d(0.0, cachedTextBound.center().y(), 0.0));
  
  arrowTransform2->setRotation((distance < 0.0) ? z : f);
  arrowTransform2->setPosition(osg::Vec3d(distance, cachedTextBound.center().y(), 0.0));
}

void LinearDimensionCallback::updateDimensionLinesInside()
{
  double tx = cachedTextBound.center().x();
  double ty = cachedTextBound.center().y();
  double tr = cachedTextBound.radius() * ((distance < 0.0) ? -1.0 : 1.0);
  
  osg::Vec3d dl1Start(0.0, ty, 0.0);
  osg::Vec3d dl1Finish(tx - tr, ty, 0.0);
  updateDimLine(dimensionLine1, dl1Start, dl1Finish);
  
  osg::Vec3d dl2Start(tx + tr, ty, 0.0);
  osg::Vec3d dl2Finish(distance, ty, 0.0);
  updateDimLine(dimensionLine2, dl2Start, dl2Finish);
}

void LinearDimensionCallback::updateDimensionLinesOutside()
{
  //get bounding box of arrow and use for length of lines.
  osg::ComputeBoundsVisitor bv;
  arrowTransform1->accept(bv);
  double length = bv.getBoundingBox().radius() * 4.0;
  
  double df = (distance < 0.0) ? -1.0 : 1.0; // direction factor.
  double d = distance;
  double ty = cachedTextBound.center().y();
  
  osg::Vec3d dl1Start(-length * df, ty, 0.0);
  osg::Vec3d dl1Finish(0.0, ty, 0.0);
  updateDimLine(dimensionLine1, dl1Start, dl1Finish);
  
  osg::Vec3d dl2Start(d, ty, 0.0);
  osg::Vec3d dl2Finish(d + (length * df), ty, 0.0);
  updateDimLine(dimensionLine2, dl2Start, dl2Finish);
}

void LinearDimensionCallback::updateDimensionLineInsideSolid()
{
  double tx = cachedTextBound.center().x();
  double ty = cachedTextBound.center().y();
  double tr = cachedTextBound.radius();
  
  osg::Vec3d p1, p2;
  RangeMask testMask(0.0, distance);
  if (testMask.isBelow(tx))
  {
    p1 = osg::Vec3d(tx + tr, ty, 0.0);
    p2 = osg::Vec3d(testMask.finish, ty, 0.0);
  }
  else if (testMask.isAbove(tx))
  {
    p1 = osg::Vec3d(testMask.start, ty, 0.0);
    p2 = osg::Vec3d(tx - tr, ty, 0.0);
  }
  else
    return;

  updateDimLine(dimensionLine1, p1, p2);
}

void LinearDimensionCallback::updateDimLine(osg::Geometry *dimLine, const osg::Vec3d &p1, const osg::Vec3d &p2)
{
  osg::Vec3Array *verts = dynamic_cast<osg::Vec3Array*>(dimLine->getVertexArray());
  assert(verts);
  assert(verts->size() == 2);
  (*verts)[0] = p1;
  (*verts)[1] = p2;
  verts->dirty();
  dimLine->dirtyBound();
}

osg::MatrixTransform* lbr::buildLinearDimension(osg::Node *textObject)
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
  osg::Geometry *extensionLine1 = buildDummyLine(); // base extension line.
  extensionLine1->setName(extensionLine1Name);
  osg::Geometry *extensionLine2 = buildDummyLine(); // moving extension line.
  extensionLine2->setName(extensionLine2Name);
  osg::Geometry *dimensionLine1 = buildDummyLine();
  dimensionLine1->setName(dimensionLine1Name);
  osg::Geometry *dimensionLine2 = buildDummyLine();
  dimensionLine2->setName(dimensionLine2Name);
  
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
  
  //assemble
  rootTransform->addChild(rootSwitch);
  rootSwitch->addChild(textObject);
  rootSwitch->addChild(extensionLine1);
  rootSwitch->addChild(extensionLine2);
  rootSwitch->addChild(dimensionLine1);
  rootSwitch->addChild(dimensionLine2);
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
