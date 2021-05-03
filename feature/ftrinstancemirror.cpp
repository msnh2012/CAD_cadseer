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

#include <gp_Ax2.hxx>
#include <TopoDS.hxx>
#include <BRepBuilderAPI_Transform.hxx>

#include "globalutilities.h"
#include "tools/occtools.h"
#include "library/lbrcsysdragger.h"
#include "library/lbrplabel.h"
#include "annex/annseershape.h"
#include "annex/anncsysdragger.h"
#include "annex/anninstancemapper.h"
#include "feature/ftrdatumplane.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "project/serial/generated/prjsrlinmsinstancemirror.h"
#include "tools/featuretools.h"
#include "feature/ftrshapecheck.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinstancemirror.h"

using namespace ftr::InstanceMirror;

using boost::uuids::uuid;

QIcon Feature::icon;


struct Feature::Stow
{
  Feature &feature;
  
  prm::Parameter planeType{QObject::tr("Plane Type"), 0, Tags::PlaneType};
  prm::Parameter csys{prm::Names::CSys, osg::Matrixd::identity(), prm::Tags::CSys};
  prm::Parameter source{QObject::tr("Source"), ftr::Picks(), Tags::Source};
  prm::Parameter plane{QObject::tr("Plane"), ftr::Picks(), Tags::Plane};
  prm::Parameter includeSource{QObject::tr("Include Source"), true, Tags::IncludeSource};

  prm::Observer csysObserver;
  prm::Observer syncObserver;

  ann::SeerShape sShape;
  ann::InstanceMapper iMapper;
  ann::CSysDragger csysDragger;

  osg::ref_ptr<lbr::PLabel> includeSourceLabel = new lbr::PLabel(&includeSource);
  
  Stow() = delete;
  Stow(Feature &fIn)
  : feature(fIn)
  , csysObserver(std::bind(&Feature::setModelDirty, &feature))
  , syncObserver(std::bind(&Stow::prmActiveSync, this))
  , csysDragger(&feature, &csys)
  {
    QStringList tStrings = //keep in sync with enum in header.
    {
      QObject::tr("Constant")
      , QObject::tr("Pick")
    };
    planeType.setEnumeration(tStrings);
    planeType.connect(syncObserver);
    planeType.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&planeType);
    
    csys.connect(csysObserver);
    feature.parameters.push_back(&csys);
    
    auto connectAndPush = [&](prm::Parameter &prmIn)
    {
      prmIn.connectValue(std::bind(&Feature::setModelDirty, &feature));
      feature.parameters.push_back(&prmIn);
    };
    connectAndPush(source);
    connectAndPush(plane);
    connectAndPush(includeSource);
    
    feature.annexes.insert(std::make_pair(ann::Type::SeerShape, &sShape));
    feature.annexes.insert(std::make_pair(ann::Type::InstanceMapper, &iMapper));
    feature.annexes.insert(std::make_pair(ann::Type::CSysDragger, &csysDragger));
    
    feature.overlaySwitch->addChild(includeSourceLabel.get());
    
