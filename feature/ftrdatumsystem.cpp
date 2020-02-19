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

osg::Matrixd applyOffset(osg::Matrixd mIn, const osg::Vec3d &offset)
{
  if (!offset.valid() || offset.length() < std::numeric_limits<float>::epsilon())
    return mIn;
  mIn.setTrans(offset * mIn);
  return mIn;
}


Feature::Feature()
: Base()
, csys(new prm::Parameter(prm::Names::CSys, osg::Matrixd::identity()))
, autoSize(new prm::Parameter(QObject::tr("Auto Size"), false))
, size(new prm::Parameter(QObject::tr("Size"), 1.0))
, offsetVector(new prm::Parameter(prm::Names::Offset, osg::Vec3d()))
, csysDragger(new ann::CSysDragger(this, csys.get()))
, display(new mdv::DatumSystem())
, autoSizeLabel(new lbr::PLabel(autoSize.get()))
, sizeLabel(new lbr::PLabel(size.get()))
, offsetVectorLabel(new lbr::PLabel(offsetVector.get()))
, scale(new osg::PositionAttitudeTransform())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionDatumSystem.svg");
  
  name = QObject::tr("Datum System");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  csys->connectValue(std::bind(&Feature::setModelDirty, this));
  parameters.push_back(csys.get());
  
  autoSize->connectValue(std::bind(&Feature::setModelDirty, this));
  parameters.push_back(autoSize.get());
  
  size->connectValue(std::bind(&Feature::setModelDirty, this));
  parameters.push_back(size.get());
  
  offsetVector->connectValue(std::bind(&Feature::setModelDirty, this));
  parameters.push_back(offsetVector.get());
  
  csysDragger->dragger->hide(lbr::CSysDragger::SwitchIndexes::LinkIcon);
  annexes.insert(std::make_pair(ann::Type::CSysDragger, csysDragger.get()));
  //update will control when this is added to overlay.
  
  autoSizeLabel->showName = true;
  autoSizeLabel->valueHasChanged();
  autoSizeLabel->constantHasChanged();
  //update will control when this is added to overlay.
  
  sizeLabel->showName = true;
  sizeLabel->valueHasChanged();
  sizeLabel->constantHasChanged();
  //update will control when this is added to overlay.
  
  offsetVectorLabel->showName = true;
  offsetVectorLabel->valueHasChanged();
  offsetVectorLabel->constantHasChanged();
  //update will control when this is added to overlay.
  
  double temp = static_cast<double>(*size);
  scale->setScale(osg::Vec3d(temp, temp, temp));
  mainTransform->addChild(scale.get());
  
  scale->addChild(display.get());
}

Feature::~Feature() = default;

void Feature::setCue(const Cue &cIn)
{
  cue = cIn;
  setModelDirty();
}

void Feature::setCSys(const osg::Matrixd &csysIn)
{
  cue.systemType = SystemType::Constant;
  
  osg::Matrixd oldSystem = static_cast<osg::Matrixd>(*csys);
  if (!csys->setValue(csysIn))
    return; // already at this csys
    
  //apply the same transformation to dragger, so dragger moves with it.
  osg::Matrixd diffMatrix = osg::Matrixd::inverse(oldSystem) * csysIn;
  csysDragger->draggerUpdate(csysDragger->dragger->getMatrix() * diffMatrix);
}

osg::Matrixd Feature::getCSys() const
{
  return static_cast<osg::Matrixd>(*csys);
}

void Feature::setSize(double sizeIn)
{
  size->setValue(sizeIn);
}

void Feature::setAutoSize(bool vIn)
{
  autoSize->setValue(vIn);
}

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
    
    if (cue.systemType == SystemType::Constant)
      updateNone(pIn);
    else if (cue.systemType == SystemType::Through3Points)
      update3Points(pIn);
    
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

void Feature::updateNone(const UpdatePayload&)
{
}

void Feature::updateLinked(const UpdatePayload &pIn)
{
  auto linkedFeatures = pIn.getFeatures(InputType::linkCSys);
  if (linkedFeatures.empty())
    throw std::runtime_error("No Features for linked csys");
  if (!linkedFeatures.front()->hasParameter(prm::Names::CSys))
    throw std::runtime_error("Feature doesn't have csys parameter");
  
  osg::Matrixd newSys = applyOffset
  (
    static_cast<osg::Matrixd>(*(linkedFeatures.front()->getParameter(prm::Names::CSys)))
    , static_cast<osg::Vec3d>(*offsetVector)
  );
  
  csys->setValueQuiet(newSys);
  csysDragger->draggerUpdate();
}

