/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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

#include <BRepFilletAPI_MakeChamfer.hxx>
#include <TopExp.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <Geom_Curve.hxx>

#include <osg/Switch>

#include "globalutilities.h"
#include "tools/idtools.h"
#include "tools/occtools.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "project/serial/generated/prjsrlchmschamfer.h"
#include "annex/annseershape.h"
#include "feature/ftrshapecheck.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrupdatepayload.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "library/lbrplabel.h"
#include "tools/featuretools.h"
#include "feature/ftrchamfer.h"

using namespace ftr::Chamfer;
using boost::uuids::uuid;

QIcon Feature::icon = QIcon(":/resources/images/constructionChamfer.svg");

Entry::Entry()
: style(QObject::tr("Style"), 0, PrmTags::style)
, edgePicks(QObject::tr("Edge Picks"), ftr::Picks(), PrmTags::edgePicks)
, facePicks(QObject::tr("Face Picks"), ftr::Picks(), PrmTags::facePicks)
, distance(prm::Names::Distance, prf::manager().rootPtr->features().chamfer().get().distance(), prm::Tags::Distance)
, dist2Angle(QObject::tr("Distance 2 Or Angle"), distance.getDouble() + 0.1, PrmTags::dist2Angle)
, styleLabel(new lbr::PLabel(&style))
, distanceLabel(new lbr::PLabel(&distance))
, dist2AngleLabel(new lbr::PLabel(&dist2Angle))
, styleObserver(std::bind(&Entry::prmActiveSync, this))
{
  QStringList styleStrings =
  {
    QObject::tr("Symmetric")
    , QObject::tr("Two Distances")
    , QObject::tr("Distance Angle")
  };
  style.setEnumeration(styleStrings);
  styleLabel->refresh();
  style.connect(styleObserver);
  
  distance.setConstraint(prm::Constraint::buildNonZeroPositive());
  //constraints on 2nd parameter are set in prmActiveSync
  
  prmActiveSync();
}

Entry::~Entry() noexcept = default;

Entry::Entry(const prj::srl::chms::Entry &eIn) : Entry()
{
  serialIn(eIn);
}

prj::srl::chms::Entry Entry::serialOut() const
{
  prj::srl::chms::Entry out
  (
    style.serialOut()
    , edgePicks.serialOut()
    , facePicks.serialOut()
    , distance.serialOut()
    , dist2Angle.serialOut()
    , styleLabel->serialOut()
    , distanceLabel->serialOut()
    , dist2AngleLabel->serialOut()
  );
  return out;
}

void Entry::serialIn(const prj::srl::chms::Entry &eIn)
{
  style.serialIn(eIn.style());
  edgePicks.serialIn(eIn.edgePicks());
  facePicks.serialIn(eIn.facePicks());
  distance.serialIn(eIn.distance());
  dist2Angle.serialIn(eIn.dist2Angle());
  styleLabel->serialIn(eIn.styleLabel());
  distanceLabel->serialIn(eIn.distanceLabel());
  dist2AngleLabel->serialIn(eIn.dist2AngleLabel());
}

prm::Parameters Entry::getParameters()
{
  return {&style, &edgePicks, &facePicks, &distance, &dist2Angle};
}

void Entry::prmActiveSync()
{
  //when switching between distance 2 and angle, it doesn't make any sense to try and use
  //the existing values. so we just set back to chamfer default.
  switch(style.getInt())
  {
    case 0: //symmetric
    {
      dist2Angle.setActive(false);
      facePicks.setActive(false);
      facePicks.setValue(ftr::Pick());
      break;
    }
    case 1: //2 distances
    {
      dist2Angle.setActive(true);
      facePicks.setActive(true);
      dist2Angle.setConstraint(prm::Constraint::buildNonZeroPositive());
      dist2Angle.setValue(prf::manager().rootPtr->features().chamfer().get().distance()  + 0.1);
      dist2Angle.setName(QObject::tr("Distance 2"));
      dist2AngleLabel->refresh();
      break;
    }
    case 2: //distance and angle
    {
      dist2Angle.setActive(true);
      facePicks.setActive(true);
      dist2Angle.setConstraint(prm::Constraint::buildNonZeroPositiveAngle());
      dist2Angle.setValue(30.0);
      dist2Angle.setName(QObject::tr("Angle"));
      dist2AngleLabel->refresh();
      break;
    }
    default:
    {
      assert(0); //unknown chamfer style
      break;
    }
  }
}

struct Feature::Stow
{
  Feature &feature;
  
  prm::Parameter mode{QObject::tr("Mode"), 0, PrmTags::mode};
  
  ann::SeerShape sShape;
  
  osg::ref_ptr<lbr::PLabel> modeLabel{new lbr::PLabel(&mode)};
  
  Entries entries;
  std::map<uuid, uuid> shapeMap; //!< map edges or vertices to faces
  
