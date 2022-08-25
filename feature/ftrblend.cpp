/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2015  Thomas S. Anderson blobfish.at.gmx.com
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

#include <assert.h>
#include <boost/optional/optional.hpp>

#include <TopoDS.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <ChFi3d_FilBuilder.hxx>
#include <TopOpeBRepBuild_HBuilder.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <BOPTools_AlgoTools.hxx>
#include <BRepTools.hxx>
#include <TColgp_Array1OfPnt2d.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRep_Tool.hxx>
#include <Geom_Curve.hxx>
#include <Law_Function.hxx>
#include <ChFiDS_FilSpine.hxx>

#include <osg/Switch>
#include "feature/ftrinputtype.h"

#include "globalutilities.h"
#include "tools/idtools.h"
#include "tools/occtools.h"
#include "tools/featuretools.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "project/serial/generated/prjsrlblnsblend.h"
#include "parameter/prmparameter.h"
#include "feature/ftrshapecheck.h"
#include "annex/annseershape.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "feature/ftrupdatepayload.h"
#include "law/lwfvessel.h"
#include "feature/ftrlawspine.h"
#include "feature/ftrblend.h"

using boost::uuids::uuid;

using namespace ftr::Blend;
QIcon Feature::icon = QIcon(":/resources/images/constructionBlend.svg");

Constant::Constant()
: contourPicks(QObject::tr("Contour Picks"), ftr::Picks(), PrmTags::contourPicks)
, radius(prm::Names::Radius, prf::manager().rootPtr->features().blend().get().radius(), prm::Tags::Radius)
, radiusLabel(new lbr::PLabel(&radius))
{
  radius.setConstraint(prm::Constraint::buildZeroPositive());
}

Constant::Constant(const prj::srl::blns::Constant &cIn)
: Constant()
{
  contourPicks.serialIn(cIn.contourPicks());
  radius.serialIn(cIn.radius());
  radiusLabel->serialIn(cIn.radiusLabel());
}

prm::Parameters Constant::getParameters()
{
  return {&contourPicks, &radius};
}

prj::srl::blns::Constant Constant::serialOut() const
{
  return prj::srl::blns::Constant
  {
    contourPicks.serialOut()
    , radius.serialOut()
    , radiusLabel->serialOut()
  };
}

Entry::Entry()
: entryPick(QObject::tr("Entry Pick"), ftr::Picks(), PrmTags::entryPick)
, radius(prm::Names::Radius, prf::manager().rootPtr->features().blend().get().radius(), prm::Tags::Radius)
, position(prm::Names::Position, 0.0, prm::Tags::Position)
, radiusLabel(new lbr::PLabel(&radius))
, positionLabel(new lbr::PLabel(&position))
{
  radius.setConstraint(prm::Constraint::buildZeroPositive());
  position.setConstraint(prm::Constraint::buildUnit());
  
  entryPick.connectValue(std::bind(&Entry::prmActiveSync, this));
}

Entry::Entry(const prj::srl::blns::Entry &eIn)
: Entry()
{
  entryPick.serialIn(eIn.entryPick());
  radius.serialIn(eIn.radius());
  position.serialIn(eIn.position());
  radiusLabel->serialIn(eIn.radiusLabel());
  positionLabel->serialIn(eIn.positionLabel());
}

prm::Parameters Entry::getParameters()
{
  return {&entryPick, &radius, &position};
}

void Entry::prmActiveSync()
{
  const auto &ps = entryPick.getPicks();
  if (ps.empty())
  {
    radius.setActive(false);
    position.setActive(false);
    return;
  }
  
  radius.setActive(true);
  if (ps.front().selectionType == slc::Type::NearestPoint)
    position.setActive(true);
  else
    position.setActive(false);
}

prj::srl::blns::Entry Entry::serialOut() const
{
  return prj::srl::blns::Entry
  (
    entryPick.serialOut()
    , radius.serialOut()
    , position.serialOut()
    , radiusLabel->serialOut()
    , positionLabel->serialOut()
  );
}

