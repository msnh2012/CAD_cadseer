/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#include <Precision.hxx>
#include <ShapeUpgrade_UnifySameDomain.hxx>
#include <ShapeFix_ShapeTolerance.hxx>
#include <ShapeFix.hxx>
#include <BRepTools_History.hxx>

#include <osg/Switch>

#include "tools/idtools.h"
#include "tools/occtools.h"
#include "globalutilities.h"
#include "project/serial/generated/prjsrlrfnsrefine.h"
#include "annex/annseershape.h"
#include "parameter/prmparameter.h"
#include "library/lbrplabel.h"
#include "library/lbrplabelgrid.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrshapecheck.h"
#include "feature/ftrrefine.h"

using namespace ftr::Refine;
using boost::uuids::uuid;
QIcon Feature::icon = QIcon(":/resources/images/constructionRefine.svg");

struct Feature::Stow
{
  Feature &feature;
  ann::SeerShape sShape;
  std::map<boost::uuids::uuid, boost::uuids::uuid> shapeMap;
  
  prm::Parameter unifyFaces{QObject::tr("Unify Faces"), true, PrmTags::unifyFaces};
  prm::Parameter unifyEdges{QObject::tr("Unify Edges"), true, PrmTags::unifyEdges};
  prm::Parameter concatBSplines{QObject::tr("Concatenate BSplines"), false, PrmTags::concatBSplines};
  prm::Parameter tightenFaces{QObject::tr("Tighten Face Tolerances"), false, PrmTags::tightenFaces};
  prm::Parameter tightenEdges{QObject::tr("Tighten Edge Tolerances"), false, PrmTags::tightenEdges};
  prm::Parameter tightenVertices{QObject::tr("Tighten Vertex Tolerances"), false, PrmTags::tightenVertices};
  prm::Parameter sameParameter{QObject::tr("Run Same Parameter"), false, PrmTags::sameParameter};
  
  osg::ref_ptr<lbr::PLabel> unifyFacesLabel{new lbr::PLabel(&unifyFaces)};
  osg::ref_ptr<lbr::PLabel> unifyEdgesLabel{new lbr::PLabel(&unifyEdges)};
  osg::ref_ptr<lbr::PLabel> concatBSplinesLabel{new lbr::PLabel(&concatBSplines)};
  osg::ref_ptr<lbr::PLabel> tightenFacesLabel{new lbr::PLabel(&tightenFaces)};
  osg::ref_ptr<lbr::PLabel> tightenEdgesLabel{new lbr::PLabel(&tightenEdges)};
  osg::ref_ptr<lbr::PLabel> tightenVerticesLabel{new lbr::PLabel(&tightenVertices)};
  osg::ref_ptr<lbr::PLabel> sameParameterLabel{new lbr::PLabel(&sameParameter)};
  
  osg::ref_ptr<osg::AutoTransform> grid = lbr::buildGrid(1);
  
  prm::Observer dirtyObserver{std::bind(&Feature::setModelDirty, &feature)};
  prm::Observer syncObserver{std::bind(&Stow::prmSync, this)};
  
  Stow(Feature &fIn)
  : feature(fIn)
  {
    feature.annexes.insert(std::make_pair(ann::Type::SeerShape, &sShape));
    auto connectStore = [&](prm::Parameter &p)
    {
      p.connect(dirtyObserver);
      feature.parameters.push_back(&p);
    };
    connectStore(unifyFaces);
    connectStore(unifyEdges);
    connectStore(concatBSplines);
    connectStore(tightenFaces);
    connectStore(tightenEdges);
    connectStore(tightenVertices);
    connectStore(sameParameter);
    
    tightenEdges.connect(syncObserver);
    tightenVertices.connect(syncObserver);
    
    grid->addChild(unifyFacesLabel.get());
    grid->addChild(unifyEdgesLabel.get());
    grid->addChild(tightenFacesLabel.get());
    grid->addChild(tightenEdgesLabel.get());
    grid->addChild(tightenVerticesLabel.get());
    grid->addChild(sameParameterLabel.get());
    grid->addChild(concatBSplinesLabel.get());
    feature.overlaySwitch->addChild(grid.get());
  }
  
