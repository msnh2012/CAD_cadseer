/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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

#include <cassert>

#include <gp_Pln.hxx>
#include <BRepOffsetAPI_DraftAngle.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopTools_ListOfShape.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <Geom_Surface.hxx>

#include <osg/Switch>

#include "globalutilities.h"
#include "tools/idtools.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "feature/ftrshapecheck.h"
#include "library/lbrplabel.h"
#include "project/serial/generated/prjsrldrfsdraft.h"
#include "annex/annseershape.h"
#include "tools/featuretools.h"
#include "tools/occtools.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinputtype.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "feature/ftrdraft.h"

using boost::uuids::uuid;
using namespace ftr::Draft;

QIcon Feature::icon = QIcon(":/resources/images/constructionDraft.svg");

struct Feature::Stow
{
  Feature &feature;
  
  prm::Parameter neutralPick{QObject::tr("Neutral Pick"), ftr::Picks(), PrmTags::neutralPick};
  prm::Parameter targetPicks{QObject::tr("Target Picks"), ftr::Picks(), PrmTags::targetPicks};
  prm::Parameter angle{prm::Names::Angle, prf::manager().rootPtr->features().draft().get().angle(), prm::Tags::Angle};
  
  ann::SeerShape sShape;
  
  osg::ref_ptr<lbr::PLabel> angleLabel{new lbr::PLabel(&angle)};
  
  Stow() = delete;
  Stow(Feature &fIn)
  : feature(fIn)
  {
    neutralPick.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&neutralPick);
    