Variable::Variable()
: contourPicks(QObject::tr("Contour Picks"), ftr::Picks(), PrmTags::contourPicks)
{}

prj::srl::blns::Variable Variable::serialOut() const
{
  prj::srl::blns::Variable out(contourPicks.serialOut());
  for (const auto &e : entries)
    out.entries().push_back(e.serialOut());
  return out;
}

void Variable::serialIn(const prj::srl::blns::Variable &vIn)
{
  contourPicks.serialIn(vIn.contourPicks());
  for (const auto &s : vIn.entries())
    entries.emplace_back(s);
}

struct Feature::Stow
{
  Feature &feature;
  
  prm::Parameter filletShape{QObject::tr("Fillet Shape"), 0, PrmTags::filletShape};
  prm::Parameter blendType{QObject::tr("Blend Type"), 0, PrmTags::blendType};
  prm::Parameter lawSpinePick{QObject::tr("Law Spine"), ftr::Picks(), PrmTags::lawSpinePick};
  prm::Parameter lawEdgePick{QObject::tr("Law Edge"), ftr::Picks(), PrmTags::lawEdgePick};
  
  ann::SeerShape sShape;
  
  osg::ref_ptr<lbr::PLabel> filletShapeLabel{new lbr::PLabel(&filletShape)};
  osg::ref_ptr<lbr::PLabel> blendTypeLabel{new lbr::PLabel(&blendType)};
  
  Constants constants;
  Variable variable;
  /*! used to map the edges/vertices that are blended away to the face generated.
   * used to map new generated face to outer wire.
   */
  std::map<uuid, uuid> shapeMap;
  
  Stow() = delete;
  Stow(Feature &fIn)
  : feature(fIn)
  {
    QStringList shapeStrings =
    {
      QObject::tr("Rational")
      , QObject::tr("QuasiAngular")
      , QObject::tr("Polynomial")
    };
    filletShape.setEnumeration(shapeStrings);
    filletShape.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&filletShape);
    filletShapeLabel->refresh();
    feature.overlaySwitch->addChild(filletShapeLabel);
    
    QStringList typeStrings =
    {
      QObject::tr("Constant")
      , QObject::tr("Variable")
      , QObject::tr("Law Spine")
    };
    blendType.setEnumeration(typeStrings);
    blendType.connectValue(std::bind(&Feature::setModelDirty, &feature));
    blendType.connectValue(std::bind(&Stow::prmActiveSync, this));
    feature.parameters.push_back(&blendType);
    blendTypeLabel->refresh();
    feature.overlaySwitch->addChild(blendTypeLabel);
    
