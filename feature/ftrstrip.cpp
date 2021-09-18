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

#include <gp_Pnt.hxx>
#include <gp_Pln.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>

#include <osg/AutoTransform>
#include <osgQOpenGL/QFontImplementation>
#include <osgText/Text>
#include <osg/Switch>

#include "subprojects/libzippp/src/libzippp.h"

#include "libreoffice/lboodshack.h"

#include "application/appapplication.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "globalutilities.h"
#include "tools/occtools.h"
#include "tools/featuretools.h"
#include "annex/annseershape.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "feature/ftrshapecheck.h"
#include "feature/ftrnest.h"
#include "feature/ftrpick.h"
#include "library/lbrplabel.h"
#include "project/serial/generated/prjsrlstpsstrip.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrstrip.h"

using boost::uuids::uuid;
using namespace ftr::Strip;
QIcon Feature::icon = QIcon(":/resources/images/constructionStrip.svg");

namespace
{
  TopoDS_Edge makeEdge(const osg::Vec3d &v1, const osg::Vec3d &v2)
  {
    gp_Pnt p1(v1.x(), v1.y(), v1.z());
    gp_Pnt p2(v2.x(), v2.y(), v2.z());
    return BRepBuilderAPI_MakeEdge(p1, p2);
  }
  
  double getGapPref()
  {
    return prf::manager().rootPtr->features().strip().get().gap();
  }
  
  static const std::map<Station, QString> stationStringMap
  {
    {Station::AerialCam, QObject::tr("Aerial Cam")}
    , {Station::Blank, QObject::tr("Blank")}
    , {Station::Cam, QObject::tr("Cam")}
    , {Station::Coin, QObject::tr("Coin")}
    , {Station::Cutoff, QObject::tr("Cutoff")}
    , {Station::Draw, QObject::tr("Draw")}
    , {Station::Flange, QObject::tr("Flange")}
    , {Station::Form, QObject::tr("Form")}
    , {Station::Idle, QObject::tr("Idle")}
    , {Station::Pierce, QObject::tr("Pierce")}
    , {Station::Restrike, QObject::tr("Restrike")}
    , {Station::Tip, QObject::tr("Tip")}
    , {Station::Trim, QObject::tr("Trim")}
  };
  
  osg::ref_ptr<osg::AutoTransform> buildStationLabel(const std::string &sIn)
  {
    //trick lbr::PLabel to build our station labels
    prm::Parameter tempPrm(QObject::tr("Station"), sIn, "No Tag");
    osg::ref_ptr<lbr::PLabel> tempLabel = new lbr::PLabel(&tempPrm);
    tempLabel->setShowName(false);
    tempLabel->setTextColor(osg::Vec4(1.0, 1.0, 1.0, 1.0));
    
    //assuming the plabel has one child that is autotransform.
    osg::ref_ptr<osg::AutoTransform> out = dynamic_cast<osg::AutoTransform*>(tempLabel->getChild(0));
    return out;
  }
}

QString ftr::Strip::getStationString(Station station)
{
  return stationStringMap.at(station);
}

QStringList ftr::Strip::getAllStationStrings()
{
  QStringList out;
  for (const auto &p : stationStringMap)
    out.append(p.second);
  return out;
}

Stations ftr::Strip::getAllStations()
{
  Stations out;
  for (const auto &p : stationStringMap)
    out.push_back(p.first);
  return out;
}

struct Feature::Stow
{
  Feature &feature;
  
  prm::Parameter part{QObject::tr("Part"), ftr::Picks(), PrmTags::part};
  prm::Parameter blank{QObject::tr("Blank"), ftr::Picks(), PrmTags::blank};
  prm::Parameter nest{QObject::tr("Nest"), ftr::Picks(), PrmTags::nest};
  prm::Parameter feedDirection{QObject::tr("Feed Direction"), osg::Vec3d(-1.0, 0.0, 0.0), PrmTags::feedDirection};
  prm::Parameter pitch{QObject::tr("Pitch"), 100.0, PrmTags::pitch};
  prm::Parameter width{QObject::tr("Width"), 100.0, PrmTags::width};
  prm::Parameter widthOffset{QObject::tr("Width Offset"), 10.0, PrmTags::widthOffset};
  prm::Parameter gap{QObject::tr("Gap"), getGapPref(), PrmTags::gap};
  prm::Parameter autoCalc{QObject::tr("Auto Calc"), true, PrmTags::autoCalc};
  prm::Parameter stripHeight{QObject::tr("Strip Height"), 1.0, PrmTags::stripHeight}; //!< used by quote to get travel.
  prm::Observer prmObserver{std::bind(&Feature::setModelDirty, &feature)};
  prm::Observer syncObserver{std::bind(&Stow::prmActiveSync, this)};
  
