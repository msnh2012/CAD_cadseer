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
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinputtype.h"
#include "parameter/prmparameter.h"
#include "feature/ftrdraft.h"

using namespace ftr;
using boost::uuids::uuid;

QIcon Draft::icon;

Draft::Draft()
: Base()
, sShape(std::make_unique<ann::SeerShape>())
, angle(buildAngleParameter())
, label(new lbr::PLabel(angle.get()))
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionDraft.svg");
  
  name = QObject::tr("Draft");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
  
  angle->setConstraint(prm::Constraint::buildNonZeroAngle());
  angle->connectValue(std::bind(&Draft::setModelDirty, this));
  parameters.push_back(angle.get());
  
  overlaySwitch->addChild(label.get());
}

Draft::~Draft() //for forward declare with osg::ref_ptr
{
}

void Draft::setTargetPicks(const Picks &psIn)
{
  targetPicks = psIn;
  setModelDirty();
}

void Draft::setNeutralPick(const Pick &pIn)
{
  neutralPick = pIn;
  setModelDirty();
}

void Draft::setAngleParameter(std::shared_ptr<prm::Parameter> prmIn)
{
  assert(prmIn);
  
  //remove old
  removeParameter(angle.get());
  if (label)
    overlaySwitch->removeChild(label.get());
  
  //add new
  angle = prmIn;
  angle->connectValue(std::bind(&Draft::setModelDirty, this));
  parameters.push_back(angle.get());
  label = new lbr::PLabel(angle.get());
  overlaySwitch->addChild(label.get());
  
  setModelDirty();
}

std::shared_ptr<prm::Parameter> Draft::buildAngleParameter()
{
  auto out = std::make_shared<prm::Parameter>
  (
    prm::Names::Angle
    , prf::manager().rootPtr->features().draft().get().angle()
    , prm::Tags::Angle
  );
  out->setConstraint(prm::Constraint::buildNonZeroAngle());
  return out;
}

