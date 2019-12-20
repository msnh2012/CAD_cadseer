/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2019 Thomas S. Anderson blobfish.at.gmx.com
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

#include <osg/BlendFunc>
#include <osgUtil/Optimizer>

#include "library/lbrconebuilder.h"
#include "library/lbrcylinderbuilder.h"
#include "library/lbrspherebuilder.h"
#include "modelviz/mdvnodemaskdefs.h"
#include "modelviz/mdvdatumsystem.h"

using namespace mdv;

osg::ref_ptr<osg::Geometry> buildArrow()
{
  osg::ref_ptr<osg::Geometry> out = new osg::Geometry();
  out->setVertexArray(new osg::Vec3Array());
  out->setColorArray(new osg::Vec4Array());
  
  //going to build at static size and use a transform node to scale.
  double height = 1.0;
  double arrowHeight = height * 0.2;
  double arrowDiameter = arrowHeight * 0.75;
  double cylinderDiameter = arrowDiameter * 0.2;
  double cylinderHeight = height - arrowHeight;
  int isoLines = std::min((std::max(static_cast<int>(height / 100.0), 1) * 16), 512);
  
  lbr::CylinderBuilder cylinderBuilder;
  cylinderBuilder.setIsoLines(isoLines);
  cylinderBuilder.setRadius(cylinderDiameter / 2.0);
  cylinderBuilder.setHeight(cylinderHeight);
  osg::ref_ptr<osg::Geometry> rawCylinder(cylinderBuilder);
  
  lbr::ConeBuilder coneBuilder;
  coneBuilder.setIsoLines(isoLines);
  coneBuilder.setRadius(arrowDiameter / 2.0);
  coneBuilder.setHeight(arrowHeight);
  osg::ref_ptr<osg::Geometry> rawCone(coneBuilder);
  
  //move the cone to end of cylinder.
  osg::Vec3 project(0.0, 0.0, cylinderHeight);
  osg::Vec3Array *coneVerts = dynamic_cast<osg::Vec3Array*>(rawCone->getVertexArray());
  assert(coneVerts);
  for (std::size_t i = 0; i < coneVerts->size(); ++i)
    (*coneVerts)[i] += project;
  
  osgUtil::Optimizer::MergeGeometryVisitor::mergeGeometry(*out, *rawCylinder);
  osgUtil::Optimizer::MergeGeometryVisitor::mergeGeometry(*out, *rawCone);
  
  return out;
}

DatumSystem::DatumSystem() : Base()
{
  setUseDisplayList(false);
  setDataVariance(osg::Object::DYNAMIC);
  setUseVertexBufferObjects(true);
  
  setNodeMask(mdv::datum);
  setName("datumSystem");
  getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
  
  osg::BlendFunc* bf = new osg::BlendFunc(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_SRC_ALPHA); 
  getOrCreateStateSet()->setAttributeAndModes(bf);
  
  setVertexArray(new osg::Vec3Array());
  setNormalArray(new osg::Vec3Array());
  setColorArray(new osg::Vec4Array());
  
  setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
  setColorBinding(osg::Geometry::BIND_PER_VERTEX);
  
  getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);
  
  buildGeometry();
  setToColor();
}

DatumSystem::DatumSystem(const DatumSystem &rhs, const osg::CopyOp& copyOperation) :
  Base(rhs, copyOperation)
{
  setUseDisplayList(false);
  setDataVariance(osg::Object::DYNAMIC);
  setUseVertexBufferObjects(true);
}

void DatumSystem::setToColor()
{
  static const std::vector<osg::Vec4f> axisColors
  {
    osg::Vec4f(1.0, 0.0, 0.0, 0.5) //red
    , osg::Vec4f(0.0, 1.0, 0.0, 0.5) //green
    , osg::Vec4f(0.0, 0.0, 1.0, 0.5) //blue
  };
  
  osg::Vec4Array *colors = dynamic_cast<osg::Vec4Array*>(getColorArray());
  assert(colors->size() >= 3 * axisOffset);
  for (std::size_t index = 0; index < 3 * axisOffset; ++index)
  {
    std::size_t axisIndex = index / axisOffset;
    assert(axisIndex <= 2);
    if (axisIndex > 2)
      continue;
    (*colors)[index] = axisColors[axisIndex];
  }
  
  for (std::size_t index = 3 * axisOffset; index < colors->size(); ++index)
    (*colors)[index] = osg::Vec4(0.7, 0.5, 0.8, 1.0);
  
  colors->dirty();
}

