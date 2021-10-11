/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2019 Thomas S. Anderson blobfish.at.gmx.com
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
#include <boost/optional.hpp>

#include <TopoDS.hxx>
#include <BRepFill_PipeShell.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <BRepTools.hxx>
#include <TopExp_Explorer.hxx>
#include <gp_Ax2.hxx>
#include <Law_Constant.hxx>
#include <Law_Linear.hxx>
#include <Law_Interpol.hxx>
#include <Law_S.hxx>
#include <Law_Composite.hxx>
#include <TColStd_HArray1OfReal.hxx> //ProjectCurveOnSurface needs this? occt 7.3
#include <ShapeConstruct_ProjectCurveOnSurface.hxx>
#include <BRep_Tool.hxx>
#include <BRep_Builder.hxx>

#include <osg/Switch>

#include "globalutilities.h"
#include "annex/annseershape.h"
#include "annex/annlawfunction.h"
#include "law/lwfcue.h"
#include "library/lbrplabel.h"
#include "modelviz/mdvlawfunction.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "tools/occtools.h"
#include "tools/featuretools.h"
#include "tools/idtools.h"
#include "feature/ftrshapecheck.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrdatumaxis.h"
#include "project/serial/generated/prjsrlswpssweep.h"
#include "feature/ftrsweep.h"

using namespace ftr::Sweep;
using boost::uuids::uuid;

QIcon Feature::icon = QIcon(":/resources/images/constructionSweep.svg");

Profile::Profile()
: pick(QObject::tr("Profile"), ftr::Picks(), PrmTags::profile)
, contact(QObject::tr("Contact"), false, PrmTags::none)
, correction(QObject::tr("Correction"), false, PrmTags::none)
, contactLabel(new lbr::PLabel(&contact))
, correctionLabel(new lbr::PLabel(&correction))
{}

Profile::Profile(const prj::srl::swps::SweepProfile &pfIn) : Profile()
{
  pick.serialIn(pfIn.pick());
  contact.serialIn(pfIn.contact());
  correction.serialIn(pfIn.correction());
  contactLabel->serialIn(pfIn.contactLabel());
  correctionLabel->serialIn(pfIn.correctionLabel());
}

Profile::~Profile() noexcept = default;

prj::srl::swps::SweepProfile Profile::serialOut() const
{
  return prj::srl::swps::SweepProfile
  (
    pick.serialOut()
    , contact.serialOut()
    , correction.serialOut()
    , contactLabel->serialOut()
    , correctionLabel->serialOut()
  );
}

Auxiliary::Auxiliary()
: pick(QObject::tr("Auxiliary Picks"), ftr::Picks(), PrmTags::auxPick)
, curvilinearEquivalence(QObject::tr("Curvelinear Equivalence"), false, PrmTags::auxCurvilinear)
, contactType(QObject::tr("Contact Type"), 0, PrmTags::auxContact)
, curvilinearEquivalenceLabel(new lbr::PLabel(&curvilinearEquivalence))
, contactTypeLabel(new lbr::PLabel(&contactType))
{
  QStringList tStrings =
  {
    QObject::tr("No Contact")
    , QObject::tr("Contact")
    , QObject::tr("Contact On Border")
  };
  contactType.setEnumeration(tStrings);
  contactTypeLabel->refresh();
}

Auxiliary::Auxiliary(const prj::srl::swps::SweepAuxiliary &auxIn)
:Auxiliary()
{
  serialIn(auxIn);
}

Auxiliary::~Auxiliary() noexcept = default;

void Auxiliary::setActive(bool stateIn)
{
  pick.setActive(stateIn);
  curvilinearEquivalence.setActive(stateIn);
  contactType.setActive(stateIn);
}

prj::srl::swps::SweepAuxiliary Auxiliary::serialOut() const
{
  return prj::srl::swps::SweepAuxiliary
  (
    pick.serialOut()
    , curvilinearEquivalence.serialOut()
    , contactType.serialOut()
    , curvilinearEquivalenceLabel->serialOut()
    , contactTypeLabel->serialOut()
  );
}

void Auxiliary::serialIn(const prj::srl::swps::SweepAuxiliary &auxIn)
{
  pick.serialIn(auxIn.pick());
  curvilinearEquivalence.serialIn(auxIn.curvilinearEquivalence());
  contactType.serialIn(auxIn.contactType());
  curvilinearEquivalenceLabel->serialIn(auxIn.curvilinearEquivalenceLabel());
  contactTypeLabel->serialIn(auxIn.contactTypeLabel());
}

Binormal::Binormal()
: picks(QObject::tr("Binormal Picks"), ftr::Picks(), PrmTags::biPicks)
, vector(QObject::tr("Vector"), osg::Vec3d(), PrmTags::biVector)
, vectorLabel(new lbr::PLabel(&vector))
{}

Binormal::~Binormal() noexcept = default;

prj::srl::swps::SweepBinormal Binormal::serialOut() const
{
  prj::srl::swps::SweepBinormal out
  (
    picks.serialOut()
    , vector.serialOut()
    , vectorLabel->serialOut()
  );
  return out;
}
void Binormal::serialIn(const prj::srl::swps::SweepBinormal &bnIn)
{
  picks.serialIn(bnIn.picks());
  vector.serialIn(bnIn.binormal());
  vectorLabel->serialIn(bnIn.binormalLabel());
}

void Binormal::setActive(bool stateIn)
{
  picks.setActive(stateIn);
  if (stateIn && picks.getPicks().empty())
    vector.setActive(true);
  else
    vector.setActive(false);
}

struct Feature::Stow
{
  Feature &feature;
  
  prm::Parameter spine{QObject::tr("Spine Pick"), ftr::Picks(), PrmTags::spine};
  prm::Parameter support{QObject::tr("Support Pick"), ftr::Picks(), PrmTags::support};
  prm::Parameter trihedron{QObject::tr("Trihedron"), 0, PrmTags::trihedron};
  prm::Parameter transition{QObject::tr("Transition"), 0, PrmTags::transition};
  prm::Parameter forceC1{QObject::tr("Force C1"), false, PrmTags::forcec1};
  prm::Parameter solid{QObject::tr("Solid"), false, PrmTags::solid};
  prm::Parameter useLaw{QObject::tr("Use Law"), false, PrmTags::useLaw};
  prm::Observer prmObserver{std::bind(&Feature::setModelDirty, &feature)};
  
  Profiles profiles;
  Auxiliary auxiliary;
  Binormal binormal;
  
  ann::SeerShape sShape;
  ann::LawFunction lawFunction;
  
