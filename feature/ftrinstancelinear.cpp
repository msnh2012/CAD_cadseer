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
#include "tools/occtools.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinstancelinear.h"

using namespace ftr::InstanceLinear;

using boost::uuids::uuid;

QIcon Feature::icon;

struct Feature::Stow
{
  Feature &feature;
  
  using PRM = prm::Parameter;
  PRM csys = PRM(prm::Names::CSys, osg::Matrixd::identity(), prm::Tags::CSys);
  PRM source = PRM(QObject::tr("Source"), ftr::Picks(), Tags::Source);
  PRM xOffset = PRM(QObject::tr("X Offset"), 20.0, Tags::XOffset);
  PRM yOffset = PRM(QObject::tr("Y Offset"), 20.0, Tags::YOffset);
  PRM zOffset = PRM(QObject::tr("Z Offset"), 20.0, Tags::ZOffset);
  PRM xCount = PRM(QObject::tr("X Count"), 2,  Tags::XCount);
  PRM yCount = PRM(QObject::tr("Y Count"), 2,  Tags::YCount);
  PRM zCount = PRM(QObject::tr("Z Count"), 2,  Tags::ZCount);
  PRM includeSource = PRM(QObject::tr("Include Source"), true, Tags::IncludeSource);
  prm::Observer prmObserver;
  
  ann::SeerShape sShape;
  ann::InstanceMapper iMapper;
  ann::CSysDragger csysDragger;
  
  osg::ref_ptr<lbr::PLabel> xOffsetLabel = new lbr::PLabel(&xOffset);
  osg::ref_ptr<lbr::PLabel> yOffsetLabel = new lbr::PLabel(&yOffset);
  osg::ref_ptr<lbr::PLabel> zOffsetLabel = new lbr::PLabel(&zOffset);
  osg::ref_ptr<lbr::PLabel> xCountLabel = new lbr::PLabel(&xCount);
  osg::ref_ptr<lbr::PLabel> yCountLabel = new lbr::PLabel(&yCount);
  osg::ref_ptr<lbr::PLabel> zCountLabel = new lbr::PLabel(&zCount);
  osg::ref_ptr<lbr::PLabel> includeSourceLabel = new lbr::PLabel(&includeSource);
  
