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
#include <BRepClass3d.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <BRep_Tool.hxx>
#include <BOPAlgo_Builder.hxx> //used for sewing edges
#include <BRep_Builder.hxx>
#include <ShapeFix_Wire.hxx>

#include <osg/Switch>

#include "globalutilities.h"
#include "tools/occtools.h"
#include "tools/idtools.h"
#include "tools/featuretools.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "annex/annseershape.h"
#include "annex/annintersectionmapper.h"
#include "project/serial/generated/prjsrlswssew.h"
#include "feature/ftrshapecheck.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrsew.h"

using namespace ftr::Sew;

using boost::uuids::uuid;
QIcon Feature::icon = QIcon(":/resources/images/constructionSew.svg");

struct Feature::Stow
{
  Feature &feature;
  prm::Parameter picks{prm::Names::Picks, ftr::Picks(), prm::Tags::Picks};
  ann::SeerShape sShape;
  ann::IntersectionMapper iMapper; //TODO add to serial
  uuid solidId{gu::createRandomId()};
  uuid shellId{gu::createRandomId()};
  uuid wireId{gu::createRandomId()}; //TODO add to serial
  
  Stow() = delete;
  Stow(Feature &fIn)
  : feature(fIn)
  {
    picks.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&picks);
    
    feature.annexes.insert(std::make_pair(ann::Type::SeerShape, &sShape));
    feature.annexes.insert(std::make_pair(ann::Type::IntersectionMapper, &iMapper));
  }
  
  void assignSolidShell()
  {
    occt::ShapeVector solids = sShape.useGetChildrenOfType(sShape.getRootOCCTShape(), TopAbs_SOLID);
    if (!solids.empty())
    {
      assert(solids.size() == 1);
      if (solids.size() > 1)
        std::cout << "WARNING: have more than 1 solid in Sew::assignSolidShell" << std::endl;
      sShape.updateId(solids.front(), solidId);
      if (!sShape.hasEvolveRecordOut(solidId))
        sShape.insertEvolve(gu::createNilId(), solidId);
      TopoDS_Shell shell = BRepClass3d::OuterShell(TopoDS::Solid(solids.front()));
      sShape.updateId(shell, shellId);
      if (!sShape.hasEvolveRecordOut(shellId))
        sShape.insertEvolve(gu::createNilId(), shellId);
    }
    else
    {
      occt::ShapeVector shells = sShape.useGetChildrenOfType(sShape.getRootOCCTShape(), TopAbs_SHELL);
      if (!shells.empty())
      {
        assert(shells.size() == 1);
        if (shells.size() > 1)
          std::cout << "WARNING: have more than 1 shell in Sew::assignSolidShell" << std::endl;
        sShape.updateId(shells.front(), shellId);
        if (!sShape.hasEvolveRecordOut(shellId))
          sShape.insertEvolve(gu::createNilId(), shellId);
      }
    }
  }
  
  void sewModifiedMatch(const BRepBuilderAPI_Sewing &builder, const ann::SeerShape &target)
  {
    occt::ShapeVector shapes = target.getAllShapes();
    for (const auto &ts : shapes)
    {
      uuid tid = target.findId(ts);
      
      auto goEvolve = [&](const TopoDS_Shape &result)
      {
        if (!sShape.hasShape(result))
        {
          //no warning this is pretty common.
          return;
        }
        
        //make sure the shape has nil for an id. Don't overwrite.
        uuid nid = sShape.findId(result);
        if (!nid.is_nil())
          return;
        
        if (sShape.hasEvolveRecordIn(tid))
          nid = sShape.evolve(tid).front(); //should only be 1 to 1
          else
          {
            nid = gu::createRandomId();
            sShape.insertEvolve(tid, nid);
          }
          sShape.updateId(result, nid);
      };
      
      //some shapes are in both modified and modifiedSubShape lists.
      if (builder.IsModified(ts))
        goEvolve(builder.Modified(ts));
      else if (builder.IsModifiedSubShape(ts))
        goEvolve(builder.ModifiedSubShape(ts));
    }
  }
  
  void goSewFaces(const occt::ShapeVector &faces, const std::vector<tls::Resolver> &resolvers)
  {
    BRepBuilderAPI_Sewing builder(0.000001, true, true, true, false);
    for (const auto &s : faces)
      builder.Add(s);
    builder.Perform(); //Perform Sewing
    //   builder.Dump();
    
    //sewing function is very liberal. Meaning that it really doesn't care if
    //faces are connected or not. It will put a dozen disconnected faces
    //into a compound and return the compound as a result. So we do some post
    //operation analysis to decide what is a success or failure. The criteria
    //for success and failure is up for debate.
    
    //for now we are just looking for a shell.
    TopoDS_Shape out;
    TopoDS_Shape output = builder.SewedShape();
    if (output.IsNull())
      throw std::runtime_error("output of sew is null");
    
    TopoDS_Shape nc = occt::getFirstNonCompound(output);
    if (nc.IsNull() || nc.ShapeType() != TopAbs_SHELL)
      throw std::runtime_error("can't find acceptable output shape");
    out = nc;
    
    //apparently sew doesn't set closed property on a closed shell.
    if (BRep_Tool::IsClosed(nc))
      nc.Closed(true);
    //try to make a solid.
    if (nc.Closed())
    {
      auto os = occt::buildSolid(nc);
      if (os)
        out = os.get();
    }
    
    ShapeCheck check(out);
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    sShape.setOCCTShape(out, feature.getId());
    for (const auto &r : resolvers)
      sShape.shapeMatch(*r.getSeerShape());
    //     stow->sShape.uniqueTypeMatch(ss);
    assignSolidShell();
    for (const auto &r : resolvers)
      sewModifiedMatch(builder, *r.getSeerShape());
  }
  
  void goSewEdges(const occt::ShapeVector &edges, const UpdatePayload &plIn)
  {
    BOPAlgo_Builder bop;
    for (const auto &e : edges) bop.AddArgument(e);
    bop.Perform();
    TopoDS_Shape bopShape = bop.Shape();
    if (bopShape.IsNull()) throw std::runtime_error("Null bop shape");
    if (bopShape.ShapeType() != TopAbs_COMPOUND) throw std::runtime_error("Unexpected bop shape type");
    occt::EdgeVector outEdges = occt::ShapeVectorCast(TopoDS::Compound(bopShape));
    
    //build the wire.
    BRep_Builder bldr;
    TopoDS_Wire result;
    bldr.MakeWire(result);
    for (const auto &e : outEdges) bldr.Add(result, e);
    
    //fix wire order and orientation
    ShapeFix_Wire fixer(result, TopoDS_Face(), Precision::Confusion());
    fixer.Perform();
    result = fixer.Wire();
    
    ShapeCheck check(result);
    if (!check.isValid()) throw std::runtime_error("shape check failed");
    
    sShape.setOCCTShape(result, feature.getId());
    iMapper.go(plIn, bop, sShape);
    
    //assign the wire id.
    occt::ShapeVector wires = sShape.useGetChildrenOfType(sShape.getRootOCCTShape(), TopAbs_WIRE);
    if (wires.empty()) throw std::runtime_error("No wire in result");
    if (wires.size() > 1)
    {
      std::ostringstream s; s << "Warning: more than 1 wire" << std::endl;
      feature.lastUpdateLog += s.str();
    }
    sShape.updateId(wires.front(), wireId);
    if (!sShape.hasEvolveRecordOut(wireId))
      sShape.insertEvolve(gu::createNilId(), wireId);
  }
};

Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  name = QObject::tr("Sew");
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
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    const auto &picks = stow->picks.getPicks();
    std::vector<tls::Resolver> resolvers;
    occt::ShapeVector faceShapes;
    occt::ShapeVector edgeShapes;
    for (const auto &p : picks)
    {
      auto &r = resolvers.emplace_back(payloadIn);
      if (!r.resolve(p) || !r.getSeerShape() || r.getSeerShape()->isNull())
      {
        std::ostringstream s; s << "Warning: Pick resolution failure" << std::endl;
        lastUpdateLog += s.str();
        resolvers.pop_back();
        continue;
      }
      occt::ShapeVector temp;
      if (slc::isObjectType(p.selectionType))
        temp = r.getSeerShape()->useGetNonCompoundChildren();
      else
        temp = r.getShapes();

      //filter out any unsupported shapes.
      for (auto s : temp)
      {
        if (s.ShapeType() == TopAbs_SHELL || s.ShapeType() == TopAbs_FACE)
          faceShapes.push_back(s);
        else if (s.ShapeType() == TopAbs_WIRE || s.ShapeType() == TopAbs_EDGE)
          edgeShapes.push_back(s);
      }
    }
    
    if (!faceShapes.empty() && edgeShapes.empty())
    {
      stow->goSewFaces(faceShapes, resolvers);
    }
    else if (faceShapes.empty() && !edgeShapes.empty())
    {
      stow->goSewEdges(edgeShapes, payloadIn);
      for (const auto &it : resolvers)
        stow->sShape.shapeMatch(*it.getSeerShape());
    }
    else
      throw std::runtime_error("Empty or incompatible shapes");
    
    for (const auto &r : resolvers)
      stow->sShape.outerWireMatch(*r.getSeerShape());
    stow->sShape.derivedMatch();
    stow->sShape.dumpNils("sew feature");
    stow->sShape.dumpDuplicates("sew feature");
    stow->sShape.ensureNoNils();
    stow->sShape.ensureNoDuplicates();
    stow->sShape.ensureEvolve();
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in sew update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in sew update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in sew update. " << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Feature::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::sws::Sew so
  (
    Base::serialOut()
    , stow->picks.serialOut()
    , stow->sShape.serialOut()
    , stow->iMapper.serialOut()
    , gu::idToString(stow->solidId)
    , gu::idToString(stow->shellId)
    , gu::idToString(stow->wireId)
  );

  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::sws::sew(stream, so, infoMap);
}

void Feature::serialRead(const prj::srl::sws::Sew &si)
{
  Base::serialIn(si.base());
  stow->picks.serialIn(si.picks());
  stow->sShape.serialIn(si.seerShape());
  stow->iMapper.serialIn(si.iMapper());
  stow->solidId = gu::stringToId(si.solidId());
  stow->shellId = gu::stringToId(si.shellId());
  stow->wireId = gu::stringToId(si.wireId());
}
