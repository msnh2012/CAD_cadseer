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
#include <algorithm>

#include <osg/BlendFunc>
#include <osgUtil/Optimizer>

#include <library/conebuilder.h>
#include <library/cylinderbuilder.h>
#include <modelviz/nodemaskdefs.h>
#include <modelviz/datumaxis.h>

using namespace mdv;

DatumAxis::DatumAxis() : Base()
{
  setUseDisplayList(false);
  setDataVariance(osg::Object::DYNAMIC);
  setUseVertexBufferObjects(true);
  
  setNodeMask(mdv::datum);
  setName("datumAxis");
  getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
  
  osg::BlendFunc* bf = new osg::BlendFunc(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_SRC_ALPHA); 
  getOrCreateStateSet()->setAttributeAndModes(bf);
  
  color = osg::Vec4(.4f, .7f, .75f, .5f);
  
  setVertexArray(new osg::Vec3Array());
  setNormalArray(new osg::Vec3Array());
  osg::Vec4Array *colorArray = new osg::Vec4Array();
  colorArray->push_back(color);
  setColorArray(colorArray);
  
  setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
  setColorBinding(osg::Geometry::BIND_OVERALL);
  
  buildGeometry();
  setAxisColor(color);
}

DatumAxis::DatumAxis(const DatumAxis &rhs, const osg::CopyOp& copyOperation) :
  Base(rhs, copyOperation)
{
  setUseDisplayList(false);
  setDataVariance(osg::Object::DYNAMIC);
  setUseVertexBufferObjects(true);
}

void DatumAxis::setHeight(double hIn)
{
  height = hIn;
  buildGeometry();
}

void DatumAxis::setAxisColor(const osg::Vec4 &colorIn)
{
  osg::Vec4 tempColor = colorIn;
  tempColor.w() = 0.5f;
  
  osg::Vec4Array *colors = dynamic_cast<osg::Vec4Array*>(getColorArray());
  assert(colors);
  (*colors)[0] = tempColor;
  
  _colorArray->dirty();
}

void DatumAxis::setToColor()
{
  setAxisColor(color);
}

void DatumAxis::setToPreHighlight()
{
  setAxisColor(colorPreHighlight);
}

void DatumAxis::setToHighlight()
{
  setAxisColor(colorHighlight);
}

void DatumAxis::buildGeometry()
{
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
  
  osg::Vec3Array *verts = dynamic_cast<osg::Vec3Array*>(getVertexArray());
  verts->clear();
  osg::Vec3Array *normals = dynamic_cast<osg::Vec3Array*>(getNormalArray());
  normals->clear();
  removePrimitiveSet(0, getNumPrimitiveSets());
  //now we should have an empty geometry.
  
  osgUtil::Optimizer::MergeGeometryVisitor::mergeGeometry(*this, *rawCylinder);
  osgUtil::Optimizer::MergeGeometryVisitor::mergeGeometry(*this, *rawCone);
  
  verts->dirty();
  normals->dirty();
}