    lawSpinePick.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&lawSpinePick);
    lawEdgePick.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&lawEdgePick);
    
    feature.annexes.insert(std::make_pair(ann::Type::SeerShape, &sShape));
    
    variable.contourPicks.connectValue(std::bind(&Feature::setModelDirty, &feature));
    prmActiveSync();
  }
  
  void clearBlends()
  {
    for (auto &constant : constants)
      detachConstant(constant);
    constants.clear();
    
    detachVariable();
    variable.contourPicks.setValue(ftr::Picks());
    variable.entries.clear();
  }
  
  void attachConstant(Constant &cIn)
  {
    cIn.contourPicks.connectValue(std::bind(&Feature::setModelDirty, &feature));
    cIn.radius.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.overlaySwitch->addChild(cIn.radiusLabel);
  }
  
  void detachConstant(Constant &cIn)
  {
    feature.overlaySwitch->removeChild(cIn.radiusLabel);
  }
  
  void attachVariable()
  {
    for  (auto &e : variable.entries)
      attachEntry(e);
  }
  
  void detachVariable()
  {
    for  (auto &e : variable.entries)
      detachEntry(e);
  }
  
  void attachEntry(Entry &eIn)
  {
    eIn.entryPick.connectValue(std::bind(&Feature::setModelDirty, &feature));
    eIn.radius.connectValue(std::bind(&Feature::setModelDirty, &feature));
    eIn.position.connectValue(std::bind(&Feature::setModelDirty, &feature));
    
    feature.overlaySwitch->addChild(eIn.radiusLabel);
    feature.overlaySwitch->addChild(eIn.positionLabel);
  }
  
  void detachEntry(Entry &eIn)
  {
    feature.overlaySwitch->removeChild(eIn.radiusLabel);
    feature.overlaySwitch->removeChild(eIn.positionLabel);
  }
  
  void prmActiveSync()
  {
    clearBlends();
    
    //we never enable lawEdgePick. derived from lawSpinePick in command.
    lawEdgePick.setActive(false);
    if (blendType.getInt() == 2)
      lawSpinePick.setActive(true);
    else
      lawSpinePick.setActive(false);
  }
  
  /* matching by interogating the ChFi3d_FilBuilder objects generated and modified methods.
   * generated only outputs the fillet faces from the edge. Nothing else.
   * tried iterating over MakeFillet contours and edges, but in certain situations
   * blends are made from vertices. blending 3 edges of a box corner is such a situation.
   * so we will iterate over all the shapes of the targe object.
   */
  void match(ChFi3d_FilBuilder &bMaker, const ann::SeerShape &targetShapeIn)
  {
    auto modified = [&](const TopoDS_Shape &sourceShape) -> occt::ShapeVector
    {
      occt::ShapeVector results;
      
      if (bMaker.Builder()->IsSplit(sourceShape, TopAbs_OUT))
      {
        occt::ShapeVector temp = occt::ShapeVectorCast(bMaker.Builder()->Splits(sourceShape, TopAbs_OUT));
        results.insert(results.end(), temp.begin(), temp.end());
      }
      if (bMaker.Builder()->IsSplit(sourceShape, TopAbs_IN))
      {
        occt::ShapeVector temp = occt::ShapeVectorCast(bMaker.Builder()->Splits(sourceShape, TopAbs_IN));
        results.insert(results.end(), temp.begin(), temp.end());
      }
      if (bMaker.Builder()->IsSplit(sourceShape, TopAbs_ON))
      {
        occt::ShapeVector temp = occt::ShapeVectorCast(bMaker.Builder()->Splits(sourceShape, TopAbs_ON));
        results.insert(results.end(), temp.begin(), temp.end());
      }
      
      return results;
    };
    
    for (const auto &cId : targetShapeIn.getAllShapeIds())
    {
      const TopoDS_Shape &currentShape = targetShapeIn.findShape(cId);
      assert(!currentShape.IsNull());
      const TopTools_ListOfShape &generated = bMaker.Generated(currentShape);
      if (!generated.IsEmpty())
      {
        //duplicated with chamfer.
        if(generated.Extent() != 1) //see ensure noFaceNils
        {
          std::ostringstream s; s << "WARNING: more than one generated shape" << std::endl;
          feature.lastUpdateLog += s.str();
        }
        const TopoDS_Shape &blendFace = generated.First();
        assert(!blendFace.IsNull());
        assert(blendFace.ShapeType() == TopAbs_FACE);
        
        std::map<uuid, uuid>::iterator mapItFace;
        bool dummy;
        std::tie(mapItFace, dummy) = shapeMap.insert(std::make_pair(cId, gu::createRandomId()));
        sShape.updateId(blendFace, mapItFace->second);
        if (dummy) //insertion took place.
          sShape.insertEvolve(gu::createNilId(), mapItFace->second);
        
        //now look for outerwire for newly generated face.
        //we use the generated face id to map to outer wire.
        std::map<uuid, uuid>::iterator mapItWire;
        std::tie(mapItWire, dummy) = shapeMap.insert(std::make_pair(mapItFace->second, gu::createRandomId()));
        //now get the wire and update the result to id.
        const TopoDS_Shape &blendFaceWire = BRepTools::OuterWire(TopoDS::Face(blendFace));
        sShape.updateId(blendFaceWire, mapItWire->second);
        if (dummy) //insertion took place.
          sShape.insertEvolve(gu::createNilId(), mapItWire->second);
      }
      occt::ShapeVector mods = modified(currentShape);
      if (!mods.empty())
      {
        if (mods.size() > 1)
        {
          std::ostringstream s; s << "WARNING: more than one modified shape" << std::endl;
          feature.lastUpdateLog += s.str();
        }
        
        for (const auto &s : mods)
        {
          //this is basically the same as SeerShape match.
          if(!sShape.hasShape(s))
            continue;
          
          uuid freshId = gu::createNilId();
          if (sShape.hasEvolveRecordIn(cId))
            freshId = sShape.evolve(cId).front(); //multiple returns?
            else
            {
              freshId = gu::createRandomId();
              sShape.insertEvolve(cId, freshId);
            }
            sShape.updateId(s, freshId);
        }
      }
    }
  }
  
  //only partial update from switch to ChFi3d_FilBuilder
  void dumpInfo(ChFi3d_FilBuilder &blendMakerIn, const ann::SeerShape &targetFeatureIn)
  {
    std::cout << std::endl << std::endl <<
    "shape out type is: " << occt::getShapeTypeString(blendMakerIn.Shape()) << std::endl <<
    "fillet dump:" << std::endl;
    
    auto shapes = targetFeatureIn.getAllShapes();
    for (const auto &currentShape : shapes)
    {
      std::cout << "ShapeType is: " << std::setw(10) << occt::getShapeTypeString(currentShape)
      << "      Generated Count is: " << blendMakerIn.Generated(currentShape).Extent()
      //       << "      Modified Count is: " << blendMakerIn.Modified(currentShape).Extent()
      //       << "      is deleted: " << ((blendMakerIn.IsDeleted(currentShape)) ? "true" : "false")
      << std::endl; 
      
      if (blendMakerIn.Generated(currentShape).Extent() > 0)
        std::cout << "   generated type is: " << 
        occt::getShapeTypeString(blendMakerIn.Generated(currentShape).First()) << std::endl;
    }
  }
  
  void ensureNoFaceNils()
  {
    /* ok this is a hack. Because of an OCCT bug, generated is missing
     * an edge to 'blended face' mapping and on another edge, it has 2 faces
     * generated from a single edge. Without the face having an id, derived
     * shapes (edges and verts) can't be assigned an id. This causes a lot
     * of id failures. So for now just assign an id and check for bug
     * in next release.
     */
    
    auto shapes = sShape.getAllShapes();
    for (const auto &shape : shapes)
    {
      if (shape.ShapeType() != TopAbs_FACE)
        continue;
      if (!sShape.findId(shape).is_nil())
        continue;
      
      sShape.updateId(shape, gu::createRandomId());
    }
  }
};

Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  name = QObject::tr("Blend");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
}

