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
#include "parameter/prmparameter.h"
#include "tools/occtools.h"
#include "tools/featuretools.h"
#include "tools/idtools.h"
#include "tools/tlsshapeid.h"
#include "feature/ftrshapecheck.h"
#include "feature/ftrupdatepayload.h"
#include "project/serial/generated/prjsrlutruntrim.h"
#include "feature/ftruntrim.h"

using namespace ftr;

QIcon Untrim::icon;

Untrim::Untrim():
Base()
, sShape(std::make_unique<ann::SeerShape>())
, offset(std::make_unique<prm::Parameter>(prm::Names::Offset, 0.1, prm::Tags::Offset))
, closeU(std::make_unique<prm::Parameter>(QObject::tr("Close U"), false))
, closeV(std::make_unique<prm::Parameter>(QObject::tr("Close V"), false))
, makeSolid(std::make_unique<prm::Parameter>(QObject::tr("Make Solid"), false))
, offsetLabel(new lbr::PLabel(offset.get()))
, closeULabel(new lbr::PLabel(closeU.get()))
, closeVLabel(new lbr::PLabel(closeV.get()))
, makeSolidLabel(new lbr::PLabel(makeSolid.get()))
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionUntrim.svg");
  
  name = QObject::tr("Untrim");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  offset->setConstraint(prm::Constraint::buildZeroPositive());
  offset->connectValue(std::bind(&Untrim::setModelDirty, this));
  parameters.push_back(offset.get());
  
  closeU->connectValue(std::bind(&Untrim::setModelDirty, this));
  parameters.push_back(closeU.get());
  
  closeV->connectValue(std::bind(&Untrim::setModelDirty, this));
  parameters.push_back(closeV.get());
  
  makeSolid->connectValue(std::bind(&Untrim::setModelDirty, this));
  parameters.push_back(makeSolid.get());
  
  overlaySwitch->addChild(offsetLabel.get());
  overlaySwitch->addChild(closeULabel.get());
  overlaySwitch->addChild(closeVLabel.get());
  overlaySwitch->addChild(makeSolidLabel.get());
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
  
  solidId = gu::createRandomId();
  shellId = gu::createRandomId();
}

Untrim::~Untrim() = default;

void Untrim::setPick(const Pick &pIn)
{
  pick = pIn;
  setModelDirty();
}

