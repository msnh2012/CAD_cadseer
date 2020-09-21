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
#include <BRep_Builder.hxx>
#include <ShapeFix_Wire.hxx>
#include <ShapeBuild_ReShape.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>

#include <osg/Switch>

#include "globalutilities.h"
#include "annex/annseershape.h"
#include "tools/occtools.h"
#include "tools/featuretools.h"
#include "tools/tlsshapeid.h"
#include "tools/idtools.h"
#include "feature/ftrshapecheck.h"
#include "feature/ftrupdatepayload.h"
#include "project/serial/generated/prjsrlfceface.h"
#include "feature/ftrface.h"

using namespace ftr;
using boost::uuids::uuid;

QIcon Face::icon;

Face::Face():
Base()
, sShape(new ann::SeerShape())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionFace.svg");
  
  name = QObject::tr("Face");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
  
  faceId = gu::createRandomId();
  wireId = gu::createRandomId();
}

Face::~Face() = default;

void Face::setPicks(const Picks &pIn)
{
  picks = pIn;
  setModelDirty();
}

void Face::updateModel(const UpdatePayload &pIn)
{
  setFailure();
  lastUpdateLog.clear();
  sShape->reset();
  try
  {
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    tls::Resolver pr(pIn);
    tls::ShapeIdContainer tempIds;
    occt::EdgeVector edges;
    for (const auto &p : picks)
    {
      if (!pr.resolve(p))
      {
        lastUpdateLog += QObject::tr("invalid pick resolution").toStdString();
        continue;
      }
      auto shapes = pr.getShapes(p);
      for (const auto &s : shapes)
      {
        auto processEdge = [&](const TopoDS_Edge &eIn)
        {
          edges.push_back(eIn);
          uuid eId = pr.getSeerShape()->findId(eIn);
          if (!sShape->hasEvolveRecordIn(eId))
            sShape->insertEvolve(eId, gu::createRandomId());
          tempIds.insert(sShape->evolve(eId).front(), eIn);
        };
        
        if (s.ShapeType() == TopAbs_EDGE)
        {
          processEdge(TopoDS::Edge(s));
        }
        else if (s.ShapeType() == TopAbs_COMPOUND || s.ShapeType() == TopAbs_WIRE)
        {
          auto ce = pr.getSeerShape()->useGetChildrenOfType(s, TopAbs_EDGE);
          for (const auto e : ce)
            processEdge(TopoDS::Edge(e));
        }
      }
    }
    
    //create an invalid wire
    BRep_Builder builder;
    TopoDS_Wire wire;
    builder.MakeWire(wire);
    for (const auto &e : edges)
      builder.Add(wire, e);
    
    //fix wire
    opencascade::handle<ShapeBuild_ReShape> context = new ShapeBuild_ReShape();
    ShapeFix_Wire wireFixer;
    wireFixer.SetContext(context);
    wireFixer.FixConnectedMode() = 1;
    wireFixer.FixReorderMode() = 1;
    wireFixer.ClosedWireMode() = Standard_True;
    wireFixer.Load(wire);
    wireFixer.Perform();
    TopoDS_Wire wireOut = wireFixer.WireAPIMake();
    if (wireOut.IsNull())
      throw std::runtime_error("wire out is null");
    
    //probe reshape to track changes made by fix wire
    for (const auto &e : edges)
    {
      TopoDS_Shape temp;
      int status = context->Status(e, temp);
      if (status > 0)
        tempIds.update(e, temp);
    }
    
    if (!sShape->hasEvolveRecordOut(wireId))
      sShape->insertEvolve(gu::createNilId(), wireId);
    tempIds.insert(wireId, wireOut);
    
    BRepBuilderAPI_MakeFace faceMaker(wireOut);
    if (!faceMaker.IsDone())
      throw std::runtime_error("failed making failed");
    TopoDS_Face faceOut = faceMaker;
    if (faceOut.IsNull())
      throw std::runtime_error("face out is null");
    if (!sShape->hasEvolveRecordOut(faceId))
      sShape->insertEvolve(gu::createNilId(), faceId);
    tempIds.insert(faceId, faceOut);
    
    ShapeCheck check(faceOut);
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    sShape->setOCCTShape(faceOut, getId());
    for (const auto &s : sShape->getAllShapes())
    {
      if (tempIds.has(s))
        sShape->updateId(s, tempIds.find(s));
    }
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

void Face::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::fce::Face so
  (
    Base::serialOut()
    , sShape->serialOut()
  );
  for (const auto &p : picks)
    so.picks().push_back(p);
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::fce::face(stream, so, infoMap);
}

void Face::serialRead(const prj::srl::fce::Face &so)
{
  Base::serialIn(so.base());
  sShape->serialIn(so.seerShape());
  for (const auto &p : so.picks())
    picks.emplace_back(p);
}
