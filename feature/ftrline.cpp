/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2019  Thomas S. Anderson blobfish.at.gmx.com
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
#include <boost/optional/optional.hpp>

#include <TopoDS.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <TopExp.hxx>

#include <osg/Switch>

#include "globalutilities.h"
#include "tools/idtools.h"
#include "tools/featuretools.h"
#include "feature/ftrshapecheck.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinputtype.h"
#include "annex/annseershape.h"
#include "project/serial/generated/prjsrllnsline.h"
#include "feature/ftrline.h"

using namespace ftr;
using boost::uuids::uuid;

QIcon Line::icon;

Line::Line()
: Base()
, sShape(new ann::SeerShape())
, lineId(gu::createRandomId())
, v0Id(gu::createRandomId())
, v1Id(gu::createRandomId())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/sketchLine.svg");
  
  name = QObject::tr("Line");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
}

Line::~Line() {}

void Line::setPicks(const Picks &pIn)
{
  picks = pIn;
  setModelDirty();
}

void Line::updateModel(const UpdatePayload &pIn)
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
    if (picks.size() != 2) //probably temp
      throw std::runtime_error("wrong number of picks");
    
    tls::Resolver resolver(pIn);
    
    resolver.resolve(picks.front());
    auto points0 = resolver.getPoints();
    if (points0.empty())
      throw std::runtime_error("No points for first pick");
    if (points0.size() > 1)
    {
      std::ostringstream s; s << "WARNING: more than 1 point for first pick. Using first" << std::endl;
      lastUpdateLog += s.str();
    }
    
    resolver.resolve(picks.back());
    auto points1 = resolver.getPoints();
    if (points1.empty())
      throw std::runtime_error("No points for second pick");
    if (points1.size() > 1)
    {
      std::ostringstream s; s << "WARNING: more than 1 point for second pick. Using first" << std::endl;
      lastUpdateLog += s.str();
    }
    
    BRepBuilderAPI_MakeEdge edgeMaker(gp_Pnt(gu::toOcc(points0.front()).XYZ()), gp_Pnt(gu::toOcc(points1.front()).XYZ()));
    if (!edgeMaker.IsDone())
      throw std::runtime_error("Couldn't construct line edge");
    sShape->setOCCTShape(TopoDS::Edge(edgeMaker), getId());
    sShape->updateId(edgeMaker.Edge(), lineId);
    sShape->updateId(TopExp::FirstVertex(edgeMaker.Edge()), v0Id);
    sShape->updateId(TopExp::LastVertex(edgeMaker.Edge()), v1Id);
    if (!sShape->hasEvolveRecordOut(lineId))
      sShape->insertEvolve(gu::createNilId(), lineId);
    if (!sShape->hasEvolveRecordOut(v0Id))
      sShape->insertEvolve(gu::createNilId(), v0Id);
    if (!sShape->hasEvolveRecordOut(v1Id))
      sShape->insertEvolve(gu::createNilId(), v1Id);
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in line update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in line update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in line update. " << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Line::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::lns::Line so
  (
    Base::serialOut()
    , sShape->serialOut()
    , gu::idToString(lineId)
    , gu::idToString(v0Id)
    , gu::idToString(v1Id)
  );
  for (const auto &p : picks)
    so.picks().push_back(p);
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::lns::line(stream, so, infoMap);
}

void Line::serialRead(const prj::srl::lns::Line &sli)
{
  Base::serialIn(sli.base());
  sShape->serialIn(sli.seerShape());
  lineId = gu::stringToId(sli.lineId());
  v0Id = gu::stringToId(sli.v0Id());
  v1Id = gu::stringToId(sli.v1Id());
  for (const auto &p : sli.picks())
    picks.emplace_back(p);
}
