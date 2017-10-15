/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#include <boost/filesystem.hpp>

#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepExtrema_Poly.hxx>
#include <BRepMesh_IncrementalMesh.hxx>

#include <osg/AutoTransform>
#include <osgQt/QFontImplementation>
#include <osgText/Text>

#include <libzippp.h>
#include <libreoffice/odshack.h>

#include <application/application.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <globalutilities.h>
#include <tools/occtools.h>
#include <feature/seershape.h>
#include <feature/shapecheck.h>
#include <feature/nest.h>
#include <project/serial/xsdcxxoutput/featurestrip.h>
#include <feature/strip.h>

using namespace ftr;

using boost::uuids::uuid;

QIcon Strip::icon;

Strip::Strip() : Base()
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionStrip.svg");
  
  name = QObject::tr("Strip");
  mainSwitch->setUserValue(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  feedDirection = osg::Vec3d(-1.0, 0.0, 0.0);
  
  pitch = std::shared_ptr<Parameter>(new Parameter(QObject::tr("Pitch"), 1.0));
  pitch->setConstraint(ParameterConstraint::buildNonZeroPositive());
  pitch->connectValue(boost::bind(&Strip::setModelDirty, this));
  parameterVector.push_back(pitch.get());
  
  width = std::shared_ptr<Parameter>(new Parameter(QObject::tr("Width"), 1.0));
  width->setConstraint(ParameterConstraint::buildNonZeroPositive());
  width->connectValue(boost::bind(&Strip::setModelDirty, this));
  parameterVector.push_back(width.get());
  
  widthOffset = std::shared_ptr<Parameter>(new Parameter(QObject::tr("Width Offset"), 1.0));
  widthOffset->setConstraint(ParameterConstraint::buildAll());
  widthOffset->connectValue(boost::bind(&Strip::setModelDirty, this));
  parameterVector.push_back(widthOffset.get());
  
  gap = std::shared_ptr<Parameter>
  (
    new Parameter
    (
      QObject::tr("Gap"),
      prf::manager().rootPtr->features().strip().get().gap()
    )
  );
  gap->setConstraint(ParameterConstraint::buildNonZeroPositive());
  gap->connectValue(boost::bind(&Strip::setModelDirty, this));
  parameterVector.push_back(gap.get());
  
  pitchLabel = new lbr::PLabel(pitch.get());
  pitchLabel->showName = true;
  pitchLabel->valueHasChanged();
  overlaySwitch->addChild(pitchLabel.get());
  
  widthLabel = new lbr::PLabel(width.get());
  widthLabel->showName = true;
  widthLabel->valueHasChanged();
  overlaySwitch->addChild(widthLabel.get());
  
  widthOffsetLabel = new lbr::PLabel(widthOffset.get());
  widthOffsetLabel->showName = true;
  widthOffsetLabel->valueHasChanged();
  overlaySwitch->addChild(widthOffsetLabel.get());
  
  gapLabel = new lbr::PLabel(gap.get());
  gapLabel->showName = true;
  gapLabel->valueHasChanged();
  overlaySwitch->addChild(gapLabel.get());
  
  stations.push_back("Blank");
  stations.push_back("Blank");
  stations.push_back("Blank");
  stations.push_back("Form");
}

void Strip::setLabelColors(const osg::Vec4 &cIn)
{
  pitchLabel->setTextColor(cIn);
  widthLabel->setTextColor(cIn);
  widthOffsetLabel->setTextColor(cIn);
  gapLabel->setTextColor(cIn);
}

//copied from lbr::PLabel::build
osg::Node* buildStationLabel(const std::string &sIn)
{
  const prf::InteractiveParameter& iPref = prf::manager().rootPtr->interactiveParameter();
  
  osg::AutoTransform *autoTransform = new osg::AutoTransform();
  autoTransform->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
  autoTransform->setAutoRotateMode(osg::AutoTransform::ROTATE_TO_SCREEN);
  autoTransform->setAutoScaleToScreen(true);
  
  osg::MatrixTransform *textScale = new osg::MatrixTransform();
  textScale->setMatrix(osg::Matrixd::scale(75.0, 75.0, 75.0));
  autoTransform->addChild(textScale);
  
  osgText::Text *text = new osgText::Text();
  text->setName(sIn);
  text->setText(sIn);
  osg::ref_ptr<osgQt::QFontImplementation> fontImplement(new osgQt::QFontImplementation(qApp->font()));
  osg::ref_ptr<osgText::Font> textFont(new osgText::Font(fontImplement.get()));
  text->setFont(textFont.get());
  text->setColor(osg::Vec4(1.0, 1.0, 1.0, 1.0));
  text->setBackdropType(osgText::Text::OUTLINE);
  text->setBackdropColor(osg::Vec4(0.0, 0.0, 0.0, 1.0));
  text->setCharacterSize(iPref.characterSize());
  text->setAlignment(osgText::Text::CENTER_CENTER);
  textScale->addChild(text);
  
  return autoTransform;
}