Feature::~Feature() = default;

Constant& Feature::addConstant()
{
  assert(stow->blendType.getInt() == 0); //constant
  stow->constants.emplace_back();
  stow->attachConstant(stow->constants.back());
  return stow->constants.back();
  // adding empty blend doesn't make feature dirty.
}

void Feature::removeConstant(int index)
{
  assert(stow->blendType.getInt() == 0); //constant
  assert(index >= 0 && index < static_cast<int>(stow->constants.size()));
  auto it = stow->constants.begin(); std::advance(it, index);
  stow->detachConstant(*it);
  stow->constants.erase(it);
  setModelDirty();
}

Constants& Feature::getConstants() const
{
  assert(stow->blendType.getInt() == 0); //constant
  return stow->constants;
}

Constant& Feature::getConstant(int index) const
{
  assert(stow->blendType.getInt() == 0); //constant
  auto it = stow->constants.begin(); std::advance(it, index);
  return *it;
}

void Feature::setVariablePicks(const Picks &pickIn)
{
  assert(stow->blendType.getInt() == 1); //variable
  stow->variable.contourPicks.setValue(pickIn);
  //picks parameter will set model dirty.
}

Entry& Feature::addVariableEntry()
{
  assert(stow->blendType.getInt() == 1); //variable
  auto &freshEntry = stow->variable.entries.emplace_back();
  stow->attachEntry(freshEntry);
  //adding empty entry shouldn't make model dirty.
  return freshEntry;
}

