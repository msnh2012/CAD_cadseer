/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2021 Thomas S. Anderson blobfish.at.gmx.com
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

#include <osg/Switch>

#include <gp_Ax3.hxx>
#include <TopoDS.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepTools_Quilt.hxx>

#include "globalutilities.h"
#include "annex/annseershape.h"
#include "library/lbrcsysdragger.h"
#include "library/lbrlineardimension.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "tools/occtools.h"
#include "tools/featuretools.h"
#include "tools/idtools.h"
#include "tools/tlsshapeid.h"
#include "feature/ftrshapecheck.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrprimitive.h"
#include "project/serial/generated/prjsrlprsmprism.h"
#include "feature/ftrprism.h"

using namespace ftr::Prism;
using boost::uuids::uuid;

QIcon Feature::icon = QIcon(":/resources/images/constructionPrism.svg");

struct Feature::Stow
{
  Feature &feature;
  Primitive primitive;
  
  prm::Parameter radiusType{QObject::tr("Radius Type"), 0, Tags::RadiusType};
  prm::Parameter sides{QObject::tr("Sides"), 3, Tags::Sides};
  prm::Parameter pyramid{QObject::tr("Pyramid"), false, Tags::Pyramid};
  
  osg::ref_ptr<lbr::PLabel> radiusTypeLabel{new lbr::PLabel(&radiusType)};
  osg::ref_ptr<lbr::PLabel> sidesLabel{new lbr::PLabel(&sides)};
  osg::ref_ptr<lbr::PLabel> pyramidLabel{new lbr::PLabel(&pyramid)};
  
  uuid apexId{gu::createRandomId()};
  std::vector<uuid> bottomVertexIds;
  std::vector<uuid> topVertexIds;
  std::vector<uuid> bottomEdgeIds;
  std::vector<uuid> topEdgeIds;
  std::vector<uuid> verticalEdgeIds;
  std::vector<uuid> wireIds;
  std::vector<uuid> faceIds;
  uuid bottomWireId{gu::createRandomId()};
  uuid topWireId{gu::createRandomId()};
  uuid bottomFaceId{gu::createRandomId()};
  uuid topFaceId{gu::createRandomId()};
  uuid shellId{gu::createRandomId()};
  uuid solidId{gu::createRandomId()};
  
  Stow() = delete;
  Stow(Feature &fIn)
  : feature(fIn)
  , primitive(Primitive::Input{fIn, fIn.parameters, fIn.annexes})
  {
    //TODO add to preferences
    primitive.addRadius(8);
    primitive.addHeight(20);
    
    QStringList radiusStrings = //keep in sync with enum in header.
    {
      QObject::tr("Circumscribed")
      , QObject::tr("Inscribed")
    };
    radiusType.setEnumeration(radiusStrings);
    radiusType.connectValue(std::bind(&Feature::setModelDirty, &feature));
    radiusTypeLabel->refresh();
    feature.parameters.push_back(&radiusType);
    
    prm::Constraint sc;
    prm::Boundary lower(3.0, prm::Boundary::End::Closed);
    prm::Boundary upper(std::numeric_limits<double>::max(), prm::Boundary::End::Closed);
    prm::Interval interval(lower, upper);
    sc.intervals.push_back(interval);
    sides.setConstraint(sc);
    sides.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&sides);
    
