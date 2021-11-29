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
#include <gp_Ax3.hxx>
#include <gp_Pln.hxx>
#include <TopoDS.hxx>
#include <BRepPrimAPI_MakeHalfSpace.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepTools.hxx>

#include <osg/Switch>

#include "globalutilities.h"
#include "tools/occtools.h"
#include "tools/idtools.h"
#include "tools/featuretools.h"
#include "library/lbrplabel.h"
#include "annex/annseershape.h"
#include "annex/annintersectionmapper.h"
#include "project/serial/generated/prjsrltrmstrim.h"
#include "feature/ftrbooleanoperation.h"
#include "feature/ftrshapecheck.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrpick.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrtrim.h"

using boost::uuids::uuid;
using namespace ftr::Trim;
QIcon Feature::icon = QIcon(":/resources/images/constructionTrim.svg");

struct Feature::Stow
{
  Feature &feature;
  prm::Parameter targetPicks{QObject::tr("Target"), ftr::Picks(), PrmTags::targetPicks};
  prm::Parameter toolPicks{QObject::tr("Tool"), ftr::Picks(), PrmTags::toolPicks};
  prm::Parameter reversed{QObject::tr("Reversed"), false, PrmTags::reversed};
  
  ann::SeerShape sShape;
  ann::IntersectionMapper iMapper;
  
  osg::ref_ptr<lbr::PLabel> reversedLabel{new lbr::PLabel(&reversed)};
  
  std::map<uuid, uuid> dpMap; //map datum plane id to face and face to outer wire.
  
  Stow() = delete;
  Stow(Feature &fIn)
  : feature(fIn)
  {
    targetPicks.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&targetPicks);
    
