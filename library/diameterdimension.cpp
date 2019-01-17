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

#include <boost/type_traits/is_volatile.hpp>
#include <boost/type_traits/is_assignable.hpp>
#include <boost/optional/optional.hpp>

#include <osg/MatrixTransform>
#include <osg/Switch>
#include <osg/LOD>
#include <osg/AutoTransform>
#include <osg/PositionAttitudeTransform>
#include <osg/Geometry>
#include <osg/Material>
#include <osg/ComputeBoundsVisitor>

#include "preferences/preferencesXML.h"
#include "preferences/manager.h"
#include "library/childnamevisitor.h"
#include "library/geometrylibrary.h"
#include "library/circlebuilderlod.h"
#include "library/diameterdimension.h"

using namespace lbr;

static const char *rootTransformName = "lbr::DiameterDimension";
static const char *rootSwitchName = "lbr::DiameterDimension::Switch";
static const char *extensionArcName = "lbr::DiameterDimension::Extension";
static const char *extensionArcTransformName = "lbr::DiameterDimension::ExtensionTransform";
static const char *dimensionLineName = "lbr::DiameterDimension::Dimension";
static const char *arrowTransformName = "lbr::DiameterDimension::arrowTransform";
static const char *arrowShapeName = "lbr::DiameterDimension::arrowShape";

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

DiameterDimensionCallback::DiameterDimensionCallback(const char *toi):
osg::NodeCallback()
, textObjectName(toi)
{
}

DiameterDimensionCallback::DiameterDimensionCallback(const std::string &toi):
osg::NodeCallback()
, textObjectName(toi)
{
}

/*! @brief set the 1st range mask
 * 
 * @parameter rmIn new value for range mask in radians.
 * @note range will be in [0, 2PI]. 
 */
void DiameterDimensionCallback::setRangeMask(const RangeMask &rmIn)
{
  if (!(rangeMask == rmIn))
  {
    rangeMask = rmIn;
    dirty = true;
  }
}

void DiameterDimensionCallback::setDiameter(double dIn)
{
  if (diameter != dIn)
  {
    diameter = dIn;
    dirty = true;
  }
}

//! @brief set new location for text in local space. Only for update.
void DiameterDimensionCallback::setTextLocation(const osg::Vec3d &nl)
{
  assert(textObject);
  textObject->setMatrix(osg::Matrixd::translate(nl));
  //shouldn't need to set dirty, as bounding box of text should trigger update.
}

void DiameterDimensionCallback::operator()(osg::Node *node, osg::NodeVisitor *visitor)
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

void DiameterDimensionCallback::init()
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
  
  ChildNameVisitor eatv(extensionArcTransformName);
  rootSwitch->accept(eatv);
  extensionArcTransform = eatv.castResult<osg::PositionAttitudeTransform>();
  if (!extensionArcTransform) return;
  
  ChildNameVisitor eav(extensionArcName);
  rootSwitch->accept(eav);
  extensionArc = eav.castResult<osg::Geometry>();
  if (!extensionArc) return;
  
  ChildNameVisitor dlv(dimensionLineName);
  rootSwitch->accept(dlv);
  dimensionLine = dlv.castResult<osg::Geometry>();
  if (!dimensionLine) return;
  
  ChildNameVisitor atv(arrowTransformName);
  rootSwitch->accept(atv);
  arrowTransform = atv.castResult<osg::AutoTransform>();
  if (!arrowTransform) return;
  
  isValid = true;
}

