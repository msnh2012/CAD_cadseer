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
#include <BRepOffsetAPI_MakeOffsetShape.hxx>

#include <osg/Switch>

#include "globalutilities.h"
#include "tools/idtools.h"
#include "tools/occtools.h"
#include "library/lbrplabel.h"
#include "annex/annseershape.h"
#include "tools/featuretools.h"
#include "project/serial/xsdcxxoutput/featureoffset.h"
#include "feature/ftrshapecheck.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinputtype.h"
#include "parameter/prmparameter.h"
#include "feature/ftroffset.h"

using namespace ftr;

using boost::uuids::uuid;

QIcon Offset::icon;

Offset::Offset():
Base(),
distance(new prm::Parameter(prm::Names::Distance, 0.1)),
sShape(new ann::SeerShape())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionOffset.svg");
  
  name = QObject::tr("Offset");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  distance->setConstraint(prm::Constraint::buildNonZero());
  distance->connectValue(std::bind(&Offset::setModelDirty, this));
  parameters.push_back(distance.get());
  
  distanceLabel = new lbr::PLabel(distance.get());
  distanceLabel->showName = true;
  distanceLabel->valueHasChanged();
  distanceLabel->constantHasChanged();
  overlaySwitch->addChild(distanceLabel.get());
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
}

Offset::~Offset(){}

void Offset::setPicks(const Picks &pIn)
{
  picks = pIn;
  setModelDirty();
}

static std::string getErrorString(const BRepOffset_MakeOffset &bIn)
{
  static const std::map<BRepOffset_Error, std::string> errors = 
  {
    std::make_pair(BRepOffset_NoError, "BRepOffset_NoError"),
    std::make_pair(BRepOffset_UnknownError, "BRepOffset_UnknownError"),
    std::make_pair(BRepOffset_BadNormalsOnGeometry, "BRepOffset_BadNormalsOnGeometry"),
    std::make_pair(BRepOffset_C0Geometry, "BRepOffset_C0Geometry"),
    std::make_pair(BRepOffset_NullOffset, "BRepOffset_NullOffset"),
    std::make_pair(BRepOffset_NotConnectedShell, "BRepOffset_NotConnectedShell")
  };
  
  BRepOffset_Error e = bIn.Error();
  std::ostringstream stream;
  stream << "failed with error: " << errors.at(e);
  return stream.str();
}

/*notes:
 * occt 7.2 checked source and 'BRepOffset_MakeOffset::myBadShape' is never set. This member is returned
 * from BRepOffset_MakeOffset::GetBadShape(), so it is useless. This suffers the same tolerance issue as the
 * hollow feature. When passing in the target shape compound, the occt offset algorithm will ignore a
 * solid and return a shell. But if we pass in a solid we will get a solid back.
 */
