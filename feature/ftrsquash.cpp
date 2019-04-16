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

#include <globalutilities.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <tools/occtools.h>
#include <tools/idtools.h>
#include <squash/squash.h>
#include <annex/seershape.h>
#include <feature/shapecheck.h>
#include <project/serial/xsdcxxoutput/featuresquash.h>
#include <tools/featuretools.h>
#include <modelviz/surfacemesh.h>
#include <feature/updatepayload.h>
#include <feature/inputtype.h>
#include <feature/squash.h>

using namespace ftr;

using boost::uuids::uuid;

QIcon Squash::icon;

Squash::Squash() : Base(), sShape(new ann::SeerShape())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionSquash.svg");
  
  name = QObject::tr("Squash");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  faceId = gu::createRandomId();
  wireId = gu::createRandomId();
  
  granularity = std::shared_ptr<prm::Parameter>
  (
    new prm::Parameter
    (
      QObject::tr("Granularity"),
      prf::manager().rootPtr->features().squash().get().granularity()
    )
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
  label->showName = true;
  label->valueHasChanged();
  label->constantHasChanged();
  overlaySwitch->addChild(label.get());
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
}

Squash::~Squash(){}

int Squash::getGranularity()
{
  return static_cast<int>(*granularity);
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
    std::vector<const Base*> tfs = payloadIn.getFeatures(InputType::target);
    if (tfs.size() != 1)
      throw std::runtime_error("wrong number of parents");
    if(!tfs.front()->hasAnnex(ann::Type::SeerShape))
      throw std::runtime_error("no seer shape");
    const ann::SeerShape &tss = tfs.front()->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
    if (tss.isNull())
      throw std::runtime_error("target seer shape is null");
    
    //no new failure state.
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
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
    
    auto resolvedPicks = tls::resolvePicks(tfs, picks, payloadIn.shapeHistory);
    for (const auto &resolved : resolvedPicks)
    {
      if (resolved.resultId.is_nil())
        continue;
      assert(tss.hasId(resolved.resultId));
      if (!tss.hasId(resolved.resultId))
        continue;
      const TopoDS_Shape &shape = tss.getOCCTShape(resolved.resultId);
      if (shape.ShapeType() != TopAbs_FACE)
      {
        std::ostringstream sm; sm << "shape is not a face in Squash::updateModel" << std::endl;
        lastUpdateLog += sm.str();
        continue;
      }
      const TopoDS_Face &f = TopoDS::Face(shape);
      static const gp_Vec zAxis(0.0, 0.0, 1.0);
      gp_Vec n = occt::getNormal(f, resolved.pick.u, resolved.pick.v);
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
    
    if (fs.empty())
      throw std::runtime_error("No holding faces");
    if (sr)
      s.Reverse();
    
    sqs::Parameters ps(s, fs);
    ps.granularity = static_cast<int>(*granularity);
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
      sShape->setOCCTShape(c);
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
        sShape->setOCCTShape(out);
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
  prj::srl::FeatureSquash so
  (
    Base::serialOut(),
    ::ftr::serialOut(picks),
    gu::idToString(faceId),
    gu::idToString(wireId),
    granularity->serialOut(),
    label->serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::squash(stream, so, infoMap);
}

void Squash::serialRead(const prj::srl::FeatureSquash &sSquashIn)
{
  Base::serialIn(sSquashIn.featureBase());
  picks = ::ftr::serialIn(sSquashIn.picks());
  faceId = gu::stringToId(sSquashIn.faceId());
  wireId = gu::stringToId(sSquashIn.wireId());
  granularity->serialIn(sSquashIn.granularity());
  label->serialIn(sSquashIn.label());
}