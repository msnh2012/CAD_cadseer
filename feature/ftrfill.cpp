/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2020 Thomas S. Anderson blobfish.at.gmx.com
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

#include <TopoDS.hxx>
#include <BRepFill_Filling.hxx>
#include <ShapeBuild_ReShape.hxx>
#include <ShapeFix_Face.hxx>
#include <ShapeFix_Wire.hxx>

#include <osg/Switch>

#include "globalutilities.h"
#include "annex/annseershape.h"
#include "library/lbrplabel.h"
#include "parameter/prmparameter.h"
#include "tools/occtools.h"
#include "tools/featuretools.h"
#include "tools/tlsshapeid.h"
#include "tools/idtools.h"
#include "feature/ftrshapecheck.h"
#include "feature/ftrupdatepayload.h"
#include "project/serial/generated/prjsrlflsfill.h"
#include "feature/ftrfill.h"

using namespace ftr::Fill;
using boost::uuids::uuid;

QIcon Feature::icon;

std::shared_ptr<prm::Parameter> ftr::Fill::buildContinuityParameter()
{
  auto out = std::make_shared<prm::Parameter>(QObject::tr("Continuity"), 0);
  
  QStringList tStrings =
  {
    QObject::tr("C0")
    , QObject::tr("G1")
    , QObject::tr("G2")
  };
  out->setEnumeration(tStrings);
  
  return out;
}

Entry::Entry()
: continuity(buildContinuityParameter())
{
  createLabel();
}

Entry::~Entry() = default;

void Entry::createLabel()
{
  assert(continuity);
  continuityLabel = new lbr::PLabel(continuity.get());
}

Feature::Feature():
Base()
, sShape(std::make_unique<ann::SeerShape>())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionFill.svg"); //fix me
  
  name = QObject::tr("Fill");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
}

Feature::~Feature() = default;

void Feature::clearEntries()
{
  for (const auto& e : entries)
  {
    removeParameter(e.continuity.get());
    overlaySwitch->removeChild(e.continuityLabel.get());
  }
  entries.clear();
  setModelDirty();
}

void Feature::addEntry(const Entry &eIn)
{
  entries.emplace_back(eIn);
  entries.back().continuity->connectValue(std::bind(&Feature::setModelDirty, this));
  parameters.push_back(entries.back().continuity.get());
  overlaySwitch->addChild(entries.back().continuityLabel.get());
  setModelDirty();
}

void Feature::updateModel(const UpdatePayload &pIn)
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
    
    tls::Resolver pr(pIn);
    tls::ShapeIdContainer tempIds;
    occt::EdgeVector edges;
    
    BRepFill_Filling filler;
    
    for (const auto &e : entries)
    {
      GeomAbs_Shape c = GeomAbs_C0;
      if (static_cast<int>(*e.continuity) == 1)
        c = GeomAbs_G1;
      else if (static_cast<int>(*e.continuity) == 2)
        c = GeomAbs_G2;
      if (e.edgePick && !e.facePick)
      {
        //edge, no face
        overlaySwitch->removeChild(e.continuityLabel.get()); //don't show, it is always c0
        if (!pr.resolve(*e.edgePick))
          continue;
        auto shapes = pr.getShapes();
        if (!shapes.empty() && shapes.front().ShapeType() == TopAbs_EDGE)
          filler.Add(TopoDS::Edge(shapes.front()), c, e.boundary);
      }
      else if (!e.edgePick && e.facePick)
      {
        //no edge, face
        if (!overlaySwitch->containsNode(e.continuityLabel.get()))
          overlaySwitch->addChild(e.continuityLabel.get());
        if (!pr.resolve(*e.facePick))
          continue;
        auto shapes = pr.getShapes();
        if (!shapes.empty() && shapes.front().ShapeType() == TopAbs_FACE)
        {
          const TopoDS_Face &f = TopoDS::Face(shapes.front());
          filler.Add(f, c); //faces without edges are always boundary
          e.continuityLabel->setMatrix(osg::Matrixd::translate(e.facePick->getPoint(f)));
        }
      }
      else if (e.edgePick && e.facePick)
      {
        //edge, face
        if (!overlaySwitch->containsNode(e.continuityLabel.get()))
          overlaySwitch->addChild(e.continuityLabel.get());
        if (!pr.resolve(*e.edgePick))
          continue;
        auto edgeShapes = pr.getShapes();
        if (edgeShapes.empty() || edgeShapes.front().ShapeType() != TopAbs_EDGE)
          continue;
        
        if (!pr.resolve(*e.facePick))
          continue;
        auto faceShapes = pr.getShapes();
        if (faceShapes.empty() || faceShapes.front().ShapeType() != TopAbs_FACE)
          continue;
        filler.Add(TopoDS::Edge(edgeShapes.front()), TopoDS::Face(faceShapes.front()), c, e.boundary);
        e.continuityLabel->setMatrix(osg::Matrixd::translate(e.facePick->getPoint(TopoDS::Face(faceShapes.front()))));
      }
    }
    
    filler.Build();
    if (!filler.IsDone())
      throw std::runtime_error("filling failed");
    
    TopoDS_Face out = filler.Face();
    if (out.IsNull())
      throw std::runtime_error("out face is null");
    
    ShapeCheck check(out);
    if (!check.isValid())
    {
      //When picking just a surface for a boundary we get a lot of same
      //parameter errors. This tries to fix those.
      opencascade::handle<ShapeBuild_ReShape> context = new ShapeBuild_ReShape();
      ShapeFix_Face faceFixer(out); //this calls 'ClearModes to create an empty operation'
      faceFixer.SetContext(context);
      faceFixer.FixWireMode() = 1;
      auto wireFixer = faceFixer.FixWireTool();
      wireFixer->FixSameParameterMode() = 1;
      wireFixer->FixEdgeCurvesMode() = 1;
      faceFixer.Perform();
      if (!faceFixer.Face().IsNull())
        out = faceFixer.Face();
      
      ShapeCheck check2(out);
      if (!check2.isValid())
        throw std::runtime_error("couldn't fix shape");
    }
    
    sShape->setOCCTShape(out, getId());
    sShape->ensureNoNils();
    sShape->ensureNoDuplicates();
    
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
  prj::srl::fls::Fill so
  (
    Base::serialOut()
    , sShape->serialOut()

  );
  for (const auto &e : entries)
  {
    prj::srl::fls::Entry eOut(e.boundary, e.continuity->serialOut(), e.continuityLabel->serialOut());
    if (e.edgePick)
      eOut.edgePick() = e.edgePick->serialOut();
    if (e.facePick)
      eOut.facePick() = e.facePick->serialOut();
    so.entries().push_back(eOut);
  }
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::fls::fill(stream, so, infoMap);
}

void Feature::serialRead(const prj::srl::fls::Fill &so)
{
  Base::serialIn(so.base());
  sShape->serialIn(so.seerShape());
  
  for (const auto &eIn : so.entries())
  {
    Entry entry;
    if (eIn.edgePick())
      entry.edgePick = Pick(eIn.edgePick().get());
    if (eIn.facePick())
      entry.facePick = Pick(eIn.facePick().get());
    entry.boundary = eIn.boundary();
    entry.continuity = buildContinuityParameter();
    entry.continuity->serialIn(eIn.continuity());
    entry.continuityLabel = new lbr::PLabel(entry.continuity.get());
    entries.push_back(entry);
  }
}