  Stow() = delete;
  Stow(Feature &fIn)
  : feature(fIn)
  {
    QStringList modeStrings =
    {
      QObject::tr("Classic")
      , QObject::tr("Throat")
      , QObject::tr("Throat Penetration")
    };
    mode.setEnumeration(modeStrings);
    mode.connectValue(std::bind(&Feature::setModelDirty, &feature));
    mode.connectValue(std::bind(&Stow::prmActiveSync, this));
    feature.parameters.push_back(&mode);
    modeLabel->refresh();
    
    feature.annexes.insert(std::make_pair(ann::Type::SeerShape, &sShape));
    
    feature.overlaySwitch->addChild(modeLabel.get());
    
    prmActiveSync();
  }
  
  void prmActiveSync()
  {
    int modeType = mode.getInt();
    switch (modeType)
    {
      case 0: //classic
      {
        //classic supports all types so we don't need to do anything.
        break;
      }
      case 1: //throat
      {
        //throat can only have symmetric, so change all entries to that.
        for (auto &e : entries)
        {
          if (e.style.getInt() != 0)
            e.style.setValue(0);
        }
        break;
      }
      case 2: //throat penetration
      {
        //throat penetration can only have two distances, so change all entries to that.
        for (auto &e : entries)
        {
          if (e.style.getInt() != 1)
            e.style.setValue(1);
        }
        break;
      }
      default:
      {
        assert(0); //unknown style
        break;
      }
    }
  }
  
  void attachEntry(Entry &eIn)
  {
    //not adding entry parameters to feature's general container.
    eIn.style.connectValue(std::bind(&Feature::setModelDirty, &feature));
    eIn.edgePicks.connectValue(std::bind(&Feature::setModelDirty, &feature));
    eIn.facePicks.connectValue(std::bind(&Feature::setModelDirty, &feature));
    eIn.distance.connectValue(std::bind(&Feature::setModelDirty, &feature));
    eIn.dist2Angle.connectValue(std::bind(&Feature::setModelDirty, &feature));
    
    feature.overlaySwitch->addChild(eIn.styleLabel);
    feature.overlaySwitch->addChild(eIn.distanceLabel);
    feature.overlaySwitch->addChild(eIn.dist2AngleLabel);
  }
  
  void detachEntry(Entry &eIn)
  {
    feature.overlaySwitch->removeChild(eIn.styleLabel);
    feature.overlaySwitch->removeChild(eIn.distanceLabel);
    feature.overlaySwitch->removeChild(eIn.dist2AngleLabel);
  }
  
  //duplicated with blend.
  void generatedMatch(BRepFilletAPI_MakeChamfer &chamferMakerIn, const ann::SeerShape &targetShapeIn)
  {
    std::vector<uuid> targetShapeIds = targetShapeIn.getAllShapeIds();
    
    for (const auto &cId : targetShapeIds)
    {
      const TopoDS_Shape &currentShape = targetShapeIn.findShape(cId);
      TopTools_ListOfShape generated = chamferMakerIn.Generated(currentShape);
      if (generated.IsEmpty())
        continue;
      if(generated.Extent() != 1)
      {
        std::ostringstream s; s << "Warning: more than one generated shape in chamfer::generatedMatch" << std::endl;
        feature.lastUpdateLog += s.str();
      }
      const TopoDS_Shape &chamferFace = generated.First();
      assert(!chamferFace.IsNull());
      assert(chamferFace.ShapeType() == TopAbs_FACE);
      
      std::map<uuid, uuid>::iterator mapItFace;
      bool dummy;
      std::tie(mapItFace, dummy) = shapeMap.insert(std::make_pair(cId, gu::createRandomId()));
      sShape.updateId(chamferFace, mapItFace->second);
      if (dummy) //insertion took place.
        sShape.insertEvolve(gu::createNilId(), mapItFace->second);
      
      //now look for outerwire for newly generated face.
      //we use the generated face id to map to outer wire.
      std::map<uuid, uuid>::iterator mapItWire;
      std::tie(mapItWire, dummy) = shapeMap.insert(std::make_pair(mapItFace->second, gu::createRandomId()));
      //now get the wire and update the result to id.
      const TopoDS_Shape &chamferedFaceWire = BRepTools::OuterWire(TopoDS::Face(chamferFace));
      sShape.updateId(chamferedFaceWire, mapItWire->second);
      if (dummy) //insertion took place.
        sShape.insertEvolve(gu::createNilId(), mapItWire->second);
    }
  }
};

Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  name = QObject::tr("Chamfer");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
}

Feature::~Feature() = default;

Entry& Feature::addSymmetric()
{
  int lMode = stow->mode.getInt();
  assert(lMode == 0 || lMode == 1); //classic or throat
  stow->entries.emplace_back();
  stow->attachEntry(stow->entries.back());
  // 'empty' entry shouldn't dirty feature.'
  return stow->entries.back();
}

