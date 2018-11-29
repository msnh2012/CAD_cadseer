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
#include <boost/variant/variant.hpp>

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

#include <osg/Switch>
#include <feature/inputtype.h>

#include <globalutilities.h>
#include <tools/idtools.h>
#include <tools/occtools.h>
#include <tools/featuretools.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <project/serial/xsdcxxoutput/featureblend.h>
#include <feature/parameter.h>
#include <feature/shapecheck.h>
#include <annex/seershape.h>
#include <feature/parameter.h>
#include <feature/updatepayload.h>
#include <feature/blend.h>

using namespace ftr;
using boost::uuids::uuid;

QIcon Blend::icon;

Blend::Blend() : Base(), sShape(new ann::SeerShape())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionBlend.svg");
  
  name = QObject::tr("Blend");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
}

Blend::~Blend(){}

std::shared_ptr< prm::Parameter > Blend::buildRadiusParameter()
{
  std::shared_ptr<prm::Parameter> out(new prm::Parameter(prm::Names::Radius, prf::manager().rootPtr->features().blend().get().radius()));
  out->setConstraint(prm::Constraint::buildZeroPositive());
  return out;
}

std::shared_ptr< prm::Parameter > Blend::buildPositionParameter()
{
  std::shared_ptr<prm::Parameter> out(new prm::Parameter(prm::Names::Position, 0.0));
  out->setConstraint(prm::Constraint::buildUnit());
  return out;
}

VariableBlend Blend::buildDefaultVariable(const ann::SeerShape &seerShapeIn, const Pick &pickIn, const ShapeHistory& historyIn)
{
  VariableBlend out;
  out.picks.push_back(pickIn);
  
  const TopoDS_Shape &rootShape = seerShapeIn.getRootOCCTShape();
  BRepFilletAPI_MakeFillet blendMaker(rootShape);
  const TopoDS_Shape &edge = seerShapeIn.getOCCTShape(pickIn.id); //TODO might be face!
  blendMaker.Add(TopoDS::Edge(edge));
  
  //not using position parameter for vertices, but building anyway for consistency
  VariableEntry entry1;
  entry1.pick.id = seerShapeIn.findId(blendMaker.FirstVertex(1));
  entry1.pick.shapeHistory = historyIn.createDevolveHistory(entry1.pick.id);
  entry1.radius = buildRadiusParameter();
  entry1.position = buildPositionParameter();
  out.entries.push_back(entry1);
  
  VariableEntry entry2;
  entry2.pick.id = seerShapeIn.findId(blendMaker.LastVertex(1));
  if (entry1.pick.id != entry2.pick.id) //periodic spine
  {
    entry2.pick.shapeHistory = historyIn.createDevolveHistory(entry2.pick.id);
    entry2.radius = buildRadiusParameter();
    entry2.radius->setValue(1.5);
    entry2.position = buildPositionParameter();
    out.entries.push_back(entry2);
  }
  
  return out;
}

void Blend::removeParameter(prm::Parameter *p)
{
  auto it = std::find(parameters.begin(), parameters.end(), p);
  if(it != parameters.end())
    parameters.erase(it);
}

void Blend::clearBlends()
{
  for (const auto &sb : simpleBlends)
  {
    removeParameter(sb.radius.get());
    overlaySwitch->removeChild(sb.label.get());
  }
  simpleBlends.clear();
  
  for (const auto &e : variableBlend.entries)
  {
    removeParameter(e.radius.get());
    removeParameter(e.position.get());
    overlaySwitch->removeChild(e.label.get());
  }
  variableBlend.picks.clear();
  variableBlend.entries.clear();
}

void Blend::addSimpleBlend(const SimpleBlend &simpleBlendIn)
{
  addSimpleBlendQuiet(simpleBlendIn);
  setModelDirty();
}

