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

#include <boost/filesystem/path.hpp>

#include <osg/Switch>
#include <osg/PositionAttitudeTransform>

#include "globalutilities.h"
#include "annex/anncsysdragger.h"
#include "library/lbrcsysdragger.h"
#include "library/lbrplabel.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "tools/occtools.h"
#include "tools/featuretools.h"
#include "tools/tlsosgtools.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinputtype.h"
#include "project/serial/generated/prjsrldtmsdatumsystem.h"
#include "modelviz/mdvdatumsystem.h"
#include "feature/ftrdatumsystem.h"

using boost::uuids::uuid;

using namespace ftr::DatumSystem;

QIcon Feature::icon;

struct Feature::Stow
{
  Feature &feature;
  
  prm::Parameter systemType;
  prm::Parameter csys;
  prm::Parameter autoSize;
  prm::Parameter size;
  prm::Parameter offsetVector;
  prm::Parameter linkedPicks;
  prm::Parameter pointsPicks;
  prm::Observer csysObserver;
  prm::Observer sizeObserver;
  prm::Observer syncObserver;
  ann::CSysDragger csysDragger;
  
  osg::ref_ptr<mdv::DatumSystem> display;
  osg::ref_ptr<lbr::PLabel> autoSizeLabel;
  osg::ref_ptr<lbr::PLabel> sizeLabel;
  osg::ref_ptr<lbr::PLabel> offsetVectorLabel;
  osg::ref_ptr<osg::PositionAttitudeTransform> scale;
  double cachedSize;
  
  Stow(Feature &fIn)
  : feature(fIn)
  , systemType(QObject::tr("Type"), 0, Tags::SystemType)
  , csys(prm::Names::CSys, osg::Matrixd::identity(), prm::Tags::CSys)
  , autoSize(prm::Names::AutoSize, false, prm::Tags::AutoSize)
  , size(prm::Names::Size, 10.0, prm::Tags::Size)
  , offsetVector(prm::Names::Offset, osg::Vec3d(), prm::Tags::Offset)
  , linkedPicks(QObject::tr("Linked"), ftr::Picks(), Tags::Linked)
  , pointsPicks(QObject::tr("Points"), ftr::Picks(), Tags::Points)
  , csysObserver(std::bind(&Feature::setModelDirty, &feature))
  , sizeObserver(std::bind(&Feature::setVisualDirty, &feature))
  , syncObserver(std::bind(&Stow::prmActiveSync, this))
  , csysDragger(&feature, &csys)
  , display(new mdv::DatumSystem())
  , autoSizeLabel(new lbr::PLabel(&autoSize))
  , sizeLabel(new lbr::PLabel(&size))
  , offsetVectorLabel(new lbr::PLabel(&offsetVector))
  , scale(new osg::PositionAttitudeTransform())
  {
    auto initPrm = [&](prm::Parameter &prmIn)
    {
      prmIn.connectValue(std::bind(&Feature::setModelDirty, &feature));
      feature.parameters.push_back(&prmIn);
    };
    
    QStringList tStrings = //keep in sync with enum in header.
    {
      QObject::tr("Constant")
      , QObject::tr("Linked")
      , QObject::tr("Points")
    };
    systemType.setEnumeration(tStrings);
    systemType.connect(syncObserver);
    autoSize.connect(syncObserver);
    
    initPrm(systemType);
    initPrm(offsetVector);
    initPrm(linkedPicks);
    initPrm(pointsPicks);
    initPrm(autoSize);
    
    csys.connect(csysObserver);
    feature.parameters.push_back(&csys);
  
    size.connect(sizeObserver);
    feature.parameters.push_back(&size);
    
    csysDragger.dragger->hide(lbr::CSysDragger::SwitchIndexes::LinkIcon);
    feature.annexes.insert(std::make_pair(ann::Type::CSysDragger, &csysDragger));
    feature.overlaySwitch->addChild(csysDragger.dragger);
    
    double temp = size.getDouble();
    scale->setScale(osg::Vec3d(temp, temp, temp));
    scale->addChild(display.get());
    feature.mainTransform->addChild(scale.get());
    
    feature.overlaySwitch->addChild(autoSizeLabel);
    feature.overlaySwitch->addChild(sizeLabel);
    feature.overlaySwitch->addChild(offsetVectorLabel);
    
    cachedSize = size.getDouble();
  }
  
