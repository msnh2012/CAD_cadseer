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
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "annex/annseershape.h"
#include "tools/occtools.h"
#include "tools/featuretools.h"
#include "tools/tlsshapeid.h"
#include "tools/idtools.h"
#include "feature/ftrshapecheck.h"
#include "feature/ftrupdatepayload.h"
#include "project/serial/generated/prjsrlfceface.h"
#include "feature/ftrface.h"

using namespace ftr::Face;
using boost::uuids::uuid;

QIcon Feature::icon = QIcon(":/resources/images/constructionFace.svg");

struct Feature::Stow
{
  Feature &feature;
  prm::Parameter picks{prm::Names::Picks, ftr::Picks(), prm::Tags::Picks};
  ann::SeerShape sShape;
  uuid wireId;
  uuid faceId;
  
  Stow() = delete;
  Stow(Feature &fIn)
  : feature(fIn)
  {
    picks.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&picks);
    feature.annexes.insert(std::make_pair(ann::Type::SeerShape, &sShape));
    wireId = gu::createRandomId();
    faceId = gu::createRandomId();
  }
};

Feature::Feature():
Base()
, stow(std::make_unique<Stow>(*this))
{
  name = QObject::tr("Face");
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
    
    tls::Resolver pr(pIn);
    tls::ShapeIdContainer tempIds;
    occt::EdgeVector edges;
    for (const auto &p : stow->picks.getPicks())
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
          if (!stow->sShape.hasEvolveRecordIn(eId))
            stow->sShape.insertEvolve(eId, gu::createRandomId());
          tempIds.insert(stow->sShape.evolve(eId).front(), eIn);
        };
        
        if (s.ShapeType() == TopAbs_EDGE)
        {
          processEdge(TopoDS::Edge(s));
        }
        else if (s.ShapeType() == TopAbs_COMPOUND || s.ShapeType() == TopAbs_WIRE)
        {
          auto ce = pr.getSeerShape()->useGetChildrenOfType(s, TopAbs_EDGE);
          for (const auto &e : ce)
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
    
    if (!stow->sShape.hasEvolveRecordOut(stow->wireId))
      stow->sShape.insertEvolve(gu::createNilId(), stow->wireId);
    tempIds.insert(stow->wireId, wireOut);
    
    BRepBuilderAPI_MakeFace faceMaker(wireOut);
    if (!faceMaker.IsDone())
      throw std::runtime_error("failed making failed");
    TopoDS_Face faceOut = faceMaker;
    if (faceOut.IsNull())
      throw std::runtime_error("face out is null");
    if (!stow->sShape.hasEvolveRecordOut(stow->faceId))
      stow->sShape.insertEvolve(gu::createNilId(), stow->faceId);
    tempIds.insert(stow->faceId, faceOut);
    
    ShapeCheck check(faceOut);
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    stow->sShape.setOCCTShape(faceOut, getId());
    for (const auto &s : stow->sShape.getAllShapes())
    {
      if (tempIds.has(s))
        stow->sShape.updateId(s, tempIds.find(s));
    }
    stow->sShape.derivedMatch();
    stow->sShape.dumpNils(getTypeString());
    stow->sShape.dumpDuplicates(getTypeString());
    stow->sShape.ensureNoNils();
    stow->sShape.ensureNoDuplicates();
    stow->sShape.ensureEvolve();
    
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
  prj::srl::fce::Face so
  (
    Base::serialOut()
    , stow->picks.serialOut()
    , stow->sShape.serialOut()
    , gu::idToString(stow->wireId)
    , gu::idToString(stow->faceId)
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::fce::face(stream, so, infoMap);
}

void Feature::serialRead(const prj::srl::fce::Face &so)
{
  Base::serialIn(so.base());
  stow->picks.serialIn(so.picks());
  stow->sShape.serialIn(so.seerShape());
  stow->wireId = gu::stringToId(so.wireId());
  stow->faceId = gu::stringToId(so.faceId());
}
