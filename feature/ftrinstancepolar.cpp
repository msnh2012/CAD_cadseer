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

#include <boost/filesystem/path.hpp>

#include <gp_Cylinder.hxx>
#include <gp_Lin.hxx>
#include <Geom_Line.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <TopoDS.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_Transform.hxx>

#include "globalutilities.h"
#include "tools/occtools.h"
#include "library/lbrplabel.h"
#include "library/lbrcsysdragger.h"
#include "annex/annseershape.h"
#include "annex/anncsysdragger.h"
#include "annex/anninstancemapper.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "project/serial/generated/prjsrlinpsinstancepolar.h"
#include "tools/featuretools.h"
#include "feature/ftrshapecheck.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinstancepolar.h"

using namespace ftr::InstancePolar;
using boost::uuids::uuid;
QIcon Feature::icon;

struct Feature::Stow
{
  Feature &feature;
  
  prm::Parameter axisType{QObject::tr("Axis Type"), 0, Tags::AxisType};
  prm::Parameter csys{prm::Names::CSys, osg::Matrixd::identity(), prm::Tags::CSys};
  prm::Parameter count{QObject::tr("Count"), 3, Tags::Count};
  prm::Parameter angle{prm::Names::Angle, 20.0, prm::Tags::Angle};
  prm::Parameter inclusiveAngle{QObject::tr("Inclusive Angle"), false, Tags::InclusiveAngle};
  prm::Parameter includeSource{QObject::tr("Include Source"), true, Tags::IncludeSource};
  prm::Parameter source{QObject::tr("Source"), ftr::Picks(), Tags::Source};
  prm::Parameter axis{QObject::tr("Axis"), ftr::Picks(), Tags::Axis};
  
  prm::Observer prmObserver;
  
  ann::SeerShape sShape;
  ann::InstanceMapper iMapper;
  ann::CSysDragger csysDragger;
  
  osg::ref_ptr<lbr::PLabel> countLabel{new lbr::PLabel(&count)};
  osg::ref_ptr<lbr::PLabel> angleLabel{new lbr::PLabel(&angle)};
  osg::ref_ptr<lbr::PLabel> inclusiveAngleLabel{new lbr::PLabel(&inclusiveAngle)};
  osg::ref_ptr<lbr::PLabel> includeSourceLabel{new lbr::PLabel(&includeSource)};
  
  Stow() = delete;
  Stow(Feature &fIn)
  : feature(fIn)
  , prmObserver(std::bind(&Feature::setModelDirty, &feature))
  , csysDragger(&feature, &csys)
  {
    auto connectAndPush = [&](prm::Parameter &prmIn)
    {
      prmIn.connectValue(std::bind(&Feature::setModelDirty, &feature));
      feature.parameters.push_back(&prmIn);
    };
    
    QStringList tStrings = //keep in sync with enum in header.
    {
      QObject::tr("Constant")
      , QObject::tr("Pick")
    };
    axisType.setEnumeration(tStrings);
    axisType.connectValue(std::bind(&Feature::setModelDirty, &feature));
    axisType.connectValue(std::bind(&Stow::prmActiveSync, this));
    feature.parameters.push_back(&axisType);
    
    csys.connect(prmObserver);
    feature.parameters.push_back(&csys);
    
    count.setConstraint(prm::Constraint::buildNonZeroPositive());
    connectAndPush(count);
    
    angle.setConstraint(prm::Constraint::buildNonZeroAngle());
    connectAndPush(angle);
    
    connectAndPush(inclusiveAngle);
    connectAndPush(includeSource);
    connectAndPush(source);
    connectAndPush(axis);
    
    feature.overlaySwitch->addChild(countLabel.get());
    feature.overlaySwitch->addChild(angleLabel.get());
    feature.overlaySwitch->addChild(inclusiveAngleLabel.get());
    feature.overlaySwitch->addChild(includeSourceLabel.get());
    
    csysDragger.dragger->unlinkToMatrix(feature.getMainTransform());
    csysDragger.dragger->hide(lbr::CSysDragger::SwitchIndexes::LinkIcon);
    feature.overlaySwitch->addChild(csysDragger.dragger);
    
    feature.annexes.insert(std::make_pair(ann::Type::SeerShape, &sShape));
    feature.annexes.insert(std::make_pair(ann::Type::InstanceMapper, &iMapper));
    feature.annexes.insert(std::make_pair(ann::Type::CSysDragger, &csysDragger));
  }
  