  void prmActiveSync()
  {
    prm::ObserverBlocker syncBlocker(syncObserver);
    
    auto st = static_cast<SystemType>(systemType.getInt());
    switch(st)
    {
      case Constant:{
        csys.setActive(true);
        offsetVector.setActive(false);
        autoSize.setValue(false);
        autoSize.setActive(false);
        size.setActive(true);
        linkedPicks.setActive(false);
        pointsPicks.setActive(false);
        break;}
      case Linked:{
        csys.setActive(false);
        offsetVector.setActive(true);
        autoSize.setValue(false);
        autoSize.setActive(false);
        size.setActive(true);
        linkedPicks.setActive(true);
        pointsPicks.setActive(false);
        break;}
      case Through3Points:{
        csys.setActive(false);
        offsetVector.setActive(true);
        autoSize.setActive(true);
        size.setActive(!autoSize.getBool());
        linkedPicks.setActive(false);
        pointsPicks.setActive(true);
        break;}
    }
  }
  
  osg::Matrixd applyOffset(osg::Matrixd mIn, const osg::Vec3d &offset)
  {
    if (!offset.valid() || offset.length() < std::numeric_limits<float>::epsilon())
      return mIn;
    mIn.setTrans(offset * mIn);
    return mIn;
  }
  
  void updateLinked(const UpdatePayload &pli)
  {
    const auto &cp = linkedPicks.getPicks();
    if (cp.size() != 1)
      throw std::runtime_error("Wrong number of linked picks");
    
    tls::Resolver resolver(pli);
    if (!resolver.resolve(cp.front()))
      throw std::runtime_error("Couldn't resolve linked pick");
    
    auto csysPrms = resolver.getFeature()->getParameters(prm::Tags::CSys);
    if (csysPrms.empty())
      throw std::runtime_error("Linked feature has no csys parameter");
    const auto &cm = csysPrms.front()->getMatrix();
    
    osg::Matrixd newSys = applyOffset(cm, offsetVector.getVector());
    
    prm::ObserverBlocker blocker(csysObserver);
    csys.setValue(newSys);
    csysDragger.draggerUpdate();
  }
  
  void update3Points(const UpdatePayload &pIn)
  {
    tls::Resolver pr(pIn);
    std::vector<osg::Vec3d> points;
    for (const auto &p : pointsPicks.getPicks())
    {
      if (!pr.resolve(p))
        throw std::runtime_error("Through3P: Pick resolution failed");
      auto tps = pr.getPoints();
      if (tps.empty())
        throw std::runtime_error("Through3P: No resolved points");
      if (tps.size() > 1)
      {
        std::ostringstream s; s << "WARNING: Through3P: more than one resolved point. Using first" << std::endl;
        feature.lastUpdateLog += s.str();
      }
      points.push_back(tps.front());
    }
    if (points.size() != 3)
      throw std::runtime_error("Through3P: couldn't get 3 points");
    std::array<std::optional<osg::Vec3d>, 4> oPoints;
    oPoints[0] = points[0];
    oPoints[1] = points[1];
    oPoints[2] = points[2];
    
    auto ocsys = tls::matrixFromPoints(oPoints);
    if (!ocsys)
      throw std::runtime_error("Through3P: couldn't derive matrix from 3 points");
    
    osg::Matrixd newSys = applyOffset(*ocsys, offsetVector.getVector());
    
    prm::ObserverBlocker blocker(csysObserver);
    csys.setValue(newSys);
    csysDragger.draggerUpdate();
    
    osg::BoundingSphere bs;
    bs.expandBy(points.at(0));
    bs.expandBy(points.at(1));
    bs.expandBy(points.at(2));
    cachedSize = bs.radius();
  }
  
