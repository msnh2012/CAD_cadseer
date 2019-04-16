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

#include <boost/filesystem/path.hpp>

#include <BOPAlgo_Builder.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

#include <osg/Switch>

#include <globalutilities.h>
#include <tools/featuretools.h>
#include <feature/booleanoperation.h>
#include <project/serial/xsdcxxoutput/featureintersect.h>
#include <feature/shapecheck.h>
#include <annex/seershape.h>
#include <annex/intersectionmapper.h>
#include <feature/updatepayload.h>
#include <feature/inputtype.h>
#include <feature/intersect.h>

using boost::uuids::uuid;

using namespace ftr;

QIcon Intersect::icon;

Intersect::Intersect() : Base(), sShape(new ann::SeerShape()), iMapper(new ann::IntersectionMapper())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionIntersect.svg");
  
  name = QObject::tr("Intersect");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
  annexes.insert(std::make_pair(ann::Type::IntersectionMapper, iMapper.get()));
}

Intersect::~Intersect(){}

void Intersect::setTargetPicks(const Picks &psIn)
{
  if (targetPicks == psIn)
    return;
  targetPicks = psIn;
  setModelDirty();
}

void Intersect::setToolPicks(const Picks &psIn)
{
  if (toolPicks == psIn)
    return;
  toolPicks = psIn;
  setModelDirty();
}

