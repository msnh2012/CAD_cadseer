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

#include <BOPAlgo_Builder.hxx>

#include <osg/Switch>

#include "globalutilities.h"
#include "annex/annseershape.h"
#include "annex/annintersectionmapper.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "library/lbrplabel.h"
#include "tools/occtools.h"
#include "tools/featuretools.h"
#include "feature/ftrbooleanoperation.h"
#include "feature/ftrshapecheck.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinputtype.h"
#include "project/serial/generated/prjsrlblsboolean.h"
#include "feature/ftrboolean.h"

using namespace ftr::Boolean;
using boost::uuids::uuid;

QIcon Feature::icon = QIcon(":/resources/images/constructionBoolean.svg");

struct Feature::Stow
{
  Feature &feature;
  ann::SeerShape sShape;
  ann::IntersectionMapper iMapper;
  prm::Parameter booleanType{QObject::tr("Boolean Type"), 0, PrmTags::booleanType};
  prm::Parameter unify{QObject::tr("Unify"), true, PrmTags::unify};
  prm::Parameter picks{QObject::tr("Picks"), ftr::Picks(), PrmTags::picks};
  
  osg::ref_ptr<lbr::PLabel> unifyLabel = new lbr::PLabel(&unify);
  
  Stow(Feature &fIn)
  : feature(fIn)
  {
    QStringList tStrings =
    {
      QObject::tr("Intersect")
      , QObject::tr("Subtract")
      , QObject::tr("Union")
    };
    booleanType.setEnumeration(tStrings);
    booleanType.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&booleanType);
    
    unify.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&unify);
    
    picks.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&picks);
    
    feature.annexes.insert(std::make_pair(ann::Type::SeerShape, &sShape));
    feature.annexes.insert(std::make_pair(ann::Type::IntersectionMapper, &iMapper));
    
    feature.overlaySwitch->addChild(unifyLabel);
  }
};

Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  name = QObject::tr("Boolean");
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
    //we dont bail on resolution failures, because we want to set a fail state
    //before we try the boolean
    const ftr::Picks &picks = stow->picks.getPicks();
    occt::ShapeVector targetOCCTShapes;
    std::vector<tls::Resolver> targetResolvers;
    
    //1 feature input we will grab child solids
    if (picks.size() == 1 && slc::isObjectType(picks.front().selectionType))
    {
      targetResolvers.emplace_back(payloadIn);
      if (!targetResolvers.back().resolve(picks.front()))
      {
        targetResolvers.pop_back();
      }
      else
      {
        const auto &inputSS = *targetResolvers.back().getSeerShape();
        occt::ShapeVector inputShapes = inputSS.useGetChildrenOfType(inputSS.getRootOCCTShape(), TopAbs_SOLID);
        std::copy(inputShapes.begin(), inputShapes.end(), std::back_inserter(targetOCCTShapes));
      }
    }
    else
    {
      for (const auto &tp : picks)
      {
        targetResolvers.emplace_back(payloadIn);
        if (!targetResolvers.back().resolve(tp))
        {
          targetResolvers.pop_back();
          continue;
        }
        auto shapes = targetResolvers.back().getShapes();
        std::copy(shapes.begin(), shapes.end(), std::back_inserter(targetOCCTShapes));
      }
    }
    
    auto setFailureState = [&]()
    {
      TopoDS_Compound tc = occt::ShapeVectorCast(targetOCCTShapes);
      stow->sShape.setOCCTShape(tc, getId());
      BOPAlgo_Builder dummy;
      stow->iMapper.go(payloadIn, dummy, stow->sShape);
      for (const auto &it : targetResolvers)
        stow->sShape.shapeMatch(*it.getSeerShape());
      for (const auto &it : targetResolvers)
        stow->sShape.uniqueTypeMatch(*it.getSeerShape());
      for (const auto &it : targetResolvers)
        stow->sShape.outerWireMatch(*it.getSeerShape());
      stow->sShape.derivedMatch();
      stow->sShape.ensureNoNils(); //just in case
      stow->sShape.ensureNoDuplicates(); //just in case
      stow->sShape.ensureEvolve();
    };
    
    if (isSkipped())
    {
      setFailureState();
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    //one set of picks. first is target and rest are tools.
    occt::ShapeVector targets, tools;
    for (const auto &s : targetOCCTShapes)
    {
      if (targets.empty())
        targets.push_back(s);
      else
        tools.push_back(s);
    }
    if (targets.empty() || tools.empty())
    {
      setFailureState();
      throw std::runtime_error("Incorrect number of shapes");
    }
    
    occt::BoundingBox bbox(targetOCCTShapes);
    stow->unifyLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(bbox.getCenter())));
    
    BOPAlgo_Operation operation = BOPAlgo_Operation::BOPAlgo_UNKNOWN;
    switch (stow->booleanType.getInt())
    {
      case 0:
      {
        operation = BOPAlgo_COMMON;
        break;
      }
      case 1:
      {
        operation = BOPAlgo_CUT;
        break;
      }
      case 2:
      {
        operation = BOPAlgo_FUSE;
        break;
      }
      default:
      {
        setFailureState();
        throw std::runtime_error("Incorrect boolean operation");
      }
    }
    
    BooleanOperation fuser(targets, tools, operation);
    fuser.Build();
    if (!fuser.IsDone())
    {
      setFailureState();
      throw std::runtime_error("OCC fuse failed");
    }
    if (stow->unify.getBool())
      fuser.SimplifyResult();
    ShapeCheck check(fuser.Shape());
    if (!check.isValid())
    {
      setFailureState();
      throw std::runtime_error("shapeCheck failed");
    }
    
    stow->sShape.setOCCTShape(fuser.Shape(), getId());
    
    stow->iMapper.go(payloadIn, fuser.getBuilder(), stow->sShape);
    
    for (const auto &it : targetResolvers)
      stow->sShape.shapeMatch(*it.getSeerShape());
    for (const auto &it : targetResolvers)
      stow->sShape.uniqueTypeMatch(*it.getSeerShape());
    for (const auto &it : targetResolvers)
      stow->sShape.outerWireMatch(*it.getSeerShape());
    stow->sShape.derivedMatch();
    stow->sShape.dumpNils(getTypeString()); //only if there are shapes with nil ids.
    stow->sShape.dumpDuplicates(getTypeString());
    stow->sShape.ensureNoNils();
    stow->sShape.ensureNoDuplicates();
    stow->sShape.ensureEvolve();
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
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Feature::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::bls::Boolean so
  (
    Base::serialOut()
    , stow->booleanType.serialOut()
    , stow->unify.serialOut()
    , stow->picks.serialOut()
    , stow->sShape.serialOut()
    , stow->iMapper.serialOut()
    , stow->unifyLabel->serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::bls::boolean(stream, so, infoMap);
}

void Feature::serialRead(const prj::srl::bls::Boolean &so)
{
  Base::serialIn(so.base());
  stow->booleanType.serialIn(so.booleanType());
  stow->unify.serialIn(so.unify());
  stow->picks.serialIn(so.picks());
  stow->sShape.serialIn(so.seerShape());
  stow->iMapper.serialIn(so.intersectionMapper());
  stow->unifyLabel->serialIn(so.unifyLabel());
}
