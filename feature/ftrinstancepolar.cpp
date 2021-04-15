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
#include <boost/optional/optional.hpp>

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
#include "feature/ftrdatumaxis.h"
#include "feature/ftrinstancepolar.h"

using namespace ftr;

using boost::uuids::uuid;

QIcon InstancePolar::icon;

InstancePolar::InstancePolar() :
Base(),
csys(std::make_unique<prm::Parameter>(prm::Names::CSys, osg::Matrixd::identity(), prm::Tags::CSys)),
count(std::make_unique<prm::Parameter>(QObject::tr("Count"), 3)),
angle(std::make_unique<prm::Parameter>(prm::Names::Angle, 20.0)),
inclusiveAngle(std::make_unique<prm::Parameter>(QObject::tr("Inclusive Angle"), false, prm::Tags::Angle)),
includeSource(std::make_unique<prm::Parameter>(QObject::tr("Include Source"), true)),
prmObserver(std::make_unique<prm::Observer>(std::bind(&InstancePolar::setModelDirty, this))),
sShape(std::make_unique<ann::SeerShape>()),
iMapper(std::make_unique<ann::InstanceMapper>()),
csysDragger(std::make_unique<ann::CSysDragger>(this, csys.get()))
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionInstancePolar.svg");
  
  name = QObject::tr("Instance Polar");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  count->setConstraint(prm::Constraint::buildNonZeroPositive());
  count->connectValue(std::bind(&InstancePolar::setModelDirty, this));
  parameters.push_back(count.get());
  
  csys->connect(*prmObserver);
  parameters.push_back(csys.get());
  
  angle->setConstraint(prm::Constraint::buildNonZeroAngle());
  angle->connectValue(std::bind(&InstancePolar::setModelDirty, this));
  parameters.push_back(angle.get());
  
  inclusiveAngle->connectValue(std::bind(&InstancePolar::setModelDirty, this));
  parameters.push_back(inclusiveAngle.get());
  
  includeSource->connectValue(std::bind(&InstancePolar::setModelDirty, this));
  parameters.push_back(includeSource.get());
  
  countLabel = new lbr::PLabel(count.get());
  overlaySwitch->addChild(countLabel.get());
  
  angleLabel = new lbr::PLabel(angle.get());
  overlaySwitch->addChild(angleLabel.get());
  
  inclusiveAngleLabel = new lbr::PLabel(inclusiveAngle.get());
  overlaySwitch->addChild(inclusiveAngleLabel.get());
  
  csysDragger->dragger->unlinkToMatrix(getMainTransform());
  csysDragger->dragger->hide(lbr::CSysDragger::SwitchIndexes::LinkIcon);
  //don't worry about adding dragger. update takes care of it.
//   overlaySwitch->addChild(csysDragger->dragger);
  
  includeSourceLabel = new lbr::PLabel(includeSource.get());
  overlaySwitch->addChild(includeSourceLabel.get());
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
  annexes.insert(std::make_pair(ann::Type::InstanceMapper, iMapper.get()));
  annexes.insert(std::make_pair(ann::Type::CSysDragger, csysDragger.get()));
}

InstancePolar::~InstancePolar() {}

void InstancePolar::setShapePick(const Pick &pIn)
{
  shapePick = pIn;
  setModelDirty();
}

void InstancePolar::setAxisPick(const Pick &aIn)
{
  axisPick = aIn;
  setModelDirty();
}

void InstancePolar::setCSys(const osg::Matrixd &mIn)
{
  csys->setValue(mIn);
}

