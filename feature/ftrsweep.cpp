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

using namespace ftr;
using boost::uuids::uuid;

QIcon Sweep::icon;

SweepProfile::SweepProfile(): SweepProfile(Pick()){}
SweepProfile::SweepProfile(const Pick &pIn): SweepProfile(pIn, false, false){}
SweepProfile::SweepProfile(const Pick &pIn, bool contactIn, bool correctionIn)
: pick(pIn)
, contact(new prm::Parameter(QObject::tr("Contact"), contactIn))
, correction(new prm::Parameter(QObject::tr("Correction"), correctionIn))
, contactLabel(new lbr::PLabel(contact.get()))
, correctionLabel(new lbr::PLabel(correction.get()))
{
  contactLabel->showName = true;
  contactLabel->valueHasChanged();
  contactLabel->constantHasChanged();
  
  correctionLabel->showName = true;
  correctionLabel->valueHasChanged();
  correctionLabel->constantHasChanged();
}
SweepProfile::~SweepProfile(){}
SweepProfile::SweepProfile(const prj::srl::swps::SweepProfile &pfIn) : SweepProfile()
{
  pick.serialIn(pfIn.pick());
  contact->serialIn(pfIn.contact());
  correction->serialIn(pfIn.correction());
  contactLabel->serialIn(pfIn.contactLabel());
  correctionLabel->serialIn(pfIn.correctionLabel());
}
prj::srl::swps::SweepProfile SweepProfile::serialOut() const
{
  return prj::srl::swps::SweepProfile
  (
    pick.serialOut()
    , contact->serialOut()
    , correction->serialOut()
    , contactLabel->serialOut()
    , correctionLabel->serialOut()
  );
}

SweepAuxiliary::SweepAuxiliary(): SweepAuxiliary(Pick()){}
SweepAuxiliary::SweepAuxiliary(const Pick &pIn): SweepAuxiliary(pIn, false, 0){}
SweepAuxiliary::SweepAuxiliary(const Pick &pIn, bool ceIn, int contactIn)
: pick(pIn)
, curvilinearEquivalence(new prm::Parameter(QObject::tr("Curvelinear Equivalence"), ceIn))
, contactType(new prm::Parameter(QObject::tr("Contact Type"), contactIn))
, curvilinearEquivalenceLabel(new lbr::PLabel(curvilinearEquivalence.get()))
, contactTypeLabel(new lbr::PLabel(contactType.get()))
{
  QStringList tStrings =
  {
    QObject::tr("No Contact")
    , QObject::tr("Contact")
    , QObject::tr("Contact On Border")
  };
  contactType->setEnumeration(tStrings);
  prm::Constraint tConstraint = prm::Constraint::buildUnit();
  tConstraint.intervals.front().upper.value = 2.0;
  contactType->setConstraint(tConstraint);
  
  curvilinearEquivalenceLabel->showName = true;
  curvilinearEquivalenceLabel->valueHasChanged();
  curvilinearEquivalenceLabel->constantHasChanged();
  
  contactTypeLabel->showName = true;
  contactTypeLabel->valueHasChanged();
  contactTypeLabel->constantHasChanged();
}
SweepAuxiliary::SweepAuxiliary(const prj::srl::swps::SweepAuxiliary &auxIn)
{
  serialIn(auxIn);
}
SweepAuxiliary::~SweepAuxiliary(){}
prj::srl::swps::SweepAuxiliary ftr::SweepAuxiliary::serialOut() const
{
  return prj::srl::swps::SweepAuxiliary
  (
    pick.serialOut()
    , curvilinearEquivalence->serialOut()
    , contactType->serialOut()
    , curvilinearEquivalenceLabel->serialOut()
    , contactTypeLabel->serialOut()
  );
}
void SweepAuxiliary::serialIn(const prj::srl::swps::SweepAuxiliary &auxIn)
{
  pick.serialIn(auxIn.pick());
  curvilinearEquivalence->serialIn(auxIn.curvilinearEquivalence());
  contactType->serialIn(auxIn.contactType());
  curvilinearEquivalenceLabel->serialIn(auxIn.curvilinearEquivalenceLabel());
  contactTypeLabel->serialIn(auxIn.contactTypeLabel());
}