    targetPicks.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&targetPicks);
    
    angle.setConstraint(prm::Constraint::buildNonZeroAngle());
    angle.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&angle);
    
    feature.annexes.insert(std::make_pair(ann::Type::SeerShape, &sShape));
    
    feature.overlaySwitch->addChild(angleLabel.get());
  }
  
  void generatedMatch(BRepOffsetAPI_DraftAngle &dMaker, const ann::SeerShape &targetShapeIn)
  {
    using boost::uuids::uuid;
    std::vector<uuid> targetShapeIds = targetShapeIn.getAllShapeIds();
    
    for (const auto &cId : targetShapeIds)
    {
      const TopTools_ListOfShape &shapes = dMaker.Generated(targetShapeIn.findShape(cId));
      if (shapes.Extent() < 1)
        continue;
      assert(shapes.Extent() == 1); //want to know about a situation where we have more than 1.
      uuid freshId;
      if (sShape.hasEvolveRecordIn(cId))
        freshId = sShape.evolve(cId).front();
      else
      {
        freshId = gu::createRandomId();
        sShape.insertEvolve(cId, freshId);
      }
      sShape.updateId(shapes.First(), freshId);
    }
  }
};

Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  name = QObject::tr("Draft");
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
    std::vector<tls::Resolver> resolvedTargets;
    for (const auto &p: stow->targetPicks.getPicks())
    {
      resolvedTargets.emplace_back(payloadIn);
      if (!resolvedTargets.back().resolve(p))
      {
        std::ostringstream stream;
        stream << "Warning: Couldn't resolve target pick: " << gu::idToString(p.shapeHistory.getRootId()) << std::endl;
        lastUpdateLog += stream.str();
        resolvedTargets.pop_back();
      }
    }
    if (resolvedTargets.empty())
      throw std::runtime_error("No input shapes");
    const ann::SeerShape &targetSeerShape = *resolvedTargets.front().getSeerShape();
    
    auto setFailureState = [&]()
    {
      stow->sShape.setOCCTShape(targetSeerShape.getRootOCCTShape(), getId());
      stow->sShape.shapeMatch(targetSeerShape);
      stow->sShape.uniqueTypeMatch(targetSeerShape);
      stow->sShape.outerWireMatch(targetSeerShape);
      stow->sShape.derivedMatch();
      stow->sShape.ensureNoNils(); //just in case
      stow->sShape.ensureNoDuplicates(); //just in case
    };
    
    if (isSkipped())
    {
      setFailureState();
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    tls::Resolver resolvedNeutral(payloadIn);
    if (stow->neutralPick.getPicks().empty() || !resolvedNeutral.resolve(stow->neutralPick.getPicks().front()))
    {
      setFailureState();
      throw std::runtime_error("Couldn't resolve neutral plane pick");
    }
    
    //establish neutral plane
    std::optional<gp_Pln> nPlane;
    auto planeFromShape = [&](const TopoDS_Shape &shapeIn)
    {
      auto axis = occt::gleanAxis(shapeIn);
      if (!axis.second)
        return;
      nPlane = gp_Pln(axis.first.Location(), axis.first.Direction());
    };
    
    if (slc::isObjectType(resolvedNeutral.getPick().selectionType))
    {
      auto csysPrms = resolvedNeutral.getFeature()->getParameters(prm::Tags::CSys);
      if (!csysPrms.empty())
      {
        const auto &csys = csysPrms.front()->getMatrix();
        nPlane = gp_Pln(gp_Pnt(gu::toOcc(csys.getTrans()).XYZ()), gp_Dir(gu::toOcc(gu::getZVector(csys))));
      }
      else if (resolvedNeutral.getSeerShape())
      {
        auto shapes = resolvedNeutral.getSeerShape()->useGetNonCompoundChildren();
        if (!shapes.empty())
          planeFromShape(shapes.front());
      }
    }
    else
    {
      auto shapes = resolvedNeutral.getShapes();
      if (!shapes.empty())
        planeFromShape(shapes.front());
    }
    if (!nPlane)
    {
      setFailureState();
      throw std::runtime_error("Couldn't derive neutral plane");
    }
    
    //not sure exactly how the pull direction affects the operation, but it does.
    //rotation of planar faces happens around intersection of neutral plane and draft face.
    //multiple target faces and 1 neutral plane.
    //not going to expose pull direction yet. Not sure how useful it is.
    //we don't need a parameter for reverse direction, user can just use negative angle.
    
    BRepOffsetAPI_DraftAngle dMaker(targetSeerShape.getRootOCCTShape());

    double localAngle = osg::DegreesToRadians(stow->angle.getDouble());
    gp_Dir direction = nPlane->Axis().Direction();
    
    bool labelDone = false; //set label position to first pick.
    for (const auto &rt : resolvedTargets)
    {
      for (const auto &s : rt.getShapes())
      {
        if (s.ShapeType() != TopAbs_FACE)
        {
          std::ostringstream s; s << "Warning: Skipping non face shape." << std::endl;
          lastUpdateLog += s.str();
          continue;
        }
        const TopoDS_Face &face = TopoDS::Face(s);
        dMaker.Add(face, direction, localAngle, *nPlane);
        if (!dMaker.AddDone())
        {
          std::ostringstream s; s << "Warning: Draft failed adding face. Removing" << std::endl;
          lastUpdateLog += s.str();
          dMaker.Remove(face);
        }
        if (!labelDone)
        {
          labelDone = true;
          stow->angleLabel->setMatrix(osg::Matrixd::translate(rt.getPick().getPoint(face)));
        }
      }
    }
    
    dMaker.Build();
    if (!dMaker.IsDone())
      throw std::runtime_error("draft maker failed");
    ShapeCheck check(dMaker.Shape());
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    stow->sShape.setOCCTShape(dMaker.Shape(), getId());
    stow->sShape.shapeMatch(targetSeerShape);
    stow->sShape.uniqueTypeMatch(targetSeerShape);
    stow->sShape.modifiedMatch(dMaker, targetSeerShape);
    stow->generatedMatch(dMaker, targetSeerShape);
    stow->sShape.outerWireMatch(targetSeerShape);
    stow->sShape.derivedMatch();
    stow->sShape.dumpNils("draft feature"); //only if there are shapes with nil ids.
    stow->sShape.dumpDuplicates("draft feature");
    stow->sShape.ensureNoNils();
    stow->sShape.ensureNoDuplicates();
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in draft update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in draft update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in draft update." << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Feature::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::drfs::Draft draftOut
  (
    Base::serialOut(),
    stow->neutralPick.serialOut(),
    stow->targetPicks.serialOut(),
    stow->angle.serialOut(),
    stow->sShape.serialOut(),
    stow->angleLabel->serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::drfs::draft(stream, draftOut, infoMap);
}

void Feature::serialRead(const prj::srl::drfs::Draft &sDraftIn)
{
  Base::serialIn(sDraftIn.base());
  stow->neutralPick.serialIn(sDraftIn.neutralPick());
  stow->targetPicks.serialIn(sDraftIn.targetPicks());
  stow->angle.serialIn(sDraftIn.angle());
  stow->sShape.serialIn(sDraftIn.seerShape());
  stow->angleLabel->serialIn(sDraftIn.plabel());
}