  void updateVisual()
  {
    if (autoSize.getBool())
    {
      prm::ObserverBlocker blocker(sizeObserver);
      size.setValue(cachedSize);
    }
    
    const osg::Matrixd &cs = csys.getMatrix(); //current system
    feature.mainTransform->setMatrix(cs);
    
    double ts = size.getDouble();
    scale->setScale(osg::Vec3d(ts, ts, ts));
    
    double lo = ts * 1.2; //label offset
    osg::Vec3d asll = osg::Vec3d (lo, 0, 0) * cs;
    autoSizeLabel->setMatrix(osg::Matrixd::translate(asll));
    
    osg::Vec3d sll = osg::Vec3d (0, lo, 0) * cs;
    sizeLabel->setMatrix(osg::Matrixd::translate(sll));
    
    osg::Vec3d ovll = osg::Vec3d (0, 0, 0) * cs;
    offsetVectorLabel->setMatrix(osg::Matrixd::translate(ovll));
  }
};


Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionDatumSystem.svg");
  
  name = QObject::tr("Datum System");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  stow->prmActiveSync();
}

Feature::~Feature() = default;

void Feature::updateModel(const UpdatePayload &pIn)
{
  setFailure();
  lastUpdateLog.clear();
  try
  {
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    auto sysType = static_cast<SystemType>(stow->systemType.getInt());
    switch (sysType)
    {
      case Constant:{
        stow->csysDragger.draggerUpdate();
        break;}
      case Linked:{
        stow->updateLinked(pIn);
        break;}
      case Through3Points:{
        stow->update3Points(pIn);
        break;}
    }
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in " << getTypeString() << " update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in " << getTypeString() << " update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in " << getTypeString() << " update." << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Feature::updateVisual()
{
  stow->updateVisual();
  setVisualClean();
}

void Feature::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::dtms::DatumSystem so
  (
    Base::serialOut()
    , stow->systemType.serialOut()
    , stow->csys.serialOut()
    , stow->autoSize.serialOut()
    , stow->size.serialOut()
    , stow->offsetVector.serialOut()
    , stow->csysDragger.serialOut()
    , stow->autoSizeLabel->serialOut()
    , stow->sizeLabel->serialOut()
    , stow->offsetVectorLabel->serialOut()
    , stow->cachedSize
  );
  
  auto ct = static_cast<SystemType>(stow->systemType.getInt());
  if (ct == Linked)
    so.picks() = stow->linkedPicks.serialOut();
  else if (ct == Through3Points)
    so.picks() = stow->pointsPicks.serialOut();
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::dtms::datumsystem(stream, so, infoMap);
}

void Feature::serialRead(const prj::srl::dtms::DatumSystem &so)
{
  Base::serialIn(so.base());
  stow->systemType.serialIn(so.systemType());
  stow->csys.serialIn(so.csys());
  stow->autoSize.serialIn(so.autoSize());
  stow->size.serialIn(so.size());
  stow->offsetVector.serialIn(so.offsetVector());
  stow->csysDragger.serialIn(so.csysDragger());
  stow->autoSizeLabel->serialIn(so.autoSizeLabel());
  stow->sizeLabel->serialIn(so.sizeLabel());
  stow->offsetVectorLabel->serialIn(so.offsetVectorLabel());
  stow->cachedSize = so.cachedSize();
  
  auto ct = static_cast<SystemType>(stow->systemType.getInt());
  if (ct == Linked && so.picks())
    stow->linkedPicks.serialIn(so.picks().get());
  else if (ct == Through3Points && so.picks())
    stow->pointsPicks.serialIn(so.picks().get());
  
  mainTransform->setMatrix(stow->csys.getMatrix());
}
