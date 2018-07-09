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

#include <QImage>
#include <QGLWidget>
#include <QDir>
#include <QDebug>

#include <osg/Switch>
#include <osg/MatrixTransform>
#include <osg/Texture2D>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/LineWidth>
#include <osgDB/ReadFile>
#include <osg/Depth>
#include <osg/TexEnv>

#include <gesture/gesturenode.h>
#include <modelviz/nodemaskdefs.h>


QString ensureFile(const char *resourceName)
{
  //alright this sucks. getting svg icons onto geode is a pain.
  //I tried something I found on the web using qimage, but looks like ass.
  //the openscenegraph svg reader only takes a filename.
  //so we will write out the resource files to the temp directory
  //so we can pass a file name to the svg reader. I don't like it either.
  
  QString alteredFileName(resourceName);
  alteredFileName.remove(":");
  alteredFileName.replace("/", "_");
  QDir tempDirectory = QDir::temp();
  QString resourceFileName(tempDirectory.absolutePath() + "/" + alteredFileName);
  if (!tempDirectory.exists(alteredFileName))
  {
    QFile resourceFile(resourceName);
    if (!resourceFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
      std::cout << "couldn't resource: " << resourceName << std::endl;
      return QString();
    }
    QByteArray buffer = resourceFile.readAll();
    resourceFile.close();
     
    QFile newFile(resourceFileName);
    if (!newFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
      std::cout << "couldn't open new temp file in gsn::buildIconGeode" << std::endl;
      return QString();
    }
//     std::cout << "newfilename is: " << resourceFileName << std::endl;
    newFile.write(buffer);
    newFile.close();
  }
  
  return resourceFileName;
}

static osg::ref_ptr<osg::Image> readImageFile(const char * resourceName, double radius)
{
  QString resourceFileName = ensureFile(resourceName);
  if (resourceFileName.isEmpty())
    return osg::ref_ptr<osg::Image>();
  
  int size = std::max(static_cast<int>(radius), 16);
  std::ostringstream opString;
  opString << size << 'x' << size;
  osg::ref_ptr<osgDB::Options> options = new osgDB::Options(opString.str());
  osg::ref_ptr<osg::Image> image = osgDB::readImageFile(resourceFileName.toStdString(), options.get());
  return image;
}

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

osg::MatrixTransform* gsn::buildNode(const char *resourceName, unsigned int mask, const NodeCue &nodeCue)
{
  osg::MatrixTransform *mainNode = new osg::MatrixTransform();
  mainNode->setMatrix(osg::Matrixd::identity());
  mainNode->setName("Node");
  mainNode->setNodeMask(mask);

  osg::Switch *aSwitch = new osg::Switch();
  aSwitch->setName("LineIconSwitch");
  mainNode->addChild(aSwitch);
  aSwitch->addChild(buildLineGeometry());
  
  osg::ref_ptr<osg::Image> forImage = readImageFile(resourceName, nodeCue.radius * 2.0);
  if (!forImage.valid())
  {
    std::cout << "Error: forground image is invalid in gsn::buildNode" << std::endl;
    forImage = new osg::Image();
  }

  osg::ref_ptr<osg::Texture2D> forTexture = new osg::Texture2D();
  forTexture->setImage(forImage);
  forTexture->setDataVariance(osg::Object::STATIC);
  forTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
  forTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
  forTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
  forTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
  
  osg::Geometry *geometry = new osg::Geometry();
  geometry->setName("Icon");
  geometry->setDataVariance(osg::Object::STATIC);
  geometry->setVertexArray(nodeCue.vertices.get());
  geometry->setTexCoordArray(0, nodeCue.tCoords0.get());
  geometry->setTexCoordArray(1, nodeCue.tCoords1.get());
  geometry->setNormalArray(nodeCue.normals.get());
  geometry->setNormalBinding(osg::Geometry::BIND_OVERALL);
  geometry->setColorArray(nodeCue.colors.get());
  geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
  geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, 0, nodeCue.vertices->size()));
  geometry->getOrCreateStateSet()->setTextureAttributeAndModes(0, nodeCue.backTexture.get(), osg::StateAttribute::ON);
  geometry->getOrCreateStateSet()->setTextureAttributeAndModes(1, forTexture.get(), osg::StateAttribute::ON);
  geometry->getOrCreateStateSet()->setTextureAttribute(1, nodeCue.texenv);
  geometry->getOrCreateStateSet()->setAttribute(nodeCue.depth.get());
  
  aSwitch->addChild(geometry);

  return mainNode;
}

using namespace gsn;

NodeCue::NodeCue(double radiusIn, int sidesIn) : radius(radiusIn)
{
  tCoords0 = new osg::Vec2Array();
  tCoords1 = new osg::Vec2Array();
  sides = (sidesIn < 4) ? static_cast<int>(radius) : sidesIn;
  double shrinkFactor = 1.41421; //inscribed square to circumscribed square
  osg::Vec3d currentPoint(radius, 0.0, 0.0);
  std::vector<osg::Vec3d> points;
  points.push_back(osg::Vec3d(0.0, 0.0, 0.0)); //center
  float p = 0.499; //cheats texture point to help with perimeter noise.
  tCoords0->push_back(osg::Vec2(0.5, 0.5)); //center point
  tCoords1->push_back(tCoords0->back()); //center point
  osg::Quat rotation(2 * M_PI / static_cast<double>(sides) , osg::Vec3d(0.0, 0.0, 1.0));
  for (int index = 0; index < sides; ++index)
  {
    points.push_back(currentPoint);
    osg::Vec3 tempPointVec(currentPoint);
    tempPointVec.normalize();
    osg::Vec2 tempTextVec(tempPointVec.x(), tempPointVec.y());
    tempTextVec *= p;
    tCoords0->push_back((tempTextVec) + osg::Vec2(0.5, 0.5));
    tCoords1->push_back((tempTextVec * shrinkFactor) + osg::Vec2(0.5, 0.5));
    currentPoint = rotation * currentPoint;
  }
  points.push_back(osg::Vec3d(radius, 0.0, 0.0));
  tCoords0->push_back(tCoords0->at(1));
  tCoords1->push_back(tCoords1->at(1));
  
  vertices = new osg::Vec3Array();
  std::vector<osg::Vec3d>::const_iterator it;
  for (it = points.begin(); it != points.end(); ++it)
    vertices->push_back(*it);

  normals = new osg::Vec3Array();
  normals->push_back(osg::Vec3(0.0, 0.0, 1.0));
  
  colors = new osg::Vec4Array();
  colors->push_back(osg::Vec4(1.0, 1.0, 1.0, 1.0));
  
  osg::ref_ptr<osg::Image> backImage = readImageFile(":/resources/images/iconBackground.svg", radius * 2.0);
  if (!backImage.valid())
  {
    std::cout << "Error: background image is invalid in NodeCue::NodeCue" << std::endl;
    backImage = new osg::Image();
  }
  
  backTexture = new osg::Texture2D();
  backTexture->setImage(backImage);
  backTexture->setDataVariance(osg::Object::STATIC);
  backTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
  backTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
  backTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
  backTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
  
  texenv = new osg::TexEnv;
  texenv->setMode(osg::TexEnv::DECAL);
  
  depth = new osg::Depth;
  depth->setRange(0.003, 1.003);
}

NodeCue::~NodeCue(){}

