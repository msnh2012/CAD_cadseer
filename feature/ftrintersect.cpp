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

#include "globalutilities.h"
#include "tools/featuretools.h"
#include "feature/ftrbooleanoperation.h"
#include "project/serial/xsdcxxoutput/featureintersect.h"
#include "feature/ftrshapecheck.h"
#include "annex/annseershape.h"
#include "annex/annintersectionmapper.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrintersect.h"

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
    occt::ShapeVector targetOCCTShapes;
    std::vector<tls::Resolver> targetResolvers;
    for (const auto &tp : targetPicks)
    {
      targetResolvers.emplace_back(payloadIn);
      targetResolvers.back().resolve(tp);
      auto shapes = targetResolvers.back().getShapes();
      std::copy(shapes.begin(), shapes.end(), std::back_inserter(targetOCCTShapes));
    }
    
    //set to new failed state.
    TopoDS_Compound tc = occt::ShapeVectorCast(targetOCCTShapes);
    sShape->setOCCTShape(tc, getId());
    BOPAlgo_Builder dummy;
    iMapper->go(payloadIn, dummy, *sShape);
    for (const auto &it : targetResolvers)
      sShape->shapeMatch(*it.getSeerShape());
    for (const auto &it : targetResolvers)
      sShape->uniqueTypeMatch(*it.getSeerShape());
    for (const auto &it : targetResolvers)
      sShape->outerWireMatch(*it.getSeerShape());
    sShape->derivedMatch();
    sShape->ensureNoNils(); //just in case
    sShape->ensureNoDuplicates(); //just in case
    
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    //tools
    occt::ShapeVector toolOCCTShapes;
    std::vector<tls::Resolver> toolResolvers;
    for (const auto &tp : toolPicks)
    {
      toolResolvers.emplace_back(payloadIn);
      toolResolvers.back().resolve(tp);
      auto shapes = toolResolvers.back().getShapes();
      std::copy(shapes.begin(), shapes.end(), std::back_inserter(toolOCCTShapes));
    }
    
    BooleanOperation intersector(targetOCCTShapes, toolOCCTShapes, BOPAlgo_COMMON);
    intersector.Build();
    if (!intersector.IsDone())
      throw std::runtime_error("OCC intersect failed");
    ShapeCheck check(intersector.Shape());
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    sShape->setOCCTShape(intersector.Shape(), getId());
    
    iMapper->go(payloadIn, intersector.getBuilder(), *sShape);
    
    for (const auto &it : targetResolvers)
      sShape->shapeMatch(*it.getSeerShape());
    for (const auto &it : toolResolvers)
      sShape->shapeMatch(*it.getSeerShape());
    for (const auto &it : targetResolvers)
      sShape->uniqueTypeMatch(*it.getSeerShape());
    for (const auto &it : targetResolvers)
      sShape->outerWireMatch(*it.getSeerShape());
    for (const auto &it : toolResolvers)
      sShape->outerWireMatch(*it.getSeerShape());
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