void Blend::addSimpleBlendQuiet(const SimpleBlend &simpleBlendIn)
{
  if (simpleBlendIn.radius)
    simpleBlends.push_back(simpleBlendIn);
  else
  {
    SimpleBlend temp = simpleBlendIn;
    temp.radius = buildRadiusParameter();
    simpleBlends.push_back(temp);
  }
  simpleBlends.back().radius->connectValue(std::bind(&Blend::setModelDirty, this));
  if (!simpleBlends.back().label)
  {
    simpleBlends.back().label = new lbr::PLabel(simpleBlends.back().radius.get());
    simpleBlends.back().label->showName = true;
  }
  simpleBlends.back().label->valueHasChanged();
  simpleBlends.back().label->constantHasChanged();
  overlaySwitch->addChild(simpleBlends.back().label.get());
  
  parameters.push_back(simpleBlends.back().radius.get());
}

void Blend::addVariableBlend(const VariableBlend &variableBlendIn)
{
  addVariableBlendQuiet(variableBlendIn);
  setModelDirty();
}

void Blend::addVariableBlendQuiet(const VariableBlend &variableBlendIn)
{
  variableBlend = variableBlendIn;
  for (auto &e : variableBlend.entries)
  {
    e.radius->connectValue(std::bind(&Blend::setModelDirty, this));
    parameters.push_back(e.radius.get());
    if (!e.label)
    {
      e.label = new lbr::PLabel(e.radius.get());
      e.label->showName = true;
    }
    e.label->valueHasChanged();
    e.label->constantHasChanged();
    overlaySwitch->addChild(e.label.get());
    
    if (!e.position)
      e.position = buildPositionParameter();
    e.position->setValue(e.pick.u);
    e.position->connectValue(std::bind(&Blend::setModelDirty, this));
    if (!e.positionLabel)
    {
      e.positionLabel = new lbr::PLabel(e.position.get());
      e.positionLabel->showName = true;
    }
    if (e.pick.selectionType == slc::Type::MidPoint || e.pick.selectionType == slc::Type::NearestPoint)
    {
      e.positionLabel->valueHasChanged();
      e.positionLabel->constantHasChanged();
      overlaySwitch->addChild(e.positionLabel.get());
      parameters.push_back(e.position.get());
    }
  }
}

/* Typical occt bullshit: v7.1
 * all radius data, reguardless of assignment method, ends up inside ChFiDS_FilSpine::parandrad.
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
 *    value set to satify the above, what happens when we assign a value to a vertex that coincides
 *    with said endpoints?
 * I think for now, I will just restrict parameter points for variable radius to vertices. No 'nearest'.
 * Combining a variable and a constant into the same feature is triggering an occt exception. v7.1.
 * 
 * switched to ChFi3d_FilBuilder in occt v7.3. this allowed us to use the more sensible parameter
 * assignment for mid and nearest points.
 */