SweepBinormal::SweepBinormal()
: SweepBinormal(Picks())
{}

SweepBinormal::SweepBinormal(const osg::Vec3d &vIn)
: SweepBinormal(Picks(), vIn)
{}

SweepBinormal::SweepBinormal(const Picks &psIn)
: SweepBinormal(psIn, osg::Vec3d(0.0, 0.0, 1.0))
{}

SweepBinormal::SweepBinormal(const Picks &psIn, const osg::Vec3d &vIn)
: picks(psIn)
, binormal(new prm::Parameter(QObject::tr("Binormal"), vIn))
, binormalLabel(new lbr::PLabel(binormal.get()))
{}
SweepBinormal::~SweepBinormal() = default;
prj::srl::swps::SweepBinormal SweepBinormal::serialOut() const
{
  prj::srl::swps::SweepBinormal out
  (
    binormal->serialOut()
    , binormalLabel->serialOut()
  );
  for (const auto &p : picks)
    out.picks().push_back(p);
  
  return out;
}
void SweepBinormal::serialIn(const prj::srl::swps::SweepBinormal &bnIn)
{
  binormal->serialIn(bnIn.binormal());
  binormalLabel->serialIn(bnIn.binormalLabel());
  for (const auto &p : bnIn.picks())
    picks.emplace_back(p);
}

Sweep::Sweep():
Base()
, sShape(new ann::SeerShape())
, lawFunction(new ann::LawFunction())
, trihedron(new prm::Parameter(QObject::tr("Trihedron"), 0))
, transition(new prm::Parameter(QObject::tr("Transition"), 0))
, forceC1(new prm::Parameter(QObject::tr("Force C1"), false))
, solid(new prm::Parameter(QObject::tr("Solid"), false))
, useLaw(new prm::Parameter(QObject::tr("Use Law"), false))
, trihedronLabel(new lbr::PLabel(trihedron.get()))
, transitionLabel(new lbr::PLabel(transition.get()))
, forceC1Label(new lbr::PLabel(forceC1.get()))
, solidLabel(new lbr::PLabel(solid.get()))
, useLawLabel(new lbr::PLabel(useLaw.get()))
, auxiliarySwitch(new osg::Switch())
, binormalSwitch(new osg::Switch())
, lawSwitch(new osg::Switch())
, solidId(gu::createRandomId())
, shellId(gu::createRandomId())
, firstFaceId(gu::createRandomId())
, lastFaceId(gu::createRandomId())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionSweep.svg");
  
  name = QObject::tr("Sweep");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
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
  trihedron->setEnumeration(tStrings);
  trihedron->connectValue(std::bind(&Sweep::setModelDirty, this));
  prm::Constraint tConstraint = prm::Constraint::buildUnit();
  tConstraint.intervals.front().upper.value = 6.0;
  trihedron->setConstraint(tConstraint);
  parameters.push_back(trihedron.get());
  
  QStringList transStrings =
  {
    QObject::tr("Modified")
    , QObject::tr("Right")
    , QObject::tr("Round")
  };
  transition->setEnumeration(transStrings);
  transition->connectValue(std::bind(&Sweep::setModelDirty, this));
  prm::Constraint transitionConstraint = prm::Constraint::buildUnit();
  transitionConstraint.intervals.front().upper.value = 2.0;
  transition->setConstraint(transitionConstraint);
  parameters.push_back(transition.get());
  
  forceC1->connectValue(std::bind(&Sweep::setModelDirty, this));
  parameters.push_back(forceC1.get());
  
  solid->connectValue(std::bind(&Sweep::setModelDirty, this));
  parameters.push_back(solid.get());
  
  useLaw->connectValue(std::bind(&Sweep::setModelDirty, this));
  parameters.push_back(useLaw.get());
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
  annexes.insert(std::make_pair(ann::Type::LawFunction, lawFunction.get()));
  
  trihedronLabel->showName = true;
  trihedronLabel->valueHasChanged();
  trihedronLabel->constantHasChanged();
  overlaySwitch->addChild(trihedronLabel.get());
  
  transitionLabel->showName = true;
  transitionLabel->valueHasChanged();
  transitionLabel->constantHasChanged();
  overlaySwitch->addChild(transitionLabel.get());
  
  forceC1Label->showName = true;
  forceC1Label->valueHasChanged();
  forceC1Label->constantHasChanged();
  overlaySwitch->addChild(forceC1Label.get());
  
  solidLabel->showName = true;
  solidLabel->valueHasChanged();
  solidLabel->constantHasChanged();
  overlaySwitch->addChild(solidLabel.get());
  
  useLawLabel->showName = true;
  useLawLabel->valueHasChanged();
  useLawLabel->constantHasChanged();
  overlaySwitch->addChild(useLawLabel.get());
  
  overlaySwitch->addChild(auxiliarySwitch.get());
  overlaySwitch->addChild(binormalSwitch.get());
  overlaySwitch->addChild(lawSwitch.get());
}