    pyramid.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&pyramid);
    
    primitive.csysDragger.dragger->linkToMatrix(radiusTypeLabel.get());
    feature.overlaySwitch->addChild(radiusTypeLabel.get());
    
    primitive.csysDragger.dragger->linkToMatrix(sidesLabel.get());
    feature.overlaySwitch->addChild(sidesLabel.get());
    
    primitive.csysDragger.dragger->linkToMatrix(pyramidLabel.get());
    feature.overlaySwitch->addChild(pyramidLabel.get());
    
    primitive.radiusIP->setMatrixDims(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(0.0, 1.0, 0.0)));
    primitive.radiusIP->setMatrixDragger(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(-1.0, 0.0, 0.0)));
    primitive.radiusIP->setDimsFlipped(true);
    primitive.radiusIP->setRotationAxis(osg::Vec3d(0.0, 0.0, 1.0), osg::Vec3d(-1.0, 0.0, 0.0));

    primitive.heightIP->setMatrixDims(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(1.0, 0.0, 0.0)));
    primitive.heightIP->setRotationAxis(osg::Vec3d(0.0, 0.0, 1.0), osg::Vec3d(0.0, -1.0, 0.0));
  }
  
  void ensureIdArrays()
  {
    int count = sides.getInt();
    int currentSize = static_cast<int>(bottomVertexIds.size());
    for (int diff = count - currentSize; diff > 0 ; --diff)
    {
      primitive.sShape.insertEvolve(gu::createNilId(), bottomVertexIds.emplace_back(gu::createRandomId()));
      primitive.sShape.insertEvolve(gu::createNilId(), topVertexIds.emplace_back(gu::createRandomId()));
      primitive.sShape.insertEvolve(gu::createNilId(), bottomEdgeIds.emplace_back(gu::createRandomId()));
      primitive.sShape.insertEvolve(gu::createNilId(), topEdgeIds.emplace_back(gu::createRandomId()));
      primitive.sShape.insertEvolve(gu::createNilId(), verticalEdgeIds.emplace_back(gu::createRandomId()));
      primitive.sShape.insertEvolve(gu::createNilId(), wireIds.emplace_back(gu::createRandomId()));
      primitive.sShape.insertEvolve(gu::createNilId(), faceIds.emplace_back(gu::createRandomId()));
    }
    
    primitive.sShape.insertEvolve(gu::createNilId(), apexId);
    primitive.sShape.insertEvolve(gu::createNilId(), bottomWireId);
    primitive.sShape.insertEvolve(gu::createNilId(), topWireId);
    primitive.sShape.insertEvolve(gu::createNilId(), bottomFaceId);
    primitive.sShape.insertEvolve(gu::createNilId(), topFaceId);
    primitive.sShape.insertEvolve(gu::createNilId(), shellId);
    primitive.sShape.insertEvolve(gu::createNilId(), solidId);
  }
  