void Draft::updateModel(const UpdatePayload &payloadIn)
{
  setFailure();
  lastUpdateLog.clear();
  sShape->reset();
  try
  {
    std::vector<const Base*> features = payloadIn.getFeatures(std::string());
    if (features.size() != 1)
      throw std::runtime_error("no parent");
    if (!features.front()->hasAnnex(ann::Type::SeerShape))
      throw std::runtime_error("no seer shape in parent");
    const ann::SeerShape &targetSeerShape = features.front()->getAnnex<ann::SeerShape>();
    if (targetSeerShape.isNull())
      throw std::runtime_error("target seer shape is null");
    
    //setup new failure state.
    sShape->setOCCTShape(targetSeerShape.getRootOCCTShape(), getId());
    sShape->shapeMatch(targetSeerShape);
    sShape->uniqueTypeMatch(targetSeerShape);
    sShape->outerWireMatch(targetSeerShape);
    sShape->derivedMatch();
    sShape->ensureNoNils(); //just in case
    sShape->ensureNoDuplicates(); //just in case
    
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    //neutral plane might be outside of target, but we prevent this at feature creation at this time.
    
    //not sure exactly how the pull direction affects the operation, but it does.
    //rotation of planar faces happens around intersection of neutral plane and draft face.
    //multiple target faces and 1 neutral plane.
    //not going to expose pull direction yet. Not sure how useful it is.
    //of course we will have a reverse direction, but the direction will be derived from the neutral plane.
    
    BRepOffsetAPI_DraftAngle dMaker(targetSeerShape.getRootOCCTShape());
    
    tls::Resolver nResolver(payloadIn);
    if (!nResolver.resolve(neutralPick))
      throw std::runtime_error("Neutral pick resolution failed");
    auto rShapes = nResolver.getShapes();
    if (rShapes.empty())
      throw std::runtime_error("Neutral pick has no shapes");
    if (rShapes.size() > 1)
    {
      std::ostringstream s; s << "Warning: more than 1 neutral face, using first." << std::endl;
      lastUpdateLog += s.str();
    }
    gp_Pln plane = derivePlaneFromShape(rShapes.front());
    double localAngle = osg::DegreesToRadians(static_cast<double>(*angle));
    gp_Dir direction = plane.Axis().Direction();
    
    bool labelDone = false; //set label position to first pick.
    tls::Resolver tResolver(payloadIn);
    for (const auto &p : targetPicks)
    {
      if (!tResolver.resolve(p))
      {
        std::ostringstream s; s << "Warning: Skipping unresolved target face." << std::endl;
        lastUpdateLog += s.str();
        continue;
      }
      for (const auto &s : tResolver.getShapes())
      {
        if (s.ShapeType() != TopAbs_FACE)
        {
          std::ostringstream s; s << "Warning: Skipping non face shape." << std::endl;
          lastUpdateLog += s.str();
          continue;
        }
        const TopoDS_Face &face = TopoDS::Face(s);
        dMaker.Add(face, direction, localAngle, plane);
        if (!dMaker.AddDone())
        {
          std::ostringstream s; s << "Warning: Draft failed adding face. Removing" << std::endl;
          lastUpdateLog += s.str();
          dMaker.Remove(face);
        }
        if (!labelDone)
        {
          labelDone = true;
          label->setMatrix(osg::Matrixd::translate(p.getPoint(face)));
        }
      }
    }
    
    dMaker.Build();
    if (!dMaker.IsDone())
      throw std::runtime_error("draft maker failed");
    ShapeCheck check(dMaker.Shape());
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    sShape->setOCCTShape(dMaker.Shape(), getId());
    sShape->shapeMatch(targetSeerShape);
    sShape->uniqueTypeMatch(targetSeerShape);
    sShape->modifiedMatch(dMaker, targetSeerShape);
    generatedMatch(dMaker, targetSeerShape);
    sShape->outerWireMatch(targetSeerShape);
    sShape->derivedMatch();
    sShape->dumpNils("draft feature"); //only if there are shapes with nil ids.
    sShape->dumpDuplicates("draft feature");
    sShape->ensureNoNils();
    sShape->ensureNoDuplicates();
    
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

gp_Pln Draft::derivePlaneFromShape(const TopoDS_Shape &shapeIn)
{
  //just planar surfaces for now.
  BRepAdaptor_Surface sAdapt(TopoDS::Face(shapeIn));
  if (sAdapt.GetType() != GeomAbs_Plane)
    throw std::runtime_error("wrong surface type");
  return sAdapt.Plane();
}

void Draft::generatedMatch(BRepOffsetAPI_DraftAngle &dMaker, const ann::SeerShape &targetShapeIn)
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
    if (sShape->hasEvolveRecordIn(cId))
      freshId = sShape->evolve(cId).front();
    else
    {
      freshId = gu::createRandomId();
      sShape->insertEvolve(cId, freshId);
    }
    
    sShape->updateId(shapes.First(), freshId);
  }
}

void Draft::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::spt::Pick neutralPickOut = neutralPick.serialOut();
  prj::srl::drfs::Draft draftOut
  (
    Base::serialOut(),
    sShape->serialOut(),
    neutralPickOut,
    angle->serialOut(),
    label->serialOut()
  );
  for (const auto &p : targetPicks)
    draftOut.targetPicks().push_back(p);
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::drfs::draft(stream, draftOut, infoMap);
}

void Draft::serialRead(const prj::srl::drfs::Draft &sDraftIn)
{
  Base::serialIn(sDraftIn.base());
  sShape->serialIn(sDraftIn.seerShape());
  neutralPick.serialIn(sDraftIn.neutralPick());
  angle->serialIn(sDraftIn.angle());
  for (const auto &p : sDraftIn.targetPicks())
    targetPicks.emplace_back(p);
  label->serialIn(sDraftIn.plabel());
}