  void prmActiveSync()
  {
    //shouldn't need to block anything
    switch (static_cast<AxisType>(axisType.getInt()))
    {
      case AxisType::Constant:{
        csys.setActive(true);
        axis.setActive(false);
        break;}
      case AxisType::Pick:{
        csys.setActive(false);
        axis.setActive(true);
        break;}
    }
  }
};

Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionInstancePolar.svg");
  
  name = QObject::tr("Instance Polar");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
}

Feature::~Feature() = default;

void Feature::updateModel(const UpdatePayload &payloadIn)
{
  setFailure();
  lastUpdateLog.clear();
  stow->sShape.reset();
  try
  {
    prm::ObserverBlocker block(stow->prmObserver);
    
    //no new failure state.
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    //get the shapes to array.
    const auto &sourcePicks = stow->source.getPicks();
    tls::Resolver resolver(payloadIn);
    if (sourcePicks.empty() || !resolver.resolve(sourcePicks.front()))
      throw std::runtime_error("Couldn't resolve source pick");
    occt::ShapeVector tShapes;
    const ann::SeerShape &tss = *resolver.getSeerShape();
    if (slc::isObjectType(sourcePicks.front().selectionType))
      tShapes = tss.useGetNonCompoundChildren();
    else
      tShapes = resolver.getShapes();
    if (tShapes.empty())
      throw std::runtime_error("No source shapes found");
    
    //set up csys. z axis will be rotation axis.
    auto currentType = static_cast<AxisType>(stow->axisType.getInt());
    //nothing to do if current type is constant.
    if (currentType == Pick)
    {
      std::optional<osg::Vec3d> newOrigin;
      std::optional<osg::Vec3d> newDirection;
      const auto &axisPicks = stow->axis.getPicks();
      if (axisPicks.empty())
        throw std::runtime_error("Axis picks are empty");
      if (!resolver.resolve(axisPicks.front()))
        throw std::runtime_error("Couldn't resolve axis pick");
      if (slc::isObjectType(axisPicks.front().selectionType))
      {
        const auto originPrms = resolver.getFeature()->getParameters(prm::Tags::Origin);
        const auto directionPrms = resolver.getFeature()->getParameters(prm::Tags::Direction);
        const auto csysPrms = resolver.getFeature()->getParameters(prm::Tags::CSys);
        if (!originPrms.empty() && !directionPrms.empty())
        {
          newOrigin = originPrms.front()->getVector();
          newDirection = directionPrms.front()->getVector();
        }
        else if (!csysPrms.empty())
        {
          newOrigin = csysPrms.front()->getMatrix().getTrans();
          newDirection = gu::getZVector(csysPrms.front()->getMatrix());
        }
        else
          throw std::runtime_error("Couldn't get axis from input feature parameters");
      }
      else
      {
        occt::ShapeVector rShapes = resolver.getShapes();
        if (rShapes.empty())
          throw std::runtime_error("No resolved shapes for axis");
        if (rShapes.size() != 1)
        {
          std::ostringstream s; s << "More than 1 axis pick resolution, using first"  << std::endl;
          lastUpdateLog.append(s.str());
        }
        gp_Ax1 axis;
        bool results;
        std::tie(axis, results) = occt::gleanAxis(rShapes.front());
        if (!results)
          throw std::runtime_error("unsupported occt shape type for axis");
        
        newDirection = gu::toOsg(gp_Vec(axis.Direction()));
        newOrigin = gu::toOsg(axis.Location());
      }
      
      //just in case.
      if (!newOrigin || !newDirection)
        throw std::runtime_error("Couldn't get origin and direction from pick");
      osg::Matrix fromTo = osg::Matrix::rotate(gu::getZVector(stow->csys.getMatrix()), *newDirection);
      osg::Matrix finalCSys = stow->csys.getMatrix() * fromTo;
      finalCSys.setTrans(*newOrigin);
      stow->csys.setValue(finalCSys);
    }
    
    //augment counts and angles for expected results.
    int ac = stow->count.getInt(); //actual count
    double aa = stow->angle.getDouble(); //actual angle
    bool ai = stow->inclusiveAngle.getBool(); //actual inclusive angle
    
    if (ai)
    {
      if (std::fabs(std::fabs(aa) - 360.0) <= Precision::Confusion()) //angle parameter has been constrained to less than or equal to 360
        aa = aa / ac;
      else
        aa = aa / (ac - 1);
    }
    else //ai = false
    {
      //stay under 360. don't overlap.
      int maxCount = static_cast<int>(360.0 / std::fabs(aa));
      ac = std::min(ac, maxCount);
      if (ac != stow->count.getInt())
      {
        std::ostringstream s; s << "Count has been limited to less than full rotation"  << std::endl;
        lastUpdateLog.append(s.str());
      }
    }
    
    //do instancing.
    occt::ShapeVector out;
    gp_Ax1 ra
    (
      gp_Pnt(gu::toOcc(stow->csys.getMatrix().getTrans()).XYZ())
      , gp_Dir(gu::toOcc(gu::getZVector(stow->csys.getMatrix())))
    );
    
    for (int index = 0; index < ac; ++index)
    {
      if (index == 0 && !(stow->includeSource.getBool()))
        continue;
      gp_Trsf rotation;
      rotation.SetRotation(ra, osg::DegreesToRadians(static_cast<double>(index) * aa));
      BRepBuilderAPI_Transform bt(rotation);
      for (const auto &tShape : tShapes)
      {
        bt.Perform(tShape, true); //was getting inverted shapes, so true to copy.
        out.push_back(bt.Shape());
      }
    }
    
    TopoDS_Compound result = occt::ShapeVectorCast(out);
    
    ShapeCheck check(result);
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    stow->sShape.setOCCTShape(result, getId());
    
    for (const auto &s : tShapes)
    {
      stow->iMapper.startMapping(tss, tss.findId(s),  payloadIn.shapeHistory);
      std::size_t sc = 0;
      for (const auto &si : out)
      {
        stow->iMapper.mapIndex(stow->sShape, si, sc);
        sc++;
      }
    }
    
    //update label locations.
    occt::BoundingBox inputBounds(tShapes);
    gp_Pnt inputCenter = inputBounds.getCenter();
    
    gp_Trsf clr; // count label rotation.
    clr.SetRotation(ra, osg::DegreesToRadians((ac - 1) * aa));
    gp_Pnt clo = inputCenter.Transformed(clr); //count label origin.
    stow->countLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(clo)));
    
    gp_Trsf alr; //angle label rotation.
    alr.SetRotation(ra, osg::DegreesToRadians(aa / 2.0));
    gp_Pnt alo = inputCenter.Transformed(alr);
    stow->angleLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(alo)));
    
    Handle_Geom_Line rl = new Geom_Line(ra);
    GeomAPI_ProjectPointOnCurve projector(inputCenter, rl);
    gp_Pnt axisPoint = projector.NearestPoint();
    stow->inclusiveAngleLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(axisPoint)));
    
    stow->includeSourceLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(inputCenter) + osg::Vec3d(0.0, 0.0, -inputBounds.getHeight() / 2.0)));
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in instance polar update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in instance polar update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in instance polar update. " << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Feature::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::inps::InstancePolar sip
  (
    Base::serialOut(),
    stow->axisType.serialOut(),
    stow->csys.serialOut(),
    stow->count.serialOut(),
    stow->angle.serialOut(),
    stow->inclusiveAngle.serialOut(),
    stow->includeSource.serialOut(),
    stow->source.serialOut(),
    stow->sShape.serialOut(),
    stow->iMapper.serialOut(),
    stow->csysDragger.serialOut(),
    stow->countLabel->serialOut(),
    stow->angleLabel->serialOut(),
    stow->inclusiveAngleLabel->serialOut(),
    stow->includeSourceLabel->serialOut()
  );
  
  auto ct = static_cast<AxisType>(stow->axisType.getInt());
  if (ct == Pick)
    sip.axis() = stow->axis.serialOut();
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::inps::instancePolar(stream, sip, infoMap);
}

void Feature::serialRead(const prj::srl::inps::InstancePolar &sip)
{
  Base::serialIn(sip.base());
  stow->axisType.serialIn(sip.axisType());
  stow->csys.serialIn(sip.csys());
  stow->count.serialIn(sip.count());
  stow->angle.serialIn(sip.angle());
  stow->inclusiveAngle.serialIn(sip.inclusiveAngle());
  stow->includeSource.serialIn(sip.includeSource());
  stow->source.serialIn(sip.source());
  stow->sShape.serialIn(sip.seerShape());
  stow->iMapper.serialIn(sip.instanceMaps());
  stow->csysDragger.serialIn(sip.csysDragger());
  stow->countLabel->serialIn(sip.countLabel());
  stow->angleLabel->serialIn(sip.angleLabel());
  stow->inclusiveAngleLabel->serialIn(sip.inclusiveAngleLabel());
  stow->includeSourceLabel->serialIn(sip.includeSourceLabel());
  
  if (sip.axis())
    stow->axis.serialIn(sip.axis().get());
}
