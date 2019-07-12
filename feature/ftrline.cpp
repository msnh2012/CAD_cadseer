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

#include <osg/Switch>

#include "globalutilities.h"
#include "tools/idtools.h"
#include "tools/featuretools.h"
#include "feature/ftrshapecheck.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinputtype.h"
#include "annex/annseershape.h"
#include "project/serial/xsdcxxoutput/featureline.h"
#include "feature/ftrline.h"

using namespace ftr;
using boost::uuids::uuid;

QIcon Line::icon;

Line::Line()
: Base()
, sShape(new ann::SeerShape())
, lineId(gu::createRandomId())
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
    
    std::vector<const Base*> pzf = pIn.getFeatures(pickZero);
    if (pzf.size() != 1)
      throw std::runtime_error("wrong number of pick zero parent features");
    if (!pzf.front()->hasAnnex(ann::Type::SeerShape))
      throw std::runtime_error("pick zero parent doesn't have seer shape.");
    const ann::SeerShape &pzss = pzf.front()->getAnnex<ann::SeerShape>();
    if (pzss.isNull())
      throw std::runtime_error("pick zero seer shape is null");
    std::vector<tls::Resolved> res0 = tls::resolvePicks(pzf.front(), picks.at(0), pIn.shapeHistory);
    boost::optional<osg::Vec3d> point0 = (!res0.empty()) ? res0.front().getPoint(pzss) : boost::none;
    
    std::vector<const Base*> pof = pIn.getFeatures(pickOne);
    if (pof.size() != 1)
      throw std::runtime_error("wrong number of pick one parent features");
    if (!pof.front()->hasAnnex(ann::Type::SeerShape))
      throw std::runtime_error("pick one parent doesn't have seer shape.");
    const ann::SeerShape &poss = pof.front()->getAnnex<ann::SeerShape>();
    if (pzss.isNull())
      throw std::runtime_error("pick one seer shape is null");
    std::vector<tls::Resolved> res1 = tls::resolvePicks(pof.front(), picks.at(1), pIn.shapeHistory);
    boost::optional<osg::Vec3d> point1 = (!res1.empty()) ? res1.front().getPoint(poss) : boost::none;
    
    //for now we are just supporting points.
    if (!point0 || !point1)
      throw std::runtime_error("didn't get 2 points");
    
    BRepBuilderAPI_MakeEdge edgeMaker(gp_Pnt(gu::toOcc(point0.get()).XYZ()), gp_Pnt(gu::toOcc(point1.get()).XYZ()));
    if (!edgeMaker.IsDone())
      throw std::runtime_error("Couldn't construct line edge");
    sShape->setOCCTShape(TopoDS::Edge(edgeMaker), getId());
    sShape->updateId(edgeMaker.Edge(), lineId);
    if (!sShape->hasEvolveRecordOut(lineId))
      sShape->insertEvolve(gu::createNilId(), lineId);
    
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
  prj::srl::FeatureLine so
  (
    Base::serialOut()
    , ftr::serialOut(picks)
    , gu::idToString(lineId)
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::line(stream, so, infoMap);
}

void Line::serialRead(const prj::srl::FeatureLine &sli)
{
  Base::serialIn(sli.featureBase());
  picks = ftr::serialIn(sli.picks());
  lineId = gu::stringToId(sli.lineId());
}