  ann::SeerShape sShape;
  
  osg::ref_ptr<lbr::PLabel> feedDirectionLabel{new lbr::PLabel(&feedDirection)};
  osg::ref_ptr<lbr::PLabel> pitchLabel{new lbr::PLabel(&pitch)};
  osg::ref_ptr<lbr::PLabel> widthLabel{new lbr::PLabel(&width)};
  osg::ref_ptr<lbr::PLabel> widthOffsetLabel{new lbr::PLabel(&widthOffset)}; //!< centerline of die.
  osg::ref_ptr<lbr::PLabel> gapLabel{new lbr::PLabel(&gap)};
  osg::ref_ptr<lbr::PLabel> autoCalcLabel{new lbr::PLabel(&autoCalc)};
  std::vector<osg::ref_ptr<osg::MatrixTransform>> stationLabels;
  
  Stations stations;
  
  Stow() = delete;
  Stow(Feature &fIn)
  : feature(fIn)
  {
    auto setupObserve = [&](prm::Parameter *param)
    {
      param->connect(prmObserver);
      feature.parameters.push_back(param);
    };
    setupObserve(&part);
    setupObserve(&blank);
    setupObserve(&nest);
    setupObserve(&feedDirection);
    setupObserve(&pitch);
    setupObserve(&width);
    setupObserve(&widthOffset);
    setupObserve(&gap);
    setupObserve(&stripHeight); //output only, so connection dirty is not needed.
    setupObserve(&autoCalc);
    autoCalc.connect(syncObserver);
    
    pitch.setConstraint(prm::Constraint::buildNonZeroPositive());
    width.setConstraint(prm::Constraint::buildNonZeroPositive());
    gap.setConstraint(prm::Constraint::buildNonZeroPositive());
    
    feature.annexes.insert(std::make_pair(ann::Type::SeerShape, &sShape));
    
    feature.overlaySwitch->addChild(feedDirectionLabel.get());
    feature.overlaySwitch->addChild(pitchLabel.get());
    feature.overlaySwitch->addChild(widthLabel.get());
    feature.overlaySwitch->addChild(widthOffsetLabel.get());
    feature.overlaySwitch->addChild(gapLabel.get());
    feature.overlaySwitch->addChild(autoCalcLabel.get());
    
    prmActiveSync();
  }
  
  void prmActiveSync()
  {
    if (autoCalc.getBool())
    {
      feedDirection.setActive(false);
      pitch.setActive(false);
      width.setActive(false);
      widthOffset.setActive(false);
      gap.setActive(false);
    }
    else
    {
      feedDirection.setActive(true);
      pitch.setActive(true);
      width.setActive(true);
      widthOffset.setActive(true);
      gap.setActive(true);
    }
  }
  
  void goAutoCalc(const TopoDS_Shape &sIn, occt::BoundingBox &bbbox)
  {
    double offset = bbbox.getDiagonal() / 2.0;
    osg::Vec3d feed = feedDirection.getVector();
    osg::Vec3d norm = feed * osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(0.0, 0.0, -1.0));
    
    gp_Ax3 orientation(gp_Pnt(0.0, 0.0, 0.0), gu::toOcc(norm), gu::toOcc(feed));
    gp_Pln plane(orientation);
    TopoDS_Face face1 = BRepBuilderAPI_MakeFace(plane, -offset, offset, -offset, offset);
    //   double linear = prf::manager().rootPtr->visual().mesh().linearDeflection();
    //   double angular = prf::manager().rootPtr->visual().mesh().angularDeflection();
    //   BRepMesh_IncrementalMesh(face1, linear, Standard_False, angular, Standard_True);
    //   BRepMesh_IncrementalMesh(sIn, linear, Standard_False, angular, Standard_True);
    
    gp_Trsf t1;
    t1.SetTranslation(gu::toOcc(gu::toOsg(bbbox.getCenter()) + (norm * offset)));
    face1.Location(TopLoc_Location(t1));
    
    TopoDS_Face face2 = face1;
    gp_Trsf t2;
    t2.SetTranslation(gu::toOcc(gu::toOsg(bbbox.getCenter()) + (norm * -offset)));
    face2.Location(TopLoc_Location(t2));
    
