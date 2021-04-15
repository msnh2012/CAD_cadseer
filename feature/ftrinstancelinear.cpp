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

#include <TopoDS.hxx>

#include "globalutilities.h"
#include "library/lbrplabel.h"
#include "library/lbrcsysdragger.h"
#include "annex/annseershape.h"
#include "annex/anncsysdragger.h"
#include "annex/anninstancemapper.h"
#include "feature/ftrdatumplane.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "feature/ftrshapecheck.h"
#include "project/serial/generated/prjsrlinlsinstancelinear.h"
#include "tools/featuretools.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinstancelinear.h"

using namespace ftr;

using boost::uuids::uuid;

QIcon InstanceLinear::icon;

InstanceLinear::InstanceLinear() :
Base(),
xOffset(std::make_unique<prm::Parameter>(QObject::tr("X Offset"), 20.0)),
yOffset(std::make_unique<prm::Parameter>(QObject::tr("Y Offset"), 20.0)),
zOffset(std::make_unique<prm::Parameter>(QObject::tr("Z Offset"), 20.0)),
xCount(std::make_unique<prm::Parameter>(QObject::tr("X Count"), 2)),
yCount(std::make_unique<prm::Parameter>(QObject::tr("Y Count"), 2)),
zCount(std::make_unique<prm::Parameter>(QObject::tr("Z Count"), 2)),
csys(std::make_unique<prm::Parameter>(prm::Names::CSys, osg::Matrixd::identity(), prm::Tags::CSys)),
includeSource(std::make_unique<prm::Parameter>(QObject::tr("Include Source"), true)),
prmObserver(std::make_unique<prm::Observer>(std::bind(&InstanceLinear::setModelDirty, this))),
sShape(std::make_unique<ann::SeerShape>()),
iMapper(std::make_unique<ann::InstanceMapper>()),
csysDragger(std::make_unique<ann::CSysDragger>(this, csys.get()))
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionInstanceLinear.svg");
  
  name = QObject::tr("Instance Linear");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  xOffset->setConstraint(prm::Constraint::buildNonZero());
  yOffset->setConstraint(prm::Constraint::buildNonZero());
  zOffset->setConstraint(prm::Constraint::buildNonZero());
  
  xCount->setConstraint(prm::Constraint::buildNonZeroPositive());
  yCount->setConstraint(prm::Constraint::buildNonZeroPositive());
  zCount->setConstraint(prm::Constraint::buildNonZeroPositive());
  
  xOffset->connectValue(std::bind(&InstanceLinear::setModelDirty, this));
  yOffset->connectValue(std::bind(&InstanceLinear::setModelDirty, this));
  zOffset->connectValue(std::bind(&InstanceLinear::setModelDirty, this));
  
  xCount->connectValue(std::bind(&InstanceLinear::setModelDirty, this));
  yCount->connectValue(std::bind(&InstanceLinear::setModelDirty, this));
  zCount->connectValue(std::bind(&InstanceLinear::setModelDirty, this));
  
  csys->connect(*prmObserver);
  
  includeSource->connectValue(std::bind(&InstanceLinear::setModelDirty, this));
  
  xOffsetLabel = new lbr::PLabel(xOffset.get());
  overlaySwitch->addChild(xOffsetLabel.get());
  yOffsetLabel = new lbr::PLabel(yOffset.get());
  overlaySwitch->addChild(yOffsetLabel.get());
  zOffsetLabel = new lbr::PLabel(zOffset.get());
  overlaySwitch->addChild(zOffsetLabel.get());
  
  xCountLabel = new lbr::PLabel(xCount.get());
  overlaySwitch->addChild(xCountLabel.get());
  yCountLabel = new lbr::PLabel(yCount.get());
  overlaySwitch->addChild(yCountLabel.get());
  zCountLabel = new lbr::PLabel(zCount.get());
  overlaySwitch->addChild(zCountLabel.get());
  
  //keeping the dragger arrows so use can select and define vector.
  csysDragger->dragger->unlinkToMatrix(getMainTransform());
  csysDragger->dragger->hide(lbr::CSysDragger::SwitchIndexes::LinkIcon);
  csysDragger->dragger->hide(lbr::CSysDragger::SwitchIndexes::Origin);
  overlaySwitch->addChild(csysDragger->dragger);
  
  includeSourceLabel = new lbr::PLabel(includeSource.get());
  overlaySwitch->addChild(includeSourceLabel.get());
  
  parameters.push_back(xOffset.get());
  parameters.push_back(yOffset.get());
  parameters.push_back(zOffset.get());
  parameters.push_back(xCount.get());
  parameters.push_back(yCount.get());
  parameters.push_back(zCount.get());
  parameters.push_back(csys.get());
  parameters.push_back(includeSource.get());
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
  annexes.insert(std::make_pair(ann::Type::InstanceMapper, iMapper.get()));
  annexes.insert(std::make_pair(ann::Type::CSysDragger, csysDragger.get()));
}