void Intersect::updateModel(const UpdatePayload &payloadIn)
{
  setFailure(); //assume failure until success.
  lastUpdateLog.clear();
  sShape->reset();
  try
  {
    //target
    std::vector<const Base*> targetFeatures = payloadIn.getFeatures(InputType::target);
    if (targetFeatures.empty())
      throw std::runtime_error("no target features");
    for (auto f = targetFeatures.begin(); f != targetFeatures.end();)
    {
      assert((*f)->hasAnnex(ann::Type::SeerShape));
      if (!(*f)->hasAnnex(ann::Type::SeerShape))
        throw std::runtime_error("target feature doesn't have seer shape");
      const ann::SeerShape &targetSeerShape = (*f)->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
      if (targetSeerShape.isNull())
        f = targetFeatures.erase(f);
      else
        f++;
    }
    if (targetFeatures.empty())
      throw std::runtime_error("target features empty after seer shape check");
    auto resolves = tls::resolvePicks(targetFeatures, targetPicks, payloadIn.shapeHistory);
    occt::ShapeVector targetOCCTShapes;
    for (const auto &resolved : resolves)
    {
      const ann::SeerShape &tShape = resolved.feature->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
      if (resolved.resultId.is_nil())
      {
        //don't include the compound.
        const auto &c = tShape.useGetNonCompoundChildren();
        std::copy(c.begin(), c.end(), std::back_inserter(targetOCCTShapes));
      }
      else
        targetOCCTShapes.push_back(tShape.getOCCTShape(resolved.resultId));
    }
    occt::uniquefy(targetOCCTShapes); //just in case.
    for (auto it = targetOCCTShapes.begin(); it != targetOCCTShapes.end();)
    {
      //remove anything below a solid.
      if (it->ShapeType() > TopAbs_SOLID)
        it = targetOCCTShapes.erase(it);
      else
        ++it;
    }
    if (targetOCCTShapes.empty())
      throw std::runtime_error("target shapes is empty");
    
    //set to new failed state.
    TopoDS_Compound tc = occt::ShapeVectorCast(targetOCCTShapes);
    sShape->setOCCTShape(tc);
    BOPAlgo_Builder dummy;
    iMapper->go(payloadIn, dummy, *sShape);
    for (const auto *it : targetFeatures)
      sShape->shapeMatch(it->getAnnex<ann::SeerShape>(ann::Type::SeerShape));
    for (const auto *it : targetFeatures)
      sShape->uniqueTypeMatch(it->getAnnex<ann::SeerShape>(ann::Type::SeerShape));
    for (const auto *it : targetFeatures)
      sShape->outerWireMatch(it->getAnnex<ann::SeerShape>(ann::Type::SeerShape));
    sShape->derivedMatch();
    sShape->ensureNoNils(); //just in case
    sShape->ensureNoDuplicates(); //just in case
    
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    //tools
    std::vector<const Base*> toolFeatures = payloadIn.getFeatures(InputType::tool);
    if (toolFeatures.empty())
      throw std::runtime_error("no tool features found");
    for (auto tf = toolFeatures.begin(); tf != toolFeatures.end();) //do some verfication.
    {
      assert((*tf)->hasAnnex(ann::Type::SeerShape)); //make user interface verify the input.
      if (!(*tf)->hasAnnex(ann::Type::SeerShape))
        throw std::runtime_error("tool feature has no seer shape");
      const ann::SeerShape &toolSeerShape = (*tf)->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
      if (toolSeerShape.isNull())
        tf = toolFeatures.erase(tf);
      else
        tf++;
    }
    if (toolFeatures.empty())
      throw std::runtime_error("tool features empty after seer shape check");
    auto resolves2 = tls::resolvePicks(toolFeatures, toolPicks, payloadIn.shapeHistory);
    occt::ShapeVector toolOCCTShapes;
    for (const auto &resolved : resolves2)
    {
      const ann::SeerShape &tShape = resolved.feature->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
      if (resolved.resultId.is_nil())
      {
        //don't include the compound.
        const auto &c = tShape.useGetNonCompoundChildren();
        std::copy(c.begin(), c.end(), std::back_inserter(toolOCCTShapes));
      }
      else
        toolOCCTShapes.push_back(tShape.getOCCTShape(resolved.resultId));
    }
    occt::uniquefy(toolOCCTShapes); //just in case.
    for (auto it = toolOCCTShapes.begin(); it != toolOCCTShapes.end();)
    {
      //remove anything below a solid.
      if (it->ShapeType() > TopAbs_SOLID)
        it = toolOCCTShapes.erase(it);
      else
        ++it;
    }
    
    BooleanOperation intersector(targetOCCTShapes, toolOCCTShapes, BOPAlgo_COMMON);
    intersector.Build();
    if (!intersector.IsDone())
      throw std::runtime_error("OCC intersect failed");
    ShapeCheck check(intersector.Shape());
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    sShape->setOCCTShape(intersector.Shape());
    
    iMapper->go(payloadIn, intersector.getBuilder(), *sShape);
    
    for (const auto *it : targetFeatures)
      sShape->shapeMatch(it->getAnnex<ann::SeerShape>(ann::Type::SeerShape));
    for (const auto *it : toolFeatures)
      sShape->shapeMatch(it->getAnnex<ann::SeerShape>(ann::Type::SeerShape));
    for (const auto *it : targetFeatures)
      sShape->uniqueTypeMatch(it->getAnnex<ann::SeerShape>(ann::Type::SeerShape));
    for (const auto *it : targetFeatures)
      sShape->outerWireMatch(it->getAnnex<ann::SeerShape>(ann::Type::SeerShape));
    for (const auto *it : toolFeatures)
      sShape->outerWireMatch(it->getAnnex<ann::SeerShape>(ann::Type::SeerShape));
    sShape->derivedMatch();
    sShape->dumpNils("intersect feature"); //only if there are shapes with nil ids.
    sShape->dumpDuplicates("intersect feature");
    sShape->ensureNoNils();
    sShape->ensureNoDuplicates();
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in intersect update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in intersect update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in intersect update." << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Intersect::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::FeatureIntersect intersectOut
  (
    Base::serialOut(),
    iMapper->serialOut(),
    ftr::serialOut(targetPicks),
    ftr::serialOut(toolPicks)
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::intersect(stream, intersectOut, infoMap);
}

void Intersect::serialRead(const prj::srl::FeatureIntersect& sIntersectIn)
{
  Base::serialIn(sIntersectIn.featureBase());
  iMapper->serialIn(sIntersectIn.intersectionMapper());
  targetPicks = ftr::serialIn(sIntersectIn.targetPicks());
  toolPicks = ftr::serialIn(sIntersectIn.toolPicks());
}