  Stow() = delete;
  Stow(Feature &fIn)
  : feature(fIn)
  , csysDragger(&feature, &csys)
  {
    xOffset.setConstraint(prm::Constraint::buildNonZero());
    yOffset.setConstraint(prm::Constraint::buildNonZero());
    zOffset.setConstraint(prm::Constraint::buildNonZero());
    xCount.setConstraint(prm::Constraint::buildNonZeroPositive());
    yCount.setConstraint(prm::Constraint::buildNonZeroPositive());
    zCount.setConstraint(prm::Constraint::buildNonZeroPositive());
    
    xOffset.connectValue(std::bind(&Feature::setModelDirty, &feature));
    yOffset.connectValue(std::bind(&Feature::setModelDirty, &feature));
    zOffset.connectValue(std::bind(&Feature::setModelDirty, &feature));
    xCount.connectValue(std::bind(&Feature::setModelDirty, &feature));
    yCount.connectValue(std::bind(&Feature::setModelDirty, &feature));
    zCount.connectValue(std::bind(&Feature::setModelDirty, &feature));
    includeSource.connectValue(std::bind(&Feature::setModelDirty, &feature));
    
    feature.parameters.push_back(&csys);
    feature.parameters.push_back(&source);
    feature.parameters.push_back(&xOffset);
    feature.parameters.push_back(&yOffset);
    feature.parameters.push_back(&zOffset);
    feature.parameters.push_back(&xCount);
    feature.parameters.push_back(&yCount);
    feature.parameters.push_back(&zCount);
    feature.parameters.push_back(&includeSource);
    
    prmObserver.valueHandler = std::bind(&Feature::setModelDirty, &feature);
    csys.connect(prmObserver);
    
    feature.annexes.insert(std::make_pair(ann::Type::SeerShape, &sShape));
    feature.annexes.insert(std::make_pair(ann::Type::InstanceMapper, &iMapper));
    feature.annexes.insert(std::make_pair(ann::Type::CSysDragger, &csysDragger));
    
    feature.overlaySwitch->addChild(xOffsetLabel.get());
    feature.overlaySwitch->addChild(yOffsetLabel.get());
    feature.overlaySwitch->addChild(zOffsetLabel.get());
    feature.overlaySwitch->addChild(xCountLabel.get());
    feature.overlaySwitch->addChild(yCountLabel.get());
    feature.overlaySwitch->addChild(zCountLabel.get());
    feature.overlaySwitch->addChild(includeSourceLabel.get());
    
    //keeping the dragger arrows so use can select and define vector.
    csysDragger.dragger->unlinkToMatrix(feature.getMainTransform());
    csysDragger.dragger->hide(lbr::CSysDragger::SwitchIndexes::LinkIcon);
    csysDragger.dragger->hide(lbr::CSysDragger::SwitchIndexes::Origin);
    feature.overlaySwitch->addChild(csysDragger.dragger);
  }
};

Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionInstanceLinear.svg");
  
  name = QObject::tr("Instance Linear");
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
    
    const auto &picks = stow->source.getPicks();
    if (picks.size() != 1)
      throw std::runtime_error("Need 1, and only 1, pick");
    tls::Resolver resolver(payloadIn);
    resolver.resolve(picks.front());
    occt::ShapeVector tShapes = resolver.getShapes();
    if (tShapes.empty())
      throw std::runtime_error("No resolved shapes found.");
    assert(resolver.getSeerShape());
    if (!slc::isShapeType(picks.front().selectionType))
    {
      assert(tShapes.size() == 1);
      assert(tShapes.front().ShapeType() == TopAbs_COMPOUND);
      tShapes = occt::getNonCompounds(tShapes.front());
    }
    if (tShapes.empty())
      throw std::runtime_error("No shapes found.");

    occt::ShapeVector out;
    osg::Vec3d xProjection = gu::getXVector(stow->csys.getMatrix()) * stow->xOffset.getDouble();
    osg::Vec3d yProjection = gu::getYVector(stow->csys.getMatrix()) * stow->yOffset.getDouble();
    osg::Vec3d zProjection = gu::getZVector(stow->csys.getMatrix()) * stow->zOffset.getDouble();
    for (const auto &tShape : tShapes)
    {
      osg::Matrixd shapeBase = gu::toOsg(tShape.Location().Transformation());
      osg::Vec3d baseTrans = shapeBase.getTrans();
      
      for (int z = 0; z < stow->zCount.getInt(); ++z)
      {
        for (int y = 0; y < stow->yCount.getInt(); ++y)
        {
          for (int x = 0; x < stow->xCount.getInt(); ++x)
          {
            if ((x == 0) && (y == 0) && (z == 0) && (!stow->includeSource.getBool()))
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
    
    stow->sShape.setOCCTShape(result, getId());
    
    for (const auto &s : tShapes)
    {
      stow->iMapper.startMapping(*resolver.getSeerShape(), resolver.getSeerShape()->findId(s),  payloadIn.shapeHistory);
      std::size_t count = 0;
      for (const auto &si : out)
      {
        stow->iMapper.mapIndex(stow->sShape, si, count);
        count++;
      }
    }
    
    //update label locations.
    //the origin of the system doesn't matter, so just put at shape center.
    occt::BoundingBox bb(tShapes);
    osg::Vec3d origin = gu::toOsg(bb.getCenter());
    osg::Matrixd tsys = stow->csys.getMatrix();
    tsys.setTrans(origin);
    stow->csys.setValue(tsys);
    stow->csysDragger.draggerUpdate();
    
    stow->xOffsetLabel->setMatrix(tsys * osg::Matrixd::translate(xProjection * 0.5));
    stow->yOffsetLabel->setMatrix(tsys * osg::Matrixd::translate(yProjection * 0.5));
    stow->zOffsetLabel->setMatrix(tsys * osg::Matrixd::translate(zProjection * 0.5));
    
    //if we do instancing only on the x axis, y and z count = 1, then
    //the y and z labels overlap at zero. so we cheat.
    auto cheat = [](int c) -> int{return std::max(c - 1, 1);};
    stow->xCountLabel->setMatrix(tsys * osg::Matrixd::translate(xProjection * cheat(stow->xCount.getInt())));
    stow->yCountLabel->setMatrix(tsys * osg::Matrixd::translate(yProjection * cheat(stow->yCount.getInt())));
    stow->zCountLabel->setMatrix(tsys * osg::Matrixd::translate(zProjection * cheat(stow->zCount.getInt())));
    
    stow->includeSourceLabel->setMatrix(osg::Matrixd::translate(origin + osg::Vec3d(0.0, 0.0, -bb.getHeight() / 2.0)));
    
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

void Feature::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::inls::InstanceLinear so
  (
    Base::serialOut(),
    stow->source.serialOut(),
    stow->csys.serialOut(),
    stow->xOffset.serialOut(),
    stow->yOffset.serialOut(),
    stow->zOffset.serialOut(),
    stow->xCount.serialOut(),
    stow->yCount.serialOut(),
    stow->zCount.serialOut(),
    stow->includeSource.serialOut(),
    stow->sShape.serialOut(),
    stow->iMapper.serialOut(),
    stow->csysDragger.serialOut(),
    stow->xOffsetLabel->serialOut(),
    stow->yOffsetLabel->serialOut(),
    stow->zOffsetLabel->serialOut(),
    stow->xCountLabel->serialOut(),
    stow->yCountLabel->serialOut(),
    stow->zCountLabel->serialOut(),
    stow->includeSourceLabel->serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::inls::instanceLinear(stream, so, infoMap);
}

void Feature::serialRead(const prj::srl::inls::InstanceLinear &sil)
{
  Base::serialIn(sil.base());
  stow->source.serialIn(sil.source());
  stow->csys.serialIn(sil.csys());
  stow->xOffset.serialIn(sil.xOffset());
  stow->yOffset.serialIn(sil.yOffset());
  stow->zOffset.serialIn(sil.zOffset());
  stow->xCount.serialIn(sil.xCount());
  stow->yCount.serialIn(sil.yCount());
  stow->zCount.serialIn(sil.zCount());
  stow->includeSource.serialIn(sil.includeSource());
  stow->sShape.serialIn(sil.seerShape());
  stow->iMapper.serialIn(sil.instanceMaps());
  stow->csysDragger.serialIn(sil.csysDragger());
  stow->xOffsetLabel->serialIn(sil.xOffsetLabel());
  stow->yOffsetLabel->serialIn(sil.yOffsetLabel());
  stow->zOffsetLabel->serialIn(sil.zOffsetLabel());
  stow->xCountLabel->serialIn(sil.xCountLabel());
  stow->yCountLabel->serialIn(sil.yCountLabel());
  stow->zCountLabel->serialIn(sil.zCountLabel());
  stow->includeSourceLabel->serialIn(sil.includeSourceLabel());
}