  osg::ref_ptr<lbr::PLabel> trihedronLabel{new lbr::PLabel(&trihedron)};
  osg::ref_ptr<lbr::PLabel> transitionLabel{new lbr::PLabel(&transition)};
  osg::ref_ptr<lbr::PLabel> forceC1Label{new lbr::PLabel(&forceC1)};
  osg::ref_ptr<lbr::PLabel> solidLabel{new lbr::PLabel(&solid)};
  osg::ref_ptr<lbr::PLabel> useLawLabel{new lbr::PLabel(&useLaw)};
  
  osg::ref_ptr<osg::Switch> auxiliarySwitch{new osg::Switch()};
  osg::ref_ptr<osg::Switch> binormalSwitch{new osg::Switch()};
  osg::ref_ptr<osg::Switch> lawSwitch{new osg::Switch()};
  
  uuid solidId{gu::createRandomId()};
  uuid shellId{gu::createRandomId()};
  uuid firstFaceId{gu::createRandomId()};
  uuid lastFaceId{gu::createRandomId()};
  std::map<uuid, uuid> outerWireMap; //!< map face id to wire id
  typedef std::map<uuid, std::vector<uuid>> InstanceMap;
  InstanceMap instanceMap; //for profile vertices and edges -> edges and face.
  std::map<uuid, uuid> firstShapeMap; //faceId to edgeId for first shapes.
  std::map<uuid, uuid> lastShapeMap; //faceId to edgeId for last shapes.
  
  Stow() = delete;
  Stow(Feature &fIn)
  : feature(fIn)
  {
    spine.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&spine);
    
    support.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&support);
    
    QStringList tStrings =
    {
      QObject::tr("Corrected Frenet")
      , QObject::tr("Frenet")
      , QObject::tr("Discrete")
      , QObject::tr("Fixed")
      , QObject::tr("Constant Binormal")
      , QObject::tr("Support")
      , QObject::tr("Auxiliary")
    };
    trihedron.setEnumeration(tStrings);
    trihedron.connectValue(std::bind(&Feature::setModelDirty, &feature));
    trihedron.connectValue(std::bind(&Stow::prmActiveSync, this));
    feature.parameters.push_back(&trihedron);
    trihedronLabel->refresh();
    
    QStringList transStrings =
    {
      QObject::tr("Modified")
      , QObject::tr("Right")
      , QObject::tr("Round")
    };
    transition.setEnumeration(transStrings);
    transition.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&transition);
    transitionLabel->refresh();
    
    forceC1.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&forceC1);
    
    solid.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&solid);
    
    useLaw.connectValue(std::bind(&Feature::setModelDirty, &feature));
    useLaw.connectValue(std::bind(&Stow::prmActiveSync, this));
    feature.parameters.push_back(&useLaw);
    
    auxiliary.pick.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&auxiliary.pick);
    
    auxiliary.curvilinearEquivalence.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&auxiliary.curvilinearEquivalence);
    
    auxiliary.contactType.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&auxiliary.contactType);
    
    binormal.picks.connectValue(std::bind(&Feature::setModelDirty, &feature));
    binormal.picks.connectValue(std::bind(&Stow::prmActiveSync, this));
    feature.parameters.push_back(&binormal.picks);
    
    binormal.vector.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&binormal.vector);
    
    feature.annexes.insert(std::make_pair(ann::Type::SeerShape, &sShape));
    feature.annexes.insert(std::make_pair(ann::Type::LawFunction, &lawFunction));
    
    feature.overlaySwitch->addChild(trihedronLabel.get());
    feature.overlaySwitch->addChild(transitionLabel.get());
    feature.overlaySwitch->addChild(forceC1Label.get());
    feature.overlaySwitch->addChild(solidLabel.get());
    feature.overlaySwitch->addChild(useLawLabel.get());
    feature.overlaySwitch->addChild(auxiliarySwitch.get());
    feature.overlaySwitch->addChild(binormalSwitch.get());
    feature.overlaySwitch->addChild(lawSwitch.get());
    
    attachLaw(); //we always keep a law attached even if we aren't using.
    prmActiveSync();
  }
  
  void prmActiveSync()
  {
    int triType = trihedron.getInt();
    switch (triType)
    {
      case 4: //binormal
      {
        support.setActive(false);
        auxiliary.setActive(false);
        binormal.setActive(true);
        break;
      }
      case 5: //support
      {
        support.setActive(true);
        auxiliary.setActive(false);
        binormal.setActive(false);
        break;
      }
      case 6: //auxiliary
      {
        support.setActive(false);
        auxiliary.setActive(true);
        binormal.setActive(false);
        break;
      }
      default:
      {
        support.setActive(false);
        auxiliary.setActive(false);
        binormal.setActive(false);
        break;
      }
    }
    
    /* we still have to disable law parameters even though they are underneath
     * the switch. If we don't then the command view will display them when
     * not applicable.
     */
    lwf::Cue &lfc = lawFunction.getCue(); //no const because we are changing active state.
    if (useLaw.getBool() && lfc.type != lwf::Type::none)
    {
      lawSwitch->setAllChildrenOn();
      for (auto *p : lfc.getParameters())
        p->setActive(true);
    }
    else
    {
      for (auto *p : lfc.getParameters())
        p->setActive(false);
      lawSwitch->setAllChildrenOff();
    }
    
    if (profiles.size() == 1)
      useLaw.setActive(true);
    else
      useLaw.setActive(false);
  }
  
  void attachLaw()
  {
    lwf::Cue &lfc = lawFunction.getCue();
    prm::Parameters prms = lfc.getParameters();
    for (prm::Parameter *p : prms)
    {
      feature.parameters.push_back(p);
      feature.parameters.back()->connectValue(std::bind(&Feature::setModelDirty, &feature));
    }
    regenerateLawViz();
  }
  
  void severLaw() //should always be followed by an attachLaw call.
  {
    feature.overlaySwitch->removeChild(lawSwitch.get());
    lawSwitch.release(); //plabels should be deleted and no longer referencing parameters
    
    lwf::Cue &lfc = lawFunction.getCue();
    for (const prm::Parameter* p : lfc.getParameters())
      feature.removeParameter(p);
    //We don't really want to set to constant.
    //this just deletes all parameters that might have been connected to the feature.
    lfc.setConstant(1.0);
  }
  
  void regenerateLawViz()
  {
    //save location for restore.
    osg::Matrixd lp = osg::Matrixd::identity();
    if (lawSwitch)
    {
      mdv::LawCallback *ocb = dynamic_cast<mdv::LawCallback*>(lawSwitch->getUpdateCallback());
      if (ocb)
        lp = ocb->getMatrix(lawSwitch.get());
    }
    
//     feature.overlaySwitch->removeChild(lawSwitch.get());
    lawSwitch = mdv::generate(lawFunction.getCue());
    feature.overlaySwitch->addChild(lawSwitch.get());
    
    //restore position.
    mdv::LawCallback *ncb = dynamic_cast<mdv::LawCallback*>(lawSwitch->getUpdateCallback());
    if (!ncb)
      return;
    ncb->setMatrix(lp, lawSwitch.get());
  }
  
  void updateLawViz(double scale)
  {
    mdv::LawCallback *cb = dynamic_cast<mdv::LawCallback*>(lawSwitch->getUpdateCallback());
    if (!cb)
      return;
    cb->setScale(scale);
    cb->setDirty(); //always make it dirty after a model update.
  }
};

Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  name = QObject::tr("Sweep");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
}

