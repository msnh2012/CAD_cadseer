/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2022 Thomas S. Anderson blobfish.at.gmx.com
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

#include <functional>
#include <boost/filesystem/path.hpp>

#include <TopoDS.hxx>
#include <BRep_Tool.hxx>

#include <osg/Switch>
#include <osg/AutoTransform>

#include "globalutilities.h"
#include "annex/annseershape.h"
#include "library/lbrplabel.h"
#include "library/lbripgroup.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "tools/occtools.h"
#include "tools/featuretools.h"
#include "feature/ftrshapecheck.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrpick.h"
#include "project/serial/generated/prjsrllwsplawspine.h"
#include "law/lwfadapter.h"
#include "law/lwfvessel.h"
#include "feature/ftrlawspine.h"

using boost::uuids::uuid;
using namespace ftr::LawSpine;
QIcon Feature::icon = QIcon(":/resources/images/constructionLawSpine.svg");

struct Feature::Stow
{
  Feature &feature;
  prm::Parameter spinePick{QObject::tr("Spine Pick"), ftr::Picks(), Tags::spinePick};
  prm::Observer observer{std::bind(&Feature::setModelDirty, &feature)};
  lwf::Vessel vessel;
  
  ann::SeerShape sShape;
  
  Stow(Feature &fIn)
  : feature(fIn)
  {
    feature.parameters.push_back(&spinePick);
    
    //we want to visualize the spine, but the shape isn't to be used externally.
    //so we don't add the SeerShape to the list of annexes.
    feature.annexes.insert(std::make_pair(ann::Type::SeerShape, &sShape));
    
    feature.overlaySwitch->addChild(vessel.getOsgGroup());
  }
  
  void syncVessel()
  {
    for (const auto *p : vessel.getRemovedParameters())
      feature.removeParameter(p);
    for (auto *p : vessel.getNewParameters())
    {
      p->connect(observer);
      feature.parameters.push_back(p);
    }
    vessel.resetNew();
    vessel.resetRemoved();
  }
};

Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  name = QObject::tr("LawSpine");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
}

Feature::~Feature() = default;

void Feature::updateModel(const UpdatePayload &pIn)
{
  setFailure();
  lastUpdateLog.clear();
  stow->sShape.reset();
  prm::ObserverBlocker(stow->observer); //don't recurse.
  try
  {
    //empty failure state.
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    stow->syncVessel();
    
    const auto &spinePicks = stow->spinePick.getPicks();
    if (spinePicks.size() != 1) throw std::runtime_error("Invalid spine pick");
    tls::Resolver pr(pIn);
    if (!pr.resolve(spinePicks.front())) throw std::runtime_error("invalid pick resolution");
    
    lwf::Adapter adapter(*pr.getSeerShape(), pr.getResolvedIds().front());
    if (!adapter.isValid()) throw std::runtime_error("Adapter is invalid");
    
    if (stow->vessel.setPeriodic(adapter.isPeriodic()) && !stow->vessel.isValid())
      throw std::runtime_error("Invalid vessel after setting periodicity");
    double r0, r1;
    std::tie(r0, r1) = adapter.getRange();
    stow->vessel.setRange(r0, r1);
    
    bool shouldForceVessel = false;
    
    /* this is an unusual pick resolution. We are not creating graph edge tags for
     * the vessel point picks, as that becomes a nightmare to sync through the gui.
     * So we manually resolve ids here assuming the same feature as spinePick, which
     * it always should be.
     */
    for (auto pp : stow->vessel.getPickPairs())
    {
      auto assign = [&](double newValue)
      {
        if (std::fabs(newValue - pp.front()->getDouble()) > std::numeric_limits<float>::epsilon())
        {
          pp.front()->setValue(newValue);
          shouldForceVessel = true;
        }
      };
      
      assert(pp.size() == 2);
      const auto &picks = pp.back()->getPicks();
      if (picks.size() != 1) throw std::runtime_error("No picks for parameter pick pair");
      
      auto resolvedIds = pIn.shapeHistory.resolveHistories(picks.front().shapeHistory, pr.getFeature()->getId());
      if (resolvedIds.size() != 1) throw std::runtime_error("Failed resolving point pick");
      auto rShape = pr.getSeerShape()->getOCCTShape(resolvedIds.front());
      if (rShape.ShapeType() == TopAbs_VERTEX)
      {
        assert(picks.front().selectionType == slc::Type::EndPoint || picks.front().selectionType == slc::Type::StartPoint);
        auto newPrm = adapter.toSpineParameter(TopoDS::Vertex(rShape));
        if (!newPrm) throw std::runtime_error("Adapter failed to get spine parameter from start vertex");
        assign(*newPrm);
      }
      else if (rShape.ShapeType() == TopAbs_EDGE)
      {
        assert(slc::isParameterPointType(picks.front().selectionType));
        auto newPrm = adapter.toSpineParameter(TopoDS::Edge(rShape), occt::deNormalize(TopoDS::Edge(rShape), picks.front().u));
        if (!newPrm) throw std::runtime_error("Adapter failed to get spine parameter from edge and parameter");
        assign(*newPrm);
      }
      else
        throw std::runtime_error("Unrecognized pick pair point type");
    }
    
    if (shouldForceVessel || !stow->vessel.isValid()) stow->vessel.forceReprocess();
    if (!stow->vessel.isValid()) throw std::runtime_error(stow->vessel.getValidationText().toStdString());
    
    for (auto gp : stow->vessel.getGridPairs())
      gp.second->asAutoTransform()->setPosition(gu::toOsg(adapter.location(gp.first->getDouble())));
    
    //I am not sure about add the actual shape?
    auto spine = adapter.buildWire();
    ShapeCheck check(spine);
    if (!check.isValid()) throw std::runtime_error("shapeCheck failed");
    stow->sShape.setOCCTShape(spine, getId());
    stow->sShape.shapeMatch(*pr.getSeerShape());
    
    //temp testing/debugging
//     opencascade::handle<Law_Function> test = stow->vessel.buildLawFunction();
    stow->vessel.outputGraphviz("vesselGraph.dot");
    
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

void Feature::initVessel()
{
  stow->vessel.resetConstant(0.0, 1.0, 1.0);
}

lwf::Vessel& Feature::getVessel(){return stow->vessel;}

void Feature::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::lwsp::LawSpine so
  (
    Base::serialOut()
    , stow->sShape.serialOut()
    , stow->spinePick.serialOut()
    , stow->vessel.serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::lwsp::lawspine(stream, so, infoMap);
}

void Feature::serialRead(const prj::srl::lwsp::LawSpine &so)
{
  Base::serialIn(so.base());
  stow->sShape.serialIn(so.seerShape());
  stow->spinePick.serialIn(so.spinePick());
  stow->vessel.serialIn(so.vessel());
  
  stow->syncVessel();
}
