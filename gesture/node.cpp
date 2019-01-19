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
#include <iostream>
#include <cstring> //for memcpy

#include <QSvgRenderer>
#include <QPainter>
#include <QImage>

#include <osg/Switch>
#include <osg/MatrixTransform>
#include <osg/Texture2D>
#include <osg/Geometry>
#include <osg/Depth>

#include <gesture/node.h>
#include <modelviz/nodemaskdefs.h>

#define _USE_MATH_DEFINES
#include <cmath>

//line is now actually a quad
static osg::Geometry* buildLineGeometry()
{
  osg::ref_ptr<osg::Geometry> geometryLine = new osg::Geometry();
  geometryLine->setDataVariance(osg::Geometry::DYNAMIC);
  geometryLine->setName("Line");
  osg::ref_ptr<osg::Vec3Array> points = new osg::Vec3Array();
  points->push_back(osg::Vec3(0.0, -1.0, 0.0));
  points->push_back(osg::Vec3(100.0, -1.0, 0.0));
  points->push_back(osg::Vec3(100.0, 1.0, 0.0));
  points->push_back(osg::Vec3(0.0, 1.0, 0.0));
  geometryLine->setVertexArray(points);

  osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
  colors->push_back(osg::Vec4(128.0/255.0, 128.0/255.0, 128.0/255.0, 1.0));
  geometryLine->setColorArray(colors.get());
  geometryLine->setColorBinding(osg::Geometry::BIND_OVERALL);
  geometryLine->addPrimitiveSet(new osg::DrawArrays(GL_QUADS, 0, 4));

  osg::Depth *depth = new osg::Depth;
  depth->setRange(0.004, 1.004);
  geometryLine->getOrCreateStateSet()->setAttribute(depth);

  return geometryLine.release();
}

namespace gsn
{
  struct NodeBuilder::Stow
  {
    Stow() = delete;
    Stow(double radiusIn, int sidesIn)
    : radius(radiusIn)
    , sides(sidesIn)
    , background(radius * 2.0, radius * 2.0, QImage::Format_RGBA8888)
    {}
    
    /* we pass the rect explicitly instead of using image.rect(), because
     * we use the rect to scale the command icons to fit inside background icon
     */
    static void goPaint(QSvgRenderer &renderer, QImage &image, const QRectF &rect)
    {
      QPainter painter(&image);
      painter.setRenderHint(QPainter::Antialiasing);
      renderer.render(&painter, rect);
    };
    
    osg::ref_ptr<osg::Image> toOsgImage(const QImage &imageIn)
    {
      osg::ref_ptr<osg::Image> out = new osg::Image();
      
      int size = static_cast<int>(radius * 2.0);
      out->allocateImage(size, size, 1, GL_RGBA, GL_UNSIGNED_BYTE);
      assert(imageIn.sizeInBytes() == out->getTotalDataSize());
      if(imageIn.sizeInBytes() != out->getTotalDataSize())
      {
        std::cout << "Error: QImage size doesn't match osg::Image size" << std::endl;
        std::cout << "QImage size: " << imageIn.sizeInBytes() << std::endl;
        std::cout << "osg::Image size: " << out->getTotalDataSize() << std::endl;
        return osg::ref_ptr<osg::Image>();
      }
      std::memcpy(out->data(), imageIn.bits(), imageIn.sizeInBytes());
      
      //don't know why images are flipped and I need the following?
      out->flipVertical();
      
      return out;
    };
    
    double radius;
    int sides;
    osg::ref_ptr<osg::Geometry> geometry;
    osg::ref_ptr<osg::Vec3Array> vertices;
    osg::ref_ptr<osg::Vec3Array> normals; //only 1 bind overall
    osg::ref_ptr<osg::Vec4Array> colors; //only 1 bind overall
    osg::ref_ptr<osg::Texture2D> texture;
    osg::ref_ptr<osg::Vec2Array> tCoords;
    osg::ref_ptr<osg::Depth> depth;
    QImage background;
  };
}

using namespace gsn;

NodeBuilder::NodeBuilder(double radiusIn, int sidesIn)
: stow(std::make_unique<Stow>(radiusIn, sidesIn))
{
  initialize();
}

NodeBuilder::~NodeBuilder() = default;