  void prmSync()
  {
    // same parameter must run if we tighten edges or vertices.
    if (tightenEdges.getBool() || tightenVertices.getBool())
    {
      prm::ObserverBlocker blocker(dirtyObserver);
      sameParameter.setValue(true);
    }
  }
  
  void historyMatch(const BRepTools_History &hIn , const ann::SeerShape &tIn)
  {
    std::vector<uuid> tIds = tIn.getAllShapeIds();
    
    for (const auto &ctId : tIds) //current target id
    {
      const TopoDS_Shape &cs = tIn.findShape(ctId);
      
      //not getting anything from generated
      //modified appears to be a one to one mapping of faces and edges
      
      const TopTools_ListOfShape &modified = hIn.Modified(cs);
      for (const auto &ms : modified)
      {
        assert(sShape.hasShape(ms));
        if (sShape.findId(ms).is_nil())
        {
          std::map<uuid, uuid>::iterator mapItFace;
          bool dummy;
          std::tie(mapItFace, dummy) = shapeMap.insert(std::make_pair(ctId, gu::createRandomId()));
          sShape.updateId(ms, mapItFace->second);
        }
        sShape.insertEvolve(ctId, sShape.findId(ms));
      }
      if (hIn.IsRemoved(cs)) //appears to remove only edges and vertices.
        sShape.insertEvolve(ctId, gu::createNilId());
    }
  }
};

Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  name = QObject::tr("Refine");
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
    std::vector<const Base*> tfs = payloadIn.getFeatures(ftr::InputType::target);
    if (tfs.size() != 1)
      throw std::runtime_error("wrong number of 'target' inputs");
    if(!tfs.front()->hasAnnex(ann::Type::SeerShape))
      throw std::runtime_error("no seer shape for target");
    const ann::SeerShape &tss = tfs.front()->getAnnex<ann::SeerShape>(); //target seer shape.
    if (tss.isNull())
      throw std::runtime_error("target seer shape is null");
    const TopoDS_Shape &shape = tss.getRootOCCTShape();
    
    occt::BoundingBox bb(shape);
    stow->grid->setPosition(gu::toOsg(bb.getCenter()));
    
    auto setFailureState = [&]()
    {
      stow->sShape.setOCCTShape(tss.getRootOCCTShape(), getId());
      stow->sShape.shapeMatch(tss);
      stow->sShape.uniqueTypeMatch(tss);
      stow->sShape.outerWireMatch(tss);
      stow->sShape.derivedMatch();
      stow->sShape.ensureNoNils(); //just in case
      stow->sShape.ensureNoDuplicates(); //just in case
      stow->sShape.ensureEvolve();
    };
    
    auto bail = [&](std::string_view m)
    {
      setFailureState();
      throw std::runtime_error(m.data());
    };
    
    if (isSkipped())
    {
      setSuccess();
      bail("feature is skipped");
    }
    
    ShapeUpgrade_UnifySameDomain usd
    (
      shape
      , stow->unifyEdges.getBool()
      , stow->unifyFaces.getBool()
      , stow->concatBSplines.getBool()
    );
    usd.Build();
    auto out = usd.Shape();
    ShapeCheck check(out);
    if (!check.isValid()) bail("shapeCheck failed");
    
    /* shape fix routines modify the input shapes. altering tolerances here will also
     * alter the tolerances of the input shape. Refine being an 'alter' feature makes
     * this condition acceptable ... somewhat. Confusion can/will arise when tightening
     * the tolerances and then turning them off will not revert back to original tolerances
     * because the incoming feature tolerances have been changed. The incoming feature
     * will have to be updated to get back the loose tolerances.
     */
    ShapeFix_ShapeTolerance tighten;
    if (stow->tightenFaces.getBool()) tighten.SetTolerance(out, Precision::Confusion(), TopAbs_FACE);
    if (stow->tightenEdges.getBool()) tighten.SetTolerance(out, Precision::Confusion(), TopAbs_EDGE);
    if (stow->tightenVertices.getBool()) tighten.SetTolerance(out, Precision::Confusion(), TopAbs_VERTEX);
    if (stow->sameParameter.getBool())
      if (!ShapeFix::SameParameter(out, false)) bail("same parameter failed");
    
    stow->sShape.setOCCTShape(out, getId());
    stow->sShape.shapeMatch(tss);
    stow->sShape.uniqueTypeMatch(tss);
    
    stow->historyMatch(*(usd.History()), tss);
    
    stow->sShape.outerWireMatch(tss);
    stow->sShape.derivedMatch();
    stow->sShape.dumpNils("refine feature");
    stow->sShape.dumpDuplicates("refine feature");
    stow->sShape.ensureNoNils();
    stow->sShape.ensureNoDuplicates();
    stow->sShape.ensureEvolve();
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in refine update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in refine update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in refine update. " << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Feature::serialWrite(const boost::filesystem::path &dIn)
{
  auto gpo = stow->grid->getPosition();
  
  prj::srl::rfns::Refine ro
  (
    Base::serialOut()
    , stow->sShape.serialOut()
    , stow->unifyFaces.serialOut()
    , stow->unifyEdges.serialOut()
    , stow->concatBSplines.serialOut()
    , stow->tightenFaces.serialOut()
    , stow->tightenEdges.serialOut()
    , stow->tightenVertices.serialOut()
    , stow->sameParameter.serialOut()
    
    , stow->unifyFacesLabel->serialOut()
    , stow->unifyEdgesLabel->serialOut()
    , stow->concatBSplinesLabel->serialOut()
    , stow->tightenFacesLabel->serialOut()
    , stow->tightenEdgesLabel->serialOut()
    , stow->tightenVerticesLabel->serialOut()
    , stow->sameParameterLabel->serialOut()
    , prj::srl::spt::Vec3d(gpo.x(), gpo.y(), gpo.z())
  );
  
  for (const auto &p : stow->shapeMap)
    ro.shapeMap().push_back(prj::srl::spt::EvolveRecord(gu::idToString(p.first), gu::idToString(p.second)));
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::rfns::refine(stream, ro, infoMap);
}

