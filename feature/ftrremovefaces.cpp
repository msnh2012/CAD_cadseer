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

#include <BRepAlgoAPI_Defeaturing.hxx>

#include <osg/Switch>

#include "globalutilities.h"
#include "tools/occtools.h"
#include "tools/idtools.h"
#include "tools/featuretools.h"
#include "feature/ftrshapecheck.h"
#include "project/serial/generated/prjsrlrmfsremovefaces.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrshapecheck.h"
#include "annex/annseershape.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "feature/ftrremovefaces.h"

using boost::uuids::uuid;
using namespace ftr::RemoveFaces;
QIcon Feature::icon = QIcon(":/resources/images/constructionRemoveFaces.svg");

struct Feature::Stow
{
  Feature &feature;
  prm::Parameter picks{QObject::tr("Face Picks"), ftr::Picks(), prm::Tags::Picks};
  ann::SeerShape sShape;
  
  Stow() = delete;
  Stow(Feature &fIn)
  : feature(fIn)
  {
    picks.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&picks);
    
    feature.annexes.insert(std::make_pair(ann::Type::SeerShape, &sShape));
  }
};

Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  name = QObject::tr("RemoveFaces");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
}

Feature::~Feature() = default;

void Feature::updateModel(const UpdatePayload &payloadIn)
{
  setFailure();
  lastUpdateLog.clear();
  stow->sShape.reset();
  try
  {
    const auto &ps = stow->picks.getPicks();
    if (ps.empty())
      throw std::runtime_error("No picks");
    
    const ftr::Base *inputFeature = nullptr;
    const ann::SeerShape *inputSeerShape = nullptr;
    
    tls::Resolver resolver(payloadIn);
    if (!resolver.resolve(ps.front()))
      throw std::runtime_error("Couldn't resolve first pick");
    inputFeature = resolver.getFeature();
    inputSeerShape = resolver.getSeerShape();
    if (!inputFeature || !inputSeerShape)
      throw std::runtime_error("Invalid Input Feature Or SeerShape");
    
    //setup failure state.
    stow->sShape.setOCCTShape(inputSeerShape->getRootOCCTShape(), getId());
    stow->sShape.shapeMatch(*inputSeerShape);
    stow->sShape.uniqueTypeMatch(*inputSeerShape);
    stow->sShape.outerWireMatch(*inputSeerShape);
    stow->sShape.derivedMatch();
    stow->sShape.ensureNoNils();
    stow->sShape.ensureNoDuplicates();
    stow->sShape.ensureEvolve();
    
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    BRepAlgoAPI_Defeaturing algo;
    algo.SetRunParallel(true);
    algo.SetToFillHistory(true);
    algo.SetShape(inputSeerShape->getRootOCCTShape());
    
    for (const auto &p : ps)
    {
      resolver.resolve(p);
      auto rShapes = resolver.getShapes();
      for (const auto &rs : rShapes)
      {
        if (rs.ShapeType() == TopAbs_FACE)
          algo.AddFaceToRemove(rs);
      }
    }
    
    algo.Build();
    if (!algo.IsDone())
    {
      std::ostringstream stream;
      algo.DumpErrors(stream);
      throw std::runtime_error(stream.str());
    }
    if (algo.HasWarnings())
    {
      std::ostringstream stream;
      algo.DumpWarnings(stream);
      throw std::runtime_error(stream.str());
    }
    
    ShapeCheck check(algo.Shape());
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    stow->sShape.setOCCTShape(algo.Shape(), getId());
    stow->sShape.shapeMatch(*inputSeerShape);
    stow->sShape.uniqueTypeMatch(*inputSeerShape);
    stow->sShape.modifiedMatch(algo, *inputSeerShape);
    stow->sShape.outerWireMatch(*inputSeerShape);
    stow->sShape.derivedMatch();
    stow->sShape.dumpNils("remove faces feature");
    stow->sShape.dumpDuplicates("remove faces feature");
    stow->sShape.ensureNoNils();
    stow->sShape.ensureNoDuplicates();
    stow->sShape.ensureEvolve();
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in remove faces update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in remove faces update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in remove faces update. " << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Feature::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::rmfs::RemoveFaces srf
  (
    Base::serialOut()
    , stow->picks.serialOut()
    , stow->sShape.serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::rmfs::removeFaces(stream, srf, infoMap);
}

void Feature::serialRead(const prj::srl::rmfs::RemoveFaces &srf)
{
  Base::serialIn(srf.base());
  stow->picks.serialIn(srf.picks());
  stow->sShape.serialIn(srf.seerShape());
}
