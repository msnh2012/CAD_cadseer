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

#include <sstream>
#include <boost/filesystem.hpp>

#include <osg/Switch>
#include <osg/Image>
#include <osgDB/ReadFile>
#include <osgDB/Registry>
#include <osgDB/ReaderWriter>
#include <osg/PositionAttitudeTransform>
#include <osg/MatrixTransform>
#include <osg/Texture2D>

#include "globalutilities.h"
#include "parameter/prmparameter.h"
#include "tools/occtools.h"
#include "feature/ftrupdatepayload.h"
#include "library/lbrcsysdragger.h"
#include "library/lbrplabel.h"
#include "annex/anncsysdragger.h"
#include "project/serial/generated/prjsrlimpsimageplane.h"
#include "feature/ftrimageplane.h"

using namespace ftr;
using boost::uuids::uuid;

QIcon ImagePlane::icon;

ImagePlane::ImagePlane():
Base()
, scale(std::make_unique<prm::Parameter>(prm::Names::Scale, 1.0, prm::Tags::Scale))
, csys(std::make_unique<prm::Parameter>(prm::Names::CSys, osg::Matrixd::identity(), prm::Tags::CSys))
, csysDragger(std::make_unique<ann::CSysDragger>(this, csys.get()))
, transform(new osg::PositionAttitudeTransform())
, scaleLabel(new lbr::PLabel(scale.get()))
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionImagePlane.svg");
  
  name = QObject::tr("ImagePlane");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  mainTransform->addChild(transform.get());
  
  scale->connectValue(std::bind(&ImagePlane::setModelDirty, this));
  scale->setConstraint(prm::Constraint::buildNonZeroPositive());
  parameters.push_back(scale.get());
  
  osg::ref_ptr<osg::MatrixTransform> labelDraggerTransform = new osg::MatrixTransform();
  labelDraggerTransform->addChild(scaleLabel.get());
  overlaySwitch->addChild(labelDraggerTransform.get());
  csysDragger->dragger->linkToMatrix(labelDraggerTransform.get());
  
  //we have to dirty feature or it won't be serialized.
  csys->connectValue(std::bind(&ImagePlane::setModelDirty, this));
  parameters.push_back(csys.get());
  
  annexes.insert(std::make_pair(ann::Type::CSysDragger, csysDragger.get()));
  overlaySwitch->addChild(csysDragger->dragger);
}

ImagePlane::~ImagePlane(){}

std::string ImagePlane::setImage(const boost::filesystem::path &p)
{
  lastUpdateLog.clear();
  
  if (!boost::filesystem::exists(p))
  {
    lastUpdateLog += "Image file not found\n";
    return std::string("Image file not found");
  }

  osg::ref_ptr<osg::Image> image = osgDB::readRefImageFile(p.string());
  if (!image.valid())
  {
    lastUpdateLog += "osg::Image not valid\n";
    return std::string("osg::Image not valid");
  }
  
  float width = 1.0;
  float height = 1.0 * (static_cast<float>(image->t()) / static_cast<float>(image->s()));
  osg::Vec3 corner(width / -2.0f, height / -2.0f, 0.0);
  osg::Vec3 widthVec(width, 0.0, 0.0);
  osg::Vec3 heightVec(0.0, height, 0.0);
  osg::ref_ptr<osg::Geometry> tg = osg::createTexturedQuadGeometry(corner, widthVec, heightVec);
  if (!tg.valid())
  {
    lastUpdateLog += "geometry is not valid\n";
    return "geometry is not valid";
  }
  
  if (geometry.valid())
  {
    transform->removeChild(geometry.get());
    geometry.release();
  }
  
  geometry = tg;
  geometry->getOrCreateStateSet()->setTextureAttributeAndModes(0, new osg::Texture2D(image), osg::StateAttribute::ON);
  transform->addChild(geometry.get());
  
  cornerVec = (widthVec + heightVec) * 0.5;
  
  return std::string();
}

void ImagePlane::setScale(double sIn)
{
  scale->setValue(sIn);
}

double ImagePlane::getScale() const
{
  return static_cast<double>(*scale);
}

void ImagePlane::updateModel(const UpdatePayload &/*pIn*/)
{
  //nothing to do. all model updating happens in setImage.

  setSuccess();
  setModelClean();
}

void ImagePlane::updateVisual()
{
  if (!geometry.valid())
    return;
  
  double s = static_cast<double>(*scale);
  osg::Vec3d sv(s, s, 1.0);
  transform->setScale(sv);
  
  scaleLabel->setMatrix(osg::Matrix::translate(cornerVec * s));

  setVisualClean();
}

void ImagePlane::serialWrite(const boost::filesystem::path &dIn)
{
  osg::ref_ptr<osgDB::Options> options = new osgDB::Options();
  options->setPluginStringData("fileType", "Ascii");
  
  osgDB::ReaderWriter *rw = osgDB::Registry::instance()->getReaderWriterForExtension("osgt");
  assert(rw);
  std::ostringstream nodeStream;
  auto wr = rw->writeNode(*geometry, nodeStream, options.get());
  assert(wr.success());
  
  //debug
//   std::ofstream fs;
//   fs.open("/home/tanderson/temp/test.osgt");
//   rw->writeNode(*geometry, fs, options.get());
  
  prj::srl::imps::ImagePlane so
  (
    Base::serialOut()
    , scale->serialOut()
    , csys->serialOut()
    , csysDragger->serialOut()
    , scaleLabel->serialOut()
    , prj::srl::spt::Vec3d(cornerVec.x(), cornerVec.y(), cornerVec.z())
    , nodeStream.str()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::imps::imageplane(stream, so, infoMap);
}

void ImagePlane::serialRead(const prj::srl::imps::ImagePlane &so)
{
  Base::serialIn(so.base());
  scale->serialIn(so.scale());
  csys->serialIn(so.csys());
  csysDragger->serialIn(so.csysDragger());
  scaleLabel->serialIn(so.scaleLabel());
  cornerVec = osg::Vec3d(so.cornerVec().x(), so.cornerVec().y(), so.cornerVec().z());
  
  osgDB::ReaderWriter *rw = osgDB::Registry::instance()->getReaderWriterForExtension("osgt");
  assert(rw);
  std::istringstream nodeStream(so.geometry());
  auto rr = rw->readNode(nodeStream);
  assert(rr.success());
  geometry = dynamic_cast<osg::Geometry*>(rr.getNode());
  assert(geometry.valid());
  transform->addChild(geometry.get());
  
  mainTransform->setMatrix(static_cast<osg::Matrix>(*csys));
}
