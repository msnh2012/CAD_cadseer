/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2020 Thomas S. Anderson blobfish.at.gmx.com
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
#include <BRepTools.hxx>
#include <BRepFill_Filling.hxx>
#include <ShapeBuild_ReShape.hxx>
#include <ShapeFix_Face.hxx>
#include <ShapeFix_Wire.hxx>

#include <osg/Switch>

#include "globalutilities.h"
#include "annex/annseershape.h"
#include "library/lbrplabel.h"
#include "parameter/prmparameter.h"
#include "tools/occtools.h"
#include "tools/featuretools.h"
#include "tools/tlsshapeid.h"
#include "tools/idtools.h"
#include "feature/ftrshapecheck.h"
#include "feature/ftrupdatepayload.h"
#include "project/serial/generated/prjsrlflsfill.h"
#include "feature/ftrfill.h"

using namespace ftr::Fill;
using boost::uuids::uuid;

QIcon Feature::icon = QIcon(":/resources/images/constructionFill.svg");

Boundary::Boundary()
: edgePick{QObject::tr("Edge Pick"), ftr::Picks(), PrmTags::edgePick}
, facePick{QObject::tr("Face Pick"), ftr::Picks(), PrmTags::facePick}
, continuity{QObject::tr("Continuity"), 0, PrmTags::continuity}
, continuityLabel(new lbr::PLabel(&continuity))
{
  QStringList tStrings =
  {
    QObject::tr("C0")
    , QObject::tr("G1")
    , QObject::tr("G2")
  };
  continuity.setEnumeration(tStrings);
}

Boundary::~Boundary() noexcept = default;

struct Feature::Stow
{
  Feature &feature;
  prm::Parameter initialPick{QObject::tr("Initial Pick"), ftr::Picks(), PrmTags::initialPick};
  prm::Parameter internalPicks{QObject::tr("Internal Picks"), ftr::Picks(), PrmTags::internalPicks};
  Boundaries boundaries;
  ann::SeerShape sShape;
  uuid faceId = gu::createRandomId();
  uuid wireId = gu::createRandomId();
  
  Stow() = delete;
  Stow(Feature &fIn)
  : feature(fIn)
  {
    initialPick.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&initialPick);
    
    internalPicks.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&internalPicks);
    
    feature.annexes.insert(std::make_pair(ann::Type::SeerShape, &sShape));
  }
  
  Boundary& addBoundary()
  {
    boundaries.emplace_back();
    auto &b = boundaries.back();
    b.edgePick.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&b.edgePick);
    b.facePick.connectValue(std::bind(&Feature::setModelDirty, &feature));
    b.facePick.connectValue(std::bind(&Stow::prmActiveSync, this));
    feature.parameters.push_back(&b.facePick);
    b.continuity.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&b.continuity);
    b.continuity.setActive(false);
    feature.overlaySwitch->addChild(b.continuityLabel.get());
    return b;
  }
  
  //watch face pick and enable or disable continuity parameter.
  void prmActiveSync()
  {
    for (auto &b : boundaries)
    {
      if (b.facePick.getPicks().empty())
        b.continuity.setActive(false);
      else
        b.continuity.setActive(true);
    }
  }
};

Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  name = QObject::tr("Fill");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
}

Feature::~Feature() = default;

Boundary& Feature::addBoundary()
{
  setModelDirty();
  return stow->addBoundary();
}