    auto getDistance = [](const TopoDS_Shape &s1, const TopoDS_Shape &s2) -> double
    {
      //shape
      double tol = 0.1;
      BRepExtrema_DistShapeShape dc(s1, s2, tol, Extrema_ExtFlag_MIN);
      if (!dc.IsDone())
        throw std::runtime_error("BRepExtrema_DistShapeShape failed");;
      if (dc.NbSolution() < 1)
        throw std::runtime_error("BRepExtrema_DistShapeShape failed");;
      return dc.Value();
      
      
      /* I couldn't get poly to work. Not sure why.
       * I am guessing it had to do with orthogonal planes
       */
      //     gp_Pnt p1, p2;
      //     double distance;
      //     if (BRepExtrema_Poly::Distance(s1, s2, p1, p2, distance))
      //       return distance;
      //     
      //     throw std::runtime_error("BRepExtrema_Poly failed");
    };
    
    double d1 = getDistance(sIn, face1);
    double d2 = getDistance(sIn, face2);
    double widthCalc = 2 * offset - d1 - d2 + 2 * gap.getDouble();
    width.setValue(widthCalc);
    
    osg::Vec3d projection = (norm * (offset - d1)) + (-norm * (offset - d2));
    osg::Vec3d center = gu::toOsg(bbbox.getCenter()) + (projection * 0.5);
    osg::Vec3d aux = feed * (feed * center);
    osg::Vec3d auxProjection = center - aux;
    double directionFactor = ((auxProjection * norm) < 0.0) ? -1.0 : 1.0;
    widthOffset.setValue(auxProjection.length() * directionFactor);
  }
};

Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  name = QObject::tr("Strip");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
}

Feature::~Feature() = default;

const Stations& Feature::getStations() const
{
  return stow->stations;
}

void Feature::setStations(const Stations &sIn)
{
  stow->stations = sIn;
  
  //we need to keep the stations and labels in sync.
  for (const auto &l : stow->stationLabels)
    overlaySwitch->removeChild(l);
  stow->stationLabels.clear();
  
  bool cv = overlaySwitch->getValue(0); //cache switch value for added labels
  for (std::size_t index = 0; index < stow->stations.size(); ++index)
  {
    osg::ref_ptr<osg::MatrixTransform> sl = new osg::MatrixTransform(); // station label
    sl->addChild(buildStationLabel(getStationString(stow->stations.at(index)).toStdString()));
    stow->stationLabels.push_back(sl);
    overlaySwitch->addChild(sl.get(), cv);
  }
  
  setModelDirty();
}