Sweep::~Sweep(){}

void Sweep::setSweepData(const SweepData &in)
{
  //remove all current profile data.
  for (const auto &p : profiles)
  {
    auto rit = std::remove_if
    (
      parameters.begin()
      , parameters.end()
      , [&](prm::Parameter *pic){return pic == p.contact.get() || pic == p.correction.get();}
    );
    parameters.erase(rit, parameters.end());
    overlaySwitch->removeChild(p.contactLabel.get());
    overlaySwitch->removeChild(p.correctionLabel.get());
  }
  //add new profile data.
  profiles = in.profiles;
  for (const auto &p : profiles)
  {
    p.contact->connectValue(std::bind(&Sweep::setModelDirty, this));
    p.correction->connectValue(std::bind(&Sweep::setModelDirty, this));
    parameters.push_back(p.contact.get());
    parameters.push_back(p.correction.get());
    overlaySwitch->addChild(p.contactLabel.get());
    overlaySwitch->addChild(p.correctionLabel.get());
  }
  
  //remove current auxiliary data.
  auto rit = std::remove_if
  (
    parameters.begin()
    , parameters.end()
    , [&](prm::Parameter *pic){return pic == auxiliary.curvilinearEquivalence.get() || pic == auxiliary.contactType.get();}
  );
  parameters.erase(rit, parameters.end());
  auxiliarySwitch->removeChild(auxiliary.curvilinearEquivalenceLabel.get());
  auxiliarySwitch->removeChild(auxiliary.contactTypeLabel.get());
  //add new auxiliary data.
  auxiliary = in.auxiliary;
  auxiliary.curvilinearEquivalence->connectValue(std::bind(&Sweep::setModelDirty, this));
  auxiliary.contactType->connectValue(std::bind(&Sweep::setModelDirty, this));
  parameters.push_back(auxiliary.curvilinearEquivalence.get());
  parameters.push_back(auxiliary.contactType.get());
  auxiliarySwitch->addChild(auxiliary.curvilinearEquivalenceLabel.get());
  auxiliarySwitch->addChild(auxiliary.contactTypeLabel.get());
  
  //remove current binormal data
  rit = std::remove_if
  (
    parameters.begin()
    , parameters.end()
    , [&](prm::Parameter *pic){return pic == binormal.binormal.get();}
  );
  parameters.erase(rit, parameters.end());
  binormalSwitch->removeChild(binormal.binormalLabel.get());
  //add new binormal data
  binormal = in.binormal;
  binormal.binormal->connectValue(std::bind(&Sweep::setModelDirty, this));
  parameters.push_back(binormal.binormal.get());
  binormalSwitch->addChild(binormal.binormalLabel.get());
  
  spine = in.spine;
  support = in.support;
  trihedron->setValue(in.trihedron); //no quiet so labels update.
  transition->setValue(in.transition);
  forceC1->setValue(in.forceC1);
  solid->setValue(in.solid);
  useLaw->setValue(in.useLaw);
  
  setModelDirty();
}

SweepData Sweep::getSweepData() const
{
  SweepData out;
  
  out.spine = spine;
  out.profiles = profiles;
  out.auxiliary = auxiliary;
  out.support = support;
  out.trihedron = static_cast<int>(*trihedron);
  out.transition = static_cast<int>(*transition);
  out.forceC1 = static_cast<bool>(*forceC1);
  out.solid = static_cast<bool>(*solid);
  out.useLaw = static_cast<bool>(*useLaw);
  
  return out;
}