Entry& Feature::addTwoDistances()
{
  int lMode = stow->mode.getInt();
  assert(lMode == 0 || lMode == 2); //classic or throat penetration
  stow->entries.emplace_back();
  stow->entries.back().style.setValue(1);
  stow->attachEntry(stow->entries.back());
  return stow->entries.back();
}

Entry& Feature::addDistanceAngle()
{
  assert(stow->mode.getInt() == 0); //classic only
  stow->entries.emplace_back();
  stow->entries.back().style.setValue(2);
  stow->attachEntry(stow->entries.back());
  return stow->entries.back();
}

void Feature::removeEntry(int index)
{
  assert(index >= 0 && index < static_cast<int>(stow->entries.size()));
  auto it = stow->entries.begin();
  for (int i = 0; i < index; ++i, ++it);
  stow->detachEntry(*it);
  stow->entries.erase(it);
  setModelDirty();
}

Entry& Feature::getEntry(int index)
{
  assert(index >= 0 && index < static_cast<int>(stow->entries.size()));
  auto it = stow->entries.begin();
  for (int i = 0; i < index; ++i, ++it);
  return *it;
}

Entries& Feature::getEntries()
{
  return stow->entries;
}

void Feature::updateModel(const UpdatePayload &payloadIn)
{
  setFailure();
  lastUpdateLog.clear();
  stow->sShape.reset();
  try
  {
    std::vector<const Base*> targetFeatures = payloadIn.getFeatures(std::string());
    if (targetFeatures.size() != 1)
      throw std::runtime_error("wrong number of parents");
    const Base* tf = targetFeatures.front();
    if (!tf->hasAnnex(ann::Type::SeerShape))
      throw std::runtime_error("parent doesn't have seer shape");
    const ann::SeerShape &targetSeerShape = tf->getAnnex<ann::SeerShape>();
    if (targetSeerShape.isNull())
      throw std::runtime_error("target seer shape is null");
    
    //setup failure state.
    stow->sShape.setOCCTShape(targetSeerShape.getRootOCCTShape(), getId());
    stow->sShape.shapeMatch(targetSeerShape);
    stow->sShape.uniqueTypeMatch(targetSeerShape);
    stow->sShape.outerWireMatch(targetSeerShape);
    stow->sShape.derivedMatch();
    stow->sShape.ensureNoNils();
    stow->sShape.ensureNoDuplicates();
    stow->sShape.ensureEvolve();
    
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    auto pairing = [&](const std::vector<uuid> &edges, const std::vector<uuid> &faces) -> std::vector<std::pair<uuid, uuid>>
    {
      std::vector<std::pair<uuid, uuid>> out;
      for (const auto &e : edges)
      {
        for (const auto &f : faces)
        {
          if (targetSeerShape.useIsEdgeOfFace(e, f))
          {
            out.push_back(std::make_pair(e, f));
          }
        }
      }
      return out;
    };
    
    tls::Resolver resolver(payloadIn);
    BRepFilletAPI_MakeChamfer chamferMaker(targetSeerShape.getRootOCCTShape());
    
    //place model label at center of input shape.
    occt::BoundingBox bb(targetSeerShape.getRootOCCTShape());
    stow->modeLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(bb.getCenter())));
    
    int lMode = stow->mode.getInt();
    chamferMaker.SetMode(static_cast<ChFiDS_ChamfMode>(lMode));
    for (const auto &e : stow->entries)
    {
      if (lMode == 1 && e.style.getInt() != 0) //throat
        throw std::runtime_error("Throat mode only uses symmetric style");
      if (lMode == 2 && e.style.getInt() != 1) //throat penetration
        throw std::runtime_error("Throat penetration mode only uses 2 distances style");
      //classic mode can use any style
      
      if (e.style.getInt() == 0) //symmetric
      {
        bool labelDone = false;
        for (const auto &ep : e.edgePicks.getPicks())
        {
          if (!resolver.resolve(ep))
            continue;
          for (const auto &s : resolver.getShapes())
          {
            if (s.ShapeType() != TopAbs_EDGE)
            {
              std::ostringstream s; s << "Warning: Skipping non edge shape." << std::endl;
              lastUpdateLog += s.str();
              continue;
            }
            chamferMaker.Add(e.distance.getDouble(), TopoDS::Edge(s));
            if (!labelDone)
            {
              labelDone = true;
              e.distanceLabel->setMatrix(osg::Matrixd::translate(ep.getPoint(TopoDS::Edge(s))));
              auto loc = gu::toOsg(chamferMaker.FirstVertex(chamferMaker.Contour(TopoDS::Edge(s))));
              e.styleLabel->setMatrix(osg::Matrixd::translate(loc));
            }
          }
        }
      }
      else if (e.style.getInt() == 1 || e.style.getInt() == 2) //two distances or distance and angle
      {
        if (e.edgePicks.getPicks().size() != e.facePicks.getPicks().size())
          throw std::runtime_error("need same number edges and faces for two distance chamfer");
        bool labelDone1 = false;
        bool labelDone2 = false;
        for (std::size_t index = 0; index < e.edgePicks.getPicks().size(); ++index)
        {
          //we have to resolve multiple edges and faces
          if (!resolver.resolve(e.edgePicks.getPicks().at(index)))
            continue;
          std::vector<uuid> edgeIds = resolver.getResolvedIds();
          
          if (!resolver.resolve(e.facePicks.getPicks().at(index)))
            continue;
          std::vector<uuid> faceIds = resolver.getResolvedIds();
          
          std::vector<std::pair<uuid, uuid>> pairs = pairing(edgeIds, faceIds);
          for (const auto &p : pairs)
          {
            const TopoDS_Edge &edge = TopoDS::Edge(targetSeerShape.getOCCTShape(p.first));
            const TopoDS_Face &face = TopoDS::Face(targetSeerShape.getOCCTShape(p.second));
            if (e.style.getInt() == 1) // 2 distances
            {
              chamferMaker.Add
              (
                e.distance.getDouble()
                , e.dist2Angle.getDouble()
                , edge
                , face
              );
            }
            else if (e.style.getInt() == 2) //distance and angle
            {
              chamferMaker.AddDA
              (
                e.distance.getDouble()
                , osg::DegreesToRadians(e.dist2Angle.getDouble())
                , edge
                , face
              );
            }
            if (!labelDone1)
            {
              labelDone1 = true;
              e.distanceLabel->setMatrix(osg::Matrixd::translate(e.edgePicks.getPicks().at(index).getPoint(edge)));
              auto loc = gu::toOsg(chamferMaker.FirstVertex(chamferMaker.Contour(edge)));
              e.styleLabel->setMatrix(osg::Matrixd::translate(loc));
            }
            if (!labelDone2)
            {
              labelDone2 = true;
              e.dist2AngleLabel->setMatrix(osg::Matrixd::translate(e.facePicks.getPicks().at(index).getPoint(face)));
            }
          }
        }
      }
    }
    chamferMaker.Build();
    if (!chamferMaker.IsDone())
      throw std::runtime_error("chamferMaker failed");
    ShapeCheck check(chamferMaker.Shape());
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");

    stow->sShape.setOCCTShape(chamferMaker.Shape(), getId());
    stow->sShape.shapeMatch(targetSeerShape);
    stow->sShape.uniqueTypeMatch(targetSeerShape);
    stow->sShape.modifiedMatch(chamferMaker, targetSeerShape);
    stow->generatedMatch(chamferMaker, targetSeerShape);
    stow->sShape.outerWireMatch(targetSeerShape);
    stow->sShape.derivedMatch();
    stow->sShape.dumpNils("chamfer feature"); //only if there are shapes with nil ids.
    stow->sShape.dumpDuplicates("chamfer feature");
    stow->sShape.ensureNoNils();
    stow->sShape.ensureNoDuplicates();
    stow->sShape.ensureEvolve();
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in chamfer update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in chamfer update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in chamfer update." << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Feature::serialWrite(const boost::filesystem::path &dIn)
{
  namespace serial = prj::srl::chms;
  
  serial::Chamfer chamferOut
  (
    Base::serialOut()
    , stow->mode.serialOut()
    , stow->sShape.serialOut()
    , stow->modeLabel->serialOut()
  );
  
  for (const auto &p : stow->shapeMap)
  {
    serial::Chamfer::ShapeMapType eRecord
    (
      gu::idToString(p.first),
     gu::idToString(p.second)
    );
    chamferOut.shapeMap().push_back(eRecord);
  }
  
  for (const auto &e : stow->entries)
    chamferOut.entries().push_back(e.serialOut());
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  serial::chamfer(stream, chamferOut, infoMap);
}

void Feature::serialRead(const prj::srl::chms::Chamfer &sChamferIn)
{
  Base::serialIn(sChamferIn.base());
  stow->mode.serialIn(sChamferIn.mode());
  stow->sShape.serialIn(sChamferIn.seerShape());
  stow->modeLabel->serialIn(sChamferIn.modeLabel());
  
  stow->shapeMap.clear();
  for (const auto &sERecord : sChamferIn.shapeMap())
    stow->shapeMap.insert(std::make_pair(gu::stringToId(sERecord.idIn()), gu::stringToId(sERecord.idOut())));
  
  for (const auto &e : sChamferIn.entries())
  {
    stow->entries.emplace_back(e);
    stow->attachEntry(stow->entries.back());
    stow->entries.back().prmActiveSync();
  }
}