/* Notes:
  * we build all topology regardless of the pyramid value.
  *   If we are building a pyramid:
  *     we use apex instead of the top vertices for vertical edges.
  *     ignore top face in quilting.
  * we build everything at absolute ignoring the csys parameter.
  *   set location after solid is build
  * Id mapping is so messy again!
  * We have to build in place. When applying transform to shape
  * after construction, child features loose placement.
  */
  void build()
  {
    ensureIdArrays();
    tls::ShapeIdContainer idHelper;
    
    bool isPyramid = pyramid.getBool();
    double lRadius = primitive.radius->getDouble();
    double lHeight = primitive.height->getDouble();
    double lSides = sides.getInt();
    double lRotation = 2.0 * osg::PI / lSides;
    int lRadiusType = radiusType.getInt();
    osg::Matrixd mtx = primitive.csys.getMatrix();
    
    osg::Vec3d point(lRadius, 0.0, 0.0);
    if (lRadiusType == 1) //inscribed
    {
      double ha = lRotation / 2.0;
      double mag = lRadius / std::cos(ha);
      point.normalize();
      point *= mag;
    }
    
    occt::VertexVector bottomVertices;
    occt::VertexVector topVertices;
    occt::EdgeVector bottomEdges;
    occt::EdgeVector topEdges;
    occt::EdgeVector verticalEdges;
    occt::WireVector wires; //so we can id them.
    occt::FaceVector faces; //so we can id them.
    osg::Vec3d topProjection(0.0, 0.0, lHeight);
    TopoDS_Vertex apex = BRepBuilderAPI_MakeVertex(gp_Pnt(gu::toOcc(topProjection).XYZ()));
    idHelper.insert(apexId, apex);
    
    auto addBottomVertex = [&](const osg::Vec3d &pIn)
    {
      gp_Pnt occPoint = gp_Pnt(gu::toOcc(pIn * mtx).XYZ());
      bottomVertices.push_back(BRepBuilderAPI_MakeVertex(occPoint));
      idHelper.insert(bottomVertexIds.at(bottomVertices.size() - 1), bottomVertices.back());
    };
    auto addTopVertex = [&](const osg::Vec3d &pIn)
    {
      gp_Pnt occPoint = gp_Pnt(gu::toOcc((pIn + topProjection) * mtx).XYZ());
      topVertices.push_back(BRepBuilderAPI_MakeVertex(occPoint));
      idHelper.insert(topVertexIds.at(topVertices.size() - 1), topVertices.back());
    };
    auto buildEdge = [&](const TopoDS_Vertex &first, const TopoDS_Vertex &last) -> TopoDS_Edge
    {
      BRepBuilderAPI_MakeEdge em(first, last);
      if (!em.IsDone())
        throw std::runtime_error("Failed making edge");
      return em.Edge();
    };
    auto buildWire = [](const occt::EdgeVector &evIn) -> TopoDS_Wire
    {
      BRepBuilderAPI_MakeWire wm;
      for (const auto &e : evIn)
      {
        wm.Add(e);
        if (!wm.IsDone())
          throw std::runtime_error("Failed added edge to face wire");
      }
      return wm.Wire();
    };
    auto buildFace = [](const TopoDS_Wire &wIn) -> TopoDS_Face
    {
      BRepBuilderAPI_MakeFace fm(wIn, true);
      if (!fm.IsDone())
        throw std::runtime_error("Failed making face from wire");
      return fm.Face();
    };
    auto buildBottomFace = [&](const occt::EdgeVector &evIn) -> TopoDS_Face
    {
      auto wire = buildWire(evIn);
      idHelper.insert(bottomWireId, wire);
      auto face = buildFace(wire);
      idHelper.insert(bottomFaceId, face);
      return face;
    };
    auto buildTopFace = [&](const occt::EdgeVector &evIn) -> TopoDS_Face
    {
      auto wire = buildWire(evIn);
      idHelper.insert(topWireId, wire);
      auto face = buildFace(wire);
      idHelper.insert(topFaceId, face);
      return face;
    };
    auto buildLateralFace = [&](const occt::EdgeVector &evIn) -> TopoDS_Face
    {
      auto wire = buildWire(evIn);
      wires.emplace_back(wire);
      idHelper.insert(wireIds.at(wires.size() - 1), wires.back());
      auto face = buildFace(wire);
      faces.emplace_back(face);
      idHelper.insert(faceIds.at(faces.size() - 1), faces.back());
      return face;
    };
    
    addBottomVertex(point);
    addTopVertex(point); //add top does projection
    if (isPyramid)
      verticalEdges.push_back(buildEdge(bottomVertices.back(), apex));
    else
      verticalEdges.push_back(buildEdge(bottomVertices.back(), topVertices.back()));
    idHelper.insert(verticalEdgeIds.at(verticalEdges.size() - 1), verticalEdges.back());
    for (int i = 0; i < lSides - 1; ++i)
    {
      point = point * osg::Matrixd::rotate(lRotation, osg::Vec3d(0.0, 0.0, 1.0));
      addBottomVertex(point);
      addTopVertex(point);
      bottomEdges.push_back(buildEdge(*(bottomVertices.end() - 2), bottomVertices.back()));
      idHelper.insert(bottomEdgeIds.at(bottomEdges.size() - 1), bottomEdges.back());
      topEdges.push_back(buildEdge(*(topVertices.end() - 2), topVertices.back()));
      idHelper.insert(topEdgeIds.at(topEdges.size() - 1), topEdges.back());
      if (isPyramid)
        verticalEdges.push_back(buildEdge(bottomVertices.back(), apex));
      else
        verticalEdges.push_back(buildEdge(bottomVertices.back(), topVertices.back()));
      idHelper.insert(verticalEdgeIds.at(verticalEdges.size() - 1), verticalEdges.back());
    }
    bottomEdges.push_back(buildEdge(bottomVertices.back(), bottomVertices.front()));
    idHelper.insert(bottomEdgeIds.at(bottomEdges.size() - 1), bottomEdges.back());
    topEdges.push_back(buildEdge(topVertices.back(), topVertices.front()));
    idHelper.insert(topEdgeIds.at(topEdges.size() - 1), topEdges.back());
    
    //construct faces.
    BRepTools_Quilt quilter;
    quilter.Add(buildBottomFace(bottomEdges).Oriented(TopAbs_REVERSED));
    if (!isPyramid)
      quilter.Add(buildTopFace(topEdges));
    
    //cheat vertical edges. just duplicate first edge to end of vector.
    verticalEdges.push_back(verticalEdges.front());
    //building lateral faces.
    for (int index = 0; index < lSides; ++index)
    {
      occt::EdgeVector ces; //current edges
      ces.push_back(TopoDS::Edge(verticalEdges.at(index).Oriented(TopAbs_REVERSED)));
      ces.push_back(bottomEdges.at(index));
      ces.push_back(verticalEdges.at(index + 1));
      if (!isPyramid)
        ces.push_back(TopoDS::Edge(topEdges.at(index).Oriented(TopAbs_REVERSED)));
      quilter.Add(buildLateralFace(ces));
    }
    
    //build the shell
    TopoDS_Shape shellShape = quilter.Shells();
    if (shellShape.IsNull())
      throw std::runtime_error("Empty shell shape from quilt");
    occt::ShellVector shells = occt::ShapeVectorCast(TopoDS::Compound(quilter.Shells()));
    if (shells.empty())
      throw std::runtime_error("No shells from quilt");
    idHelper.insert(shellId, shells.front());
    
    //build the solid
    BRepBuilderAPI_MakeSolid solidMaker(shells.front());
    if (!solidMaker.IsDone())
      throw std::runtime_error("Couldn't make solid from shell");
    idHelper.insert(solidId, solidMaker.Solid());
    
    auto compound = occt::compoundWrap(solidMaker.Solid());
    idHelper.insert(feature.getId(), compound);
    occt::ShapeVector all = occt::mapShapes(compound);
    
    ShapeCheck check(compound);
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    primitive.sShape.setOCCTShape(compound, feature.getId());
    for (const auto &s : all)
    {
      if (!idHelper.has(s))
      {
        std::cout << "no modified shape from transform" << std::endl;
        continue;
      }
      const auto &sId = idHelper.find(s);
      if (!primitive.sShape.hasShape(s))
      {
        std::cout << "modified shape is not in seer shape." << std::endl;
        continue;
      }
      primitive.sShape.updateId(s, sId);
    }
//     primitive.sShape.dumpShapeIdContainer(std::cout);
//     primitive.sShape.dumpEvolveContainer(std::cout);
    primitive.sShape.ensureNoNils();
    primitive.sShape.ensureNoDuplicates();
  }
  
  void updateIPs()
  {
    primitive.IPsToCsys();
    
    static const auto rot = osg::Matrixd::rotate(osg::Quat(osg::PI_2, osg::Vec3d(-1.0, 0.0, 0.0)));
    auto trans = osg::Matrixd::translate(0.0, 0.0, primitive.height->getDouble() / 2.0);
    primitive.radiusIP->setMatrixDragger(rot * trans);
    primitive.radiusIP->mainDim->setSqueeze(primitive.height->getDouble() / 2.0);
    primitive.radiusIP->mainDim->setExtensionOffset(primitive.height->getDouble() / 2.0);
    
    primitive.heightIP->mainDim->setSqueeze(primitive.radius->getDouble());
    primitive.heightIP->mainDim->setExtensionOffset(primitive.radius->getDouble());
    
    osg::Vec3d radiusTypeOffset(0.0, primitive.radius->getDouble(), 0.0);
    radiusTypeOffset = radiusTypeOffset * primitive.csys.getMatrix();
    radiusTypeLabel->setMatrix(osg::Matrixd::translate(radiusTypeOffset));
    
    osg::Vec3d sidesOffset(0.0, -primitive.radius->getDouble(), 0.0);
    sidesOffset = sidesOffset * primitive.csys.getMatrix();
    sidesLabel->setMatrix(osg::Matrixd::translate(sidesOffset));
    
    osg::Vec3d pyramidOffset(0.0, 0.0, primitive.height->getDouble());
    pyramidOffset = pyramidOffset * primitive.csys.getMatrix();
    pyramidLabel->setMatrix(osg::Matrixd::translate(pyramidOffset));
  }
};

Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))