void DatumSystem::setToPreHighlight()
{
  osg::Vec4Array *colors = dynamic_cast<osg::Vec4Array*>(getColorArray());
  for (std::size_t index = 0; index < colors->size(); ++index)
    (*colors)[index] = colorPreHighlight;
  
  colors->dirty();
}

void DatumSystem::setToHighlight()
{
  osg::Vec4Array *colors = dynamic_cast<osg::Vec4Array*>(getColorArray());
  for (std::size_t index = 0; index < colors->size(); ++index)
    (*colors)[index] = colorHighlight;
  
  colors->dirty();
}

void DatumSystem::buildGeometry()
{
  //going to build at static size and use a transform node to scale.
  double height = 1.0;
  double arrowHeight = height * 0.2;
  double arrowDiameter = arrowHeight * 0.75;
  double cylinderDiameter = arrowDiameter * 0.2;
  double cylinderHeight = height - arrowHeight;
  int isoLines = std::min((std::max(static_cast<int>(height / 100.0), 1) * 16), 512);
  
  lbr::CylinderBuilder cylinderBuilder;
  cylinderBuilder.setIsoLines(isoLines);
  cylinderBuilder.setRadius(cylinderDiameter / 2.0);
  cylinderBuilder.setHeight(cylinderHeight);
  osg::ref_ptr<osg::Geometry> rawCylinder(cylinderBuilder);
  
  lbr::ConeBuilder coneBuilder;
  coneBuilder.setIsoLines(isoLines);
  coneBuilder.setRadius(arrowDiameter / 2.0);
  coneBuilder.setHeight(arrowHeight);
  osg::ref_ptr<osg::Geometry> rawCone(coneBuilder);
  
  //move the cone to end of cylinder.
  osg::Vec3 project(0.0, 0.0, cylinderHeight);
  osg::Vec3Array *coneVerts = dynamic_cast<osg::Vec3Array*>(rawCone->getVertexArray());
  assert(coneVerts);
  for (std::size_t i = 0; i < coneVerts->size(); ++i)
    (*coneVerts)[i] += project;
  
  //z axis
  osg::ref_ptr<osg::Geometry> builderZ = new osg::Geometry();
  osgUtil::Optimizer::MergeGeometryVisitor::mergeGeometry(*builderZ, *rawCylinder);
  osgUtil::Optimizer::MergeGeometryVisitor::mergeGeometry(*builderZ, *rawCone);
  axisOffset = dynamic_cast<osg::Vec3Array*>(builderZ->getVertexArray())->size();
  builderZ->setColorArray(new osg::Vec4Array(axisOffset));
  
  auto rotateGeometry = [](osg::Geometry *geometry, const osg::Matrixd &rotation)
  {
    osg::Vec3Array *vertices = dynamic_cast<osg::Vec3Array*>(geometry->getVertexArray());
    osg::Vec3Array *normals = dynamic_cast<osg::Vec3Array*>(geometry->getNormalArray());
    osg::Vec4Array *colors = dynamic_cast<osg::Vec4Array*>(geometry->getColorArray());
    assert(vertices->size() == normals->size() && normals->size() == colors->size());
    for (std::size_t index = 0; index < vertices->size(); ++index)
    {
      (*vertices)[index] = (*vertices)[index] * rotation;
      (*normals)[index] = (*normals)[index] * rotation;
    }
  };
  
  osg::ref_ptr<osg::Geometry> builderY = new osg::Geometry(*builderZ, osg::CopyOp::DEEP_COPY_ALL);
  rotateGeometry(builderY.get(), osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(-1.0, 0.0, 0.0)));
  osg::ref_ptr<osg::Geometry> builderX = new osg::Geometry(*builderZ, osg::CopyOp::DEEP_COPY_ALL);
  rotateGeometry(builderX.get(), osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(0.0, 1.0, 0.0)));
  
  osgUtil::Optimizer::MergeGeometryVisitor::mergeGeometry(*this, *builderX);
  osgUtil::Optimizer::MergeGeometryVisitor::mergeGeometry(*this, *builderY);
  osgUtil::Optimizer::MergeGeometryVisitor::mergeGeometry(*this, *builderZ);
  //add sphere.
  
  lbr::SphereBuilder sBuilder;
  sBuilder.setRadius(0.05);
  sBuilder.setIsoLines(16);
  osg::ref_ptr<osg::Geometry> sGeometry(sBuilder);
  sGeometry->setColorArray(new osg::Vec4Array(dynamic_cast<osg::Vec3Array*>(sGeometry->getVertexArray())->size()));
  osgUtil::Optimizer::MergeGeometryVisitor::mergeGeometry(*this, *sGeometry);
}