void Feature::removeBoundary(int index)
{
  assert (index >=0 && index < static_cast<int>(stow->boundaries.size()));
  auto lit = stow->boundaries.begin();
  for (int i = 0; i < index; ++i, ++lit);
  const auto &b = *lit;
  removeParameter(&b.edgePick);
  removeParameter(&b.facePick);
  removeParameter(&b.continuity);
  overlaySwitch->removeChild(b.continuityLabel.get());
  stow->boundaries.erase(lit);
  setModelDirty();
}
Boundaries& Feature::getBoundaries() const
{
  return stow->boundaries;
}

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
    
    tls::Resolver pr(pIn);
    tls::ShapeIdContainer tempIds;
    auto forceEvolve = [&](const uuid &oldId) -> uuid
    {
      if (stow->sShape.hasEvolveRecordIn(oldId))
        return stow->sShape.evolve(oldId).front();
      uuid freshId = gu::createRandomId();
      stow->sShape.insertEvolve(oldId, freshId);
      return freshId;
    };
    
    BRepFill_Filling filler;
    
    //initial face if applicable.
    const ftr::Picks &ip = stow->initialPick.getPicks();
    if (!ip.empty())
    {
      if (!pr.resolve(ip.front()))
        throw std::runtime_error("Couldn't resolve initial surface");
      occt::ShapeVector shapes = pr.getShapes();
      if (shapes.empty() || shapes.front().ShapeType() != TopAbs_FACE)
        throw std::runtime_error("No or invalid initial surface");
      const auto oldId = pr.getSeerShape()->findId(shapes.front());
      tempIds.insert(forceEvolve(oldId), shapes.front());
      filler.LoadInitSurface(TopoDS::Face(shapes.front()));
      
    }
    
    //do boundaries.
    for (const auto &e : stow->boundaries)
    {
      GeomAbs_Shape c = GeomAbs_C0;
      if (e.continuity.getInt() == 1)
        c = GeomAbs_G1;
      else if (e.continuity.getInt() == 2)
        c = GeomAbs_G2;
      const auto &ep = e.edgePick.getPicks();
      const auto &fp = e.facePick.getPicks();
      if (!ep.empty() && fp.empty())
      {
        //edge, no face
        
        //this brings up the question of prmActiveSync between the face selection
        //and the continuity parameter????
        if (!pr.resolve(ep.front()))
          continue;
        auto shapes = pr.getShapes();
        if (!shapes.empty() && shapes.front().ShapeType() == TopAbs_EDGE)
        {
          const auto oldId = pr.getSeerShape()->findId(shapes.front());
          tempIds.insert(forceEvolve(oldId), shapes.front());
          filler.Add(TopoDS::Edge(shapes.front()), c);
        }
      }
      else if (ep.empty() && !fp.empty())
      {
        //no edge, face
        if (!overlaySwitch->containsNode(e.continuityLabel.get()))
          overlaySwitch->addChild(e.continuityLabel.get());
        if (!pr.resolve(fp.front()))
          continue;
        auto shapes = pr.getShapes();
        if (!shapes.empty() && shapes.front().ShapeType() == TopAbs_FACE)
        {
          const TopoDS_Face &f = TopoDS::Face(shapes.front());
          filler.Add(f, c); //faces without edges are always boundary. id mapping not relevant.
          e.continuityLabel->setMatrix(osg::Matrixd::translate(fp.front().getPoint(f)));
        }
      }
      else if (!ep.empty() && !fp.empty())
      {
        //edge, face
        if (!pr.resolve(ep.front()))
          continue;
        auto edgeShapes = pr.getShapes();
        if (edgeShapes.empty() || edgeShapes.front().ShapeType() != TopAbs_EDGE)
          continue;
        uuid oldEdgeId = pr.getSeerShape()->findId(edgeShapes.front());
        tempIds.insert(forceEvolve(oldEdgeId), edgeShapes.front());
        
        if (!pr.resolve(fp.front()))
          continue;
        auto faceShapes = pr.getShapes();
        if (faceShapes.empty() || faceShapes.front().ShapeType() != TopAbs_FACE)
          continue;
        uuid oldFaceId = pr.getSeerShape()->findId(faceShapes.front());
        tempIds.insert(forceEvolve(oldFaceId), faceShapes.front());
        
        filler.Add(TopoDS::Edge(edgeShapes.front()), TopoDS::Face(faceShapes.front()), c);
        e.continuityLabel->setMatrix(osg::Matrixd::translate(fp.front().getPoint(TopoDS::Face(faceShapes.front()))));
      }
    }
    
    //do internals. Id mapping not needed for internals
    for (const auto &ip : stow->internalPicks.getPicks())
    {
      if (!pr.resolve(ip))
        continue;
      if (slc::isPointType(ip.selectionType))
      {
        auto points = pr.getPoints();
        if (!points.empty())
          filler.Add(gu::toOcc(points.front()).XYZ());
      }
      else if (ip.selectionType == slc::Type::Edge)
      {
        auto shapes = pr.getShapes();
        if (shapes.empty())
          continue;
        assert(shapes.front().ShapeType() == TopAbs_EDGE);
        TopoDS_Edge e = TopoDS::Edge(shapes.front());
        filler.Add(e, GeomAbs_C0, false);
      }
    }
    
    filler.Build();
    if (!filler.IsDone())
      throw std::runtime_error("filling failed");
    
    TopoDS_Face out = filler.Face();
    if (out.IsNull())
      throw std::runtime_error("out face is null");
    
    ShapeCheck check(out);
    if (!check.isValid())
    {
      //When picking just a surface for a boundary we get a lot of same
      //parameter errors. This tries to fix those.
      opencascade::handle<ShapeBuild_ReShape> context = new ShapeBuild_ReShape();
      ShapeFix_Face faceFixer(out); //this calls 'ClearModes to create an empty operation'
      faceFixer.SetContext(context);
      faceFixer.FixWireMode() = 1;
      auto wireFixer = faceFixer.FixWireTool();
      wireFixer->FixSameParameterMode() = 1;
      wireFixer->FixEdgeCurvesMode() = 1;
      faceFixer.Perform();
      if (!faceFixer.Face().IsNull())
        out = faceFixer.Face();
      
      ShapeCheck check2(out);
      if (!check2.isValid())
        throw std::runtime_error("couldn't fix shape");
    }
    
    stow->sShape.setOCCTShape(out, getId());
    stow->sShape.updateId(out, stow->faceId);
    stow->sShape.updateId(BRepTools::OuterWire(out), stow->wireId);
    for (const auto &s : stow->sShape.getAllShapes())
    {
      if (tempIds.has(s))
        stow->sShape.updateId(s, tempIds.find(s));
    }
    stow->sShape.ensureNoNils();
    stow->sShape.ensureNoDuplicates();
    
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