void Feature::serialRead(const prj::srl::rfns::Refine &rfIn)
{
  Base::serialIn(rfIn.base());
  stow->sShape.serialIn(rfIn.sShape());
  
  stow->unifyFaces.serialIn(rfIn.unifyFaces());
  stow->unifyEdges.serialIn(rfIn.unifyEdges());
  stow->concatBSplines.serialIn(rfIn.concatBSplines());
  stow->tightenFaces.serialIn(rfIn.tightenFaces());
  stow->tightenEdges.serialIn(rfIn.tightenEdges());
  stow->tightenVertices.serialIn(rfIn.tightenVertices());
  stow->sameParameter.serialIn(rfIn.sameParameter());
  
  stow->unifyFacesLabel->serialIn(rfIn.unifyFacesLabel());
  stow->unifyEdgesLabel->serialIn(rfIn.unifyEdgesLabel());
  stow->concatBSplinesLabel->serialIn(rfIn.concatBSplinesLabel());
  stow->tightenFacesLabel->serialIn(rfIn.tightenFacesLabel());
  stow->tightenEdgesLabel->serialIn(rfIn.tightenEdgesLabel());
  stow->tightenVerticesLabel->serialIn(rfIn.tightenVerticesLabel());
  stow->sameParameterLabel->serialIn(rfIn.sameParameterLabel());
  stow->grid->setPosition(osg::Vec3d(rfIn.gridLocation().x(), rfIn.gridLocation().y(), rfIn.gridLocation().z()));
  dynamic_cast<lbr::PLabelGridCallback*>(stow->grid->getUpdateCallback())->setDirtyParameters();
  
  stow->shapeMap.clear();
  for (const auto &r : rfIn.shapeMap())
    stow->shapeMap.insert(std::make_pair(gu::stringToId(r.idIn()), gu::stringToId(r.idOut())));
}