void Feature::update3Points(const UpdatePayload &pIn)
{
  //for now I just copied this from datum plane.
  if (cue.picks.size() != 3)
    throw std::runtime_error("Through3P: Wrong number of picks");
  
  tls::Resolver pr(pIn);
  std::vector<osg::Vec3d> points;
  for (const auto &p : cue.picks)
  {
    if (!pr.resolve(p))
      throw std::runtime_error("Through3P: Pick resolution failed");
    auto tps = pr.getPoints();
    if (tps.empty())
      throw std::runtime_error("Through3P: No resolved points");
    if (tps.size() > 1)
    {
      std::ostringstream s; s << "WARNING: Through3P: more than one resolved point. Using first" << std::endl;
      lastUpdateLog += s.str();
    }
    points.push_back(tps.front());
  }
  if (points.size() != 3)
    throw std::runtime_error("Through3P: couldn't get 3 points");
  
  auto ocsys = tls::matrixFrom3Points(points);
  if (!ocsys)
    throw std::runtime_error("Through3P: couldn't derive matrix from 3 points");
  
  osg::Matrixd newSys = applyOffset(ocsys.get(), static_cast<osg::Vec3d>(*offsetVector));
  
  csys->setValueQuiet(newSys);
  csysDragger->draggerUpdate();
  
  if (static_cast<bool>(*autoSize))
  {
    osg::BoundingSphere bs;
    bs.expandBy(points.at(0));
    bs.expandBy(points.at(1));
    bs.expandBy(points.at(2));
    size->setValueQuiet(bs.radius());
    sizeLabel->valueHasChanged();
  }
}

void Feature::updateVisual()
{
  updateVisualInternal();
  setVisualClean();
}

void Feature::updateVisualInternal()
{
  mainTransform->setMatrix(static_cast<osg::Matrixd>(*csys));
  
  auto addLabel = [&](osg::Node *child)
  {
    //don't add duplicate
    if (!overlaySwitch->containsNode(child))
      overlaySwitch->addChild(child);
  };
  auto removeLabel = [&](osg::Node *child)
  {
    //looked at osg code and we don't need to test for child present. it will ignore if not.
    overlaySwitch->removeChild(child);
  };
  
  if (cue.systemType == SystemType::Constant)
  {
    addLabel(csysDragger->dragger.get());
    removeLabel(offsetVectorLabel.get());
    addLabel(sizeLabel.get());
    removeLabel(autoSizeLabel.get());
  }
  else if (cue.systemType == SystemType::Linked)
  {
    //force size for now. maybe we should try to derive a size from input feature?
    removeLabel(csysDragger->dragger.get());
    addLabel(offsetVectorLabel.get());
    removeLabel(autoSizeLabel.get());
    addLabel(sizeLabel.get());
  }
  else
  {
    removeLabel(csysDragger->dragger.get());
    addLabel(offsetVectorLabel.get());
    if (static_cast<bool>(*autoSize))
      removeLabel(sizeLabel.get());
    else
      addLabel(sizeLabel.get());
    addLabel(autoSizeLabel.get());
  }
  
  osg::Matrixd cs = static_cast<osg::Matrixd>(*csys); //current system
  
  double ts = static_cast<double>(*size);
  scale->setScale(osg::Vec3d(ts, ts, ts));
  
  double lo = ts * 1.2; //label offset
  osg::Vec3d asll = osg::Vec3d (lo, 0, 0) * cs;
  autoSizeLabel->setMatrix(osg::Matrixd::translate(asll));
  
  osg::Vec3d sll = osg::Vec3d (0, lo, 0) * cs;
  sizeLabel->setMatrix(osg::Matrixd::translate(sll));
  
  osg::Vec3d ovll = osg::Vec3d (0, 0, 0) * cs;
  offsetVectorLabel->setMatrix(osg::Matrixd::translate(ovll));
}

void Feature::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::dtms::DatumSystem so
  (
    Base::serialOut()
    , static_cast<int>(cue.systemType)
    , csys->serialOut()
    , autoSize->serialOut()
    , size->serialOut()
    , offsetVector->serialOut()
    , csysDragger->serialOut()
    , autoSizeLabel->serialOut()
    , sizeLabel->serialOut()
    , offsetVectorLabel->serialOut()
  );
  for (const auto &p : cue.picks)
    so.picks().push_back(p);
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::dtms::datumsystem(stream, so, infoMap);
}

void Feature::serialRead(const prj::srl::dtms::DatumSystem &so)
{
  Base::serialIn(so.base());
  cue.systemType = static_cast<DatumSystem::SystemType>(so.systemType());
  csys->serialIn(so.csys());
  autoSize->serialIn(so.autoSize());
  size->serialIn(so.size());
  offsetVector->serialIn(so.offsetVector());
  csysDragger->serialIn(so.csysDragger());
  autoSizeLabel->serialIn(so.autoSizeLabel());
  sizeLabel->serialIn(so.sizeLabel());
  offsetVectorLabel->serialIn(so.offsetVectorLabel());
  for (const auto &p : so. picks())
    cue.picks.emplace_back(p);
  
  updateVisualInternal();
}