//cones will probably need some special attention. The uv bounds
//don't consider apex and we can lengthen them right through the apex.
//I created a solid with the cone surface lengthened through the apex,
//bopargcheck didn't bitch, so maybe leave it.
void Untrim::updateModel(const UpdatePayload &pIn)
{
  setFailure();
  lastUpdateLog.clear();
  sShape->reset();
  try
  {
    tls::Resolver pr(pIn);
    if (!pr.resolve(pick))
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
    sShape->setOCCTShape(shapeToUntrim, getId());
    sShape->shapeMatch(*pr.getSeerShape());
    sShape->uniqueTypeMatch(*pr.getSeerShape());
    sShape->outerWireMatch(*pr.getSeerShape());
    sShape->derivedMatch();
    sShape->ensureNoNils(); //just in case
    sShape->ensureNoDuplicates(); //just in case
    
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    BRepAdaptor_Surface as(TopoDS::Face(shapeToUntrim));
    opencascade::handle<Geom_Surface> surfaceCopy = dynamic_cast<Geom_Surface*>(as.Surface().Surface()->Copy().get());
    
    double ov = static_cast<double>(*offset);
    
    double u0 = as.FirstUParameter() - ov;
    double u1 = as.LastUParameter() + ov;
    double v0 = as.FirstVParameter() - ov;
    double v1 = as.LastVParameter() + ov;
    
    if (as.IsUPeriodic())
    {
      if (static_cast<bool>(*closeU))
      {
        u0 = 0.0;
        u1 = as.UPeriod();
      }
      u0 = std::max(0.0, u0);
      u1 = std::min(as.UPeriod(), u1);
      if (!overlaySwitch->containsNode(closeULabel.get()))
        overlaySwitch->addChild(closeULabel.get());
    }
    else
      overlaySwitch->removeChild(closeULabel.get());
    
    if (as.IsVPeriodic())
    {
      if (static_cast<bool>(*closeV))
      {
        v0 = 0.0;
        v1 = as.VPeriod();
      }
      v0 = std::max(0.0, v0);
      v1 = std::min(as.VPeriod(), v1);
      if (!overlaySwitch->containsNode(closeVLabel.get()))
        overlaySwitch->addChild(closeVLabel.get());
    }
    else
      overlaySwitch->removeChild(closeVLabel.get());
    
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
    if (!sShape->hasEvolveRecordIn(originalId))
      sShape->insertEvolve(originalId, gu::createRandomId());
    
    BRepBuilderAPI_MakeFace fm(surfaceCopy, u0, u1, v0, v1, Precision::Confusion());
    if (!fm.IsDone())
      throw std::runtime_error("couldn't make face");
    TopoDS_Shape untrimmedFace = fm.Face();
    untrimmedFace.Location(as.Trsf());
    TopoDS_Shape out = untrimmedFace;
    tempIds.insert(sShape->evolve(originalId).front(), untrimmedFace);
    //here we are going to assume that the edges always come in the same order.
    auto uvEdgeIt = uvEdgeIds.begin();
    for (const auto &e : static_cast<occt::EdgeVector>(occt::ShapeVectorCast(occt::mapShapes(untrimmedFace))))
    {
      if (uvEdgeIt == uvEdgeIds.end())
      {
        uvEdgeIds.push_back(gu::createRandomId());
        tempIds.insert(uvEdgeIds.back(), e);
        uvEdgeIt = uvEdgeIds.end(); //push_back might invalidate
        continue;
      }
      else
      {
        tempIds.insert(*uvEdgeIt, e);
        uvEdgeIt++;
      }
    }
    
    
    if (static_cast<bool>(*makeSolid))
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
            if (!sShape->hasEvolveRecordOut(shellId))
              sShape->insertEvolve(gu::createNilId(), shellId);
            tempIds.insert(shellId, shell);
            if (!sShape->hasEvolveRecordOut(solidId))
              sShape->insertEvolve(gu::createNilId(), solidId);
            tempIds.insert(solidId, out);
            
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
              if (edgeToCap.count(edgeId) == 0)
              {
                uuid freshFace = gu::createRandomId();
                uuid freshWire = gu::createRandomId();
                edgeToCap.insert({edgeId, std::make_pair(freshFace, freshWire)});
                sShape->insertEvolve(gu::createNilId(), freshFace);
                sShape->insertEvolve(gu::createNilId(), freshWire);
              }
              auto it = edgeToCap.find(edgeId);
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
    offsetLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(olp.Transformed(as.Trsf()))));
    
    //update position of close u label. be on u period half way in v
    gp_Pnt culp;
    surfaceCopy->D0(u0, midV, culp);
    closeULabel->setMatrix(osg::Matrixd::translate(gu::toOsg(culp.Transformed(as.Trsf()))));
    
    //update position of close v label. be half way in u on period in v
    gp_Pnt cvlp;
    surfaceCopy->D0(midU, 0.0, cvlp);
    closeVLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(cvlp.Transformed(as.Trsf()))));
    
    //update position of make solid label. at max u and v.
    gp_Pnt mslp;
    surfaceCopy->D0(u0, v1, mslp);
    makeSolidLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(mslp.Transformed(as.Trsf()))));
    
    ShapeCheck check(out);
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    sShape->setOCCTShape(out, getId());
    for (const auto &s : sShape->getAllShapes())
    {
      if (tempIds.has(s))
        sShape->updateId(s, tempIds.find(s));
    }
    sShape->outerWireMatch(*pr.getSeerShape());
    sShape->derivedMatch();
    sShape->dumpNils(getTypeString());
    sShape->dumpDuplicates(getTypeString());
    sShape->ensureNoNils();
    sShape->ensureNoDuplicates();
    
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

void Untrim::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::utr::Untrim so
  (
    Base::serialOut()
    , sShape->serialOut()
    , pick.serialOut()
    , offset->serialOut()
    , closeU->serialOut()
    , closeV->serialOut()
    , makeSolid->serialOut()
    , offsetLabel->serialOut()
    , closeULabel->serialOut()
    , closeVLabel->serialOut()
    , makeSolidLabel->serialOut()
    , gu::idToString(solidId)
    , gu::idToString(shellId)
  );
  
  for (const auto &id : uvEdgeIds)
    so.uvEdgeIds().push_back(gu::idToString(id));
  for (const auto &entry : edgeToCap)
    so.edgeToCap().push_back({gu::idToString(entry.first), gu::idToString(entry.second.first), gu::idToString(entry.second.second)});
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::utr::untrim(stream, so, infoMap);
}

void Untrim::serialRead(const prj::srl::utr::Untrim &so)
{
  Base::serialIn(so.base());
  sShape->serialIn(so.seerShape());
  pick.serialIn(so.pick());
  offset->serialIn(so.offset());
  closeU->serialIn(so.closeU());
  closeV->serialIn(so.closeV());
  makeSolid->serialIn(so.makeSolid());
  offsetLabel->serialIn(so.offsetLabel());
  closeULabel->serialIn(so.closeULabel());
  closeVLabel->serialIn(so.closeVLabel());
  makeSolidLabel->serialIn(so.makeSolidLabel());
  solidId = gu::stringToId(so.solidId());
  shellId = gu::stringToId(so.shellId());
  
  for (const auto &sid : so.uvEdgeIds())
    uvEdgeIds.emplace_back(gu::stringToId(sid));
  for (const auto &entry : so.edgeToCap())
    edgeToCap.insert(std::make_pair(gu::stringToId(entry.keyId()), std::make_pair(gu::stringToId(entry.faceId()), gu::stringToId(entry.wireId()))));
  
//   offsetLabel->valueHasChanged();
//   offsetLabel->constantHasChanged();
}
