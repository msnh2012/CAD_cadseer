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

#include <osg/Switch>

#include <TopoDS.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>

#include "globalutilities.h"
#include "tools/occtools.h"
#include "library/lbrplabel.h"
#include "annex/annseershape.h"
#include "feature/ftrshapecheck.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinputtype.h"
#include "tools/featuretools.h"
#include "tools/idtools.h"
#include "project/serial/generated/prjsrlrvlsrevolve.h"
#include "feature/ftrrevolve.h"

using namespace ftr::Revolve;
using boost::uuids::uuid;
QIcon Feature::icon = QIcon(":/resources/images/constructionRevolve.svg");

struct Feature::Stow
{
  Feature &feature;
  prm::Parameter axisType{QObject::tr("Axis Type"), 0, PrmTags::axisType};
  prm::Parameter profilePicks{QObject::tr("Profile"), ftr::Picks(), PrmTags::profilePicks};
  prm::Parameter axisPicks{QObject::tr("Axis Picks"), ftr::Picks(), PrmTags::axisPicks};
  prm::Parameter axisOrigin{QObject::tr("Axis Origin"), osg::Vec3d(), PrmTags::axisOrigin};
  prm::Parameter axisDirection{QObject::tr("Axis Direction"), osg::Vec3d(), PrmTags::axisDirection};
  prm::Parameter angle{prm::Names::Angle, 360.0, prm::Tags::Angle};
  prm::Observer syncObserver{std::bind(&Stow::prmActiveSync, this)};
  prm::Observer blockObserver{std::bind(&Feature::setModelDirty, &feature)};
  
  ann::SeerShape sShape;
  
  osg::ref_ptr<lbr::PLabel> axisTypeLabel{new lbr::PLabel(&axisType)};
  osg::ref_ptr<lbr::PLabel> axisOriginLabel{new lbr::PLabel(&axisOrigin)};
  osg::ref_ptr<lbr::PLabel> axisDirectionLabel{new lbr::PLabel(&axisDirection)};
  osg::ref_ptr<lbr::PLabel> angleLabel{new lbr::PLabel(&angle)};
  
  std::map<uuid, uuid> generatedMap; //map transition shapes.
  std::map<uuid, uuid> lastMap; //map 'top' shapes.
  std::map<uuid, uuid> oWireMap; //map new face to outer wire.
  
  Stow() = delete;
  Stow(Feature &fIn)
  : feature(fIn)
  {
    QStringList types =
    {
      QObject::tr("Picks")
      , QObject::tr("Parameters")
    };
    axisType.setEnumeration(types);
    axisType.connectValue(std::bind(&Feature::setModelDirty, &feature));
    axisType.connect(syncObserver);
    feature.parameters.push_back(&axisType);
    
    profilePicks.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&profilePicks);
    
    axisPicks.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&axisPicks);
    
    axisOrigin.connect(blockObserver);
    feature.parameters.push_back(&axisOrigin);
    
    axisDirection.connect(blockObserver);
    feature.parameters.push_back(&axisDirection);
    
    angle.setConstraint(prm::Constraint::buildNonZeroAngle());
    angle.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&angle);
    
    feature.annexes.insert(std::make_pair(ann::Type::SeerShape, &sShape));
    
    feature.overlaySwitch->addChild(axisTypeLabel.get());
    feature.overlaySwitch->addChild(axisOriginLabel.get());
    feature.overlaySwitch->addChild(axisDirectionLabel.get());
    feature.overlaySwitch->addChild(angleLabel.get());
    
    prmActiveSync();
    axisTypeLabel->refresh(); //need to update after set enumeration.
  }
  
  void prmActiveSync()
  {
    int type = axisType.getInt();
    if (type == 0) //picks
    {
      axisPicks.setActive(true);
      axisOrigin.setActive(false);
      axisDirection.setActive(false);
    }
    else if (type == 1) //parameters
    {
      axisPicks.setActive(false);
      axisOrigin.setActive(true);
      axisDirection.setActive(true);
    }
  }
  
  void updateLabels(occt::BoundingBox &bb)
  {
    axisOriginLabel->setMatrix(osg::Matrixd::translate(axisOrigin.getVector()));
    axisDirectionLabel->setMatrix(osg::Matrixd::translate(axisOrigin.getVector() + axisDirection.getVector()));
    angleLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(bb.getCenter())));
    
    osg::Vec3d projection = axisDirection.getVector() * 0.5;
    osg::Vec3d location = axisOrigin.getVector() + projection;
    axisTypeLabel->setMatrix(osg::Matrixd::translate(location));
  }
};

Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  name = QObject::tr("Revolve");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
}