void Offset::updateModel(const UpdatePayload &payloadIn)
{
  setFailure();
  lastUpdateLog.clear();
  sShape->reset();
  try
  {
    std::vector<const Base*> tfs = payloadIn.getFeatures(std::string());
    if (tfs.size() != 1)
      throw std::runtime_error("wrong number of parents");
    if (!tfs.front()->hasAnnex(ann::Type::SeerShape))
      throw std::runtime_error("parent doesn't have seer shape.");
    const ann::SeerShape &tss = tfs.front()->getAnnex<ann::SeerShape>();
    if (tss.isNull())
      throw std::runtime_error("target seer shape is null");
    
    //setup failure state.
    sShape->setOCCTShape(tss.getRootOCCTShape(), getId());
    sShape->shapeMatch(tss);
    sShape->uniqueTypeMatch(tss);
    sShape->outerWireMatch(tss);
    sShape->derivedMatch();
    sShape->ensureNoNils();
    sShape->ensureNoDuplicates();
    
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    if (picks.empty())
      throw std::runtime_error("No picks");
    
    //we basically have 2 modes. 1 is where we offset the whole shape. 2 is where we offset faces.
    osg::Matrixd labelPosition = osg::Matrixd::identity();
    if (slc::isObjectType(picks.front().selectionType))
    {
      //this offsetting the whole shape.
      if (picks.size() != 1)
        throw std::runtime_error("Should have only 1 pick for whole object");
      tls::Resolver pr(payloadIn);
      if (!pr.resolve(picks.front()))
        throw std::runtime_error("Pick resolution failed for whole object");
      if (!pr.getSeerShape() || pr.getSeerShape()->isNull())
        throw std::runtime_error("No or invalid seer shape for whole object");
      
      TopoDS_Shape sto; //shape to offset.
      for (const auto &s : pr.getSeerShape()->useGetNonCompoundChildren())
      {
        if
        (
          (s.ShapeType() == TopAbs_SOLID) ||
          (s.ShapeType() == TopAbs_SHELL) ||
          (s.ShapeType() == TopAbs_FACE)
        )
        {
          sto = s;
          break;
        }
      }
      if (sto.IsNull())
        throw std::runtime_error("target input doesn't have solid, shell or face");
      
      BRepOffsetAPI_MakeOffsetShape builder;
      builder.PerformByJoin
      (
        sto,
        static_cast<double>(*distance),
        1.0e-06, //same tolerance as the sewing default.
        BRepOffset_Skin,
        Standard_False,
        Standard_False,
        GeomAbs_Intersection,
        Standard_False //maybe be true if have internal edges in results?
      );
      if (!builder.IsDone())
        throw std::runtime_error(getErrorString(builder.MakeOffset()));
      ShapeCheck check(builder.Shape());
      if (!check.isValid())
        throw std::runtime_error("shapeCheck failed");
      
      sShape->setOCCTShape(builder.Shape(), getId());
      sShape->shapeMatch(*pr.getSeerShape());
      sShape->uniqueTypeMatch(*pr.getSeerShape());
      sShape->modifiedMatch(builder, *pr.getSeerShape());
      offsetMatch(builder.MakeOffset(), *pr.getSeerShape());
      sShape->outerWireMatch(*pr.getSeerShape());
      sShape->derivedMatch();
      sShape->dumpNils("offset feature");
      sShape->dumpDuplicates("offset feature");
      sShape->ensureNoNils();
      sShape->ensureNoDuplicates();
      
      occt::BoundingBox bb(builder.Shape());
      labelPosition = osg::Matrixd::translate(gu::toOsg(bb.getCenter()));
    }
    else
    {
      bool labelDone = false;
      BRepOffset_MakeOffset builder;
      builder.Initialize
      (
        tss.getRootOCCTShape(),
        0.0, //actual offsets will be assigned in loop.
        1.0e-06, //same tolerance as the sewing default.
        BRepOffset_Skin, //offset mode
        Standard_False, //intersection
        Standard_False, //self intersection
        GeomAbs_Intersection, //join type.
        Standard_False, //thickening.
        Standard_False //remove internal edges
      );
      
      tls::Resolver pr(payloadIn);
      for (const auto &p : picks)
      {
        if (p.selectionType != slc::Type::Face)
          throw std::runtime_error("Non face selection type");
        if (!pr.resolve(p))
          throw std::runtime_error("Failed to resolve face pick");
        for (const auto &s : pr.getShapes())
        {
          if (s.ShapeType() != TopAbs_FACE)
            continue;
          if (!labelDone)
          {
            labelDone = true;
            labelPosition = osg::Matrixd::translate(p.getPoint(TopoDS::Face(s)));
          }
          builder.SetOffsetOnFace(TopoDS::Face(s), static_cast<double>(*distance));
        }
      }
      
      builder.MakeOffsetShape();
      if (!builder.IsDone())
        throw std::runtime_error(getErrorString(builder));
      ShapeCheck check(builder.Shape());
      if (!check.isValid())
        throw std::runtime_error("shapeCheck failed");
      
      sShape->setOCCTShape(builder.Shape(), getId());
      sShape->shapeMatch(tss);
      sShape->uniqueTypeMatch(tss);
      offsetMatch(builder, tss);
      sShape->outerWireMatch(tss);
      sShape->derivedMatch();
      sShape->dumpNils("offset feature");
      sShape->dumpDuplicates("offset feature");
      sShape->ensureNoNils();
      sShape->ensureNoDuplicates();
    }
    distanceLabel->setMatrix(labelPosition);
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in offset update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in offset update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in offset update. " << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Offset::offsetMatch(const BRepOffset_MakeOffset &offseter, const ann::SeerShape &tss)
{
  const BRepAlgo_Image &oFaces = offseter.OffsetFacesFromShapes();
  const BRepAlgo_Image &oEdges = offseter.OffsetEdgesFromShapes();
  occt::ShapeVector as = tss.getAllShapes();
  
  //testing results
//   int fc = 0;
//   int ec = 0;
//   for (const auto &s : as)
//   {
//     if (oFaces.HasImage(s))
//       fc++;
//     if (oEdges.HasImage(s))
//       ec++;
//   }
//   std::cout << std::endl << "face count is: " << fc
//   << "    edge count is: " << ec << std::endl;
  //output of offseting 1 face of a box
    //"face count is: 6    edge count is: 12"
  //output of offseting feature consisting of extracted, tangent region of a box with 1 edge blended.
    //face count is: 3    edge count is: 10
  //it looks like offset is creating all new shapes.
  
  auto update = [&](const TopoDS_Shape &original, const TopoDS_Shape &offsetShape)
  {
    assert(tss.hasShape(original));
    assert(sShape->hasShape(offsetShape));
    uuid oid = tss.findId(original);
    uuid nid = gu::createRandomId();
    if (sShape->hasEvolveRecordIn(oid))
      nid = sShape->evolve(oid).front(); //should be 1 to 1.
    else
      sShape->insertEvolve(oid, nid);
    sShape->updateId(offsetShape, nid);
  };
  
  for (const auto &s : as)
  {
    if (oFaces.HasImage(s))
    {
      const TopTools_ListOfShape &images = oFaces.Image(s);
      if (!images.IsEmpty())
      {
        if (images.Extent() > 1)
          std::cout << "WARNING: more than 1 face image in Offset::offsetMatch" << std::endl;
        update(s, images.First());
      }
    }
    if (oEdges.HasImage(s))
    {
      const TopTools_ListOfShape &images = oEdges.Image(s);
      if (!images.IsEmpty())
      {
        if (images.Extent() > 1)
          std::cout << "WARNING: more than 1 edge image in Offset::offsetMatch" << std::endl;
        update(s, images.First());
      }
    }
  }
}

void Offset::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::FeatureOffset so
  (
    Base::serialOut(),
    ftr::serialOut(picks),
    distance->serialOut(),
    distanceLabel->serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::offset(stream, so, infoMap);
}

void Offset::serialRead(const prj::srl::FeatureOffset &so)
{
  Base::serialIn(so.featureBase());
  distance->serialIn(so.distance());
  picks = ftr::serialIn(so.picks());
  distanceLabel->serialIn(so.distanceLabel());
}
