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

#include <tuple>
#include <boost/filesystem/path.hpp>

#include <osg/Switch>

#include <BRep_Tool.hxx>
#include <TopoDS.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <GeomAPI.hxx>
#include <gp_Pln.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepLib.hxx>
#include <NCollection_StdAllocator.hxx>

#include "globalutilities.h"
#include "annex/annseershape.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "tools/occtools.h"
#include "tools/featuretools.h"
#include "tools/idtools.h"
#include "tools/tlsshapeid.h"
#include "feature/ftrshapecheck.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrsketch.h"
#include "project/serial/generated/prjsrlmpcmappcurve.h"
#include "feature/ftrmappcurve.h"

using namespace ftr::MapPCurve;
using boost::uuids::uuid;
QIcon Feature::icon = QIcon(":/resources/images/constructionMapPCurve.svg");

struct Feature::Stow
{
  Feature &feature;
  prm::Parameter facePick{QObject::tr("Face Pick"), ftr::Picks(), Tags::facePick};
  prm::Parameter edgePicks{QObject::tr("Edge Picks"), ftr::Picks(), Tags::edgePicks};
  ann::SeerShape sShape;
  
  Stow() = delete;
  Stow(Feature &fIn)
  : feature(fIn)
  {
    facePick.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&facePick);
    
    edgePicks.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&edgePicks);
    
    feature.annexes.insert(std::make_pair(ann::Type::SeerShape, &sShape));
  }
};

Feature::Feature():
Base()
, stow(std::make_unique<Stow>(*this))
{
  name = QObject::tr("MapPCurve");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
}

Feature::~Feature(){}

/* Only supporting mapping a sketch for now. Sketch has a csys and makes it easy to build
 * PCurves from the wire. Free wires and edges have no reference csys, so in order to map
 * those, there will have to be some reference csys. Not sure how we will do that.
 */
void Feature::updateModel(const UpdatePayload &pIn)
{
  setFailure();
  lastUpdateLog.clear();
  stow->sShape.reset();
  try
  {
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    if (stow->facePick.getPicks().empty())
      throw std::runtime_error("no selected face");
    tls::Resolver pr(pIn);
    
    if (!pr.resolve(stow->facePick.getPicks().front()))
      throw std::runtime_error("invalid pick resolution for face");
    auto faceShapes = pr.getShapes();
    if (faceShapes.empty())
      throw std::runtime_error("no shapes for resolution of face");
    const TopoDS_Shape &face = faceShapes.front();
    if (face.ShapeType() != TopAbs_FACE)
      throw std::runtime_error("shape for face is not a face");
    TopLoc_Location surfaceLocation;
    opencascade::handle<Geom_Surface> surface = BRep_Tool::Surface(TopoDS::Face(face), surfaceLocation);
    opencascade::handle<Geom_Surface> surfaceCopy = dynamic_cast<Geom_Surface*>(surface->Copy().get());
    
    occt::EdgeVector freeEdges;
    occt::WireVector wires;
    tls::ShapeIdContainer tempIdMap; //store shapes and ids to assign to seershape after the fact.
    for (const auto &ep : stow->edgePicks.getPicks())
    {
      if (!pr.resolve(ep))
        throw std::runtime_error("invalid pick resolution for edge");
      const ann::SeerShape *iss = pr.getSeerShape(); //input seershape. maybe nullptr.
      if (slc::isObjectType(ep.selectionType))
      {
        if (pr.getFeature()->getType() == ftr::Type::Sketch && iss)
        {
          const auto *sketch = dynamic_cast<const Sketch::Feature*>(pr.getFeature());
          assert(sketch);
          auto systemParameters =  sketch->getParameters(prm::Tags::CSys);
          assert(!systemParameters.empty());
          const prm::Parameter *sketchSysParameter = sketch->getParameters(prm::Tags::CSys).front();
          osg::Matrixd sketchSys = sketchSysParameter->getMatrix();
          gp_Pln sketchPlane(gu::toOcc(sketchSys));
          for (const auto &w : iss->useGetChildrenOfType(iss->getRootOCCTShape(), TopAbs_WIRE))
          {
            BRepBuilderAPI_MakeWire nwm; //new wire maker.
            const TopoDS_Wire &wire = TopoDS::Wire(w);
            assert(iss->hasShape(wire));
            for (BRepTools_WireExplorer we(wire); we.More(); we.Next())
            {
              assert(iss->hasShape(we.Current()));
              uuid oldEdgeId = iss->findId(we.Current());
              if (!stow->sShape.hasEvolveRecordIn(oldEdgeId))
                stow->sShape.insertEvolve(oldEdgeId, gu::createRandomId());
              BRepAdaptor_Curve curveAdaptor(we.Current());
              opencascade::handle<Geom2d_Curve> curve2d = GeomAPI::To2d(curveAdaptor.Curve().Curve(), sketchPlane);
              BRepBuilderAPI_MakeEdge em(curve2d, surfaceCopy, curveAdaptor.FirstParameter(), curveAdaptor.LastParameter());
              if (!BRepLib::BuildCurve3d(em))
                throw std::runtime_error("Couldn't build 3d curve");
              nwm.Add(em);
              if (nwm.Error() != BRepBuilderAPI_WireDone)
                throw std::runtime_error("Adding edge to wire failed");
              tempIdMap.insert(stow->sShape.evolve(oldEdgeId).front(), nwm.Edge());
            }
            wires.push_back(nwm);
            uuid oldWireId = iss->findId(wire);
            if (!stow->sShape.hasEvolveRecordIn(oldWireId))
              stow->sShape.insertEvolve(oldWireId, gu::createRandomId());
            tempIdMap.insert(stow->sShape.evolve(oldWireId).front(), wires.back());
          }
        }
      }
    }
    
    occt::ShapeVector all;
    all.insert(all.end(), wires.begin(), wires.end());
    all.insert(all.end(), freeEdges.begin(), freeEdges.end());
    TopoDS_Compound out = occt::ShapeVectorCast(all);
    
    ShapeCheck check(out);
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    stow->sShape.setOCCTShape(out, getId());
    for (const auto &s : stow->sShape.getAllShapes())
    {
      if (tempIdMap.has(s))
        stow->sShape.updateId(s, tempIdMap.find(s));
    }
    //create vertex ids from edges. This is a hack as we loose vertex evolution from input feature.
    //BRepTools_WireExplorer seems to skip the first vertex and it is a PIA to try and sync the
    //vertices between WireExplorer with the WireMaker.
    stow->sShape.derivedMatch();
    stow->sShape.dumpNils(getTypeString());
    stow->sShape.ensureNoNils();
    
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
  prj::srl::mpc::MapPCurve so
  (
    Base::serialOut()
    , stow->facePick.serialOut()
    , stow->edgePicks.serialOut()
    , stow->sShape.serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::mpc::mappcurve(stream, so, infoMap);
}

void Feature::serialRead(const prj::srl::mpc::MapPCurve &so)
{
  Base::serialIn(so.base());
  stow->facePick.serialIn(so.facePick());
  stow->edgePicks.serialIn(so.edgePicks());
  stow->sShape.serialIn(so.seerShape());
}