void Feature::removeVariableEntry(int index)
{
  assert(stow->blendType.getInt() == 1); //variable
  assert(index >= 0 && index < static_cast<int>(stow->variable.entries.size()));
  auto it = stow->variable.entries.begin(); std::advance(it, index);
  stow->detachEntry(*it);
  stow->variable.entries.erase(it);
  setModelDirty();
}

Variable& Feature::getVariable() const
{
  return stow->variable;
}

/* Typical occt bullshit: v7.1
 * all radius data, regardless of assignment method, ends up inside ChFiDS_FilSpine::parandrad.
 * 'BRepFilletAPI_MakeFillet::Add' are all thin wrappers around the
 *    'BRepFilletAPI_MakeFillet::SetRadius' functions.
 * The radius assignments are tied to the edge passed in not the entire spine.
 *    this is contrary to the documentation!
 * 'Add' and 'SetRadius' that invole 'Law_Function', clears out all radius data. misleading.
 * I think we don't have to worry about law function. Set the radius data
 *    and the appropriate law function will be inferred.... I hope.
 * https://www.opencascade.com/doc/occt-6.9.1/overview/html/occt_user_guides__modeling_algos.html#occt_modalg_6_1_1
 *    has an example of using parameter and radius to create an evolved blend. notice the
 *    parameters are related to the actual edge length. This is confusing as fuck, because if you
 *    look at the setRadius function for (radius, edge) and (radius, radius, edge) they set the parameter
 *    radius array with parameters in range [0,1]. Ok the conversion from length to unit parameter is
 *    is happening in BRepFilletAPI_MakeFillet::SetRadius. BRepFilletAPI_MakeFillet::Length returns the
 *    length of the whole spine not individual edges. So that isn't much help. I will have to get the length
 *    of the edge from somewhere else. Can do it, but a pain in the ass, seeing as I save the position on
 *    edge already in parameter [0,1]
 * Using parameter and radius set function:
 *    if the array has only 1 member it will be assigned to the beginning of related edge.
 *    if the array has only 2 members it will be assigned to the beginning and end of related edge.
 * So trying to assign a radius to 'nearest' point will have to reference an edge and that
 *    edges endpoints will have to have a radius value, thus 3 values. So if we have the edges endpoints radius
 *    value set to satisfy the above, what happens when we assign a value to a vertex that coincides
 *    with said endpoints?
 * I think for now, I will just restrict parameter points for variable radius to vertices. No 'nearest'.
 * Combining a variable and a constant into the same feature is triggering an occt exception. v7.1.
 * 
 * switched to ChFi3d_FilBuilder in occt v7.3. this allowed us to use the more sensible parameter
 * assignment for mid and nearest points.
 */