void Blend::updateModel(const UpdatePayload &payloadIn)
{
  setFailure();
  lastUpdateLog.clear();
  sShape->reset();
  try
  {
    std::vector<const Base*> features = payloadIn.getFeatures(InputType::target);
    if (features.size() != 1)
      throw std::runtime_error("wrong number of target inputs");
    if (!features.front()->hasAnnex(ann::Type::SeerShape))
      throw std::runtime_error("no seer shape in parent");
    const ann::SeerShape &targetSeerShape = features.front()->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
    if (targetSeerShape.isNull())
      throw std::runtime_error("target seer shape is null");
    
    //parent is good. Set up seershape in case of failure.
    sShape->setOCCTShape(targetSeerShape.getRootOCCTShape());
    sShape->shapeMatch(targetSeerShape);
    sShape->uniqueTypeMatch(targetSeerShape);
    sShape->outerWireMatch(targetSeerShape);
    sShape->derivedMatch();
    sShape->ensureNoNils();
    sShape->ensureNoDuplicates();
    
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    ChFi3d_FilBuilder bMaker(targetSeerShape.getRootOCCTShape());
    bMaker.SetFilletShape(static_cast<ChFi3d_FilletShape>(filletShape));
    for (const auto &simpleBlend : simpleBlends)
    {
      bool labelDone = false; //set label position to first pick.
      for (const auto &pick : simpleBlend.picks)
      {
        auto resolvedPicks = tls::resolvePicks(features.front(), pick, payloadIn.shapeHistory);
        for (const auto &resolved : resolvedPicks) //resolved pair
        {
          if (resolved.resultId.is_nil())
            continue;
          updateShapeMap(resolved.resultId, pick.shapeHistory);
          assert(targetSeerShape.hasId(resolved.resultId)); //project history out of sync with seershape.
          TopoDS_Shape tempShape = targetSeerShape.getOCCTShape(resolved.resultId);
          assert(!tempShape.IsNull());
          assert(tempShape.ShapeType() == TopAbs_EDGE);
          bMaker.Add(static_cast<double>(*(simpleBlend.radius)), TopoDS::Edge(tempShape));
          //update location of parameter label.
          if (!labelDone)
          {
            labelDone = true;
            simpleBlend.label->setMatrix(osg::Matrixd::translate(pick.getPoint(TopoDS::Edge(tempShape))));
          }
        }
      }
    }
    if (simpleBlends.empty())
    {
      for (const auto &pick : variableBlend.picks)
      {
        auto resolvedPicks = tls::resolvePicks(features.front(), pick, payloadIn.shapeHistory);
        for (const auto &resolved : resolvedPicks)
        {
          if (resolved.resultId.is_nil())
              continue;
          updateShapeMap(resolved.resultId, pick.shapeHistory);
          assert(targetSeerShape.hasId(resolved.resultId)); //project history out of sync with seershape.
          TopoDS_Shape tempShape = targetSeerShape.getOCCTShape(resolved.resultId);
          assert(!tempShape.IsNull());
          assert(tempShape.ShapeType() == TopAbs_EDGE); //TODO faces someday.
          bMaker.Add(TopoDS::Edge(tempShape));
        }
      }
      for (auto &e : variableBlend.entries)
      {
        bool labelDone = false;
        auto resolvedEntryPicks = tls::resolvePicks(features.front(), e.pick, payloadIn.shapeHistory);
        for (const auto &rep : resolvedEntryPicks)
        {
          if (rep.resultId.is_nil())
            continue;
          const TopoDS_Shape &blendShape = targetSeerShape.getOCCTShape(rep.resultId);
          if (blendShape.IsNull())
            continue;
          if
          (
            (
              (rep.pick.selectionType == slc::Type::StartPoint)
              || (rep.pick.selectionType == slc::Type::EndPoint)
            )
            && blendShape.ShapeType() == TopAbs_VERTEX
          )
          {
            const TopoDS_Vertex &v = TopoDS::Vertex(blendShape);
            //set radius on all applicable edges.
            for (int ci = 1; ci < bMaker.NbElements() + 1; ++ci)
            {
              if (!(bMaker.RelativeAbscissa(ci, v) < 0.0)) //tests if vertex is on contour
              {
                bMaker.SetRadius(static_cast<double>(*e.radius), ci, v);
                if (!labelDone)
                {
                  e.label->setMatrix(osg::Matrixd::translate(gu::toOsg(v)));
                  labelDone = true;
                }
              }
            }
          }
          else if
          (
            (
              (rep.pick.selectionType == slc::Type::MidPoint)
              || (rep.pick.selectionType == slc::Type::NearestPoint)
            )
            && blendShape.ShapeType() == TopAbs_EDGE
          )
          {
            const TopoDS_Edge &edge = TopoDS::Edge(blendShape);
            int ei;
            int ci = bMaker.Contains(edge, ei);
            if (ci > 0)
            {
              double p = static_cast<double>(*e.position);
              e.pick.u = p; //keep pick in sync with parameter. we use pick.u in dialog.
              gp_XY pair(p, static_cast<double>(*e.radius));
              bMaker.SetRadius(pair, ci, ei);
            }
            if (!labelDone)
            {
              e.label->setMatrix(osg::Matrixd::translate(e.pick.getPoint(edge)));
              e.positionLabel->setMatrix(osg::Matrixd::translate(e.pick.getPoint(edge) + osg::Vec3d(0.0, 0.0, 2.0)));
              labelDone = true;
            }
          }
        }
        //TODO deal with edges.
      }
    }
    bMaker.Compute();
    if (!bMaker.IsDone())
      throw std::runtime_error("blendMaker failed");
    ShapeCheck check(bMaker.Shape());
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    sShape->setOCCTShape(bMaker.Shape());
    sShape->shapeMatch(targetSeerShape);
    sShape->uniqueTypeMatch(targetSeerShape);
    match(bMaker, targetSeerShape);
    ensureNoFaceNils(); //hack see notes in function.
    sShape->outerWireMatch(targetSeerShape);
    sShape->derivedMatch();
    sShape->dumpNils("blend feature");
    sShape->dumpDuplicates("blend feature");
    sShape->ensureNoNils();
    sShape->ensureNoDuplicates();
    
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

/* matching by interogating the ChFi3d_FilBuilder objects generated and modified methods.
 * generated only outputs the fillet faces from the edge. Nothing else.
 * tried iterating over MakeFillet contours and edges, but in certain situations
 * blends are made from vertices. blending 3 edges of a box corner is such a situation.
 * so we will iterate over all the shapes of the targe object.
 */
void Blend::match(ChFi3d_FilBuilder &bMaker, const ann::SeerShape &targetShapeIn)
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
        lastUpdateLog += s.str();
      }
      const TopoDS_Shape &blendFace = generated.First();
      assert(!blendFace.IsNull());
      assert(blendFace.ShapeType() == TopAbs_FACE);
      
      std::map<uuid, uuid>::iterator mapItFace;
      bool dummy;
      std::tie(mapItFace, dummy) = shapeMap.insert(std::make_pair(cId, gu::createRandomId()));
      sShape->updateId(blendFace, mapItFace->second);
      if (dummy) //insertion took place.
        sShape->insertEvolve(gu::createNilId(), mapItFace->second);
      
      //now look for outerwire for newly generated face.
      //we use the generated face id to map to outer wire.
      std::map<uuid, uuid>::iterator mapItWire;
      std::tie(mapItWire, dummy) = shapeMap.insert(std::make_pair(mapItFace->second, gu::createRandomId()));
      //now get the wire and update the result to id.
      const TopoDS_Shape &blendFaceWire = BRepTools::OuterWire(TopoDS::Face(blendFace));
      sShape->updateId(blendFaceWire, mapItWire->second);
      if (dummy) //insertion took place.
        sShape->insertEvolve(gu::createNilId(), mapItWire->second);
    }
    occt::ShapeVector mods = modified(currentShape);
    if (!mods.empty())
    {
      if (mods.size() > 1)
      {
        std::ostringstream s; s << "WARNING: more than one modified shape" << std::endl;
        lastUpdateLog += s.str();
      }
      
      for (const auto &s : mods)
      {
        //this is basically the same as SeerShape match.
        if(!sShape->hasShape(s))
          continue;
        
        uuid freshId = gu::createNilId();
        if (sShape->hasEvolveRecordIn(cId))
          freshId = sShape->evolve(cId).front(); //multiple returns?
        else
        {
          freshId = gu::createRandomId();
          sShape->insertEvolve(cId, freshId);
        }
        sShape->updateId(s, freshId);
      }
    }
  }
}