void NodeBuilder::initialize()
{
  stow->geometry = new osg::Geometry();
  stow->geometry->setName("Icon");
  stow->geometry->setDataVariance(osg::Object::STATIC);
  
  stow->sides = (stow->sides < 4) ? static_cast<int>(stow->radius) : stow->sides;
  osg::Vec3d currentPoint(stow->radius, 0.0, 0.0);
  std::vector<osg::Vec3d> points;
  osg::Vec2Array *tCoords = new osg::Vec2Array();
  points.push_back(osg::Vec3d(0.0, 0.0, 0.0)); //center
  tCoords->push_back(osg::Vec2(0.5, 0.5)); //center point
  float p = 0.498; //cheats texture point to help with perimeter noise.
  osg::Quat rotation(2 * M_PI / static_cast<double>(stow->sides) , osg::Vec3d(0.0, 0.0, 1.0));
  for (int index = 0; index < stow->sides; ++index)
  {
    points.push_back(currentPoint);
    osg::Vec3 tempPointVec(currentPoint);
    tempPointVec.normalize();
    osg::Vec2 tempTextVec(tempPointVec.x(), tempPointVec.y());
    tempTextVec *= p;
    tCoords->push_back((tempTextVec) + osg::Vec2(0.5, 0.5));
    currentPoint = rotation * currentPoint;
  }
  points.push_back(osg::Vec3d(stow->radius, 0.0, 0.0));
  tCoords->push_back(tCoords->at(1));
  
  osg::Vec3Array *vertices = new osg::Vec3Array();
  std::vector<osg::Vec3d>::const_iterator it;
  for (it = points.begin(); it != points.end(); ++it)
    vertices->push_back(*it);
  stow->geometry->setVertexArray(vertices);
  stow->geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, 0, vertices->size()));
  stow->geometry->setTexCoordArray(0, tCoords);

  osg::Vec3Array *normals = new osg::Vec3Array();
  normals->push_back(osg::Vec3(0.0, 0.0, 1.0));
  stow->geometry->setNormalArray(normals);
  stow->geometry->setNormalBinding(osg::Geometry::BIND_OVERALL);
  
  osg::Vec4Array *colors = new osg::Vec4Array();
  colors->push_back(osg::Vec4(1.0, 1.0, 1.0, 1.0));
  stow->geometry->setColorArray(colors);
  stow->geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
  
  osg::Depth *depth = new osg::Depth();
  depth->setRange(0.003, 1.003);
  stow->geometry->getOrCreateStateSet()->setAttribute(depth);
  
  QSvgRenderer svgRenderer(QString(":/resources/images/iconBackground.svg"));
  stow->background.fill(Qt::black);
  if (svgRenderer.isValid())
    stow->goPaint(svgRenderer, stow->background, stow->background.rect());
}

osg::MatrixTransform* NodeBuilder::buildNode(const char *resource, unsigned int mask)
{
  float size = static_cast<float>(stow->radius * 2.0);
  float shrink = size / 1.41421; //inscribed square to circumscribed square
  float offset = (size - shrink) / 2.0;
  
  QImage combinedImage = stow->background; //make a copy to render.
  QSvgRenderer renderer(QString::fromUtf8(resource));
  if (renderer.isValid())
    stow->goPaint(renderer, combinedImage, QRectF(offset, offset, shrink, shrink));
  
  osg::ref_ptr<osg::Image> osgImage = stow->toOsgImage(combinedImage);
  assert(osgImage);
  if (!osgImage)
    return new osg::MatrixTransform();
  
  osg::Texture2D *texture = new osg::Texture2D();
  texture->setImage(osgImage);
  texture->setDataVariance(osg::Object::STATIC);
  texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
  texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
  texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
  texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
  osg::Geometry *aClone = static_cast<osg::Geometry*>(stow->geometry->clone(osg::CopyOp()));
  osg::StateSet *state = static_cast<osg::StateSet*>(aClone->getOrCreateStateSet()->clone(osg::CopyOp()));
  state->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
  aClone->setStateSet(state);
  
  osg::MatrixTransform *mainNode = new osg::MatrixTransform();
  mainNode->setMatrix(osg::Matrixd::identity());
  mainNode->setName("Node");
  mainNode->setNodeMask(mask);

  osg::Switch *aSwitch = new osg::Switch();
  aSwitch->setName("LineIconSwitch");
  mainNode->addChild(aSwitch);
  aSwitch->addChild(buildLineGeometry());
  aSwitch->addChild(aClone);
  
  return mainNode;
}