void Sweep::severLaw()
{
  const lwf::Cue &lfc = lawFunction->getCue();
  
  //remove law parameters from general parameter container.
  //note this doesn't delete the parameters and they may still be 'connected' to feature.
  //cue containers will need to be cleared to remove parameter to feature connection.
  std::vector<const prm::Parameter*> prms = lfc.getParameters();
  auto rit = parameters.end();
  for (const prm::Parameter* p : prms)
  {
    rit = std::remove_if
    (
      parameters.begin()
      , rit
      , [&](prm::Parameter *pic){return pic == p;}
    );
  }
  parameters.erase(rit, parameters.end());
  
  /*TODO
   * remove parameter labels from switch.
   * clear parameter labels container.
   * turn off law function visual switch.
   * 
   */
}

void Sweep::attachLaw()
{
  lwf::Cue &lfc = lawFunction->getCue();
  
  prm::Parameters prms = lfc.getParameters();
  for (prm::Parameter *p : prms)
  {
    parameters.push_back(p);
    parameters.back()->connectValue(std::bind(&Sweep::setModelDirty, this));
  }
}

void Sweep::regenerateLawViz()
{
  //save location for restore.
  osg::Matrixd lp = osg::Matrixd::identity();
  mdv::LawCallback *ocb = dynamic_cast<mdv::LawCallback*>(lawSwitch->getUpdateCallback());
  if (ocb)
    lp = ocb->getMatrix(lawSwitch.get());
  
  overlaySwitch->removeChild(lawSwitch.get());
  lawSwitch = mdv::generate(lawFunction->getCue());
  overlaySwitch->addChild(lawSwitch.get());
  
  //restore position.
  mdv::LawCallback *ncb = dynamic_cast<mdv::LawCallback*>(lawSwitch->getUpdateCallback());
  if (!ncb)
    return;
  ncb->setMatrix(lp, lawSwitch.get());
}

void Sweep::updateLawViz(double scale)
{
  mdv::LawCallback *cb = dynamic_cast<mdv::LawCallback*>(lawSwitch->getUpdateCallback());
  if (!cb)
    return;
  cb->setScale(scale);
  cb->setDirty(); //always make it dirty after an model update.
}

void Sweep::updateModel(const UpdatePayload &pIn)
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
      resolver.resolve(p);
      if (!resolver.getFeature() || !resolver.getSeerShape())
          throw std::runtime_error("invalid pick resolution");
      
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
    TopoDS_Wire spineShape = getWire(spine);
    globalBounder.add(spineShape);
    