    csysDragger.dragger->unlinkToMatrix(feature.getMainTransform());
    csysDragger.dragger->hide(lbr::CSysDragger::SwitchIndexes::LinkIcon);
    feature.overlaySwitch->addChild(csysDragger.dragger);
  }
  
  void prmActiveSync()
  {
    //shouldn't need to block anything.
    auto ct = static_cast<PlaneType>(planeType.getInt());
    if (ct == Constant)
    {
      csysDragger.draggerUpdate();
      csys.setActive(true);
      plane.setActive(false);
    }
    else if (ct == Pick)
    {
      csys.setActive(false);
      plane.setActive(true);
    }
  }
};

Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionInstanceMirror.svg");
  
  name = QObject::tr("Instance Mirror");
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
    //no new failure state.
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    //get the shapes to mirror.
    const auto &picks = stow->source.getPicks();
    if (picks.empty())
      throw std::runtime_error("No source shapes to mirror");
    
    tls::Resolver resolver(payloadIn);
    resolver.resolve(picks.front());
    occt::ShapeVector tShapes = resolver.getShapes();
    if (tShapes.empty())
      throw std::runtime_error("Empty pick resolution");
    assert(resolver.getSeerShape());
    const ann::SeerShape &tss = *resolver.getSeerShape();
    if (slc::isObjectType(picks.front().selectionType))
    {
      assert(tShapes.size() == 1);
      assert(tShapes.front().ShapeType() == TopAbs_COMPOUND);
      tShapes = occt::getNonCompounds(tShapes.front());
    }
    if (tShapes.empty())
      throw std::runtime_error("No shapes found.");
    
    //get the plane
    auto ct = static_cast<PlaneType>(stow->planeType.getInt());
    //nothing to do if it is constant.
    if (ct == Pick)
    {
      prm::ObserverBlocker block(stow->csysObserver);
      const auto &pp = stow->plane.getPicks();
      if (pp.empty())
        throw std::runtime_error("No plane from pick");
      if (!resolver.resolve(pp.front()))
        throw std::runtime_error("Resolution of plane pick failed");
      if (resolver.getFeature())
      {
        if (slc::isObjectType(pp.front().selectionType))
        {
          auto prms = resolver.getFeature()->getParameters(prm::Tags::CSys);
          if (prms.empty())
            throw std::runtime_error("Couldn't get parameter with csys tag");
          stow->csys.setValue(prms.front()->getMatrix());
        }
        else if (resolver.getSeerShape())
        {
          occt::ShapeVector pShapes = resolver.getShapes();
          if (pShapes.empty())
            throw std::runtime_error("no mirror plane shape");
          if (pShapes.size() > 1)
          {
            std::ostringstream s; s << "WARNING: more than 1 resolved shape for mirror plane. Using first." << std::endl;
            lastUpdateLog += s.str();
          }

          TopoDS_Shape dsShape = pShapes.front();
          if (dsShape.IsNull())
            throw std::runtime_error("couldn't find occt shape");
          if(dsShape.ShapeType() == TopAbs_FACE)
          {
            auto axisPair = occt::gleanAxis(dsShape);
            if (!axisPair.second)
              throw std::runtime_error("Could glean axis from face");
            osg::Vec3d origin = gu::toOsg(axisPair.first.Location());
            osg::Vec3d norm = gu::toOsg(axisPair.first.Direction());
            const osg::Matrixd &cs = stow->csys.getMatrix(); // current system
            osg::Vec3d cz = gu::getZVector(cs);
            if (norm.isNaN())
              throw std::runtime_error("invalid normal from face");
            osg::Matrixd workSystem = cs * osg::Matrixd::rotate(cz, norm);
            workSystem.setTrans(origin);
            stow->csys.setValue(workSystem);
          }
          else
            throw std::runtime_error("unsupported occt shape type");
        }
        else
          throw std::runtime_error("feature doesn't have seer shape");
      }
    }
    
    gp_Trsf mt; //mirror transform.
    mt.SetMirror(gu::toOcc(stow->csys.getMatrix()));
    BRepBuilderAPI_Transform bt(mt);
    
    occt::ShapeVector out;
    for (const auto &tShape : tShapes)
    {
      if (stow->includeSource.getBool())
        out.push_back(tShape);
      bt.Perform(tShape);
      out.push_back(bt.Shape());
    }
    
    TopoDS_Compound result = occt::ShapeVectorCast(out);
    
    ShapeCheck check(result);
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    stow->sShape.setOCCTShape(result, getId());
    
    for (const auto &s : tShapes)
    {
      stow->iMapper.startMapping(tss, tss.findId(s),  payloadIn.shapeHistory);
      std::size_t count = 0;
      for (const auto &si : out)
      {
        stow->iMapper.mapIndex(stow->sShape, si, count);
        count++;
      }
    }
    
    occt::BoundingBox bb(tShapes);
    stow->includeSourceLabel->setMatrix
      (osg::Matrixd::translate(gu::toOsg(bb.getCenter()) + osg::Vec3d(0.0, 0.0, -bb.getHeight() / 2.0)));
    stow->csysDragger.draggerUpdate();
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in instance mirror update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in instance mirror update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in instance mirror update. " << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Feature::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::inms::InstanceMirror so
  (
    Base::serialOut(),
    stow->planeType.serialOut(),
    stow->csys.serialOut(),
    stow->source.serialOut(),
    stow->plane.serialOut(),
    stow->includeSource.serialOut(),
    stow->sShape.serialOut(),
    stow->iMapper.serialOut(),
    stow->csysDragger.serialOut(),
    stow->includeSourceLabel->serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::inms::instanceMirror(stream, so, infoMap);
}

void Feature::serialRead(const prj::srl::inms::InstanceMirror &sim)
{
  Base::serialIn(sim.base());
  stow->planeType.serialIn(sim.planeType());
  stow->csys.serialIn(sim.csys());
  stow->source.serialIn(sim.source());
  stow->plane.serialIn(sim.plane());
  stow->includeSource.serialIn(sim.includeSource());
  stow->sShape.serialIn(sim.seerShape());
  stow->iMapper.serialIn(sim.instanceMaps());
  stow->csysDragger.serialIn(sim.csysDragger());
  stow->includeSourceLabel->serialIn(sim.includeSourceLabel());
}