void Blend::updateShapeMap(const boost::uuids::uuid &resolvedId, const ShapeHistory &pick)
{
  for (const auto &historyId : pick.getAllIds())
  {
    assert(shapeMap.count(historyId) < 2);
    auto it = shapeMap.find(historyId);
    if (it == shapeMap.end())
      continue;
    auto entry = std::make_pair(resolvedId, it->second);
    shapeMap.erase(it);
    shapeMap.insert(entry);
  }
}

void Blend::ensureNoFaceNils()
{
  //ok this is a hack. Because of an OCCT bug, generated is missing
  //an edge to blended face mapping and on another edge has 2 faces
  //generated from a single edge. Without the face having an id, derived
  //shapes (edges and verts) can't be assigned an id. This causes a lot
  //of id failures. So for now just assign an id and check for bug
  //in next release.
  
  
  auto shapes = sShape->getAllShapes();
  for (const auto &shape : shapes)
  {
    if (shape.ShapeType() != TopAbs_FACE)
      continue;
    if (!sShape->findId(shape).is_nil())
      continue;
    
    sShape->updateId(shape, gu::createRandomId());
  }
}

//only partial update from switch to ChFi3d_FilBuilder
void Blend::dumpInfo(ChFi3d_FilBuilder &blendMakerIn, const ann::SeerShape &targetFeatureIn)
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