Feature::~Feature() = default;

Profile& Feature::addProfile()
{
  stow->profiles.emplace_back();
  auto &p = stow->profiles.back();
  
  //not going to add these parameters to features general container. no point.
  p.pick.connectValue(std::bind(&Feature::setModelDirty, this));
  p.contact.connectValue(std::bind(&Feature::setModelDirty, this));
  p.correction.connectValue(std::bind(&Feature::setModelDirty, this));
  
  overlaySwitch->addChild(p.contactLabel);
  overlaySwitch->addChild(p.correctionLabel);
  
  stow->prmActiveSync();
  
  //adding profile itself shouldn't make the feature dirty.
  return stow->profiles.back();
}

void Feature::removeProfile(int index)
{
  assert (index >=0 && index < static_cast<int>(stow->profiles.size()));
  auto lit = stow->profiles.begin();
  for (int i = 0; i < index; ++i, ++lit);
  const auto &p = *lit;
  overlaySwitch->removeChild(p.contactLabel);
  overlaySwitch->removeChild(p.correctionLabel);
  stow->profiles.erase(lit);
  
  stow->prmActiveSync();
  
  setModelDirty();
}

Profiles& Feature::getProfiles() const
{
  return stow->profiles;
}

void Feature::setLaw(const lwf::Cue &cIn)
{
  stow->severLaw();
  stow->lawFunction.setCue(cIn);
  stow->attachLaw();
  setModelDirty();
}