void Feature::updateModel(const UpdatePayload &payloadIn)
{
  setFailure();
  lastUpdateLog.clear();
  stow->sShape.reset();
  try
  {
    prm::ObserverBlocker block(stow->prmObserver);
    
    auto getPickResolve = [&](std::string_view tag) -> tls::Resolver
    {
      const auto &picks = getParameter(tag)->getPicks();
      if (picks.empty())
        throw std::runtime_error(std::string("No ") + std::string(tag) + " Picks");
      tls::Resolver resolver(payloadIn);
      if (!resolver.resolve(picks.front()))
        throw std::runtime_error(std::string("Failed To Resolve ") + std::string(tag) + " Picks");
      if (resolver.getShapes().empty())
        throw std::runtime_error(std::string("No Shapes For ") + std::string(tag) + " Picks");
      return resolver;
    };
    
    tls::Resolver partResolver = getPickResolve(PrmTags::part);
    tls::Resolver blankResolver = getPickResolve(PrmTags::blank);
    tls::Resolver nestResolver = getPickResolve(PrmTags::nest);
    
    TopoDS_Shape ps = partResolver.getShapes().front();
    TopoDS_Shape bs = blankResolver.getShapes().front();
    //we don't need nest shape
    
    occt::BoundingBox bbbox(bs); //blank bounding box.
    
    if (stow->autoCalc.getBool())
    {
      const Nest *nf = dynamic_cast<const Nest *>(nestResolver.getFeature());
      if (!nf)
        throw std::runtime_error("Nest Input Is Not A Nest Feature");
      stow->feedDirection.setValue(nf->getFeedDirection());
      stow->pitch.setValue(nf->getPitch());
      stow->gap.setValue(nf->getGap());
      
      stow->goAutoCalc(bs, bbbox);
    }
    
    if (stow->stations.empty())
      throw std::runtime_error("No Stations Defined");
    
    occt::ShapeVector shapes;
    shapes.push_back(ps); //original part shape.
    shapes.push_back(bs); //original blank shape.
    //find first not blank index
    std::size_t nb = 0;
    for (std::size_t index = 0; index < stow->stations.size(); ++index)
    {
      if (stow->stations.at(index) != Station::Blank)
      {
        nb = index;
        break;
      }
    }
    
    osg::Vec3d lFeed = stow->feedDirection.getVector();
    osg::Vec3d fNorm = lFeed * osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(0.0, 0.0, -1.0));
      
    for (std::size_t i = 1; i < nb + 1; ++i) //blanks
      shapes.push_back(occt::instanceShape(bs, gu::toOcc(-lFeed), stow->pitch.getDouble() * i));
    for (std::size_t i = 1; i < stow->stations.size() - nb; ++i) //parts
      shapes.push_back(occt::instanceShape(ps, gu::toOcc(lFeed), stow->pitch.getDouble() * i));
    
    //add edges representing the incoming strip.
    double edgeLength = nb * stow->pitch.getDouble();
    osg::Vec3d centerLinePoint = fNorm * stow->widthOffset.getDouble();
    centerLinePoint += lFeed * (gu::toOsg(bbbox.getCenter()) * lFeed);
    
    osg::Vec3d backLinePoint = centerLinePoint + (fNorm * stow->width.getDouble() / 2.0);
    osg::Vec3d backLineEnd1 = backLinePoint;
    osg::Vec3d backLineEnd2 = backLinePoint + (-lFeed * edgeLength);
    shapes.push_back(makeEdge(backLineEnd1, backLineEnd2));
    
    osg::Vec3d frontLinePoint = centerLinePoint + (-fNorm * stow->width.getDouble() / 2.0);
    osg::Vec3d frontLineEnd1 = frontLinePoint;
    osg::Vec3d frontLineEnd2 = frontLinePoint + (-lFeed * edgeLength);
    shapes.push_back(makeEdge(frontLineEnd1, frontLineEnd2));
    
    TopoDS_Shape out = static_cast<TopoDS_Compound>(occt::ShapeVectorCast(shapes));
    ShapeCheck check(out);
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    //for now, we are only going to have consistent ids for face and outer wire.
    stow->sShape.setOCCTShape(out, getId());
    stow->sShape.ensureNoNils();
    
    //update label locations
    osg::Vec3d plLoc = //pitch label location.
      gu::toOsg(bbbox.getCenter())
      + (-lFeed * stow->pitch.getDouble() * 0.5)
      + (fNorm * bbbox.getWidth() * 0.5);
    stow->pitchLabel->setMatrix(osg::Matrixd::translate(plLoc));
    
    osg::Vec3d glLoc = //gap label location.
      gu::toOsg(bbbox.getCenter())
      + (-lFeed * stow->pitch.getDouble() * 0.5)
      + (-fNorm * bbbox.getWidth() * 0.5);
    stow->gapLabel->setMatrix(osg::Matrixd::translate(glLoc));
    
    osg::Vec3d wolLoc = //width offset location.
      gu::toOsg(bbbox.getCenter())
      + (-lFeed * (stow->pitch.getDouble() * (static_cast<double>(nb) + 0.5)));
    stow->widthOffsetLabel->setMatrix(osg::Matrixd::translate(wolLoc));
    
    osg::Vec3d wlLoc = //width location.
      gu::toOsg(bbbox.getCenter())
      + (-lFeed * (stow->pitch.getDouble() * (static_cast<double>(nb) + 0.5)))
      + (fNorm * bbbox.getWidth() * 0.5);
    stow->widthLabel->setMatrix(osg::Matrixd::translate(wlLoc));
    
    osg::Vec3d acLoc = gu::toOsg(bbbox.getCenter()); //autocalc label location
    stow->autoCalcLabel->setMatrix(osg::Matrixd::translate(acLoc + osg::Vec3d(0.0, bbbox.getWidth(), 0.0)));
    
    osg::Vec3d fdLoc = gu::toOsg(bbbox.getCenter()) + osg::Vec3d(0.0, -bbbox.getWidth(), 0.0);
    stow->feedDirectionLabel->setMatrix(osg::Matrixd::translate(fdLoc));
    
    for (std::size_t i = 0; i < nb; ++i)
    {
      stow->stationLabels.at(i)->setMatrix(osg::Matrixd::translate(gu::toOsg(bbbox.getCenter()) + (-lFeed * stow->pitch.getDouble() * (i + 1))));
    }
    for (std::size_t i = nb; i < stow->stations.size(); ++i)
    {
      stow->stationLabels.at(i)->setMatrix(osg::Matrixd::translate(gu::toOsg(bbbox.getCenter()) + (lFeed * stow->pitch.getDouble() * (i - nb))));
    }
    
    //boundingbox of whole strip. used for travel estimation.
    occt::BoundingBox sbb(out);
    stow->stripHeight.setValue(sbb.getHeight());
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in strip update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in strip update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in strip update. " << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

/* The serializing of the station labels is FUBAR
 * but I am not going to do anything about it now.
 */
void Feature::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::stps::Strip stripOut
  (
    Base::serialOut()
    , stow->part.serialOut()
    , stow->blank.serialOut()
    , stow->nest.serialOut()
    , stow->feedDirection.serialOut()
    , stow->pitch.serialOut()
    , stow->width.serialOut()
    , stow->widthOffset.serialOut()
    , stow->gap.serialOut()
    , stow->autoCalc.serialOut()
    , stow->stripHeight.serialOut()
    , stow->sShape.serialOut()
    , stow->feedDirectionLabel->serialOut()
    , stow->pitchLabel->serialOut()
    , stow->widthLabel->serialOut()
    , stow->widthOffsetLabel->serialOut()
    , stow->gapLabel->serialOut()
    , stow->autoCalcLabel->serialOut()
  );
  
  assert(stow->stations.size() && stow->stationLabels.size());
  for (std::size_t i = 0; i < stow->stations.size(); ++i)
  {
    auto index = stow->stations.at(i);
    prm::Parameter tempPrm(QObject::tr("Station"), std::string("junk"), "No Tag");
    osg::ref_ptr<lbr::PLabel> tempLabel = new lbr::PLabel(&tempPrm);
    tempLabel->setShowName(false);
    tempLabel->setTextColor(osg::Vec4(1.0, 1.0, 1.0, 1.0));
    tempLabel->setMatrix(stow->stationLabels.at(i)->getMatrix());
    stripOut.stations().push_back(prj::srl::stps::Station(index, tempLabel->serialOut()));
  }
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::stps::strip(stream, stripOut, infoMap);
}

