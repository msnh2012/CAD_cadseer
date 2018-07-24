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

#include <globalutilities.h>
#include <tools/occtools.h>
#include <tools/idtools.h>
#include <library/plabel.h>
#include <annex/seershape.h>
#include <annex/intersectionmapper.h>
#include <project/serial/xsdcxxoutput/featuretrim.h>
#include <feature/booleanoperation.h>
#include <feature/shapecheck.h>
#include <feature/updatepayload.h>
#include <feature/parameter.h>
#include <feature/inputtype.h>
#include <feature/datumplane.h>
#include <feature/trim.h>

using namespace ftr;

using boost::uuids::uuid;

QIcon Trim::icon;

Trim::Trim():
Base(),
reversed(new prm::Parameter(QObject::tr("Reversed"), false)),
sShape(new ann::SeerShape()),
iMapper(new ann::IntersectionMapper()),
dpFaceId(gu::createRandomId()),
dpWireId(gu::createRandomId())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionTrim.svg");
  
  name = QObject::tr("Trim");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  reversed->connectValue(boost::bind(&Trim::setModelDirty, this));
  parameters.push_back(reversed.get());
  
  reversedLabel = new lbr::PLabel(reversed.get());
  reversedLabel->showName = true;
  reversedLabel->valueHasChanged();
  reversedLabel->constantHasChanged();
  overlaySwitch->addChild(reversedLabel.get());
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
  annexes.insert(std::make_pair(ann::Type::IntersectionMapper, iMapper.get()));
}

Trim::~Trim(){}

TopoDS_Solid Trim::makeHalfSpace(const ann::SeerShape &seerShapeIn, const TopoDS_Shape &shapeIn)
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
  if (static_cast<bool>(*reversed))
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

void Trim::updateModel(const UpdatePayload &payloadIn)
{
  setFailure();
  lastUpdateLog.clear();
  sShape->reset();
  try
  {
    std::vector<const Base*> trfs = payloadIn.getFeatures(InputType::target);
    if (trfs.size() != 1)
      throw std::runtime_error("wrong number of target parents");
    if (!trfs.front()->hasAnnex(ann::Type::SeerShape))
      throw std::runtime_error("target parent doesn't have seer shape.");
    const ann::SeerShape &trss = trfs.front()->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
    if (trss.isNull())
      throw std::runtime_error("target seer shape is null");
    
    std::vector<const Base*> tlfs = payloadIn.getFeatures(InputType::tool);
    if (tlfs.size() != 1)
      throw std::runtime_error("wrong number of tool parents");
    
    TopoDS_Solid toolSolid;
    bool fromDatum = false;
    const ann::SeerShape *tlss = nullptr;
    if (tlfs.front()->getType() == ftr::Type::DatumPlane)
    {
      fromDatum = true;
      const DatumPlane *dp = static_cast<const DatumPlane*>(tlfs.front());
      osg::Matrixd ts = dp->getSystem();
      BRepBuilderAPI_MakeFace fm(gp_Pln(gp_Ax3(gu::toOcc(ts))));
      if (!fm.IsDone())
        throw std::runtime_error("couldn't make face from datum plane");
      osg::Vec3d rp(0.0, 0.0, 1.0);
      if (static_cast<bool>(*reversed))
        rp = -rp;
      rp = rp * ts;
      BRepPrimAPI_MakeHalfSpace hsm(fm.Face(), gp_Pnt(gu::toOcc(rp).XYZ()));
      if (!hsm.IsDone())
        throw std::runtime_error("couldn't make half space from datum plane");
      toolSolid = hsm.Solid();
      reversedLabel->setMatrix(ts);
      
      //create an id linked to datum plane. this might cause problems as
      //dplane feature id won't be in shape history
      const uuid& dpId = tlfs.front()->getId();
      if (!sShape->hasEvolveRecordIn(dpId))
      {
        uuid fid = gu::createRandomId();
        sShape->insertEvolve(dpId, fid);
        sShape->insertEvolve(fid, gu::createRandomId()); //used for outer wire
      }
    }
    else
    {
      if (!tlfs.front()->hasAnnex(ann::Type::SeerShape))
        throw std::runtime_error("tool parent doesn't have seer shape.");
      tlss = &tlfs.front()->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
      if (tlss->isNull())
        throw std::runtime_error("tool seer shape is null");
      
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
      toolSolid = makeHalfSpace(*tlss, toolShape);
      occt::BoundingBox bb(toolShape);
      reversedLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(bb.getCenter())));
    }
    if (toolSolid.IsNull())
      throw std::runtime_error("couldn't construct tool solid");
    
    BooleanOperation subtracter(trss.getRootOCCTShape(), toolSolid, BOPAlgo_CUT);
    subtracter.Build();
    if (!subtracter.IsDone())
      throw std::runtime_error("OCC subtraction failed");
    ShapeCheck check(subtracter.Shape());
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    sShape->setOCCTShape(subtracter.Shape());
    
    //WARNING: can't find keyId for face split in IntersectionMapper::go     from datum plane trim
    iMapper->go(payloadIn, subtracter.getBuilder(), *sShape);
    
    sShape->shapeMatch(trss);
    if (!fromDatum)
      sShape->shapeMatch(*tlss);
    else
      datumPlaneId(tlfs.front()->getId());
    
    sShape->uniqueTypeMatch(trss);
    sShape->outerWireMatch(trss);
    if (!fromDatum)
      sShape->outerWireMatch(*tlss);
    sShape->derivedMatch();
    sShape->dumpNils(getTypeString()); //only if there are shapes with nil ids.
    sShape->dumpDuplicates(getTypeString());
    sShape->ensureNoNils();
    sShape->ensureNoDuplicates();
    
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

void Trim::datumPlaneId(const uuid &bId)
{
  assert(sShape->hasEvolveRecordIn(bId));
  uuid fid = sShape->evolve(bId).front();
  assert(sShape->hasEvolveRecordIn(fid));
  uuid wid = sShape->evolve(fid).front();
  
  //find nil face
  TopoDS_Shape f;
  for (const auto &s : sShape->getAllNilShapes())
  {
    if (s.ShapeType() == TopAbs_FACE)
    {
      sShape->updateId(s, fid);
      const auto &ws = BRepTools::OuterWire(TopoDS::Face(s));
      sShape->updateId(ws, wid);
      break;
    }
  }
}

void Trim::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::FeatureTrim so
  (
    Base::serialOut(),
    reversed->serialOut(),
    reversedLabel->serialOut(),
    iMapper->serialOut(),
    gu::idToString(dpFaceId),
    gu::idToString(dpWireId)
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::trim(stream, so, infoMap);
}

void Trim::serialRead(const prj::srl::FeatureTrim &si)
{
  Base::serialIn(si.featureBase());
  reversed->serialIn(si.reversed());
  reversedLabel->serialIn(si.reversedLabel());
  iMapper->serialIn(si.intersectionMapper());
  dpFaceId = gu::stringToId(si.dpFaceId());
  dpWireId = gu::stringToId(si.dpWireId());
}