void Feature::updateModel(const UpdatePayload &pIn)
{
  setFailure();
  lastUpdateLog.clear();
  stow->sShape.reset();
  try
  {
    prm::ObserverBlocker block(stow->prmObserver);
    
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    auto getFeature = [&](const std::string& tagIn) -> const Base&
    {
      std::vector<const Base*> tfs = pIn.getFeatures(tagIn);
      if (tfs.size() != 1)
        throw std::runtime_error("wrong number of parents");
      return *(tfs.front());
    };
    
    auto getSeerShape = [&](const Base &bfIn) -> const ann::SeerShape&
    {
      if (!bfIn.hasAnnex(ann::Type::SeerShape))
        throw std::runtime_error("parent doesn't have seer shape.");
      const ann::SeerShape &tss = bfIn.getAnnex<ann::SeerShape>();
      if (tss.isNull())
        throw std::runtime_error("target seer shape is null");
      return tss;
    };
    
    auto isValidShape = [](const TopoDS_Shape &sIn)
    {
      if (sIn.ShapeType() == TopAbs_WIRE || sIn.ShapeType() == TopAbs_EDGE)
        return true;
      return false;
    };
    
    auto ensureWire = [](const TopoDS_Shape &sIn) -> TopoDS_Wire
    {
      if (sIn.ShapeType() == TopAbs_WIRE)
        return TopoDS::Wire(sIn);
      if (sIn.ShapeType() == TopAbs_EDGE)
      {
        BRepBuilderAPI_MakeWire wm(TopoDS::Edge(sIn));
        if (!wm.IsDone())
          throw std::runtime_error("couldn't create wire from edge");
        return wm.Wire();
      }
      
      throw std::runtime_error("unsupported shape type in ensureWire");
    };
    
    auto getWire = [&](const Pick &p) -> TopoDS_Wire
    {
      boost::optional<TopoDS_Wire> out;
      tls::Resolver resolver(pIn);
      if (!resolver.resolve(p) || !resolver.getSeerShape())
        throw std::runtime_error("Invalid pick resolution");
      
      if (!slc::isShapeType(p.selectionType))
      {
        auto shapes = resolver.getSeerShape()->useGetNonCompoundChildren();
        if (shapes.empty())
          throw std::runtime_error("no shapes found");
        if (shapes.size() > 1)
        {
          std::ostringstream s; s << "Warning more than 1 shape found. Using first" << std::endl;
          lastUpdateLog += s.str();
        }
        if (!isValidShape(shapes.front()))
          throw std::runtime_error("invalid shape from feature");
        out = ensureWire(shapes.front());
      }
      else
      {
        auto rShapes = resolver.getShapes();
        if (rShapes.empty())
          throw std::runtime_error("no shapes from pick resolution");
        if (rShapes.size() > 1)
        {
          std::ostringstream s; s << "Warning more than 1 resolved shape. Using first" << std::endl;
          lastUpdateLog += s.str();
        }
        if (!isValidShape(rShapes.front()))
          throw std::runtime_error("invalid shape from resolve");
        out = ensureWire(rShapes.front());
      }
      
      if (!out)
        throw std::runtime_error("couldn't get wire");
      return out.get();
    };
    
    occt::BoundingBox globalBounder; //use this to place labels
    if (stow->spine.getPicks().empty())
      throw std::runtime_error("No Spine Picks");
    TopoDS_Wire spineShape = getWire(stow->spine.getPicks().front());
    globalBounder.add(spineShape);
    
//     occt::WireVector profileShapes;
    BRepFill_PipeShell sweeper(spineShape);
    switch (stow->trihedron.getInt())
    {
      case 0: // corrected frenet
        sweeper.Set(false);
        break;
      case 1: // frenet
        sweeper.Set(true);
        break;
      case 2: // discrete
        sweeper.SetDiscrete();
        break;
      case 3: //fixed
      {
        //I wasn't able to change results by varying the parameter.
        //thinking it is just there to satisfy the 'Set' overload.
        sweeper.Set(gp_Ax2());
        break;
      }
      case 4: //constant bi normal.
      {
        auto getBinormalShape = [&](const Base &feature, const Pick &pick) -> TopoDS_Shape
        {
          TopoDS_Shape out;
          assert(feature.hasAnnex(ann::Type::SeerShape));
          const ann::SeerShape &seerShape = getSeerShape(feature);
          if (pick.isEmpty())
          {
            auto shapes = seerShape.useGetNonCompoundChildren();
            if (shapes.empty())
              throw std::runtime_error("no non compound children for binormal pick");
            out = shapes.front();
          }
          else
          {
            tls::Resolver resolver(pIn);
            auto rShapes = resolver.getShapes(pick);
            if (rShapes.empty())
              throw std::runtime_error("couldn't resolve binormal 1st pick");
            if (rShapes.size() > 1)
            {
              std::ostringstream s; s << "Warning more than 1 resolved pick for binormal. Using first" << std::endl;
              lastUpdateLog += s.str();
            }
            out = rShapes.front();
          }
          return out;
        };
        
        const auto &bps = stow->binormal.picks.getPicks();
        stow->binormalSwitch->setAllChildrenOff();
        if (bps.size() == 0)
        {
          stow->binormalSwitch->setAllChildrenOn();
        }
        else if (bps.size() == 1)
        {
          const Base &f0 = getFeature(bps.front().tag);
          if (f0.hasAnnex(ann::Type::SeerShape))
          {
            TopoDS_Shape shape = getBinormalShape(f0, bps.front());
            if (shape.IsNull())
              throw std::runtime_error("TopoDS_Shape is null for binormal pick");
            auto axis = occt::gleanAxis(shape);
            if (!axis.second)
              throw std::runtime_error("couldn't glean axis for binormal");
            stow->binormal.vector.setValue(gu::toOsg(axis.first.Direction()));
          }
          else
          {
            auto directionPrms = f0.getParameters(prm::Tags::Direction);
            if (!directionPrms.empty())
              stow->binormal.vector.setValue(directionPrms.front()->getVector());
          }
        }
        else if (bps.size() == 2)
        {
          tls::Resolver resolver(pIn);
          auto getPoint = [&](int index) -> osg::Vec3d
          {
            resolver.resolve(bps.at(index));
            auto thePoints = resolver.getPoints();
            if (thePoints.empty())
              throw std::runtime_error("couldn't resolve inn binormal getPoint");
            if (thePoints.size() > 1)
            {
              std::ostringstream s; s << "Warning more than 1 resolved binormal point. Using first" << std::endl;
              lastUpdateLog += s.str();
            }
            return thePoints.front();
          };
          
          if
          (
            (!slc::isPointType(bps.at(0).selectionType))
            || (!slc::isPointType(bps.at(1).selectionType))
          )
            throw std::runtime_error("need points only for 2 binormal picks");
            
          osg::Vec3d pd = getPoint(0) - getPoint(1);
          if (!pd.valid() || pd.length() < std::numeric_limits<float>::epsilon())
            throw std::runtime_error("invalid direction for 2 binormal picks");
          pd.normalize();
          stow->binormal.vector.setValue(pd);
        }
        else
          throw std::runtime_error("unsupported number of binormal picks");
        sweeper.Set(gp_Dir(gu::toOcc(stow->binormal.vector.getVector())));
        break;
      }
      case 5: //support
      {
        tls::Resolver resolver(pIn);
        if (stow->support.getPicks().empty())
          throw std::runtime_error("No support picks");
        auto rShapes = resolver.getShapes(stow->support.getPicks().front());
        if (rShapes.empty())
          throw std::runtime_error("invalid support pick resolution");
        if (rShapes.size() > 1)
        {
          std::ostringstream s; s << "Warning more than 1 resolved support pick. Using first" << std::endl;
          lastUpdateLog += s.str();
        }
        const TopoDS_Shape &supportShape = rShapes.front();
        if (/*supportShape.ShapeType() != TopAbs_SHELL && */supportShape.ShapeType() != TopAbs_FACE)
          throw std::runtime_error("invalid shape type for support");
        const TopoDS_Face &supportFace = TopoDS::Face(supportShape);
        opencascade::handle<Geom_Surface> supportSurface = BRep_Tool::Surface(supportFace);
        if (supportSurface.IsNull())
          throw std::runtime_error("support surface is null");
        
        //put spine on support shape. For now we are assuming that we have
        //1 spine edge and and one support face.
        //projection is a PIA. we will want to move this somewhere else.
        ann::SeerShape temp;
        temp.setOCCTShape(spineShape, gu::createRandomId());
        occt::ShapeVector childShapes = temp.useGetChildrenOfType(temp.getRootOCCTShape(), TopAbs_EDGE);
        occt::EdgeVector childEdges = occt::ShapeVectorCast(childShapes);
        if (childEdges.empty())
          throw std::runtime_error("no spine child shapes for support");
        TopoDS_Edge spineEdge = childEdges.front();
        double p1, p2;
        opencascade::handle<Geom_Curve> spineCurve = BRep_Tool::Curve(spineEdge, p1, p2);
        if (spineCurve.IsNull())
          throw std::runtime_error("spine curve is null");
        opencascade::handle<Geom2d_Curve> pCurve;
        ShapeConstruct_ProjectCurveOnSurface projector;
        projector.SetSurface(supportSurface);
        if (!projector.Perform(spineCurve, p1, p2, pCurve))
          throw std::runtime_error("projection failed");
        BRep_Builder builder;
        builder.UpdateEdge(spineEdge, pCurve, supportFace, Precision::Confusion());
        sweeper.Set(supportFace);
        
        globalBounder.add(supportShape);
        break;
      }
      case 6: // auxiliary
      {
        const auto &aps = stow->auxiliary.pick.getPicks();
        if (aps.empty())
          throw std::runtime_error("No auxiliary pick");
        TopoDS_Wire wire = getWire(aps.front());
        if (!wire.IsNull())
        {
          bool cle = stow->auxiliary.curvilinearEquivalence.getBool();
          BRepFill_TypeOfContact toc = static_cast<BRepFill_TypeOfContact>(stow->auxiliary.contactType.getInt());
          sweeper.Set(wire, cle, toc);
          globalBounder.add(wire);
          occt::BoundingBox wb(wire);
          stow->auxiliary.curvilinearEquivalenceLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(wb.getCorners().at(0))));
          stow->auxiliary.contactTypeLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(wb.getCorners().at(6))));
        }
        else
          throw std::runtime_error("auxiliary spine wire is invalid");
        break;
      }
      default:
        sweeper.Set(false);
    }
    sweeper.SetTransition(static_cast<BRepFill_TransitionStyle>(stow->transition.getInt())); //default is BRepFill_Modified = 0
    occt::WireVector profileWires; //use for shape mapping
    std::vector<std::reference_wrapper<const ann::SeerShape>> seerShapeProfileRefs; //use for shape mapping
    for (const auto &p : stow->profiles)
    {
      if (p.pick.getPicks().empty())
        continue;
      
      const Base &bf = getFeature(p.pick.getPicks().front().tag);
      const ann::SeerShape &ss = getSeerShape(bf);
      TopoDS_Wire wire = getWire(p.pick.getPicks().front());
      seerShapeProfileRefs.push_back(ss);
      
      //this is temp.
      if (stow->profiles.size() == 1 && stow->useLaw.getBool() && stow->lawFunction.getCue().type != lwf::Type::none)
      {
        lwf::Cue &lfc = stow->lawFunction.getCue();
        lfc.smooth(); //need to smooth when changing parameters from PLabels.
        lfc.alignConstant(); //need to make sure boundary values consistent from PLabel edits.
        opencascade::handle<Law_Function> law = lfc.buildLawFunction();
        //must be a bug in how the bounds work in composite law.
        //Law_Composite::Prepare calculates the bounds and is called from several other public functions.
        //However, it is not called in Law_Composite::Bounds. I am getting and 'infinite boundary'
        //error from sweep and I am assuming it is calling 'Bounds' before prepare has been called.
        //by calling the Law_Composite::Value the bounds get calculated and now sweep will succeed
        //with composite law. Hence the following meaningless call.
        law->Value(0.5);
        
        sweeper.SetLaw(wire, law, p.contact.getBool(), p.correction.getBool());
        
        stow->updateLawViz(globalBounder.getDiagonal());
        stow->lawSwitch->setAllChildrenOn();
      }
      else
      {
        sweeper.Add(wire, p.contact.getBool(), p.correction.getBool());
        stow->lawSwitch->setAllChildrenOff();
      }
      
      globalBounder.add(wire);
      occt::BoundingBox bounder(wire);
      p.contactLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(bounder.getCorners().at(0))));
      p.correctionLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(bounder.getCorners().at(6))));
      profileWires.push_back(wire);
    }
    
    sweeper.SetForceApproxC1(stow->forceC1.getBool());
    
    if (!sweeper.Build())
      throw std::runtime_error("Sweep operation failed");
    
    if (stow->solid.getBool())
      sweeper.MakeSolid();
    
    ShapeCheck check(sweeper.Shape());
    if (!check.isValid())
      throw std::runtime_error("Shape check failed");
    
    stow->sShape.setOCCTShape(sweeper.Shape(), getId());
    stow->sShape.updateId(stow->sShape.getRootOCCTShape(), this->getId());
    stow->sShape.setRootShapeId(this->getId());
    if (!stow->sShape.hasEvolveRecordOut(this->getId()))
      stow->sShape.insertEvolve(gu::createNilId(), this->getId());
    
    /* Notes about sweeps generated method:
     * No entries for the spine. This might be a bug, because the occt code appears
     *   to have support for mapping between spine edges and resultant faces.
     * Edges in profiles create shells. Even if only 1 resultant face.
     * Vertices in profiles create wires. Even if only 1 resultant edge.
     * FYI: Face seems are at spine junctions not at profile transitions.
     * 
     * if solid is false first and last shape will be wires.
     * if solid is true first and last shape will be faces.
     * 
     * appears to be a bug in "Generated": I am getting the same wire
     * from 2 different vertices in the same input wire. HOW is this possible?
     * Note: input wire is NOT periodic.
     * This appears to be only with multiple profiles. The last profile 
     * in a series doesn't have this problem. I tested this for 1,2,3,4 profiles.
     * Also with 1 and 3 segment spine. Spine count didn't seem to matter.
     * Note the 4 profile had the 3rd and 4th profile accurate. Make last profile master?
     * Profiles don't have to selected by user in order. This means we can't count
     * on the last profile not having vertex generated bug. Sweep changes the order
     * of the profiles in order to make a valid surface, but this re-ordering isn't
     * reflected in the TopTools_ListOfShape returned from Sweep::Profiles. Bummer.
     * 
     * Sweep isn't using any shapes from the original profiles in the resultant shape.
     * Even with the most basic sweep. Everything must be derived from 'Generated'
     * Is 'FirstShape' orientated the same as the first passed in profile?
     *     meaning can we use order to map input edge to output edge.
     *     I guess not, 'FirstShape' doesn't even match the first profile passed in!
     * 
     * Sweeps with multiple profiles:
     * Sweep profiles have the same number of edges. Each edge of a profile is tied
     * to the corresponding edge in the other profiles. This set of edges are tied
     * to all generated faces regardless if outside the face. Each edge set may produce
     * multiple faces, so we need some kind of instance mapping. Problem is, the edges
     * of the edge set, have a different result face ordering in 'Generated' array.
     * So maybe we make the first profile 'master'? Ignore the edgeSet?
     * Note: The shell returned from generated(edge), is unique for each edge, even though
     * the shells contain the same faces. This is not true for Generated(vertex). I am guessing
     * this is a side effect of the ordering constraint of wires, that is not present for shells.
     * 
     * Considered making an id set of edges to match to generated faces. Problem is the generated
     * face orders of the shells are different for each edge. This would cause faces to 'swap'
     * ids with certain profile changes. Yuck! I would rather have a new id that causes child features
     * to fail instead of child features referencing the 'wrong' face. That might cause the child
     * feature to successfully update, but not in the way as intended by user.
     * 
     * Ok. Make first profile 'master' and tie its edge id to a vector of face ids.
     * all other profiles will be ignored as far as id mapping is concerned.
     * 
     * Use profile edges to generate instanced face ids.
     * Use profile vertices to generate instanced edge ids. These edges 'parallel' to the spine.
     * Only edges left at this point should be the first and last shape. We should be able to
     * get the last edge with a dedicated map from the face id.
     * have a first and last face id, if solid and first and last shape is a face.
     * Map for outer wire for each face.
     * have a shell id
     * have a solid id, if solid.
     * 
     */
    
    //following is used for inspecting results from Generated method.
    /*
    auto getSubShapes = [](const TopoDS_Shape &sIn, TopAbs_ShapeEnum typeIn) -> occt::ShapeVector
    {
      ann::SeerShape temp;
      temp.setOCCTShape(sIn, gu::createRandomId);
      return temp.useGetChildrenOfType(sIn, typeIn);
    };
    
    auto outputShapeInfo = [](const TopoDS_Shape &sIn)
    {
      std::cout << occt::getShapeTypeString(sIn) << "=" << occt::getShapeHash(sIn);
    };
    
    auto generatedShapes = [&](const TopoDS_Shape &sIn) -> occt::ShapeVector
    {
      TopTools_ListOfShape los;
      sweeper.Generated(sIn, los);
      return occt::ShapeVectorCast(los);
    };
    
    auto outputGenerated = [&](const TopoDS_Shape &sIn)
    {
      auto gs = generatedShapes(sIn);
      std::cout << " generated count: " << gs.size() << " ";
      for (const auto &s : gs)
        outputShapeInfo(s);
    };
    
    auto outputWire = [&](const TopoDS_Wire &wIn, const std::string &prefix)
    {
      int edgeIndex = 0;
      BRepTools_WireExplorer wexp(wIn);
      for (; wexp.More(); wexp.Next())
      {
        outputShapeInfo(wexp.Current());
        std::cout << " ";
        std::string fileName = "/home/tanderson/temp/";
        fileName += prefix + std::to_string(edgeIndex) + ".brep";
//         BRepTools::Write(wexp.Current(), fileName.c_str());
        edgeIndex++;
      }
    };
    
    std::cout << "profiles ordered by pick:" << std::endl;
    int wireIndex = 0;
    for (const auto &w : profileWires)
    {
      std::cout << "wire #" << wireIndex; outputGenerated(w); std::cout << std::endl;
      occt::ShapeVector edges = getSubShapes(w, TopAbs_EDGE);
      int edgeIndex = 0;
      for (const auto &e : edges)
      {
        std::cout << "    edge #" << edgeIndex; outputGenerated(e);
        std::cout << " -> ";
        occt::ShapeVector resultShell = generatedShapes(e);
        for (const auto &rs : resultShell)
        {
          occt::ShapeVector resultFaces = getSubShapes(rs, TopAbs_FACE);
          for (const auto &rf : resultFaces)
          {
            outputShapeInfo(rf);
            std::cout << "    ";
          }
          std::cout << "    ";
        }
        std::cout << std::endl;
        edgeIndex++;
      }
      
      occt::ShapeVector verts = getSubShapes(w, TopAbs_VERTEX);
      int vertIndex = 0;
      for (const auto &v : verts)
      {
        std::cout << "    "; outputShapeInfo(v); std::cout << "#" << vertIndex; outputGenerated(v);
        std::cout << " -> ";
        occt::ShapeVector resultWire = generatedShapes(v);
        for (const auto &rw : resultWire)
        {
          occt::ShapeVector resultEdges = getSubShapes(rw, TopAbs_EDGE);
          for (const auto &re : resultEdges )
          {
            outputShapeInfo(re);
            std::cout << "    ";
          }
        }
        std::cout << std::endl;
        vertIndex++;
      }
      
      wireIndex++;
    }
    
    std::cout << std::endl << "spine:"; outputGenerated(spineShape); std::cout << std::endl;
    occt::ShapeVector spineEdges = getSubShapes(spineShape, TopAbs_EDGE);
    int spineCount = 0;
    for (const auto &se : spineEdges)
    {
      std::cout << "spine edge #" << spineCount; outputGenerated(se); std::cout << std::endl;
      spineCount++;
    }
    
    const TopoDS_Shape &firstShape = sweeper.FirstShape();
    std::cout << "first shape: ";
    if (firstShape.IsNull())
      std::cout << "is null" << std::endl;
    else
      outputShapeInfo(firstShape);
    std::cout << std::endl;
    
    const TopoDS_Shape &lastShape = sweeper.LastShape();
    std::cout << "last shape: ";
    if (lastShape.IsNull())
      std::cout << "is null" << std::endl;
    else
      outputShapeInfo(lastShape);
    std::cout << std::endl;
    
    std::cout << std::endl << "first profile" << std::endl;
    outputWire(profileWires.front(), "firstProfile");
    std::cout << std::endl << "first shape" << std::endl;
    outputWire(TopoDS::Wire(firstShape), "firstShape");
    */
    
    //begin id mapping scheme.
    auto getInputId = [&](const TopoDS_Shape& sIn) -> uuid //maybe nil.
    {
      for (const auto &sspf : seerShapeProfileRefs)
        if (sspf.get().hasShape(sIn))
          return sspf.get().findId(sIn);
      return gu::createNilId();
    };
    
    auto getOutputId = [&](const uuid &inputId, std::size_t index) -> uuid
    {
      uuid outputId = gu::createNilId();
      auto mapIt = stow->instanceMap.find(inputId);
      if (mapIt == stow->instanceMap.end())
      {
        assert(index == 0);
        outputId = gu::createRandomId();
        std::vector<uuid> resultIds(1, outputId);
        stow->instanceMap.insert(std::make_pair(inputId, resultIds));
      }
      else
      {
        std::vector<uuid> &resultIds = mapIt->second;
        while (resultIds.size() <= index)
          resultIds.push_back(gu::createRandomId());
        outputId = resultIds.at(index);
      }
      assert(!outputId.is_nil());
      return outputId;
    };
    
    auto goIdWire = [&](const TopoDS_Shape &vertexIn, const TopoDS_Wire &wireIn)
    {
      std::size_t edgeIndex = 0;
      BRepTools_WireExplorer rexp(wireIn); //result explorer
      for (; rexp.More(); rexp.Next(), ++edgeIndex)
      {
        uuid inputId = getInputId(vertexIn);
        if (inputId.is_nil())
        {
          std::cout << "Warning: no input id for vertex in: " << BOOST_CURRENT_FUNCTION << std::endl;
          continue;
        }
        uuid outputId = getOutputId(inputId, edgeIndex);
        if (outputId.is_nil())
        {
          std::cout << "Warning: output id for vertex is nil in: " << BOOST_CURRENT_FUNCTION << std::endl;
          continue;
        }
        stow->sShape.updateId(rexp.Current(), outputId);
        if (!stow->sShape.hasEvolveRecordOut(outputId))
          stow->sShape.insertEvolve(gu::createNilId(), outputId);
      }
    };
    
    std::set<int> processed; //duplicate vertex generation bug.
    TopoDS_Shape duplicateVertex; //vertex that returns a duplicate generated wire.
    auto goVertex = [&](const TopoDS_Vertex &cv)
    {
      TopTools_ListOfShape gwl; //generated wire list
      sweeper.Generated(cv, gwl);
      if (!gwl.IsEmpty())
      {
        if (processed.count(occt::getShapeHash(gwl.First())) == 0)
        {
          processed.insert(occt::getShapeHash(gwl.First()));
          if (gwl.First().ShapeType() == TopAbs_WIRE)
            goIdWire(cv, TopoDS::Wire(gwl.First()));
        }
        else 
          duplicateVertex = cv;
      }
      else
        std::cout << "Warning: no generated for vertex in: " << BOOST_CURRENT_FUNCTION << std::endl;
    };
    
    const TopoDS_Wire &masterProfile = profileWires.front();
    BRepTools_WireExplorer we(masterProfile);
    for (; we.More(); we.Next())
    {
      const TopoDS_Edge &ce = we.Current();
      TopTools_ListOfShape gsl; //generated shell list
      sweeper.Generated(ce, gsl); //list of 1 shell.
      if (!gsl.IsEmpty())
      {
        std::size_t faceIndex = 0;
        for (TopExp_Explorer ex(gsl.First(), TopAbs_FACE); ex.More(); ex.Next(), ++faceIndex)
        {
          uuid inputId = getInputId(ce);
          if (inputId.is_nil())
          {
            std::cout << "Warning: no input id for edge in: " << BOOST_CURRENT_FUNCTION << std::endl;
            continue;
          }
          uuid outputId = getOutputId(inputId, faceIndex);
          if (outputId.is_nil())
          {
            std::cout << "Warning: output id for edge is nil in: " << BOOST_CURRENT_FUNCTION << std::endl;
            continue;
          }
          stow->sShape.updateId(ex.Current(), outputId);
          if (!stow->sShape.hasEvolveRecordIn(inputId))
            stow->sShape.insertEvolve(inputId, outputId);
        }
      }
      else
        std::cout << "Warning: no generated for edge in: " << BOOST_CURRENT_FUNCTION << std::endl;
      
      goVertex(we.CurrentVertex());
    }
    if (!BRep_Tool::IsClosed(masterProfile)) //do last vertex.
    {
      goVertex(we.CurrentVertex());
    }
    if (!duplicateVertex.IsNull())
    {
      //work around for generated bug. Appears to be working.
      if (profileWires.size() > 1)
      {
        occt::WireVector::iterator it = profileWires.begin();
        it++; //skip master profile
        for (; it != profileWires.end(); ++it)
        {
          TopExp_Explorer wexp(*it, TopAbs_VERTEX);
          for (; wexp.More(); wexp.Next())
          {
            TopTools_ListOfShape gwl; //generated wire list
            sweeper.Generated(wexp.Current(), gwl);
            if (!gwl.IsEmpty())
            {
              if (processed.count(occt::getShapeHash(gwl.First())) == 0)
              {
                processed.insert(occt::getShapeHash(gwl.First()));
                goIdWire(duplicateVertex, TopoDS::Wire(gwl.First()));
                goto getOut;
              }
            }
          }
        }
        getOut: ;
      }
      else
        std::cout << "Warning: can't fix duplicated vertex bug, not enough profiles, in: " << BOOST_CURRENT_FUNCTION << std::endl;
    }
    
    //first and last shapes.
    TopoDS_Wire firstWire;
    TopoDS_Wire lastWire;
    if (sweeper.FirstShape().ShapeType() == TopAbs_FACE && sweeper.LastShape().ShapeType() == TopAbs_FACE)
    {
      stow->sShape.updateId(sweeper.FirstShape(), stow->firstFaceId);
      if (!stow->sShape.hasEvolveRecordOut(stow->firstFaceId))
        stow->sShape.insertEvolve(gu::createNilId(), stow->firstFaceId);
      firstWire = BRepTools::OuterWire(TopoDS::Face(sweeper.FirstShape()));
      
      stow->sShape.updateId(sweeper.LastShape(), stow->lastFaceId);
      if (!stow->sShape.hasEvolveRecordOut(stow->lastFaceId))
        stow->sShape.insertEvolve(gu::createNilId(), stow->lastFaceId);
      lastWire = BRepTools::OuterWire(TopoDS::Face(sweeper.LastShape()));
    }
    else if (sweeper.FirstShape().ShapeType() == TopAbs_WIRE && sweeper.LastShape().ShapeType() == TopAbs_WIRE)
    {
      firstWire = TopoDS::Wire(sweeper.FirstShape());
      lastWire = TopoDS::Wire(sweeper.LastShape());
    }
    if ((!firstWire.IsNull()) && (!lastWire.IsNull()))
    {
      auto getFirstLastId = [&](const uuid &idIn, std::map<uuid, uuid> &mapId) -> uuid
      {
        auto it = mapId.find(idIn);
        if (it != mapId.end())
          return it->second;
        uuid freshId = gu::createRandomId();
        mapId.insert(std::make_pair(idIn, freshId));
        return freshId;
      };
      
      auto goMapEdge = [&](const TopoDS_Wire &wireIn, std::map<uuid, uuid> &mapId)
      {
        BRepTools_WireExplorer we(wireIn);
        for (; we.More(); we.Next())
        {
          auto parentFaces = stow->sShape.useGetParentsOfType(we.Current(), TopAbs_FACE);
          for (const auto &pf : parentFaces)
          {
            uuid pfid = stow->sShape.findId(pf);
            if (pfid.is_nil() || pfid == stow->firstFaceId)
              continue;
            uuid freshId = getFirstLastId(pfid, mapId);
            stow->sShape.updateId(we.Current(), freshId);
            if (!stow->sShape.hasEvolveRecordOut(freshId))
              stow->sShape.insertEvolve(gu::createNilId(), freshId);
            break;
          }
        }
      };
      
      goMapEdge(firstWire, stow->firstShapeMap);
      goMapEdge(lastWire, stow->lastShapeMap);
    }
    
    //now do outerwires from faces.
    auto faces = stow->sShape.useGetChildrenOfType(stow->sShape.getRootOCCTShape(), TopAbs_FACE);
    for (const auto &f : faces)
    {
      uuid fId = stow->sShape.findId(f);
      if (fId.is_nil())
        continue;
      uuid owId = gu::createNilId();
      auto it = stow->outerWireMap.find(fId);
      if (it != stow->outerWireMap.end())
        owId = it->second;
      else
      {
        owId = gu::createRandomId();
        stow->outerWireMap.insert(std::make_pair(fId, owId));
      }
      if (!stow->sShape.hasEvolveRecordOut(owId))
        stow->sShape.insertEvolve(gu::createNilId(), owId);
      const TopoDS_Wire &ow = BRepTools::OuterWire(TopoDS::Face(f));
      stow->sShape.updateId(ow, owId);
    }
    
    stow->sShape.derivedMatch();
    
    //update id for shell and solid. Note: we are not checking for multiple shells or solids.
    for (const auto &s : stow->sShape.getAllNilShapes())
    {
      if (s.ShapeType() == TopAbs_SOLID)
        stow->sShape.updateId(s, stow->solidId);
      if (s.ShapeType() == TopAbs_SHELL)
        stow->sShape.updateId(s, stow->shellId);
    }
    if (!stow->sShape.hasEvolveRecordOut(stow->solidId))
      stow->sShape.insertEvolve(gu::createNilId(), stow->solidId);
    if (!stow->sShape.hasEvolveRecordOut(stow->shellId))
      stow->sShape.insertEvolve(gu::createNilId(), stow->shellId);
    
    //shapeMatch is useless. sweep never uses any of the original geometry.
    std::cout << std::endl;