void DiameterDimensionCallback::updateLayout()
{
  if (diameter <= 0.0)
    return;
  osg::Vec3d textPosition = textObject->getMatrix().getTrans();
  if (textPosition.length() < std::numeric_limits<float>::epsilon())
    return;
  
  osg::Vec3d projection = textPosition;
  projection.normalize();
  
  osg::Vec3d arrowPoint = projection * (diameter / 2.0);
  arrowTransform->setPosition(arrowPoint);
  
  osg::Quat rotation;
  rotation.makeRotate(osg::Vec3d(1.0, 0.0, 0.0), projection); //text outside of arc/circle
  rotation *= osg::Quat(osg::PI, osg::Vec3d(0.0, 0.0, 1.0));
  double offset = -cachedTextBound.radius();
  if (textPosition.length() < (diameter / 2.0)) //text inside of arc/circle
  {
    rotation.makeRotate(osg::Vec3d(1.0, 0.0, 0.0), projection);
    offset = cachedTextBound.radius();
  }
  arrowTransform->setRotation(rotation);
  
  osg::Vec3Array *verts = dynamic_cast<osg::Vec3Array*>(dimensionLine->getVertexArray());
  assert(verts);
  assert(verts->size() == 2);
  (*verts)[0] = arrowPoint;
  (*verts)[1] = textPosition + projection * offset;
  verts->dirty();
  dimensionLine->dirtyBound();
  
  if (rangeMask.isValid())
  {
    //assume normal range
    double textAngle = vectorAngle(osg::Vec3d(1.0, 0.0, 0.0), projection);
    if (rangeMask.isIn(textAngle))
    {
      rootSwitch->setChildValue(extensionArcTransform, false);
    }
    else
    {
      //we are always above because we rotate the dimension to align to start of arc
      extensionArcTransform->removeChild(0, 1);
      
      osg::Quat rotate;
      if ((osg::PI * 2 - textAngle) < (textAngle - rangeMask.finish))
      {
        //we are closer to start. go counter clockwise.
        double span = std::max(osg::PI * 2.0 - textAngle, static_cast<double>(std::numeric_limits<float>::epsilon()));
        CircleBuilderLOD b(diameter / 2.0, span);
        extensionArcTransform->addChild(static_cast<osg::LOD*>(b));
        rotate.makeRotate(textAngle, osg::Vec3d(0.0, 0.0, 1.0));
      }
      else
      {
        //we are closer to end.
        double span = std::max(textAngle - rangeMask.finish, static_cast<double>(std::numeric_limits<float>::epsilon()));
        CircleBuilderLOD b(diameter / 2.0, span);
        extensionArcTransform->addChild(static_cast<osg::LOD*>(b));
        rotate.makeRotate(rangeMask.finish, osg::Vec3d(0.0, 0.0, 1.0));
      }
      
      extensionArcTransform->setAttitude(rotate);
      rootSwitch->setChildValue(extensionArcTransform, true);
    }
  }
}

osg::MatrixTransform* lbr::buildDiameterDimension(osg::MatrixTransform *textObject)
{
  osg::MatrixTransform* rootTransform = new osg::MatrixTransform();
  rootTransform->setName(rootTransformName);
  osg::Switch *rootSwitch = new osg::Switch();
  rootSwitch->setName(rootSwitchName);
  
  osg::Geometry *extensionArc = new osg::Geometry(); //will be replace by a circle lod.
  extensionArc->setName(extensionArcName);
  
  osg::PositionAttitudeTransform *extensionArcTransform = new osg::PositionAttitudeTransform();
  extensionArcTransform->setName(extensionArcTransformName);
  
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
  osg::Geometry *dimensionLine = buildDummyLine();
  dimensionLine->setName(dimensionLineName);
  
  osg::AutoTransform *arrowTransform = new osg::AutoTransform();
  arrowTransform->setName(arrowTransformName);
  arrowTransform->setAutoScaleToScreen(true);
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
  rootSwitch->addChild(extensionArcTransform);
  extensionArcTransform->addChild(extensionArc);
  rootSwitch->addChild(dimensionLine);
  rootSwitch->addChild(arrowTransform);
  arrowTransform->addChild(arrowShape);
  arrowShape->addChild(arrow);
  
  osg::Material *material = new osg::Material();
  material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0, 1.0, 0.0, 1.0));
  material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0, 1.0, 0.0, 1.0));
  rootTransform->getOrCreateStateSet()->setAttributeAndModes(material);
  rootTransform->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
  
  return rootTransform;
}
