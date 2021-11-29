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
#include <BRepAdaptor_Surface.hxx>
#include <GeomAdaptor_Surface.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <BRepTools.hxx>

#include <osg/Switch>

#include "globalutilities.h"
#include "annex/annseershape.h"
#include "library/lbrplabel.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "tools/occtools.h"
#include "tools/featuretools.h"
#include "tools/idtools.h"
#include "tools/tlsshapeid.h"
#include "feature/ftrshapecheck.h"
#include "feature/ftrupdatepayload.h"
#include "project/serial/generated/prjsrlutruntrim.h"
#include "feature/ftruntrim.h"

using uuid = boost::uuids::uuid;
using namespace ftr::Untrim;
QIcon Feature::icon = QIcon(":/resources/images/constructionUntrim.svg");

struct Feature::Stow
{
  Feature &feature;
  prm::Parameter picks{prm::Names::Picks, ftr::Picks(), prm::Tags::Picks};
  prm::Parameter offset{prm::Names::Offset, 0.1, prm::Tags::Offset};
  prm::Parameter closeU{QObject::tr("Close U"), false, PrmTags::closeU};
  prm::Parameter closeV{QObject::tr("Close V"), false, PrmTags::closeV};
  prm::Parameter makeSolid{QObject::tr("Make Solid"), false, PrmTags::makeSolid};
  
  ann::SeerShape sShape;
  
  osg::ref_ptr<lbr::PLabel> offsetLabel{new lbr::PLabel(&offset)};
  osg::ref_ptr<lbr::PLabel> closeULabel{new lbr::PLabel(&closeU)};
  osg::ref_ptr<lbr::PLabel> closeVLabel{new lbr::PLabel(&closeV)};
  osg::ref_ptr<lbr::PLabel> makeSolidLabel{new lbr::PLabel(&makeSolid)};
  
  uuid solidId{gu::createRandomId()};
  uuid shellId{gu::createRandomId()};
  std::vector<uuid> uvEdgeIds; //ids from creating face from surface and u v ranges.
  std::map<uuid, std::pair<uuid, uuid>> edgeToCap; //first is face, second is wire.
  
  Stow() = delete;
  Stow(Feature &fIn)
  : feature(fIn)
  {
    picks.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&picks);
    
    offset.setConstraint(prm::Constraint::buildZeroPositive());
    offset.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&offset);
    
    closeU.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&closeU);
    
    closeV.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&closeV);
    
    makeSolid.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&makeSolid);
    
    feature.annexes.insert(std::make_pair(ann::Type::SeerShape, &sShape));
    
    feature.overlaySwitch->addChild(offsetLabel.get());
    feature.overlaySwitch->addChild(closeULabel.get());
    feature.overlaySwitch->addChild(closeVLabel.get());
    feature.overlaySwitch->addChild(makeSolidLabel.get());
  }
};

Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  name = QObject::tr("Untrim");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
}

Feature::~Feature() = default;

