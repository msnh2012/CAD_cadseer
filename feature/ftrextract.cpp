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
#include <boost/optional/optional.hpp>

#include <TopoDS.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepLib.hxx>
#include <BRepTools_Quilt.hxx>

#include <osg/Switch>

#include "globalutilities.h"
#include "tools/occtools.h"
#include "annex/annseershape.h"
#include "feature/ftrshapecheck.h"
#include "project/serial/xsdcxxoutput/featureextract.h"
#include "feature/ftrupdatepayload.h"
#include "tools/featuretools.h"
#include "feature/ftrinputtype.h"
#include "parameter/prmparameter.h"
#include "library/lbrplabel.h"
#include "feature/ftrextract.h"

using namespace ftr;

using boost::uuids::uuid;

QIcon Extract::icon;

Extract::Extract()
: Base()
, sShape(new ann::SeerShape())
, angle(buildAngleParameter())
, label(new lbr::PLabel(angle.get()))
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionExtract.svg");
  
  name = QObject::tr("Extract");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
  
  angle->connectValue(std::bind(&Extract::setModelDirty, this));
  parameters.push_back(angle.get());
  
  label->showName = true;
  label->valueHasChanged();
  label->constantHasChanged();
//   overlaySwitch->addChild(label.get());
}

Extract::~Extract(){}

std::shared_ptr<prm::Parameter> Extract::buildAngleParameter(double deg)
{
  std::shared_ptr<prm::Parameter> out(new prm::Parameter(prm::Names::Angle, 0.0));
  //set the value after constraints have been set.
  
  prm::Boundary lower(0.0, prm::Boundary::End::Closed);
  prm::Boundary upper(180.0, prm::Boundary::End::Open);
  prm::Interval interval(lower, upper);
  
  prm::Constraint c;
  c.intervals.push_back(interval);
  out->setConstraint(c);
  
  out->setValue(deg);
  
  return out;
}

void Extract::setAngleParameter(std::shared_ptr<prm::Parameter> prmIn)
{
  assert(prmIn);
  
  //remove old
  auto rit = std::remove_if
  (
    parameters.begin()
    , parameters.end()
    , [&](prm::Parameter *pic){return pic == angle.get();}
  );
  if (angle)
    parameters.erase(rit, parameters.end());
  if (label)
    overlaySwitch->removeChild(label.get());
  
  //add new
  angle = prmIn;
  angle->connectValue(std::bind(&Extract::setModelDirty, this));
  parameters.push_back(angle.get());
  label = new lbr::PLabel(angle.get());
  label->showName = true;
  label->valueHasChanged();
  label->constantHasChanged();
  overlaySwitch->addChild(label.get());
  
  setModelDirty();
}

void Extract::setPicks(const Picks &psIn)
{
  picks = psIn;
  setModelDirty();
}

void Extract::updateModel(const UpdatePayload &payloadIn)
{
  setFailure();
  lastUpdateLog.clear();
  sShape->reset();
  try
  {
    //no new failure state.
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    std::vector<const Base*> parents = payloadIn.getFeatures(std::string());
    if (parents.size() != 1)
      throw std::runtime_error("Wrong number of parent features");
    if (!parents.front()->hasAnnex(ann::Type::SeerShape))
      throw std::runtime_error("Parent feature doesn't have seer shape");
    const ann::SeerShape &targetSeerShape = parents.front()->getAnnex<ann::SeerShape>();
    
    boost::optional<osg::Matrixd> accrueLocation;
    tls::Resolver pr(payloadIn);
    occt::ShapeVector outShapes;
    occt::FaceVector faces;
    for (const auto &p : picks)
    {
      if (!pr.resolve(p))
        continue;
      for (const auto &s : pr.getShapes())
      {
        //only supporting tangent accrue of faces at this time.
        if (s.ShapeType() == TopAbs_FACE)
        {
          TopoDS_Face f = TopoDS::Face(s);
          if (p.accrue == slc::Accrue::Tangent)
          {
            if (!accrueLocation)
              accrueLocation = osg::Matrixd::translate(p.getPoint(f));
            occt::FaceVector tempFaces = occt::walkTangentFaces(pr.getSeerShape()->getRootOCCTShape(), f, static_cast<double>(*angle));
            std::copy(tempFaces.begin(), tempFaces.end(), std::back_inserter(faces));
          }
          else
            faces.push_back(f);
        }
        else
          outShapes.push_back(s);
      }
    }
    if (accrueLocation)
      label->setMatrix(accrueLocation.get());
    
    //update feature visibility before shape construction in case of error.
    if (accrueLocation)
      overlaySwitch->addChild(label.get());
    else
      overlaySwitch->removeChild(label.get());
    
    //apparently just throw all faces into the quilter and it will sort it out?
    if (faces.size() > 1)
    {
      BRepTools_Quilt quilter;
      for (const auto &f : faces)
        quilter.Add(f);
      occt::ShellVector quilts = occt::ShapeVectorCast(TopoDS::Compound(quilter.Shells()));
      for (const auto &s : quilts)
        outShapes.push_back(s);
    }
    else if (!faces.empty())
      outShapes.push_back(faces.front());
    
    //make sure we don't make a compound containing compounds.
    occt::ShapeVector nonCompounds;
    for (const auto &s : outShapes)
    {
      occt::ShapeVector temp = occt::getNonCompounds(s);
      std::copy(temp.begin(), temp.end(), std::back_inserter(nonCompounds));
    }

    TopoDS_Compound out = static_cast<TopoDS_Compound>(occt::ShapeVectorCast(nonCompounds));
    
    if (out.IsNull())
      throw std::runtime_error("null shape");
    
    ShapeCheck check(out);
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    //these seem to be taking care of it.
    sShape->setOCCTShape(out, getId());
    sShape->shapeMatch(targetSeerShape);
    sShape->uniqueTypeMatch(targetSeerShape);
    sShape->outerWireMatch(targetSeerShape);
    sShape->derivedMatch();
    sShape->dumpNils("extract feature");
    sShape->dumpDuplicates("extract feature");
    sShape->ensureNoNils();
    sShape->ensureNoDuplicates();
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in extract update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in extract update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in extract update." << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Extract::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::FeatureExtract extractOut
  (
    Base::serialOut()
    , ::ftr::serialOut(picks)
    , angle->serialOut()
    , label->serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::extract(stream, extractOut, infoMap);
}

void Extract::serialRead(const prj::srl::FeatureExtract &sExtractIn)
{
  Base::serialIn(sExtractIn.featureBase());
  picks = ::ftr::serialIn(sExtractIn.picks());
  angle->serialIn(sExtractIn.angle());
  label->serialIn(sExtractIn.label());
  
  //this sucks!
  for (const auto &p : picks)
  {
    if (p.accrue == slc::Accrue::Tangent)
    {
      overlaySwitch->addChild(label.get());
      break;
    }
  }
}