Feature::~Feature() = default;

void Feature::updateModel(const UpdatePayload &pIn)
{
  setFailure();
  lastUpdateLog.clear();
  stow->sShape.reset();
  try
  {
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    prm::ObserverBlocker block(stow->blockObserver);
    const auto &picks = stow->profilePicks.getPicks();
    if (picks.size() != 1) //only 1 for now.
      throw std::runtime_error("wrong number of target picks.");
    tls::Resolver tResolver(pIn);
    tResolver.resolve(picks.front());
    if (!tResolver.getFeature() || !tResolver.getSeerShape() || tResolver.getShapes().empty())
      throw std::runtime_error("target resolution failed.");
    
    occt::ShapeVector str; //shapes to revolve
    if (slc::isObjectType(picks.front().selectionType))
    {
      //treat sketches special
      if (tResolver.getFeature()->getType() == Type::Sketch)
      {
        occt::ShapeVectorCast cast(TopoDS::Compound(tResolver.getSeerShape()->getRootOCCTShape()));
        occt::WireVector wv = static_cast<occt::WireVector>(cast);
        if (wv.empty())
          throw std::runtime_error("No wires found in sketch");
        TopoDS_Face ftr = occt::buildFace(wv);
        if (ftr.IsNull())
          throw std::runtime_error("Face construction for sketch failed");
        ShapeCheck fc(ftr);
        if (!fc.isValid())
          throw std::runtime_error("Face check failed");
        str.push_back(ftr);
      }
      else //not a sketch
      {
        occt::ShapeVector shapes = tResolver.getSeerShape()->useGetNonCompoundChildren();
        for (const auto &s : shapes)
        {
          TopAbs_ShapeEnum t = s.ShapeType();
          if
          (
            t == TopAbs_SHELL
            || t == TopAbs_FACE
            || t == TopAbs_WIRE
            || t == TopAbs_EDGE
            || t == TopAbs_VERTEX
          )
          str.push_back(s);
        }
      }
    }
    else
    {
      str = tResolver.getShapes();
    }
    
    if (str.empty())
      throw std::runtime_error("Nothing to revolve");
    occt::BoundingBox bb;
    bb.add(str);
    int axisType = stow->axisType.getInt();
    const auto &axisPicks = stow->axisPicks.getPicks();
    if (axisType == 0) //picks
    {
      if (axisPicks.empty() || axisPicks.size() > 2)
        throw std::runtime_error("Wrong number of axis picks");
      tls::Resolver aResolver(pIn);
      if (axisPicks.size() == 1)
      {
        if (!aResolver.resolve(axisPicks.front()))
          throw std::runtime_error("Unable to resolve single axis pick");
        if (slc::isObjectType(axisPicks.front().selectionType))
        {
          const auto originPrms = aResolver.getFeature()->getParameters(prm::Tags::Origin);
          const auto directionPrms = aResolver.getFeature()->getParameters(prm::Tags::Direction);
          if (!originPrms.empty() && !directionPrms.empty())
          {
            stow->axisOrigin.setValue(originPrms.front()->getVector());
            stow->axisDirection.setValue(directionPrms.front()->getVector());
          }
        }
        else
        {
          if (!aResolver.getSeerShape())
            throw std::runtime_error("No seer shape for single axis pick");
          auto rShapes = aResolver.getShapes();
          if (rShapes.empty())
            throw std::runtime_error("No resolved shapes from single axis pick");
          if (rShapes.size() > 1)
          {
            std::ostringstream s; s << "WARNING: Multiple resolve shapes from single axis pick " << std::endl;
            lastUpdateLog += s.str();
          }
          auto glean = occt::gleanAxis(rShapes.front());
          if (!glean.second)
            throw std::runtime_error("Couldn't glean axis from single axis pick");
          stow->axisOrigin.setValue(gu::toOsg(glean.first.Location()));
          stow->axisDirection.setValue(gu::toOsg(glean.first.Direction()));
        }
      }
      else //axisPicks size == 2
      {
        if (!slc::isPointType(axisPicks.front().selectionType) || !slc::isPointType(axisPicks.back().selectionType))
          throw std::runtime_error("Wrong type for 2 pick axis");
        aResolver.resolve(axisPicks.front());
        auto points0 = aResolver.getPoints();
        aResolver.resolve(axisPicks.back());
        auto points1 = aResolver.getPoints();
        if (points0.empty() || points1.empty())
          throw std::runtime_error("Failed to resolve 2 axis pick points");
        if (points0.size() > 1 || points1.size() > 1)
        {
          std::ostringstream s; s << "WARNING: More than 1 shape resolved for 2 pick axis." << std::endl;
          lastUpdateLog += s.str();
        }
        osg::Vec3d tAxis = points0.front() - points1.front();
        tAxis.normalize();
        stow->axisOrigin.setValue(points1.front());
        stow->axisDirection.setValue(tAxis);
      }
    }
    
    TopoDS_Compound strc = static_cast<TopoDS_Compound>(occt::ShapeVectorCast(str));
    gp_Ax1 axis
    (
      gp_Pnt(gu::toOcc(stow->axisOrigin.getVector()).XYZ()),
      gp_Dir(gu::toOcc(stow->axisDirection.getVector()))
    );
    double la = stow->angle.getDouble();
    BRepPrimAPI_MakeRevol revolver(strc, axis, osg::DegreesToRadians(la));
    revolver.Check(); //throw occ exception if failed
    
    stow->sShape.setOCCTShape(revolver.Shape(), getId());
    stow->sShape.shapeMatch(*tResolver.getSeerShape());
    
    int count = -1;
    for (const auto &s : occt::mapShapes(strc))
    {
      count++;
      if (!tResolver.getSeerShape()->hasShape(s))
        continue; //skip at least the generated compound.
      uuid oldId = tResolver.getSeerShape()->findId(s);
      
      //because we don't move the original shapes we use SeerShape::shapeMatch
      //and we can ignore id mapping for the original shapes.
      
      //this is for the 'transition' shapes and uses generated map.
      //transition from FirstShapes to LastShapes.
      //I am getting a wire that generates a shell but the shell is not in the shape. bug?
      const TopTools_ListOfShape &generated = revolver.Generated(s);
      if (!generated.IsEmpty() && stow->sShape.hasShape(generated.First()))
      {
        uuid gId = gu::createRandomId();
        auto gIt = stow->generatedMap.find(oldId);
        if (gIt == stow->generatedMap.end())
          stow->generatedMap.insert(std::make_pair(oldId, gId));
        else
          gId = gIt->second;
        stow->sShape.updateId(generated.First(), gId);
        if (!stow->sShape.hasEvolveRecord(gu::createNilId(), gId))
        {
          assert(!stow->sShape.hasEvolveRecordOut(gId));
          stow->sShape.insertEvolve(gu::createNilId(), gId);
        }
        if (generated.Extent() > 1)
          std::cout << "Warning: more than 1 generated shape in revolve." << std::endl;
      }
      
      //this is for the new shapes at the top of the prism.
      //in BRepPrimAPI_MakePrism terminology these are 'LastShape'
      const TopoDS_Shape &last = revolver.LastShape(s);
      if (!last.IsNull() && stow->sShape.hasShape(last))
      {
        uuid lId = gu::createRandomId();
        auto lIt = stow->lastMap.find(oldId);
        if (lIt == stow->lastMap.end())
          stow->lastMap.insert(std::make_pair(oldId, lId));
        else
          lId = lIt->second;
        stow->sShape.updateId(last, lId);
        if (!stow->sShape.hasEvolveRecord(gu::createNilId(), lId))
        {
          assert(!stow->sShape.hasEvolveRecordOut(lId));
          stow->sShape.insertEvolve(gu::createNilId(), lId);
        }
      }
    }
    
    stow->sShape.outerWireMatch(*tResolver.getSeerShape());
    
    //this should assign ids to nil outerwires based upon parent face id.
    occt::ShapeVector ns = stow->sShape.getAllNilShapes();
    for (const auto &s : ns)
    {
      if (s.ShapeType() != TopAbs_WIRE)
        continue;
      occt::ShapeVector ps = stow->sShape.useGetParentsOfType(s, TopAbs_FACE);
      if (ps.empty())
        continue;
      uuid fId = stow->sShape.findId(ps.front());
      if (fId.is_nil())
        continue;
      uuid wId = gu::createRandomId();
      auto it = stow->oWireMap.find(fId);
      if (it == stow->oWireMap.end())
        stow->oWireMap.insert(std::make_pair(fId, wId));
      else
        wId = it->second;
      stow->sShape.updateId(s, wId);
      if (!stow->sShape.hasEvolveRecord(gu::createNilId(), wId))
      {
        assert(!stow->sShape.hasEvolveRecordOut(wId));
        stow->sShape.insertEvolve(gu::createNilId(), wId);
      }
    }
    
    stow->sShape.derivedMatch();
    stow->sShape.uniqueTypeMatch(*tResolver.getSeerShape());
    stow->sShape.dumpNils("revolve feature");
    stow->sShape.dumpDuplicates("revolve feature");
    stow->sShape.ensureNoNils();
    stow->sShape.ensureNoDuplicates();
    
    stow->updateLabels(bb);
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in revolve update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in revolve update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in revolve update. " << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Feature::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::rvls::Revolve so
  (
    Base::serialOut()
    , stow->axisType.serialOut()
    , stow->profilePicks.serialOut()
    , stow->axisPicks.serialOut()
    , stow->axisOrigin.serialOut()
    , stow->axisDirection.serialOut()
    , stow->angle.serialOut()
    , stow->sShape.serialOut()
    , stow->axisTypeLabel->serialOut()
    , stow->axisOriginLabel->serialOut()
    , stow->axisDirectionLabel->serialOut()
    , stow->angleLabel->serialOut()
  );

  for (const auto &p : stow->generatedMap)
    so.generatedMap().push_back(prj::srl::spt::EvolveRecord(gu::idToString(p.first), gu::idToString(p.second)));
  for (const auto &p : stow->lastMap)
    so.lastMap().push_back(prj::srl::spt::EvolveRecord(gu::idToString(p.first), gu::idToString(p.second)));
  for (const auto &p : stow->oWireMap)
    so.oWireMap().push_back(prj::srl::spt::EvolveRecord(gu::idToString(p.first), gu::idToString(p.second)));
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::rvls::revolve(stream, so, infoMap);
}

void Feature::serialRead(const prj::srl::rvls::Revolve &so)
{
  Base::serialIn(so.base());
  stow->axisType.serialIn(so.axisType());
  stow->profilePicks.serialIn(so.profilePicks());
  stow->axisPicks.serialIn(so.axisPicks());
  stow->axisOrigin.serialIn(so.axisOrigin());
  stow->axisDirection.serialIn(so.axisDirection());
  stow->angle.serialIn(so.angle());
  stow->sShape.serialIn(so.seerShape());
  stow->axisTypeLabel->serialIn(so.axisTypeLabel());
  stow->axisOriginLabel->serialIn(so.axisOriginLabel());
  stow->axisDirectionLabel->serialIn(so.axisDirectionLabel());
  stow->angleLabel->serialIn(so.angleLabel());
  
  for (const auto &p : so.generatedMap())
    stow->generatedMap.insert(std::make_pair(gu::stringToId(p.idIn()), gu::stringToId(p.idOut())));
  for (const auto &p : so.lastMap())
    stow->lastMap.insert(std::make_pair(gu::stringToId(p.idIn()), gu::stringToId(p.idOut())));
  for (const auto &p : so.oWireMap())
    stow->oWireMap.insert(std::make_pair(gu::stringToId(p.idIn()), gu::stringToId(p.idOut())));
}