void Blend::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::FeatureBlend::ShapeMapType shapeMapOut;
  for (const auto &p : shapeMap)
  {
    prj::srl::EvolveRecord eRecord
    (
      gu::idToString(p.first),
      gu::idToString(p.second)
    );
    shapeMapOut.evolveRecord().push_back(eRecord);
  }
  
  prj::srl::SimpleBlends sBlendsOut;
  for (const auto &sBlend : simpleBlends)
  {
    prj::srl::Picks bPicksOut = ::ftr::serialOut(sBlend.picks);
    prj::srl::SimpleBlend sBlendOut
    (
      bPicksOut,
      sBlend.radius->serialOut(),
      sBlend.label->serialOut()
    );
    sBlendsOut.array().push_back(sBlendOut);
  }

  prj::srl::VariableEntries vEntriesOut;
  for (const auto vEntry : variableBlend.entries)
  {
    prj::srl::VariableEntry vEntryOut
    (
      vEntry.pick.serialOut(),
      vEntry.position->serialOut(),
      vEntry.radius->serialOut(),
      vEntry.label->serialOut(),
      vEntry.positionLabel->serialOut()
    );
    vEntriesOut.array().push_back(vEntryOut);
  }
  
  prj::srl::VariableBlend vBlendOut
  (
    ::ftr::serialOut(variableBlend.picks),
    vEntriesOut
  );
  
  prj::srl::FeatureBlend blendOut
  (
    Base::serialOut(),
    shapeMapOut,
    sBlendsOut,
    vBlendOut,
    static_cast<int>(filletShape)
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::blend(stream, blendOut, infoMap);
}

void Blend::serialRead(const prj::srl::FeatureBlend& sBlendIn)
{
  Base::serialIn(sBlendIn.featureBase());
  
  shapeMap.clear();
  for (const prj::srl::EvolveRecord &sERecord : sBlendIn.shapeMap().evolveRecord())
  {
    std::pair<uuid, uuid> record;
    record.first = gu::stringToId(sERecord.idIn());
    record.second = gu::stringToId(sERecord.idOut());
    shapeMap.insert(record);
  }
  
  for (const auto &simpleBlendIn : sBlendIn.simpleBlends().array())
  {
    SimpleBlend simpleBlend;
    for (const auto &bPickIn : simpleBlendIn.blendPicks().array())
    {
      Pick pick;
      pick.serialIn(bPickIn);
      simpleBlend.picks.push_back(pick);
    }
    simpleBlend.radius = buildRadiusParameter();
    simpleBlend.radius->serialIn(simpleBlendIn.radius());
    simpleBlend.label = new lbr::PLabel(simpleBlend.radius.get());
    simpleBlend.label->showName = true;
    simpleBlend.label->serialIn(simpleBlendIn.plabel());
    addSimpleBlendQuiet(simpleBlend);
  }
  
  VariableBlend vBlend;
  vBlend.picks = ::ftr::serialIn(sBlendIn.variableBlend().blendPicks());
  for (const auto &entryIn : sBlendIn.variableBlend().variableEntries().array())
  {
    VariableEntry entry;
    entry.pick.serialIn(entryIn.blendPick());
    entry.position = buildPositionParameter();
    entry.position->serialIn(entryIn.position());
    entry.radius = buildRadiusParameter();
    entry.radius->serialIn(entryIn.radius());
    entry.label = new lbr::PLabel(entry.radius.get());
    entry.label->showName = true;
    entry.label->serialIn(entryIn.plabel());
    entry.positionLabel = new lbr::PLabel(entry.position.get());
    entry.positionLabel->showName = true;
    entry.positionLabel->serialIn(entryIn.positionLabel());
    vBlend.entries.push_back(entry);
  }
  addVariableBlendQuiet(vBlend);
  
  filletShape = static_cast<Shape>(sBlendIn.filletShape());
}
