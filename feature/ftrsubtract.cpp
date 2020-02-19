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
#include <BRepAlgoAPI_Cut.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <BRepTools.hxx>
#include <BOPDS_DS.hxx>

#include <osg/Switch>

#include "globalutilities.h"
#include "feature/ftrbooleanoperation.h"
#include "feature/ftrshapecheck.h"
#include "project/serial/generated/prjsrlsbtssubtract.h"
#include "annex/annseershape.h"
#include "annex/annintersectionmapper.h"
#include "tools/featuretools.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrsubtract.h"

using boost::uuids::uuid;

using namespace ftr;

QIcon Subtract::icon;

Subtract::Subtract() : Base(), sShape(new ann::SeerShape()), iMapper(new ann::IntersectionMapper())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionSubtract.svg");
  
  name = QObject::tr("Subtract");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
  annexes.insert(std::make_pair(ann::Type::IntersectionMapper, iMapper.get()));
}

Subtract::~Subtract(){}

void Subtract::setTargetPicks(const Picks &psIn)
{
  if (targetPicks == psIn)
    return;
  targetPicks = psIn;
  setModelDirty();
}

void Subtract::setToolPicks(const Picks &psIn)
{
  if (toolPicks == psIn)
    return;
  toolPicks = psIn;
  setModelDirty();
}

void Subtract::updateModel(const UpdatePayload &payloadIn)
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
    
    BooleanOperation subtracter(targetOCCTShapes, toolOCCTShapes, BOPAlgo_CUT);
    subtracter.Build();
    if (!subtracter.IsDone())
      throw std::runtime_error("OCC subtraction failed");
    ShapeCheck check(subtracter.Shape());
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    sShape->setOCCTShape(subtracter.Shape(), getId());
    
    iMapper->go(payloadIn, subtracter.getBuilder(), *sShape);
    
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
    sShape->dumpNils(getTypeString()); //only if there are shapes with nil ids.
    sShape->dumpDuplicates(getTypeString());
    sShape->ensureNoNils();
    sShape->ensureNoDuplicates();
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in subtract update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in subtract update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in subtract update." << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Subtract::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::sbts::Subtract subtractOut
  (
    Base::serialOut(),
    sShape->serialOut(),
    iMapper->serialOut()
  );
  for (const auto &p : targetPicks)
    subtractOut.targetPicks().push_back(p);
  for (const auto &p : toolPicks)
    subtractOut.toolPicks().push_back(p);
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::sbts::subtract(stream, subtractOut, infoMap);
}

void Subtract::serialRead(const prj::srl::sbts::Subtract &sSubtractIn)
{
  Base::serialIn(sSubtractIn.base());
  sShape->serialIn(sSubtractIn.seerShape());
  iMapper->serialIn(sSubtractIn.intersectionMapper());
  for (const auto &p : sSubtractIn.targetPicks())
    targetPicks.emplace_back(p);
  for (const auto &p : sSubtractIn.toolPicks())
    toolPicks.emplace_back(p);
}