//     stow->sShape.dumpShapeIdContainer(std::cout);
    stow->sShape.dumpNils("sweep feature");
    stow->sShape.dumpDuplicates("sweep feature");
    stow->sShape.ensureNoNils();
    stow->sShape.ensureNoDuplicates();
    
    const std::vector<gp_Pnt> &cornerPoints = globalBounder.getCorners();
    stow->transitionLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(cornerPoints.at(4))));
    stow->trihedronLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(globalBounder.getCenter())));
    stow->forceC1Label->setMatrix(osg::Matrixd::translate(gu::toOsg(cornerPoints.at(7))));
    stow->solidLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(cornerPoints.at(5))));
    stow->useLawLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(cornerPoints.at(6))));
    if (stow->trihedron.getInt() == 6)
      stow->auxiliarySwitch->setAllChildrenOn();
    else
      stow->auxiliarySwitch->setAllChildrenOff();
    
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
  osg::Matrixd m = osg::Matrixd::identity();
  double scale = 1.0;
  mdv::LawCallback *ocb = dynamic_cast<mdv::LawCallback*>(stow->lawSwitch->getUpdateCallback());
  if (ocb)
  {
    m = ocb->getMatrix(stow->lawSwitch.get());
    scale = ocb->getScale();
  }
  prj::srl::spt::Matrixd mOut
  (
    m(0,0), m(0,1), m(0,2), m(0,3),
    m(1,0), m(1,1), m(1,2), m(1,3),
    m(2,0), m(2,1), m(2,2), m(2,3),
    m(3,0), m(3,1), m(3,2), m(3,3)
  );
  
  prj::srl::swps::Sweep so
  (
    Base::serialOut()
    , stow->sShape.serialOut()
    , stow->lawFunction.getCue().serialOut()
    , stow->trihedron.serialOut()
    , stow->transition.serialOut()
    , stow->forceC1.serialOut()
    , stow->solid.serialOut()
    , stow->useLaw.serialOut()
    , stow->spine.serialOut()
    , stow->auxiliary.serialOut()
    , stow->support.serialOut()
    , stow->binormal.serialOut()
    , stow->trihedronLabel->serialOut()
    , stow->transitionLabel->serialOut()
    , stow->forceC1Label->serialOut()
    , stow->solidLabel->serialOut()
    , stow->useLawLabel->serialOut()
    , mOut
    , scale
    , gu::idToString(stow->solidId)
    , gu::idToString(stow->shellId)
    , gu::idToString(stow->firstFaceId)
    , gu::idToString(stow->lastFaceId)
  );
  
  for (const auto &p : stow->profiles)
    so.profiles().push_back(p.serialOut());
  
  auto serializeMap = [](const std::map<uuid, uuid> &map) -> prj::srl::spt::SeerShape::EvolveContainerSequence
  {
    prj::srl::spt::SeerShape::EvolveContainerSequence out;
    for (const auto &p : map)
    {
      prj::srl::spt::SeerShape::EvolveContainerType r
      (
        gu::idToString(p.first),
        gu::idToString(p.second)
      );
      out.push_back(r);
    }
    return out;
  };
  
  so.outerWireMap() = serializeMap(stow->outerWireMap);
  so.firstShapeMap() = serializeMap(stow->firstShapeMap);
  so.lastShapeMap() = serializeMap(stow->lastShapeMap);
  
  for (const auto &p : stow->instanceMap)
  {
    prj::srl::swps::Instance instanceOut(gu::idToString(p.first));
    for (const auto &id : p.second)
      instanceOut.values().push_back(gu::idToString(id));
    so.instanceMap().push_back(instanceOut);
  }
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::swps::sweep(stream, so, infoMap);
}

