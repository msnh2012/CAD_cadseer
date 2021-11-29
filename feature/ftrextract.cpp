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
#include <BRepLib.hxx>
#include <BRepTools_Quilt.hxx>

#include <osg/Switch>

#include "globalutilities.h"
#include "tools/occtools.h"
#include "annex/annseershape.h"
#include "feature/ftrshapecheck.h"
#include "project/serial/generated/prjsrlextsextract.h"
#include "feature/ftrupdatepayload.h"
#include "tools/featuretools.h"
#include "feature/ftrinputtype.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "library/lbrplabel.h"
#include "feature/ftrextract.h"

using namespace ftr::Extract;

using boost::uuids::uuid;

QIcon Feature::icon = QIcon(":/resources/images/constructionExtract.svg");

struct Feature::Stow
{
  Feature &feature;
  
  prm::Parameter picks{prm::Names::Picks, ftr::Picks(), prm::Tags::Picks};
  prm::Parameter angle{prm::Names::Angle, 0.0, prm::Tags::Angle};
  
  ann::SeerShape sShape;
  
  osg::ref_ptr<lbr::PLabel> angleLabel{new lbr::PLabel(&angle)};
  
  Stow() = delete;
  Stow(Feature &fIn)
  : feature(fIn)
  {
    picks.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&picks);
    
    angle.connectValue(std::bind(&Feature::setModelDirty, &feature));
    prm::Interval interval
    (
      prm::Boundary(0.0, prm::Boundary::End::Closed)
      , prm::Boundary(180.0, prm::Boundary::End::Open)
    );
    prm::Constraint c;
    c.intervals.push_back(interval);
    angle.setConstraint(c);
    angle.setActive(false);
    feature.parameters.push_back(&angle);
    
    feature.overlaySwitch->addChild(angleLabel.get());
    
    feature.annexes.insert(std::make_pair(ann::Type::SeerShape, &sShape));
  }
};

Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  name = QObject::tr("Extract");
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
    for (const auto &p : stow->picks.getPicks())
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
            occt::FaceVector tempFaces = occt::walkTangentFaces(pr.getSeerShape()->getRootOCCTShape(), f, stow->angle.getDouble());
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
      stow->angleLabel->setMatrix(accrueLocation.get());
    
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
    stow->sShape.setOCCTShape(out, getId());
    stow->sShape.shapeMatch(targetSeerShape);
    stow->sShape.uniqueTypeMatch(targetSeerShape);
    stow->sShape.outerWireMatch(targetSeerShape);
    stow->sShape.derivedMatch();
    stow->sShape.dumpNils("extract feature");
    stow->sShape.dumpDuplicates("extract feature");
    stow->sShape.ensureNoNils();
    stow->sShape.ensureNoDuplicates();
    stow->sShape.ensureEvolve();
    
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

void Feature::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::exts::Extract extractOut
  (
    Base::serialOut()
    , stow->picks.serialOut()
    , stow->angle.serialOut()
    , stow->sShape.serialOut()
    , stow->angleLabel->serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::exts::extract(stream, extractOut, infoMap);
}

void Feature::serialRead(const prj::srl::exts::Extract &sExtractIn)
{
  Base::serialIn(sExtractIn.base());
  stow->picks.serialIn(sExtractIn.picks());
  stow->angle.serialIn(sExtractIn.angle());
  stow->sShape.serialIn(sExtractIn.seerShape());
  stow->angleLabel->serialIn(sExtractIn.label());
}