//     occt::WireVector profileShapes;
    BRepFill_PipeShell sweeper(spineShape);
    switch (static_cast<int>(*trihedron))
    {
      case 0:
        sweeper.Set(false);
        break;
      case 1:
        sweeper.Set(true);
        break;
      case 2:
        sweeper.SetDiscrete();
        break;
      case 3:
      {
        //I wasn't able to change results by varying the parameter.
        //thinking it is just there to satisfy the 'Set' overload.
        sweeper.Set(gp_Ax2());
        break;
      }
      case 4:
      {
        //constant bi normal.
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
        
        binormalSwitch->setAllChildrenOff();
        if (binormal.picks.size() == 0)
        {
          binormalSwitch->setAllChildrenOn();
        }
        else if (binormal.picks.size() == 1)
        {
          const Base &f0 = getFeature(std::string(binormalTag) + "0");
          if (f0.hasAnnex(ann::Type::SeerShape))
          {
            TopoDS_Shape shape = getBinormalShape(f0, binormal.picks.at(0));
            if (shape.IsNull())
              throw std::runtime_error("TopoDS_Shape is null for binormal pick");
            auto axis = occt::gleanAxis(shape);
            if (!axis.second)
              throw std::runtime_error("couldn't glean axis for binormal");
            binormal.binormal->setValueQuiet(gu::toOsg(axis.first.Direction()));
          }
          else
          {
            if (f0.getType() == ftr::Type::DatumAxis)
            {
              const ftr::DatumAxis &da = static_cast<const ftr::DatumAxis&>(f0);
              binormal.binormal->setValueQuiet(da.getDirection());
            }
          }
        }
        else if (binormal.picks.size() == 2)
        {
          tls::Resolver resolver(pIn);
          auto getPoint = [&](int index) -> osg::Vec3d
          {
            resolver.resolve(binormal.picks.at(index));
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
            (!slc::isPointType(binormal.picks.at(0).selectionType))
            || (!slc::isPointType(binormal.picks.at(1).selectionType))
          )
            throw std::runtime_error("need points only for 2 binormal picks");
            
          osg::Vec3d pd = getPoint(0) - getPoint(1);
          if (!pd.valid() || pd.length() < std::numeric_limits<float>::epsilon())
            throw std::runtime_error("invalid direction for 2 binormal picks");
          pd.normalize();
          binormal.binormal->setValueQuiet(pd);
        }
        else
          throw std::runtime_error("unsupported number of binormal picks");
        sweeper.Set(gp_Dir(gu::toOcc(static_cast<osg::Vec3d>(*binormal.binormal))));
        break;
      }
      case 5:
      {
        tls::Resolver resolver(pIn);
        auto rShapes = resolver.getShapes(support);
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
      case 6:
      {
        TopoDS_Wire wire = getWire(auxiliary.pick);
        if (!wire.IsNull())
        {
          bool cle = static_cast<bool>(*auxiliary.curvilinearEquivalence);
          BRepFill_TypeOfContact toc = static_cast<BRepFill_TypeOfContact>(static_cast<int>(*auxiliary.contactType));
          sweeper.Set(wire, cle, toc);
          globalBounder.add(wire);
          occt::BoundingBox wb(wire);
          auxiliary.curvilinearEquivalenceLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(wb.getCorners().at(0))));
          auxiliary.contactTypeLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(wb.getCorners().at(6))));
        }
        else
          throw std::runtime_error("auxiliary spine wire is invalid");
        break;
      }
      default:
        sweeper.Set(false);
    }
    sweeper.SetTransition(static_cast<BRepFill_TransitionStyle>(static_cast<int>(*transition))); //default is BRepFill_Modified = 0
    occt::WireVector profileWires; //use for shape mapping
    std::vector<std::reference_wrapper<const ann::SeerShape>> seerShapeProfileRefs; //use for shape mapping
    for (const auto &p : profiles)
    {
      const Base &bf = getFeature(p.pick.tag);
      const ann::SeerShape &ss = getSeerShape(bf);
      TopoDS_Wire wire = getWire(p.pick);
      seerShapeProfileRefs.push_back(ss);
      
      //this is temp.
      if (profiles.size() == 1 && (static_cast<bool>(*useLaw)))
      {
        lwf::Cue &lfc = lawFunction->getCue();
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
        
        sweeper.SetLaw(wire, law, static_cast<bool>(*p.contact), static_cast<bool>(*p.correction));
        
        updateLawViz(globalBounder.getDiagonal());
        lawSwitch->setAllChildrenOn();
      }
      else
      {
        sweeper.Add(wire, static_cast<bool>(*p.contact), static_cast<bool>(*p.correction));
        lawSwitch->setAllChildrenOff();
      }
      
      globalBounder.add(wire);
      occt::BoundingBox bounder(wire);
      p.contactLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(bounder.getCorners().at(0))));
      p.correctionLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(bounder.getCorners().at(6))));
      profileWires.push_back(wire);
    }
    
    sweeper.SetForceApproxC1(static_cast<bool>(*forceC1));
    
    if (!sweeper.Build())
      throw std::runtime_error("Sweep operation failed");
    
    if (static_cast<bool>(*solid))
      sweeper.MakeSolid();
    
    ShapeCheck check(sweeper.Shape());
    if (!check.isValid())
      throw std::runtime_error("Shape check failed");
    
    sShape->setOCCTShape(sweeper.Shape(), getId());
    sShape->updateId(sShape->getRootOCCTShape(), this->getId());
    sShape->setRootShapeId(this->getId());
    if (!sShape->hasEvolveRecordOut(this->getId()))
      sShape->insertEvolve(gu::createNilId(), this->getId());
    
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
      auto mapIt = instanceMap.find(inputId);
      if (mapIt == instanceMap.end())
      {
        assert(index == 0);
        outputId = gu::createRandomId();
        std::vector<uuid> resultIds(1, outputId);
        instanceMap.insert(std::make_pair(inputId, resultIds));
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
        sShape->updateId(rexp.Current(), outputId);
        if (!sShape->hasEvolveRecordOut(outputId))
          sShape->insertEvolve(gu::createNilId(), outputId);
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
          sShape->updateId(ex.Current(), outputId);
          if (!sShape->hasEvolveRecordIn(inputId))
            sShape->insertEvolve(inputId, outputId);
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
      sShape->updateId(sweeper.FirstShape(), firstFaceId);
      if (!sShape->hasEvolveRecordOut(firstFaceId))
        sShape->insertEvolve(gu::createNilId(), firstFaceId);
      firstWire = BRepTools::OuterWire(TopoDS::Face(sweeper.FirstShape()));
      
      sShape->updateId(sweeper.LastShape(), lastFaceId);
      if (!sShape->hasEvolveRecordOut(lastFaceId))
        sShape->insertEvolve(gu::createNilId(), lastFaceId);
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
          auto parentFaces = sShape->useGetParentsOfType(we.Current(), TopAbs_FACE);
          for (const auto &pf : parentFaces)
          {
            uuid pfid = sShape->findId(pf);
            if (pfid.is_nil() || pfid == firstFaceId)
              continue;
            uuid freshId = getFirstLastId(pfid, mapId);
            sShape->updateId(we.Current(), freshId);
            if (!sShape->hasEvolveRecordOut(freshId))
              sShape->insertEvolve(gu::createNilId(), freshId);
            break;
          }
        }
      };
      
      goMapEdge(firstWire, firstShapeMap);
      goMapEdge(lastWire, lastShapeMap);
    }
    
    //now do outerwires from faces.
    auto faces = sShape->useGetChildrenOfType(sShape->getRootOCCTShape(), TopAbs_FACE);
    for (const auto &f : faces)
    {
      uuid fId = sShape->findId(f);
      if (fId.is_nil())
        continue;
      uuid owId = gu::createNilId();
      auto it = outerWireMap.find(fId);
      if (it != outerWireMap.end())
        owId = it->second;
      else
      {
        owId = gu::createRandomId();
        outerWireMap.insert(std::make_pair(fId, owId));
      }
      if (!sShape->hasEvolveRecordOut(owId))
        sShape->insertEvolve(gu::createNilId(), owId);
      const TopoDS_Wire &ow = BRepTools::OuterWire(TopoDS::Face(f));
      sShape->updateId(ow, owId);
    }
    
    sShape->derivedMatch();
    
    //update id for shell and solid. Note: we are not checking for multiple shells or solids.
    for (const auto &s : sShape->getAllNilShapes())
    {
      if (s.ShapeType() == TopAbs_SOLID)
        sShape->updateId(s, solidId);
      if (s.ShapeType() == TopAbs_SHELL)
        sShape->updateId(s, shellId);
    }
    if (!sShape->hasEvolveRecordOut(solidId))
      sShape->insertEvolve(gu::createNilId(), solidId);
    if (!sShape->hasEvolveRecordOut(shellId))
      sShape->insertEvolve(gu::createNilId(), shellId);
    
    //shapeMatch is useless. sweep never uses any of the original geometry.
    std::cout << std::endl;