//cones will probably need some special attention. The uv bounds
//don't consider apex and we can lengthen them right through the apex.
//I created a solid with the cone surface lengthened through the apex,
//bopargcheck didn't bitch, so maybe leave it.
void Feature::updateModel(const UpdatePayload &pIn)
{
  setFailure();
  lastUpdateLog.clear();
  stow->sShape.reset();
  try
  {
    const auto &picks = stow->picks.getPicks();
    if (picks.empty())
      throw std::runtime_error("Empty picks");
    tls::Resolver pr(pIn);
    if (!pr.resolve(picks.front()))
      throw std::runtime_error("invalid pick resolution");
    
    auto shapes = pr.getShapes();
    if (shapes.empty())
      throw std::runtime_error("no resolved shapes");
    
    auto shapeToUntrim = shapes.front(); //only one shape per feature.
    if (shapeToUntrim.ShapeType() == TopAbs_COMPOUND)
    {
      auto cs = occt::getFirstNonCompound(shapeToUntrim);
      if (cs.IsNull())
        throw std::runtime_error("empty compound shape");
      shapeToUntrim = cs;
    }
    if (shapeToUntrim.IsNull() || shapeToUntrim.ShapeType() != TopAbs_FACE)
      throw std::runtime_error("Invalid shape to untrim");

    //setup new failure state.
    stow->sShape.setOCCTShape(shapeToUntrim, getId());
    stow->sShape.shapeMatch(*pr.getSeerShape());
    stow->sShape.uniqueTypeMatch(*pr.getSeerShape());
    stow->sShape.outerWireMatch(*pr.getSeerShape());
    stow->sShape.derivedMatch();
    stow->sShape.ensureNoNils(); //just in case
    stow->sShape.ensureNoDuplicates(); //just in case
    
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    BRepAdaptor_Surface as(TopoDS::Face(shapeToUntrim));
    opencascade::handle<Geom_Surface> surfaceCopy = dynamic_cast<Geom_Surface*>(as.Surface().Surface()->Copy().get());
    
    double ov = stow->offset.getDouble();
    
    double u0 = as.FirstUParameter() - ov;
    double u1 = as.LastUParameter() + ov;
    double v0 = as.FirstVParameter() - ov;
    double v1 = as.LastVParameter() + ov;
    
    if (as.IsUPeriodic())
    {
      if (stow->closeU.getBool())
      {
        u0 = 0.0;
        u1 = as.UPeriod();
      }
      u0 = std::max(0.0, u0);
      u1 = std::min(as.UPeriod(), u1);
      stow->closeU.setActive(true);
    }
    else
      stow->closeU.setActive(false);
    
    if (as.IsVPeriodic())
    {
      if (stow->closeV.getBool())
      {
        v0 = 0.0;
        v1 = as.VPeriod();
      }
      v0 = std::max(0.0, v0);
      v1 = std::min(as.VPeriod(), v1);
      stow->closeV.setActive(true);
    }
    else
      stow->closeV.setActive(false);
    
    //make sure we are not over the surface bounds
    double bu0, bu1, bv0, bv1;
    surfaceCopy->Bounds(bu0, bu1, bv0, bv1);
    u0 = std::max(u0, bu0);
    u1 = std::min(u1, bu1);
    v0 = std::max(v0, bv0);
    v1 = std::min(v1, bv1);
    
    tls::ShapeIdContainer tempIds;
    auto &originalId = pr.getSeerShape()->findId(shapeToUntrim);
    assert(!originalId.is_nil());
    if (!stow->sShape.hasEvolveRecordIn(originalId))
      stow->sShape.insertEvolve(originalId, gu::createRandomId());
    
    BRepBuilderAPI_MakeFace fm(surfaceCopy, u0, u1, v0, v1, Precision::Confusion());
    if (!fm.IsDone())
      throw std::runtime_error("couldn't make face");
    TopoDS_Shape untrimmedFace = fm.Face();
    untrimmedFace.Location(as.Trsf());
    TopoDS_Shape out = untrimmedFace;
    tempIds.insert(stow->sShape.evolve(originalId).front(), untrimmedFace);
    //here we are going to assume that the edges always come in the same order.
    auto uvEdgeIt = stow->uvEdgeIds.begin();
    for (const auto &e : static_cast<occt::EdgeVector>(occt::ShapeVectorCast(occt::mapShapes(untrimmedFace))))
    {
      if (uvEdgeIt == stow->uvEdgeIds.end())
      {
        stow->uvEdgeIds.push_back(gu::createRandomId());
        tempIds.insert(stow->uvEdgeIds.back(), e);
        if (!stow->sShape.hasEvolveRecordOut(stow->uvEdgeIds.back()))
          stow->sShape.insertEvolve(gu::createNilId(), stow->uvEdgeIds.back());
        uvEdgeIt = stow->uvEdgeIds.end(); //push_back might invalidate
        continue;
      }
      else
      {
        tempIds.insert(*uvEdgeIt, e);
        if (!stow->sShape.hasEvolveRecordOut(*uvEdgeIt))
          stow->sShape.insertEvolve(gu::createNilId(), *uvEdgeIt);
        uvEdgeIt++;
      }
    }
    
    
    if (stow->makeSolid.getBool())
    {
      //BRepBuilderAPI_MakeShell making corrupt shape when off of period boundaries
      BRep_Builder builder;
      TopoDS_Shell shell;
      builder.MakeShell(shell);
      builder.Add(shell, untrimmedFace);
      
      auto processShell = [&]() -> bool
      {
        if (BRep_Tool::IsClosed(shell))
        {
          shell.Closed(true);
          auto result = occt::buildSolid(shell);
          if (result)
          {
            out = result.get();
            if (!stow->sShape.hasEvolveRecordOut(stow->shellId))
              stow->sShape.insertEvolve(gu::createNilId(), stow->shellId);
            tempIds.insert(stow->shellId, shell);
            if (!stow->sShape.hasEvolveRecordOut(stow->solidId))
              stow->sShape.insertEvolve(gu::createNilId(), stow->solidId);
            tempIds.insert(stow->solidId, out);
            
            return true;
          }
        }
        
        return false;
      };
      
      if (!processShell())
      {
        // try to cap
        occt::WireVector closed;
        occt::WireVector open;
        std::tie(closed, open) = occt::getBoundaryWires(shell);
        
        BRepBuilderAPI_Sewing sewer(0.000001, true, true, true, false);
        sewer.Add(untrimmedFace);
        for (const auto &w : closed)
        {
          BRepBuilderAPI_MakeFace cm(w);
          if (cm.IsDone())
          {
            const TopoDS_Face &face = cm.Face();
            const TopoDS_Wire &wire = BRepTools::OuterWire(face);
            sewer.Add(face);
            occt::EdgeVector edges = static_cast<occt::EdgeVector>(occt::ShapeVectorCast(occt::mapShapes(w)));
            if (!edges.empty() && tempIds.has(edges.front()))
            {
              //should only be one edge in wire.
              auto edgeId = tempIds.find(edges.front());
              if (stow->edgeToCap.count(edgeId) == 0)
              {
                uuid freshFace = gu::createRandomId();
                uuid freshWire = gu::createRandomId();
                stow->edgeToCap.insert({edgeId, std::make_pair(freshFace, freshWire)});
                stow->sShape.insertEvolve(gu::createNilId(), freshFace);
                stow->sShape.insertEvolve(gu::createNilId(), freshWire);
              }
              auto it = stow->edgeToCap.find(edgeId);
              tempIds.insert(it->second.first, face);
              tempIds.insert(it->second.second, wire);
            }
          }
          else
            lastUpdateLog += QObject::tr("Failed To Create Cap ").toStdString();
        }
        sewer.Perform();
        TopoDS_Shape sewn = sewer.SewedShape();
        if (!sewn.IsNull())
        {
          if (sewn.ShapeType() == TopAbs_SHELL)
          {
            shell = TopoDS::Shell(sewn);
            processShell();
          }
          if (sewn.ShapeType() == TopAbs_COMPOUND)
          {
            occt::ShapeVectorCast svc(TopoDS::Compound(sewn));
            occt::ShellVector sv = svc;
            if (!sv.empty())
            {
              shell = sv.front();
              processShell();
            }
          }
        }
        else
          lastUpdateLog += "sewn shape is null ";
      }
    }
    
    //used multiple times
    double midU = u0 + ((u1 - u0) / 2.0);
    double midV = v0 + ((v1 - v0) / 2.0);
    
    //update position of offset label. center of surface.
    gp_Pnt olp;
    surfaceCopy->D0(midU, midV, olp);
    stow->offsetLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(olp.Transformed(as.Trsf()))));
    
    //update position of close u label. be on u period half way in v
    gp_Pnt culp;
    surfaceCopy->D0(u0, midV, culp);
    stow->closeULabel->setMatrix(osg::Matrixd::translate(gu::toOsg(culp.Transformed(as.Trsf()))));
    
    //update position of close v label. be half way in u on period in v
    gp_Pnt cvlp;
    surfaceCopy->D0(midU, 0.0, cvlp);
    stow->closeVLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(cvlp.Transformed(as.Trsf()))));
    
    //update position of make solid label. at max u and v.
    gp_Pnt mslp;
    surfaceCopy->D0(u0, v1, mslp);
    stow->makeSolidLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(mslp.Transformed(as.Trsf()))));
    
    ShapeCheck check(out);
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    stow->sShape.setOCCTShape(out, getId());
    for (const auto &s : stow->sShape.getAllShapes())
    {
      if (tempIds.has(s))
        stow->sShape.updateId(s, tempIds.find(s));
    }
    stow->sShape.outerWireMatch(*pr.getSeerShape());
    stow->sShape.derivedMatch();
    stow->sShape.dumpNils(getTypeString());
    stow->sShape.dumpDuplicates(getTypeString());
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
  prj::srl::utr::Untrim so
  (
    Base::serialOut()
    , stow->sShape.serialOut()
    , stow->picks.serialOut()
    , stow->offset.serialOut()
    , stow->closeU.serialOut()
    , stow->closeV.serialOut()
    , stow->makeSolid.serialOut()
    , stow->offsetLabel->serialOut()
    , stow->closeULabel->serialOut()
    , stow->closeVLabel->serialOut()
    , stow->makeSolidLabel->serialOut()
    , gu::idToString(stow->solidId)
    , gu::idToString(stow->shellId)
  );
  
  for (const auto &id : stow->uvEdgeIds)
    so.uvEdgeIds().push_back(gu::idToString(id));
  for (const auto &entry : stow->edgeToCap)
    so.edgeToCap().push_back({gu::idToString(entry.first), gu::idToString(entry.second.first), gu::idToString(entry.second.second)});
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::utr::untrim(stream, so, infoMap);
}

void Feature::serialRead(const prj::srl::utr::Untrim &so)
{
  Base::serialIn(so.base());
  stow->sShape.serialIn(so.seerShape());
  stow->picks.serialIn(so.pick());
  stow->offset.serialIn(so.offset());
  stow->closeU.serialIn(so.closeU());
  stow->closeV.serialIn(so.closeV());
  stow->makeSolid.serialIn(so.makeSolid());
  stow->offsetLabel->serialIn(so.offsetLabel());
  stow->closeULabel->serialIn(so.closeULabel());
  stow->closeVLabel->serialIn(so.closeVLabel());
  stow->makeSolidLabel->serialIn(so.makeSolidLabel());
  stow->solidId = gu::stringToId(so.solidId());
  stow->shellId = gu::stringToId(so.shellId());
  
  for (const auto &sid : so.uvEdgeIds())
    stow->uvEdgeIds.emplace_back(gu::stringToId(sid));
  for (const auto &entry : so.edgeToCap())
    stow->edgeToCap.insert(std::make_pair(gu::stringToId(entry.keyId()), std::make_pair(gu::stringToId(entry.faceId()), gu::stringToId(entry.wireId()))));
}