{
  name = QObject::tr("Prism");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
}

Feature::~Feature() = default;

void Feature::updateModel(const UpdatePayload &pIn)
{
  setFailure();
  lastUpdateLog.clear();
  stow->primitive.sShape.reset();
  try
  {
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    //nothing for constant.
    if (static_cast<Primitive::CSysType>(stow->primitive.csysType.getInt()) == Primitive::Linked)
    {
      const auto &picks = stow->primitive.csysLinked.getPicks();
      if (picks.empty())
        throw std::runtime_error("No picks for csys link");
      tls::Resolver resolver(pIn);
      if (!resolver.resolve(picks.front()))
        throw std::runtime_error("Couldn't resolve csys link pick");
      auto csysPrms = resolver.getFeature()->getParameters(prm::Tags::CSys);
      if (csysPrms.empty())
        throw std::runtime_error("csys link feature has no csys parameter");
      prm::ObserverBlocker(stow->primitive.csysObserver);
      stow->primitive.csys.setValue(csysPrms.front()->getMatrix());
      stow->primitive.csysDragger.draggerUpdate();
    }
    stow->build();
    mainTransform->setMatrix(osg::Matrixd::identity());
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
  stow->updateIPs();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Feature::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::prsm::Prism so
  (
    Base::serialOut()
    , stow->radiusType.serialOut()
    , stow->sides.serialOut()
    , stow->pyramid.serialOut()
    , stow->primitive.radius->serialOut()
    , stow->primitive.height->serialOut()
    , stow->primitive.csysType.serialOut()
    , stow->primitive.csys.serialOut()
    , stow->primitive.csysLinked.serialOut()
    , stow->primitive.csysDragger.serialOut()
    , stow->primitive.sShape.serialOut()
    , stow->primitive.radiusIP->serialOut()
    , stow->primitive.heightIP->serialOut()
    , stow->radiusTypeLabel->serialOut()
    , stow->sidesLabel->serialOut()
    , stow->pyramidLabel->serialOut()
    , gu::idToString(stow->apexId)
    , gu::idToString(stow->bottomWireId)
    , gu::idToString(stow->topWireId)
    , gu::idToString(stow->bottomFaceId)
    , gu::idToString(stow->topFaceId)
    , gu::idToString(stow->shellId)
    , gu::idToString(stow->solidId)
  );
  
  auto idsOut = [](const std::vector<uuid> &idsIn) -> prj::srl::prsm::Prism::BottomVertexIdsSequence
  {
    prj::srl::prsm::Prism::BottomVertexIdsSequence out;
    for (const auto &id : idsIn)
      out.push_back(gu::idToString(id));
    return out;
  };
  so.bottomVertexIds() = idsOut(stow->bottomVertexIds);
  so.topVertexIds() = idsOut(stow->topVertexIds);
  so.bottomEdgeIds() = idsOut(stow->bottomEdgeIds);
  so.topEdgeIds() = idsOut(stow->topEdgeIds);
  so.verticalEdgeIds() = idsOut(stow->verticalEdgeIds);
  so.wireIds() = idsOut(stow->wireIds);
  so.faceIds() = idsOut(stow->faceIds);
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::prsm::prism(stream, so, infoMap);
}

void Feature::serialRead(const prj::srl::prsm::Prism &so)
{
  Base::serialIn(so.base());
  stow->radiusType.serialIn(so.radiusType());
  stow->sides.serialIn(so.sides());
  stow->pyramid.serialIn(so.pyramid());
  stow->primitive.radius->serialIn(so.radius());
  stow->primitive.height->serialIn(so.height());
  stow->primitive.csysType.serialIn(so.csysType());
  stow->primitive.csys.serialIn(so.csys());
  stow->primitive.csysLinked.serialIn(so.csysLinked());
  stow->primitive.csysDragger.serialIn(so.csysDragger());
  stow->primitive.sShape.serialIn(so.seerShape());
  stow->primitive.radiusIP->serialIn(so.radiusIP());
  stow->primitive.heightIP->serialIn(so.heightIP());
  stow->radiusTypeLabel->serialIn(so.radiusTypeLabel());
  stow->sidesLabel->serialIn(so.sidesLabel());
  stow->pyramidLabel->serialIn(so.pyramidLabel());
  stow->apexId = gu::stringToId(so.apexId());
  stow->bottomWireId = gu::stringToId(so.bottomWireId());
  stow->topWireId = gu::stringToId(so.topWireId());
  stow->bottomFaceId = gu::stringToId(so.bottomFaceId());
  stow->topFaceId = gu::stringToId(so.topFaceId());
  stow->shellId = gu::stringToId(so.shellId());
  stow->solidId = gu::stringToId(so.solidId());
  
  auto idsIn = [](const prj::srl::prsm::Prism::BottomVertexIdsSequence &ids) -> std::vector<uuid>
  {
    std::vector<uuid> out;
    for (const auto &id : ids)
      out.push_back(gu::stringToId(id));
    return out;
  };
  stow->bottomVertexIds = idsIn(so.bottomVertexIds());
  stow->topVertexIds = idsIn(so.topVertexIds());
  stow->bottomEdgeIds = idsIn(so.bottomEdgeIds());
  stow->topEdgeIds = idsIn(so.topEdgeIds());
  stow->verticalEdgeIds = idsIn(so.verticalEdgeIds());
  stow->wireIds = idsIn(so.wireIds());
  stow->faceIds = idsIn(so.faceIds());
}