void Feature::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::fls::Fill so
  (
    Base::serialOut()
    , stow->initialPick.serialOut()
    , stow->internalPicks.serialOut()
    , stow->sShape.serialOut()
    , gu::idToString(stow->faceId)
    , gu::idToString(stow->wireId)
  );
  for (const auto &e : stow->boundaries)
  {
    prj::srl::fls::Boundary eOut
    (
      e.edgePick.serialOut()
      , e.facePick.serialOut()
      , e.continuity.serialOut()
      , e.continuityLabel->serialOut()
    );
    so.boundaries().push_back(eOut);
  }
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::fls::fill(stream, so, infoMap);
}

void Feature::serialRead(const prj::srl::fls::Fill &so)
{
  Base::serialIn(so.base());
  stow->initialPick.serialIn(so.initialPick());
  stow->internalPicks.serialIn(so.internalPicks());
  stow->sShape.serialIn(so.seerShape());
  stow->faceId = gu::stringToId(so.faceId());
  stow->wireId = gu::stringToId(so.wireId());
  
  for (const auto &eIn : so.boundaries())
  {
    Boundary &entry = stow->addBoundary();
    entry.edgePick.serialIn(eIn.edgePick());
    entry.facePick.serialIn(eIn.facePick());
    entry.continuity.serialIn(eIn.continuity());
    entry.continuityLabel->serialIn(eIn.continuityLabel());
  }
}