//     sShape->dumpShapeIdContainer(std::cout);
    sShape->dumpNils("sweep feature");
    sShape->dumpDuplicates("sweep feature");
    sShape->ensureNoNils();
    sShape->ensureNoDuplicates();
    
    const std::vector<gp_Pnt> &cornerPoints = globalBounder.getCorners();
    transitionLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(cornerPoints.at(4))));
    trihedronLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(globalBounder.getCenter())));
    forceC1Label->setMatrix(osg::Matrixd::translate(gu::toOsg(cornerPoints.at(7))));
    solidLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(cornerPoints.at(5))));
    useLawLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(cornerPoints.at(6))));
    if (static_cast<int>(*trihedron) == 6)
      auxiliarySwitch->setAllChildrenOn();
    else
      auxiliarySwitch->setAllChildrenOff();
    
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

void Sweep::serialWrite(const boost::filesystem::path &dIn)
{
  osg::Matrixd m = osg::Matrixd::identity();
  double scale = 1.0;
  mdv::LawCallback *ocb = dynamic_cast<mdv::LawCallback*>(lawSwitch->getUpdateCallback());
  if (ocb)
  {
    m = ocb->getMatrix(lawSwitch.get());
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
    , sShape->serialOut()
    , lawFunction->getCue().serialOut()
    , trihedron->serialOut()
    , transition->serialOut()
    , forceC1->serialOut()
    , solid->serialOut()
    , useLaw->serialOut()
    , spine.serialOut()
    , auxiliary.serialOut()
    , support.serialOut()
    , binormal.serialOut()
    , trihedronLabel->serialOut()
    , transitionLabel->serialOut()
    , forceC1Label->serialOut()
    , solidLabel->serialOut()
    , useLawLabel->serialOut()
    , mOut
    , scale
    , gu::idToString(solidId)
    , gu::idToString(shellId)
    , gu::idToString(firstFaceId)
    , gu::idToString(lastFaceId)
  );
  
  for (const auto &p : profiles)
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
  
  so.outerWireMap() = serializeMap(outerWireMap);
  so.firstShapeMap() = serializeMap(firstShapeMap);
  so.lastShapeMap() = serializeMap(lastShapeMap);
  
  for (const auto &p : instanceMap)
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

void Sweep::serialRead(const prj::srl::swps::Sweep &so)
{
  Base::serialIn(so.base());
  sShape->serialIn(so.seerShape());
  lawFunction->setCue(lwf::Cue(so.lawFunction()));
  trihedron->serialIn(so.trihedron());
  transition->serialIn(so.transition());
  forceC1->serialIn(so.forceC1());
  solid->serialIn(so.solid());
  useLaw->serialIn(so.useLaw());
  spine.serialIn(so.spine());
  auxiliary.serialIn(so.auxiliary());
  support.serialIn(so.support());
  binormal.serialIn(so.binormal());
  trihedronLabel->serialIn(so.trihedronLabel());
  transitionLabel->serialIn(so.transitionLabel());
  forceC1Label->serialIn(so.forceC1Label());
  solidLabel->serialIn(so.solidLabel());
  useLawLabel->serialIn(so.useLawLabel());
  solidId = gu::stringToId(so.solidId());
  shellId = gu::stringToId(so.shellId());
  firstFaceId = gu::stringToId(so.firstFaceId());
  lastFaceId = gu::stringToId(so.lastFaceId());
  
  for (const auto &p : so.profiles())
    profiles.emplace_back(p);
  
  auto serializeMap = [](const prj::srl::spt::SeerShape::EvolveContainerSequence &container) -> std::map<uuid, uuid>
  {
    std::map<uuid, uuid> out;
    for (const auto &r : container)
      out.insert(std::make_pair(gu::stringToId(r.idIn()), gu::stringToId(r.idOut())));
    return out;
  };
  outerWireMap = serializeMap(so.outerWireMap());
  firstShapeMap = serializeMap(so.firstShapeMap());
  lastShapeMap = serializeMap(so.lastShapeMap());
  
  for (const auto &p : so.instanceMap())
  {
    std::vector<uuid> valuesIn;
    for (const auto &v : p.values())
      valuesIn.push_back(gu::stringToId(v));
    instanceMap.insert(std::make_pair(gu::stringToId(p.key()), valuesIn));
  }
  
  regenerateLawViz();
  const auto &mIn = so.lawVizMatrix();
  osg::Matrixd m
  (
    mIn.i0j0(), mIn.i0j1(), mIn.i0j2(), mIn.i0j3(),
    mIn.i1j0(), mIn.i1j1(), mIn.i1j2(), mIn.i1j3(),
    mIn.i2j0(), mIn.i2j1(), mIn.i2j2(), mIn.i2j3(),
    mIn.i3j0(), mIn.i3j1(), mIn.i3j2(), mIn.i3j3()
  );
  mdv::LawCallback *ocb = dynamic_cast<mdv::LawCallback*>(lawSwitch->getUpdateCallback());
  if (ocb)
  {
    ocb->setMatrix(m, lawSwitch.get());
    ocb->setScale(so.lawVizScale());
  }
  if (static_cast<bool>(*useLaw))
  {
    lawSwitch->setAllChildrenOn();
    attachLaw();
  }
  else
    lawSwitch->setAllChildrenOff();
}