void Strip::updateModel(const UpdatePayload &payloadIn)
{
  setFailure();
  try
  {
    if (payloadIn.updateMap.count(part) != 1)
      throw std::runtime_error("couldn't find 'part' input");
    const ftr::Base *pbf = payloadIn.updateMap.equal_range(part).first->second;
    if(!pbf->hasSeerShape())
      throw std::runtime_error("no seer shape for part");
    const SeerShape &pss = pbf->getSeerShape(); //part seer shape.
    const TopoDS_Shape &ps = pss.getRootOCCTShape(); //part shape.
      
    if (payloadIn.updateMap.count(blank) != 1)
      throw std::runtime_error("couldn't find 'blank' input");
    const ftr::Base *bbf = payloadIn.updateMap.equal_range(blank).first->second;
    if(!bbf->hasSeerShape())
      throw std::runtime_error("no seer shape for blank");
    const SeerShape &bss = bbf->getSeerShape(); //blank seer shape.
    const TopoDS_Shape &bs = bss.getRootOCCTShape(); //blank shape.
      
    if (payloadIn.updateMap.count(nest) != 1)
      throw std::runtime_error("couldn't find 'nest' input");
    const ftr::Base *nbf = payloadIn.updateMap.equal_range(nest).first->second;
    if(!nbf->hasSeerShape())
      throw std::runtime_error("no seer shape for nest");
    const SeerShape &nss = nbf->getSeerShape(); //nest seer shape.
    const TopoDS_Shape ns = nss.getRootOCCTShape(); //nest shape.
    
    occt::BoundingBox bbbox(bs); //blank bounding box.
    
    if (autoCalc)
    {
      setLabelColors(osg::Vec4(1.0, 0.0, 0.0, 1.0));
      
      const Nest *nf = dynamic_cast<const Nest *>(nbf);
      assert(nf);
      if (!nf)
        throw std::runtime_error("Bad cast to Nest");
      pitch->setValueQuiet(nf->getPitch());
      pitchLabel->valueHasChanged();
      gap->setValueQuiet(nf->getGap());
      gapLabel->valueHasChanged();
      
      //these assume x feed direction. fix when we change feed direction to a parameter.
      width->setValueQuiet(bbbox.getWidth() + 2 * gap->getValue());
      widthLabel->valueHasChanged();
      widthOffset->setValueQuiet(bbbox.getCenter().X());
      widthOffsetLabel->valueHasChanged();
    }
    else
    {
      setLabelColors(osg::Vec4(0.0, 0.0, 1.0, 1.0));
    }
    
    occt::ShapeVector shapes;
    shapes.push_back(ps); //original part shape.
    shapes.push_back(bs); //original blank shape.
    //find first not blank index
    std::size_t nb = 0;
    for (std::size_t index = 0; index < stations.size(); ++index)
    {
      if (stations.at(index) != "Blank")
      {
        nb = index;
        break;
      }
    }
      
    for (std::size_t i = 1; i < nb + 1; ++i)
      shapes.push_back(occt::instanceShape(bs, gu::toOcc(-feedDirection), pitch->getValue() * i));
    for (std::size_t i = 1; i < stations.size() - nb; ++i)
      shapes.push_back(occt::instanceShape(ps, gu::toOcc(feedDirection), pitch->getValue() * i));
    
    TopoDS_Shape out = static_cast<TopoDS_Compound>(occt::ShapeVectorCast(shapes));
    
    ShapeCheck check(out);
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    //for now, we are only going to have consistent ids for face and outer wire.
    seerShape->setOCCTShape(out);
    seerShape->ensureNoNils();
    
    //update label locations
    osg::Vec3d fNorm = feedDirection * osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(0.0, 0.0, -1.0));
    
    osg::Vec3d plLoc = //pitch label location.
      gu::toOsg(bbbox.getCenter())
      + (-feedDirection * pitch->getValue() * 0.5)
      + (fNorm * bbbox.getWidth() * 0.5);
    pitchLabel->setMatrix(osg::Matrixd::translate(plLoc));
    
    osg::Vec3d glLoc = //gap label location.
      gu::toOsg(bbbox.getCenter())
      + (-feedDirection * pitch->getValue() * 0.5)
      + (-fNorm * bbbox.getWidth() * 0.5);
    gapLabel->setMatrix(osg::Matrixd::translate(glLoc));
    
    osg::Vec3d wolLoc = //width offset location.
      gu::toOsg(bbbox.getCenter())
      + (-feedDirection * (pitch->getValue() * (static_cast<double>(nb) + 0.5)));
    widthOffsetLabel->setMatrix(osg::Matrixd::translate(wolLoc));
    
    osg::Vec3d wlLoc = //width location.
      gu::toOsg(bbbox.getCenter())
      + (-feedDirection * (pitch->getValue() * (static_cast<double>(nb) + 0.5)))
      + (fNorm * bbbox.getWidth() * 0.5);
    widthLabel->setMatrix(osg::Matrixd::translate(wlLoc));
    
    for (const auto &l : stationLabels)
    {
      for (const auto &pg : l->getParents()) //parent group
        pg->removeChild(l.get());
    }
    stationLabels.clear();
    bool cv = overlaySwitch->getValue(0); //overlaySwitch always has at least four children.
    for (std::size_t i = 1; i < nb + 1; ++i)
    {
      osg::ref_ptr<osg::MatrixTransform> sl = new osg::MatrixTransform(); // station label
      sl->setMatrix(osg::Matrixd::translate(gu::toOsg(bbbox.getCenter()) + (-feedDirection * pitch->getValue() * i)));
      sl->addChild(buildStationLabel("Blank"));
      stationLabels.push_back(sl);
      overlaySwitch->addChild(sl.get(), cv);
    }
    for (std::size_t i = nb; i < stations.size(); ++i)
    {
      osg::ref_ptr<osg::MatrixTransform> sl = new osg::MatrixTransform(); // station label
      sl->setMatrix(osg::Matrixd::translate(gu::toOsg(bbbox.getCenter()) + (feedDirection * pitch->getValue() * (i - nb))));
      sl->addChild(buildStationLabel(stations.at(i).toStdString()));
      stationLabels.push_back(sl);
      overlaySwitch->addChild(sl.get(), cv);
    }
    
    //boundingbox of whole strip. used for travel estimation.
    occt::BoundingBox sbb(out);
    stripHeight = sbb.getHeight();
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::cout << std::endl << "Error in strip update. " << e.GetMessageString() << std::endl;
  }
  catch (std::exception &e)
  {
    std::cout << std::endl << "Error in strip update. " << e.what() << std::endl;
  }
  catch (...)
  {
    std::cout << std::endl << "Unknown error in strip update. " << std::endl;
  }
  setModelClean();
}

void Strip::serialWrite(const QDir &dIn)
{
  prj::srl::FeatureStrip::StationsType so;
  for (const auto &s : stations)
    so.array().push_back(s.toStdString());
  
  prj::srl::FeatureStrip stripOut
  (
    Base::serialOut(),
    pitch->serialOut(),
    width->serialOut(),
    widthOffset->serialOut(),
    gap->serialOut(),
    autoCalc,
    stripHeight,
    so
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::strip(stream, stripOut, infoMap);
}

void Strip::serialRead(const prj::srl::FeatureStrip &sIn)
{
  Base::serialIn(sIn.featureBase());
  pitch->serialIn(sIn.pitch());
  width->serialIn(sIn.width());
  widthOffset->serialIn(sIn.widthOffset());
  gap->serialIn(sIn.gap());
  autoCalc = sIn.autoCalc();
  stripHeight = sIn.stripHeight();
  stations.clear();
  for (const auto &stringIn : sIn.stations().array())
    stations.push_back(QString::fromStdString(stringIn));
}