void Feature::serialRead(const prj::srl::swps::Sweep &so)
{
  Base::serialIn(so.base());
  stow->sShape.serialIn(so.seerShape());
  stow->trihedron.serialIn(so.trihedron());
  stow->transition.serialIn(so.transition());
  stow->forceC1.serialIn(so.forceC1());
  stow->solid.serialIn(so.solid());
  stow->useLaw.serialIn(so.useLaw());
  stow->spine.serialIn(so.spine());
  stow->auxiliary.serialIn(so.auxiliary());
  stow->support.serialIn(so.support());
  stow->binormal.serialIn(so.binormal());
  stow->trihedronLabel->serialIn(so.trihedronLabel());
  stow->transitionLabel->serialIn(so.transitionLabel());
  stow->forceC1Label->serialIn(so.forceC1Label());
  stow->solidLabel->serialIn(so.solidLabel());
  stow->useLawLabel->serialIn(so.useLawLabel());
  stow->solidId = gu::stringToId(so.solidId());
  stow->shellId = gu::stringToId(so.shellId());
  stow->firstFaceId = gu::stringToId(so.firstFaceId());
  stow->lastFaceId = gu::stringToId(so.lastFaceId());
  
  for (const auto &p : so.profiles())
  {
    auto &pro = stow->profiles.emplace_back(p);
    
    pro.pick.connectValue(std::bind(&Feature::setModelDirty, this));
    pro.contact.connectValue(std::bind(&Feature::setModelDirty, this));
    pro.correction.connectValue(std::bind(&Feature::setModelDirty, this));
    
    overlaySwitch->addChild(pro.contactLabel);
    overlaySwitch->addChild(pro.correctionLabel);
  }
  
  auto serializeMap = [](const prj::srl::spt::SeerShape::EvolveContainerSequence &container) -> std::map<uuid, uuid>
  {
    std::map<uuid, uuid> out;
    for (const auto &r : container)
      out.insert(std::make_pair(gu::stringToId(r.idIn()), gu::stringToId(r.idOut())));
    return out;
  };
  stow->outerWireMap = serializeMap(so.outerWireMap());
  stow->firstShapeMap = serializeMap(so.firstShapeMap());
  stow->lastShapeMap = serializeMap(so.lastShapeMap());
  
  for (const auto &p : so.instanceMap())
  {
    std::vector<uuid> valuesIn;
    for (const auto &v : p.values())
      valuesIn.push_back(gu::stringToId(v));
    stow->instanceMap.insert(std::make_pair(gu::stringToId(p.key()), valuesIn));
  }
  
  stow->severLaw();
  stow->lawFunction.setCue(lwf::Cue(so.lawFunction()));
  stow->attachLaw();
  const auto &mIn = so.lawVizMatrix();
  osg::Matrixd m
  (
    mIn.i0j0(), mIn.i0j1(), mIn.i0j2(), mIn.i0j3(),
    mIn.i1j0(), mIn.i1j1(), mIn.i1j2(), mIn.i1j3(),
    mIn.i2j0(), mIn.i2j1(), mIn.i2j2(), mIn.i2j3(),
    mIn.i3j0(), mIn.i3j1(), mIn.i3j2(), mIn.i3j3()
  );
  mdv::LawCallback *ocb = dynamic_cast<mdv::LawCallback*>(stow->lawSwitch->getUpdateCallback());
  if (ocb)
  {
    ocb->setMatrix(m, stow->lawSwitch.get());
    ocb->setScale(so.lawVizScale());
  }
  stow->prmActiveSync();
}