void Feature::updateModel(const UpdatePayload &payloadIn)
{
  setFailure();
  lastUpdateLog.clear();
  stow->sShape.reset();
  try
  {
    // on error, find first feature and seershape and assign this seershape.
    auto bail = [&](std::string_view message)
    {
      std::vector<const Base*> features = payloadIn.getFeatures(std::string());
      const Base *firstFeature = nullptr;
      for (const auto *f : features)
      {
        if (f->getType() == ftr::Type::LawSpine) continue; //we allow law spines that are different features
        firstFeature = f;
        break;
      }
      if (firstFeature && firstFeature->hasAnnex(ann::Type::SeerShape))
      {
        const ann::SeerShape &targetSeerShape = features.front()->getAnnex<ann::SeerShape>();
        if (!targetSeerShape.isNull())
        {
          stow->sShape.setOCCTShape(targetSeerShape.getRootOCCTShape(), getId());
          stow->sShape.shapeMatch(targetSeerShape);
          stow->sShape.uniqueTypeMatch(targetSeerShape);
          stow->sShape.outerWireMatch(targetSeerShape);
          stow->sShape.derivedMatch();
          stow->sShape.ensureNoNils();
          stow->sShape.ensureNoDuplicates();
          stow->sShape.ensureEvolve();
        }
      }
      throw std::runtime_error(message.data());
    };
    
    if (isSkipped())
    {
      setSuccess();
      bail("feature is skipped");
    }
    
    auto placeLabels = [&](const TopoDS_Shape &sIn)
    {
      occt::BoundingBox bb(sIn);
      auto points = bb.getCorners();
      auto corner0 = gu::toOsg(points.at(0));
      auto projection = gu::toOsg(points.at(6)) - corner0;
      stow->filletShapeLabel->setMatrix(osg::Matrixd::translate(corner0 + projection / 3.0));
      stow->blendTypeLabel->setMatrix(osg::Matrixd::translate(corner0 + projection * 2.0 / 3.0));
    };
    
    std::unique_ptr<ChFi3d_FilBuilder> bMaker;
    const ann::SeerShape *targetSeerShape = nullptr;
    std::optional<uuid> solidSheetId;
    auto goBlendMaker = [&](const ann::SeerShape *ssIn, const uuid &edgeIdIn)
    {
      assert(ssIn->hasId(edgeIdIn));
      auto parents = ssIn->useGetParentsOfType(edgeIdIn, TopAbs_SOLID);
      if (parents.empty()) parents = ssIn->useGetParentsOfType(edgeIdIn, TopAbs_SHELL);
      if (parents.empty()) bail("Can't find solid or shell parent from edge");
      if (!bMaker) //all valid or none valid
      {
        solidSheetId = parents.front();
        targetSeerShape = ssIn;
        const auto &occShape = targetSeerShape->getOCCTShape(*solidSheetId);
        placeLabels(occShape);
        bMaker = std::make_unique<ChFi3d_FilBuilder>(occShape);
      }
      else //make sure subsequent edges belong to same shape.
      {
        assert(targetSeerShape); assert(solidSheetId);
        if (targetSeerShape != ssIn) bail("Edges from different seer shapes");
        if (solidSheetId != parents.front()) bail("Edges from different occt shapes");
      }
    };
    
    tls::Resolver resolver(payloadIn);
    if (stow->blendType.getInt() == 0) //constants
    {
      for (const auto &constantBlend : stow->constants)
      {
        bool labelDone = false; //set label position to first pick.
        for (const auto &pick : constantBlend.contourPicks.getPicks())
        {
          if (!resolver.resolve(pick)) continue; //just skip? no fail?
          for (const auto &resultId : resolver.getResolvedIds())
          {
            assert(!resultId.is_nil());
            goBlendMaker(resolver.getSeerShape(), resultId);
            TopoDS_Shape tempShape = targetSeerShape->getOCCTShape(resultId);
            assert(!tempShape.IsNull());
            assert(tempShape.ShapeType() == TopAbs_EDGE);
            bMaker->Add(constantBlend.radius.getDouble(), TopoDS::Edge(tempShape));
            //update location of parameter label.
            if (!labelDone)
            {
              labelDone = true;
              constantBlend.radiusLabel->setMatrix(osg::Matrixd::translate(pick.getPoint(TopoDS::Edge(tempShape))));
            }
          }
        }
      }
    }
    else if (stow->blendType.getInt() == 1) //variable
    {
      TopTools_MapOfShape needed, have, intersection;
      for (const auto &pick : stow->variable.contourPicks.getPicks())
      {
        if (!resolver.resolve(pick)) continue;
        for (const auto &resultId : resolver.getResolvedIds())
        {
          assert(!resultId.is_nil());
          goBlendMaker(resolver.getSeerShape(), resultId);
          TopoDS_Shape tempShape = targetSeerShape->getOCCTShape(resultId);
          assert(!tempShape.IsNull());
          assert(tempShape.ShapeType() == TopAbs_EDGE); //TODO faces someday.
          bMaker->Add(TopoDS::Edge(tempShape));
          int index = bMaker->Contains(TopoDS::Edge(tempShape));
          if (index < 1) bail("invalid contour index for added edge");
          needed.Add(bMaker->FirstVertex(index));
          needed.Add(bMaker->LastVertex(index));
        }
      }
      for (auto &e : stow->variable.entries)
      {
        bool labelDone = false;
        const auto &ePicks = e.entryPick.getPicks();
        if (ePicks.empty() || !resolver.resolve(ePicks.front())) continue;
        for (const auto &resultId : resolver.getResolvedIds())
        {
          assert(!resultId.is_nil());
          const TopoDS_Shape &blendShape = targetSeerShape->getOCCTShape(resultId);
          if (blendShape.IsNull()) continue;
          if
          (
            (
              (resolver.getPick().selectionType == slc::Type::StartPoint)
              || (resolver.getPick().selectionType == slc::Type::EndPoint)
            )
            && blendShape.ShapeType() == TopAbs_VERTEX
          )
          {
            const TopoDS_Vertex &v = TopoDS::Vertex(blendShape);
            //set radius on all applicable edges.
            for (int ci = 1; ci < bMaker->NbElements() + 1; ++ci)
            {
              if (!(bMaker->RelativeAbscissa(ci, v) < 0.0)) //tests if vertex is on contour
              {
                have.Add(v);
                bMaker->SetRadius(e.radius.getDouble(), ci, v);
                if (!labelDone)
                {
                  e.radiusLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(v)));
                  labelDone = true;
                }
              }
            }
          }
          else if
          (
            (
              (resolver.getPick().selectionType == slc::Type::MidPoint)
              || (resolver.getPick().selectionType == slc::Type::NearestPoint)
            )
            && blendShape.ShapeType() == TopAbs_EDGE
          )
          {
            const TopoDS_Edge &edge = TopoDS::Edge(blendShape);
            int ei;
            int ci = bMaker->Contains(edge, ei);
            if (ci > 0)
            {
              double p = e.position.getDouble();
              //TODO Do I really need to do this?
//               e.entryPick.getPicks().front().u = p; //keep pick in sync with parameter. we use pick.u in dialog.
              gp_XY pair(p, e.radius.getDouble());
              bMaker->SetRadius(pair, ci, ei);
            }
            if (!labelDone)
            {
              e.radiusLabel->setMatrix(osg::Matrixd::translate(resolver.getPick().getPoint(edge)));
              e.positionLabel->setMatrix(osg::Matrixd::translate(resolver.getPick().getPoint(edge) + osg::Vec3d(0.0, 0.0, 2.0)));
              labelDone = true;
            }
          }
        }
        //TODO deal with edges.
      }
      intersection.Intersection(needed, have);
      if (needed.Size() != intersection.Size()) bail("Spine end doesn't have radius set");
    }
    else if (stow->blendType.getInt() == 2) // law spine
    {
      opencascade::handle<Law_Function> lawFunction;
      const auto &spinePicks = stow->lawSpinePick.getPicks();
      if (spinePicks.empty()) bail("No law spine");
      if (!resolver.resolve(spinePicks.front())) bail("Couldn't resolve spine pick");
      const auto &lawSpine = dynamic_cast<const ftr::LawSpine::Feature&>(*resolver.getFeature());
      if (lawSpine.isModelDirty()) bail("Law spine is out of date");
      lawFunction = lawSpine.getVessel().buildLawFunction();
      
      const auto &edgePicks = stow->lawEdgePick.getPicks();
      if (edgePicks.empty()) bail("No law edge");
      if (!resolver.resolve(edgePicks.front())) bail("Couldn't resolve edge pick");
      const auto &edgeIds = resolver.getResolvedIds();
      if (edgeIds.empty()) bail("No law edge ids from resolver");
      
      goBlendMaker(resolver.getSeerShape(), edgeIds.front());
      TopoDS_Shape tempShape = targetSeerShape->getOCCTShape(edgeIds.front());
      assert(!tempShape.IsNull());
      assert(tempShape.ShapeType() == TopAbs_EDGE);
      bMaker->Add(TopoDS::Edge(tempShape));
      opencascade::handle<ChFiDS_FilSpine> SSpine = dynamic_cast<ChFiDS_FilSpine*>(bMaker->Value(1).get());
      assert(SSpine);
      SSpine->SetRadius(lawFunction, 0);
    }
    if (!bMaker) bail("Invalid blend maker");
    bMaker->SetFilletShape(static_cast<ChFi3d_FilletShape>(stow->filletShape.getInt()));
    bMaker->Compute();
    if (!bMaker->IsDone()) bail("blendMaker failed");
    ShapeCheck check(bMaker->Shape());
    if (!check.isValid()) bail("shapeCheck failed");
    
    stow->sShape.setOCCTShape(bMaker->Shape(), getId());
    stow->sShape.shapeMatch(*targetSeerShape);
    stow->sShape.uniqueTypeMatch(*targetSeerShape);
    stow->match(*bMaker, *targetSeerShape);
    stow->ensureNoFaceNils(); //hack see notes in function.
    stow->sShape.outerWireMatch(*targetSeerShape);
    stow->sShape.derivedMatch();
    stow->sShape.dumpNils("blend feature");
    stow->sShape.dumpDuplicates("blend feature");
    stow->sShape.ensureNoNils();
    stow->sShape.ensureNoDuplicates();
    stow->sShape.ensureEvolve();
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in blend update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in blend update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in blend update." << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Feature::serialWrite(const boost::filesystem::path &dIn)
{
  namespace serial = prj::srl::blns;
  
  serial::Blend blendOut
  (
    Base::serialOut()
    , stow->filletShape.serialOut()
    , stow->blendType.serialOut()
    , stow->lawSpinePick.serialOut()
    , stow->lawEdgePick.serialOut()
    , stow->sShape.serialOut()
  );
  
  switch (stow->blendType.getInt())
  {
    case 0: //constant
    {
      for (const auto &sBlend : stow->constants)
        blendOut.constants().push_back(sBlend.serialOut());
      break;
    }
    case 1: //variable
    {
      blendOut.variable() = stow->variable.serialOut();
      break;
    }
    case 2: //law spine
    {
      break;
    }
    default:
    {
      assert(0); //unknown blend type.
      break;
    }
  }
  
  for (const auto &p : stow->shapeMap)
  {
    blendOut.shapeMap().push_back
    (
      serial::Blend::ShapeMapType
      (
        gu::idToString(p.first),
        gu::idToString(p.second)
      )
    );
  }
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  serial::blend(stream, blendOut, infoMap);
}

void Feature::serialRead(const prj::srl::blns::Blend& sBlendIn)
{
  Base::serialIn(sBlendIn.base());
  stow->filletShape.serialIn(sBlendIn.filletShape());
  stow->blendType.serialIn(sBlendIn.blendType());
  stow->lawSpinePick.serialIn(sBlendIn.lawSpinePick());
  stow->lawEdgePick.serialIn(sBlendIn.lawEdgePick());
  stow->sShape.serialIn(sBlendIn.seerShape());
  
  switch (stow->blendType.getInt())
  {
    case 0: //constant
    {
      for (const auto &c : sBlendIn.constants())
      {
        stow->constants.emplace_back(c);
        stow->attachConstant(stow->constants.back());
      }
      break;
    }
    case 1: //variable
    {
      assert(sBlendIn.variable());
      stow->variable.serialIn(sBlendIn.variable().get());
      stow->attachVariable();
      break;
    }
    case 2: //lawspine
    {
      //do nothing special.
      break;
    }
    default:
    {
      assert(0); //unknown blend type.
      break;
    }
  }
  
  for (const auto &p : sBlendIn.shapeMap())
    stow->shapeMap.insert(std::make_pair(gu::stringToId(p.idIn()), gu::stringToId(p.idOut())));
}