    toolPicks.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&toolPicks);
    
    reversed.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&reversed);
    
    feature.annexes.insert(std::make_pair(ann::Type::SeerShape, &sShape));
    feature.annexes.insert(std::make_pair(ann::Type::IntersectionMapper, &iMapper));
    
    feature.overlaySwitch->addChild(reversedLabel.get());
  }
  
  TopoDS_Solid makeHalfSpace(const ann::SeerShape &seerShapeIn, const TopoDS_Shape &shapeIn)
  {
    occt::BoundingBox bb(shapeIn);
    TopoDS_Vertex cv = BRepBuilderAPI_MakeVertex(bb.getCenter());
    BRepExtrema_DistShapeShape e(cv, shapeIn, Extrema_ExtFlag_MIN);
    if (!e.IsDone() || e.NbSolution() < 1)
      return TopoDS_Solid();
    TopoDS_Shape support2 = e.SupportOnShape2(1);
    assert(seerShapeIn.hasShape(support2)); //degenerate edge?
    if (!seerShapeIn.hasShape(support2))
      return TopoDS_Solid();
    if (support2.ShapeType() != TopAbs_FACE)
    {
      occt::ShapeVector parentFaces = seerShapeIn.useGetParentsOfType(support2, TopAbs_FACE);
      assert(!parentFaces.empty());
      if (parentFaces.empty())
        return TopoDS_Solid();
      support2 = parentFaces.front();
    }
    
    double u,v;
    bool results;
    std::tie(u, v, results) = occt::pointToParameter(TopoDS::Face(support2), e.PointOnShape2(1));
    if (!results)
      return TopoDS_Solid();
    gp_Vec normal = occt::getNormal(TopoDS::Face(support2), u, v);
    if (reversed.getBool())
      normal.Reverse();
    //maybe we should go less than unit to avoid cross thin boundary?
    gp_Pnt refPoint = e.PointOnShape2(1).Translated(normal);
    //what about shell orientation?
    
    TopoDS_Solid out;
    if (shapeIn.ShapeType() == TopAbs_SHELL)
      out = BRepPrimAPI_MakeHalfSpace(TopoDS::Shell(shapeIn), refPoint);
    if (shapeIn.ShapeType() == TopAbs_FACE)
      out = BRepPrimAPI_MakeHalfSpace(TopoDS::Face(shapeIn), refPoint);
    
    return out;
  }
  
  void datumPlaneId(const uuid &bId)
  {
    //get ids in map
    auto fIt = dpMap.find(bId);
    assert(fIt != dpMap.end());
    auto wIt = dpMap.find(fIt->second);
    assert(wIt != dpMap.end());
    
    //make sure ids are in evolve container.
    if (!sShape.hasEvolveRecordOut(fIt->second))
      sShape.insertEvolve(gu::createNilId(), fIt->second);
    if (!sShape.hasEvolveRecordOut(wIt->second))
      sShape.insertEvolve(gu::createNilId(), wIt->second);
    
    //find nil face
    TopoDS_Shape f;
    for (const auto &s : sShape.getAllNilShapes())
    {
      if (s.ShapeType() == TopAbs_FACE)
      {
        sShape.updateId(s, fIt->second);
        const auto &ws = BRepTools::OuterWire(TopoDS::Face(s));
        sShape.updateId(ws, wIt->second);
        break;
      }
    }
  }
};

Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  name = QObject::tr("Trim");
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
    tls::Resolver targetResolver(payloadIn);
    const auto &targetPicks = stow->targetPicks.getPicks();
    if (targetPicks.empty() || !targetResolver.resolve(targetPicks.front()))
      throw std::runtime_error("invalid target pick");
    
    tls::Resolver toolResolver(payloadIn);
    const auto &toolPicks = stow->toolPicks.getPicks();
    if (toolPicks.empty() || !toolResolver.resolve(toolPicks.front()))
      throw std::runtime_error("invalid tool pick");
    
    TopoDS_Solid toolSolid;
    if (slc::isObjectType(toolResolver.getPick().selectionType))
    {
      //what should take precedence, seershape or csys parameter.
      const auto *tlss = toolResolver.getSeerShape();
      if (tlss)
      {
        TopoDS_Shape toolShape;
        occt::ShapeVector shells = tlss->useGetChildrenOfType(tlss->getRootOCCTShape(), TopAbs_SHELL);
        if (!shells.empty())
          toolShape = shells.front();
        else
        {
          occt::ShapeVector faces = tlss->useGetChildrenOfType(tlss->getRootOCCTShape(), TopAbs_FACE);
          if (!faces.empty())
            toolShape = faces.front();
        }
        if (toolShape.IsNull())
          throw std::runtime_error("no tool shape found");
        toolSolid = stow->makeHalfSpace(*tlss, toolShape);
        occt::BoundingBox bb(toolShape);
        stow->reversedLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(bb.getCenter())));
      }
      else // no seer shape
      {
        auto csysPrms = toolResolver.getFeature()->getParameters(prm::Tags::CSys);
        if (!csysPrms.empty())
        {
          osg::Matrixd ts = csysPrms.front()->getMatrix();
          BRepBuilderAPI_MakeFace fm(gp_Pln(gp_Ax3(gu::toOcc(ts))));
          if (!fm.IsDone())
            throw std::runtime_error("couldn't make face from datum plane");
          osg::Vec3d rp(0.0, 0.0, 1.0);
          if (stow->reversed.getBool())
            rp = -rp;
          rp = rp * ts;
          BRepPrimAPI_MakeHalfSpace hsm(fm.Face(), gp_Pnt(gu::toOcc(rp).XYZ()));
          if (!hsm.IsDone())
            throw std::runtime_error("couldn't make half space from datum plane");
          toolSolid = hsm.Solid();
          stow->reversedLabel->setMatrix(ts);
          
          //ensure we have a map from feature id to new face id and from face to wire.
          auto result = stow->dpMap.insert(std::make_pair(toolResolver.getFeature()->getId(), gu::createRandomId()));
          result = stow->dpMap.insert(std::make_pair(result.first->second, gu::createRandomId()));
        }
      }
    }
    else //tool is not object type
    {
      auto shapes = toolResolver.getShapes();
      if (!shapes.empty())
      {
        if (shapes.front().ShapeType() == TopAbs_SHELL || shapes.front().ShapeType() == TopAbs_FACE)
        {
          toolSolid = stow->makeHalfSpace(*toolResolver.getSeerShape(), shapes.front());
          occt::BoundingBox bb(shapes.front());
          stow->reversedLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(bb.getCenter())));
        }
      }
    }
    
    if (toolSolid.IsNull())
      throw std::runtime_error("No Tool");
    
    BooleanOperation subtracter(targetResolver.getShapes().front(), toolSolid, BOPAlgo_CUT);
    subtracter.Build();
    if (!subtracter.IsDone())
      throw std::runtime_error("OCC subtraction failed");
    ShapeCheck check(subtracter.Shape());
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    stow->sShape.setOCCTShape(subtracter.Shape(), getId());
    
    //WARNING: can't find keyId for face split in IntersectionMapper::go     from datum plane trim
    stow->iMapper.go(payloadIn, subtracter.getBuilder(), stow->sShape);
    
    stow->sShape.shapeMatch(*targetResolver.getSeerShape());
    if (toolResolver.getSeerShape())
      stow->sShape.shapeMatch(*toolResolver.getSeerShape());
    else
      stow->datumPlaneId(toolResolver.getFeature()->getId());
    
    stow->sShape.uniqueTypeMatch(*targetResolver.getSeerShape());
    stow->sShape.outerWireMatch(*targetResolver.getSeerShape());
    if (toolResolver.getSeerShape())
      stow->sShape.outerWireMatch(*toolResolver.getSeerShape());
    stow->sShape.derivedMatch();
    stow->sShape.dumpNils(getTypeString()); //only if there are shapes with nil ids.
    stow->sShape.dumpDuplicates(getTypeString());
    stow->sShape.ensureNoNils();
    stow->sShape.ensureNoDuplicates();
    stow->sShape.ensureEvolve();
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in trim update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in trim update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in trim update. " << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Feature::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::trms::Trim so
  (
    Base::serialOut()
    , stow->targetPicks.serialOut()
    , stow->toolPicks.serialOut()
    , stow->reversed.serialOut()
    , stow->sShape.serialOut()
    , stow->iMapper.serialOut()
    , stow->reversedLabel->serialOut()
  );
  
  for (const auto &p : stow->dpMap)
    so.dpMap().push_back(prj::srl::spt::EvolveRecord(gu::idToString(p.first), gu::idToString(p.second)));
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::trms::trim(stream, so, infoMap);
}

void Feature::serialRead(const prj::srl::trms::Trim &si)
{
  Base::serialIn(si.base());
  stow->targetPicks.serialIn(si.targetPicks());
  stow->toolPicks.serialIn(si.toolPicks());
  stow->reversed.serialIn(si.reversed());
  stow->sShape.serialIn(si.seerShape());
  stow->iMapper.serialIn(si.intersectionMapper());
  stow->reversedLabel->serialIn(si.reversedLabel());
  
  for (const auto &r : si.dpMap())
    stow->dpMap.insert(std::make_pair(gu::stringToId(r.idIn()), gu::stringToId(r.idOut())));
}