InstanceLinear::~InstanceLinear() {}

void InstanceLinear::setPick(const Pick &pIn)
{
  pick = pIn;
  setModelDirty();
}

void InstanceLinear::setCSys(const osg::Matrixd &mIn)
{
  csys->setValue(mIn);
  csysDragger->draggerUpdate();
}

bool InstanceLinear::getIncludeSource()
{
  return static_cast<bool>(*includeSource);
}

void InstanceLinear::setIncludeSource(bool in)
{
  includeSource->setValue(in);
}

void InstanceLinear::updateModel(const UpdatePayload &payloadIn)
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
    
    tls::Resolver resolver(payloadIn);
    resolver.resolve(pick);
    occt::ShapeVector tShapes = resolver.getShapes();
    if (tShapes.empty())
      throw std::runtime_error("No resolved shapes found.");
    assert(resolver.getSeerShape());
    if (!slc::isShapeType(pick.selectionType))
    {
      assert(tShapes.size() == 1);
      assert(tShapes.front().ShapeType() == TopAbs_COMPOUND);
      tShapes = occt::getNonCompounds(tShapes.front());
    }
    if (tShapes.empty())
      throw std::runtime_error("No shapes found.");

    occt::ShapeVector out;
    osg::Vec3d xProjection = gu::getXVector(static_cast<osg::Matrixd>(*csys)) * static_cast<double>(*xOffset);
    osg::Vec3d yProjection = gu::getYVector(static_cast<osg::Matrixd>(*csys)) * static_cast<double>(*yOffset);
    osg::Vec3d zProjection = gu::getZVector(static_cast<osg::Matrixd>(*csys)) * static_cast<double>(*zOffset);
    for (const auto &tShape : tShapes)
    {
      osg::Matrixd shapeBase = gu::toOsg(tShape.Location().Transformation());
      osg::Vec3d baseTrans = shapeBase.getTrans();
      
      for (int z = 0; z < static_cast<int>(*zCount); ++z)
      {
        for (int y = 0; y < static_cast<int>(*yCount); ++y)
        {
          for (int x = 0; x < static_cast<int>(*xCount); ++x)
          {
            if ((x == 0) && (y == 0) && (z == 0) && (!static_cast<bool>(*includeSource)))
              continue;
            osg::Vec3d no = baseTrans; //new origin
            no += zProjection * static_cast<double>(z);
            no += yProjection * static_cast<double>(y);
            no += xProjection * static_cast<double>(x);
            
            gp_Trsf nt = tShape.Location().Transformation(); //new Transformation
            nt.SetTranslationPart(gu::toOcc(no));
            TopLoc_Location loc(nt);
            
            out.push_back(tShape.Located(loc));
          }
        }
      }
    }
    
    TopoDS_Compound result = occt::ShapeVectorCast(out);
    
    ShapeCheck check(result);
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    sShape->setOCCTShape(result, getId());
    
    for (const auto &s : tShapes)
    {
      iMapper->startMapping(*resolver.getSeerShape(), resolver.getSeerShape()->findId(s),  payloadIn.shapeHistory);
      std::size_t count = 0;
      for (const auto &si : out)
      {
        iMapper->mapIndex(*sShape, si, count);
        count++;
      }
    }
    
    //update label locations.
    //the origin of the system doesn't matter, so just put at shape center.
    occt::BoundingBox bb(tShapes);
    osg::Vec3d origin = gu::toOsg(bb.getCenter());
    osg::Matrixd tsys = static_cast<osg::Matrixd>(*csys);
    tsys.setTrans(origin);
    csys->setValue(tsys);
    csysDragger->draggerUpdate();
    
    xOffsetLabel->setMatrix(tsys * osg::Matrixd::translate(xProjection * 0.5));
    yOffsetLabel->setMatrix(tsys * osg::Matrixd::translate(yProjection * 0.5));
    zOffsetLabel->setMatrix(tsys * osg::Matrixd::translate(zProjection * 0.5));
    
    //if we do instancing only on the x axis, y and z count = 1, then
    //the y and z labels overlap at zero. so we cheat.
    auto cheat = [](int c) -> int{return std::max(c - 1, 1);};
    xCountLabel->setMatrix(tsys * osg::Matrixd::translate(xProjection * cheat(static_cast<int>(*xCount))));
    yCountLabel->setMatrix(tsys * osg::Matrixd::translate(yProjection * cheat(static_cast<int>(*yCount))));
    zCountLabel->setMatrix(tsys * osg::Matrixd::translate(zProjection * cheat(static_cast<int>(*zCount))));
    
    includeSourceLabel->setMatrix(osg::Matrixd::translate(origin + osg::Vec3d(0.0, 0.0, -bb.getHeight() / 2.0)));
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in instance linear update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in instance linear update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in instance linear update. " << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void InstanceLinear::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::inls::InstanceLinear so
  (
    Base::serialOut(),
    sShape->serialOut(),
    iMapper->serialOut(),
    csysDragger->serialOut(),
    xOffset->serialOut(),
    yOffset->serialOut(),
    zOffset->serialOut(),
    xCount->serialOut(),
    yCount->serialOut(),
    zCount->serialOut(),
    csys->serialOut(),
    includeSource->serialOut(),
    pick.serialOut(),
    xOffsetLabel->serialOut(),
    yOffsetLabel->serialOut(),
    zOffsetLabel->serialOut(),
    xCountLabel->serialOut(),
    yCountLabel->serialOut(),
    zCountLabel->serialOut(),
    includeSourceLabel->serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::inls::instanceLinear(stream, so, infoMap);
}

void InstanceLinear::serialRead(const prj::srl::inls::InstanceLinear &sil)
{
  Base::serialIn(sil.base());
  sShape->serialIn(sil.seerShape());
  iMapper->serialIn(sil.instanceMaps());
  csysDragger->serialIn(sil.csysDragger());
  xOffset->serialIn(sil.xOffset());
  yOffset->serialIn(sil.yOffset());
  zOffset->serialIn(sil.zOffset());
  xCount->serialIn(sil.xCount());
  yCount->serialIn(sil.yCount());
  zCount->serialIn(sil.zCount());
  csys->serialIn(sil.csys());
  includeSource->serialIn(sil.includeSource());
  pick.serialIn(sil.pick());
  xOffsetLabel->serialIn(sil.xOffsetLabel());
  yOffsetLabel->serialIn(sil.yOffsetLabel());
  zOffsetLabel->serialIn(sil.zOffsetLabel());
  xCountLabel->serialIn(sil.xCountLabel());
  yCountLabel->serialIn(sil.yCountLabel());
  zCountLabel->serialIn(sil.zCountLabel());
  includeSourceLabel->serialIn(sil.includeSourceLabel());
}
    