void Feature::serialRead(const prj::srl::stps::Strip &sIn)
{
  Base::serialIn(sIn.base());
  stow->part.serialIn(sIn.part());
  stow->blank.serialIn(sIn.blank());
  stow->nest.serialIn(sIn.nest());
  stow->feedDirection.serialIn(sIn.feedDirection());
  stow->pitch.serialIn(sIn.pitch());
  stow->width.serialIn(sIn.width());
  stow->widthOffset.serialIn(sIn.widthOffset());
  stow->gap.serialIn(sIn.gap());
  stow->autoCalc.serialIn(sIn.autoCalc());
  stow->stripHeight.serialIn(sIn.stripHeight());
  stow->sShape.serialIn(sIn.seerShape());
  stow->feedDirectionLabel->serialIn(sIn.feedDirectionLabel());
  stow->pitchLabel->serialIn(sIn.pitchLabel());
  stow->widthLabel->serialIn(sIn.widthLabel());
  stow->widthOffsetLabel->serialIn(sIn.widthOffsetLabel());
  stow->gapLabel->serialIn(sIn.gapLabel());
  stow->autoCalcLabel->serialIn(sIn.autoCalcLabel());
  
  stow->stations.clear();
  stow->stationLabels.clear();
  for (const auto &stationIn : sIn.stations())
  {
    stow->stations.push_back(static_cast<Station>(stationIn.index()));
    prm::Parameter tempPrm(QObject::tr("Station"), getStationString(stow->stations.back()).toStdString(), "No Tag");
    osg::ref_ptr<lbr::PLabel> tempLabel = new lbr::PLabel(&tempPrm);
    tempLabel->serialIn(stationIn.label());
    tempLabel->setShowName(false);
    tempLabel->setTextColor(osg::Vec4(1.0, 1.0, 1.0, 1.0));
    osg::ref_ptr<osg::MatrixTransform> sl = new osg::MatrixTransform(); // station label
    sl->setMatrix(tempLabel->getMatrix());
    sl->addChild(tempLabel->getChild(0));
    stow->stationLabels.push_back(sl);
    overlaySwitch->addChild(sl.get());
  }
}
