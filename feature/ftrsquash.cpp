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

#include <TopoDS.hxx>
#include <BRepTools.hxx>
#include <Precision.hxx>

#include <osg/Switch>

#include "globalutilities.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "tools/occtools.h"
#include "tools/idtools.h"
#include "squash/sqssquash.h"
#include "annex/annseershape.h"
#include "feature/ftrshapecheck.h"
#include "project/serial/generated/prjsrlsqsssquash.h"
#include "tools/featuretools.h"
#include "modelviz/mdvsurfacemesh.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrsquash.h"

using namespace ftr;

using boost::uuids::uuid;

QIcon Squash::icon;

Squash::Squash() : Base(), sShape(std::make_unique<ann::SeerShape>())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionSquash.svg");
  
  name = QObject::tr("Squash");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  faceId = gu::createRandomId();
  wireId = gu::createRandomId();
  
  granularity = std::make_shared<prm::Parameter>
  (
    QObject::tr("Granularity"),
    prf::manager().rootPtr->features().squash().get().granularity()
  );
  
  prm::Boundary lower(1.0, prm::Boundary::End::Closed);
  prm::Boundary upper(5.0, prm::Boundary::End::Closed);
  prm::Interval interval(lower, upper);
  prm::Constraint c;
  c.intervals.push_back(interval);
  granularity->setConstraint(c);
  
  granularity->connectValue(std::bind(&Squash::setModelDirty, this));
  parameters.push_back(granularity.get());
  
  label = new lbr::PLabel(granularity.get());
  overlaySwitch->addChild(label.get());
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
}

Squash::~Squash(){}

int Squash::getGranularity()
{
  return granularity->getInt();
}

void Squash::setGranularity(int vIn)
{
  granularity->setValue(vIn);
}

void Squash::updateModel(const UpdatePayload &payloadIn)
{
  lastUpdateLog.clear();
  overlaySwitch->removeChildren(0, overlaySwitch->getNumChildren());
  overlaySwitch->addChild(label.get());
  
  setFailure();
  sShape->reset();
  try
  {
    //no new failure state.
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    std::vector<const Base*> tfs = payloadIn.getFeatures(InputType::target);
    if (tfs.size() != 1)
      throw std::runtime_error("wrong number of parents");
    if(!tfs.front()->hasAnnex(ann::Type::SeerShape))
      throw std::runtime_error("no seer shape");
    const ann::SeerShape &tss = tfs.front()->getAnnex<ann::SeerShape>();
    if (tss.isNull())
      throw std::runtime_error("target seer shape is null");
    
    
    //get the shell
    TopoDS_Shape ss = occt::getFirstNonCompound(tss.getRootOCCTShape());
    if (ss.IsNull())
      throw std::runtime_error("Shape is null");
    if (ss.ShapeType() != TopAbs_SHELL)
      throw std::runtime_error("Shape is not a shell");
    TopoDS_Shell s = TopoDS::Shell(ss);
    
    //get the holding faces.
    occt::FaceVector fs;
    bool od = false; //orientation determined.
    bool sr = false; //should reverse.
    
    
    
    
    tls::Resolver tResolver(payloadIn);
    for (const auto &p : picks)
    {
      if (!tResolver.resolve(p))
      {
        std::ostringstream sm; sm << "WARNING: Skipping unresolved a pick." << std::endl;
        lastUpdateLog += sm.str();
        continue;
      }
      for (const auto &rs : tResolver.getShapes())
      {
        if (rs.ShapeType() != TopAbs_FACE)
        {
          std::ostringstream sm; sm << "WARNING: Resolved shape is not a face" << std::endl;
          lastUpdateLog += sm.str();
          continue;
        }
        const TopoDS_Face &f = TopoDS::Face(rs);
        static const gp_Vec zAxis(0.0, 0.0, 1.0);
        gp_Vec n = occt::getNormal(f, p.u, p.v);
        if (!n.IsParallel(zAxis, Precision::Confusion())) //Precision::Angular was too sensitive.
          throw std::runtime_error("lock face that is not parallel to z axis");
        if (!od)
        {
          od = true;
          if (n.IsOpposite(zAxis, Precision::Angular()))
            sr = true;
        }
        if (sr)
          fs.push_back(TopoDS::Face(f.Reversed())); //might not need to reverse these?
        else
          fs.push_back(f);
      }
    }
    
    if (fs.empty())
      throw std::runtime_error("No holding faces");
    if (sr)
      s.Reverse();
    
    sqs::Parameters ps(s, fs);
    ps.granularity = granularity->getInt();
    try
    {
      sqs::squash(ps);
    }
    catch (const std::exception &e)
    {
      lastUpdateLog += ps.message;
      if(ps.mesh3d)
      {
        osg::Switch *viz = mdv::generate(*ps.mesh3d);
        if (viz)
          overlaySwitch->insertChild(0, viz);
      }
      throw;
    }
    if(ps.mesh3d)
    {
      osg::Switch *viz = mdv::generate(*ps.mesh3d);
      if (viz)
        overlaySwitch->insertChild(0, viz);
    }
    if(ps.mesh2d)
    {
      osg::Switch *viz = mdv::generate(*ps.mesh2d);
      if (viz)
        overlaySwitch->insertChild(0, viz);
    }
    TopoDS_Face out = ps.ff;
    
    auto setEdges = [&]()
    {
      //this is incase face is bad, this should show something.
      //we don't really care about id evolution with these edges.
      TopoDS_Compound c = occt::ShapeVectorCast(ps.es);
      sShape->setOCCTShape(c, getId());
      sShape->ensureNoNils();
      std::ostringstream s; s << "Face was invalid, using edges" << std::endl;
      lastUpdateLog += s.str();
    };
    
    if (!out.IsNull())
    {
      ShapeCheck check(out);
      if (check.isValid())
      {
        //for now, we are only going to have consistent ids for face and outer wire.
        sShape->setOCCTShape(out, getId());
        sShape->updateId(out, faceId);
        const TopoDS_Shape &ow = BRepTools::OuterWire(out);
        sShape->updateId(ow, wireId);
        sShape->ensureNoNils();
      }
      else
        setEdges();
    }
    else
      setEdges();
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in squash update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in squash update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in squash update. " << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Squash::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::sqss::Squash so
  (
    Base::serialOut(),
    sShape->serialOut(),
    gu::idToString(faceId),
    gu::idToString(wireId),
    granularity->serialOut(),
    label->serialOut()
  );
  for (const auto &p : picks)
    so.picks().push_back(p);
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::sqss::squash(stream, so, infoMap);
}

void Squash::serialRead(const prj::srl::sqss::Squash &sSquashIn)
{
  Base::serialIn(sSquashIn.base());
  sShape->serialIn(sSquashIn.seerShape());
  faceId = gu::stringToId(sSquashIn.faceId());
  wireId = gu::stringToId(sSquashIn.wireId());
  granularity->serialIn(sSquashIn.granularity());
  label->serialIn(sSquashIn.label());
  for (const auto &p : sSquashIn.picks())
    picks.emplace_back(p);
}