void InstancePolar::updateModel(const UpdatePayload &payloadIn)
{
  setFailure();
  lastUpdateLog.clear();
  sShape->reset();
  try
  {
    prm::ObserverBlocker block(*prmObserver);
    
    //no new failure state.
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    //get the shapes to array.
    tls::Resolver resolver(payloadIn);
    resolver.resolve(shapePick);
    occt::ShapeVector tShapes = resolver.getShapes();
    if (tShapes.empty())
      throw std::runtime_error("invalid shape pick resolution");
    assert(resolver.getSeerShape());
    const ann::SeerShape &tss = *resolver.getSeerShape();
    if (!slc::isShapeType(shapePick.selectionType))
    {
      assert(tShapes.size() == 1);
      assert(tShapes.front().ShapeType() == TopAbs_COMPOUND);
      tShapes = occt::getNonCompounds(tShapes.front());
    }
    if (tShapes.empty())
      throw std::runtime_error("No shapes found.");
    
    boost::optional<osg::Vec3d> newOrigin;
    boost::optional<osg::Vec3d> newDirection;
    std::vector<const Base*> rafs = payloadIn.getFeatures(InstancePolar::rotationAxis);
    resolver.resolve(axisPick);
    if (resolver.getFeature())
    {
      //we have a rotation axis selection so make sure the csysdragger is hidden.
      overlaySwitch->removeChild(csysDragger->dragger.get()); //ok if not present.
      
      if (resolver.getFeature()->getType() == ftr::Type::DatumAxis)
      {
        const ftr::DatumAxis *da = dynamic_cast<const ftr::DatumAxis*>(resolver.getFeature());
        assert(da);
        newOrigin = da->getOrigin();
        newDirection = da->getDirection();
      }
      else
      {
        //can't do 2 points at this time?
        if (!resolver.getSeerShape())
          throw std::runtime_error("Input axis feature doesn't have seershape");
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
      assert(newDirection && newOrigin);
      csys->setValue(osg::Matrixd::rotate(osg::Vec3d(0.0, 0.0, 1.0), newDirection.get()) * osg::Matrixd::translate(newOrigin.get()));
      csysDragger->draggerUpdate(); //keep dragger in sync with parameter.
    }
    else
    {
      //make sure dragger is visible
      if(!overlaySwitch->containsNode(csysDragger->dragger.get()))
        overlaySwitch->addChild(csysDragger->dragger.get());
      newOrigin = static_cast<osg::Matrixd>(*csys).getTrans();
      newDirection = gu::getZVector(static_cast<osg::Matrixd>(*csys));
    }
    
    int ac = static_cast<int>(*count); //actual count
    double aa = static_cast<double>(*angle); //actual angle
    bool ai = static_cast<bool>(*inclusiveAngle); //actual inclusive angle
    
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
      if (ac != static_cast<int>(*count))
      {
        std::ostringstream s; s << "Count has been limited to less than full rotation"  << std::endl;
        lastUpdateLog.append(s.str());
      }
    }
    
    occt::ShapeVector out;
    gp_Ax1 ra(gp_Pnt(gu::toOcc(newOrigin.get()).XYZ()), gp_Dir(gu::toOcc(newDirection.get())));
    
    for (int index = 0; index < ac; ++index)
    {
      if (index == 0 && !(static_cast<bool>(*includeSource)))
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
    
    sShape->setOCCTShape(result, getId());
    
    for (const auto &s : tShapes)
    {
      iMapper->startMapping(tss, tss.findId(s),  payloadIn.shapeHistory);
      std::size_t sc = 0;
      for (const auto &si : out)
      {
        iMapper->mapIndex(*sShape, si, sc);
        sc++;
      }
    }
    
    //update label locations.
    occt::BoundingBox inputBounds(tShapes);
    gp_Pnt inputCenter = inputBounds.getCenter();
    
    gp_Trsf clr; // count label rotation.
    clr.SetRotation(ra, osg::DegreesToRadians((ac - 1) * aa));
    gp_Pnt clo = inputCenter.Transformed(clr); //count label origin.
    countLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(clo)));
    
    gp_Trsf alr; //angle label rotation.
    alr.SetRotation(ra, osg::DegreesToRadians(aa / 2.0));
    gp_Pnt alo = inputCenter.Transformed(alr);
    angleLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(alo)));
    
    Handle_Geom_Line rl = new Geom_Line(ra);
    GeomAPI_ProjectPointOnCurve projector(inputCenter, rl);
    gp_Pnt axisPoint = projector.NearestPoint();
    inclusiveAngleLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(axisPoint)));
    
    includeSourceLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(inputCenter) + osg::Vec3d(0.0, 0.0, -inputBounds.getHeight() / 2.0)));
    
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

void InstancePolar::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::inps::InstancePolar sip
  (
    Base::serialOut(),
    sShape->serialOut(),
    iMapper->serialOut(),
    csysDragger->serialOut(),
    csys->serialOut(),
    count->serialOut(),
    angle->serialOut(),
    inclusiveAngle->serialOut(),
    includeSource->serialOut(),
    countLabel->serialOut(),
    angleLabel->serialOut(),
    inclusiveAngleLabel->serialOut(),
    includeSourceLabel->serialOut(),
    shapePick.serialOut(),
    axisPick.serialOut(),
    overlaySwitch->containsNode(csysDragger->dragger.get())
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::inps::instancePolar(stream, sip, infoMap);
}

void InstancePolar::serialRead(const prj::srl::inps::InstancePolar &sip)
{
  Base::serialIn(sip.base());
  sShape->serialIn(sip.seerShape());
  iMapper->serialIn(sip.instanceMaps());
  csysDragger->serialIn(sip.csysDragger());
  csys->serialIn(sip.csys());
  count->serialIn(sip.count());
  angle->serialIn(sip.angle());
  inclusiveAngle->serialIn(sip.inclusiveAngle());
  includeSource->serialIn(sip.includeSource());
  countLabel->serialIn(sip.countLabel());
  angleLabel->serialIn(sip.angleLabel());
  inclusiveAngleLabel->serialIn(sip.inclusiveAngleLabel());
  includeSourceLabel->serialIn(sip.includeSourceLabel());
  shapePick.serialIn(sip.shapePick());
  axisPick.serialIn(sip.axisPick());
  if (sip.draggerVisible())
    overlaySwitch->addChild(csysDragger->dragger.get());
}
