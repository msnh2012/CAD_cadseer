/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2018  Thomas S. Anderson blobfish.at.gmx.com
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

#include <osg/Switch>

#include <TopoDS.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BOPAlgo_Tools.hxx>
#include <GeomPlate_BuildAveragePlane.hxx>
#include <Geom_Plane.hxx>
#include <gp_Pln.hxx>
#include <BRepClass3d.hxx>

#include "globalutilities.h"
#include "tools/occtools.h"
#include "tools/tlsosgtools.h"
#include "annex/annseershape.h"
#include "library/lbrplabel.h"
#include "library/lbripgroup.h"
#include "feature/ftrshapecheck.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinputtype.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "tools/featuretools.h"
#include "tools/idtools.h"
#include "tools/tlsshapeid.h"
#include "project/serial/generated/prjsrlexrsextrude.h"
#include "feature/ftrextrude.h"

using namespace ftr::Extrude;
using boost::uuids::uuid;

QIcon Feature::icon = QIcon(":/resources/images/constructionExtrude.svg");

namespace
{
  const std::map<BRepBuilderAPI_FaceError, std::string> errorMap
  {
    std::make_pair(BRepBuilderAPI_FaceDone, "BRepBuilderAPI_FaceDone")
    , std::make_pair(BRepBuilderAPI_NoFace, "BRepBuilderAPI_NoFace")
    , std::make_pair(BRepBuilderAPI_NotPlanar, "BRepBuilderAPI_NotPlanar")
    , std::make_pair(BRepBuilderAPI_CurveProjectionFailed, "BRepBuilderAPI_CurveProjectionFailed")
    , std::make_pair(BRepBuilderAPI_ParametersOutOfRange, "BRepBuilderAPI_ParametersOutOfRange")
  };
  
  std::optional<osg::Vec3d> inferDirection(const occt::ShapeVector &svIn)
  {
    TopTools_IndexedMapOfShape vertices;
    for (const auto &s : svIn)
      TopExp::MapShapes(s, TopAbs_VERTEX, vertices);
    
    /*
    notes:
    TColgp_Array1OfPnt is NOT dynamic and needs size upon construction.
    TColgp_Array1OfPnt wants 1st element as FIRST constructor argument
      while TColgp_HArray1OfPnt wants 1st element as LAST constructor argument.
    I looked at code for NCollection_Array1 and it appears to handle 1-based index shift.
    I looked at code for GeomPlate_BuildAveragePlane and it doesn't look at 
      ncollection::LowerBound, just starts at 1. I think that is pretty common so
      better stick to 1 based range when constructing. 
    After all this time I still couldn't get it to work using ref from std::vector
      so just copy the points into.
    */
    opencascade::handle<TColgp_HArray1OfPnt> hPoints = new TColgp_HArray1OfPnt(1, vertices.Size());
    for (int index = 1; index <= vertices.Size(); ++index)
      (*hPoints)[index] = BRep_Tool::Pnt(TopoDS::Vertex(vertices(index)));
    
    //not sure what is wanted by NbBoundPoints, second parameter.
    //I tried 2 for last parameter, 'max flux', but didn't work. Using 1 inertia.
    GeomPlate_BuildAveragePlane builder(hPoints, 0, Precision::Confusion(), 1, 1);
    if (builder.IsPlane())
    {
      opencascade::handle<Geom_Plane> pln = builder.Plane();
      return gu::toOsg(pln->Pln().Axis().Direction());
    }
    return std::nullopt;
  }
  
  /* How did this get so big and complicated?
 * 
 * Part of the problem is this feature was an attempt to 'wall off'
 * the seer shape evolve container. Evolve container feeds the project
 * shape graph and I wanted to keep the shape graph lean, so evolve needs to be lean also.
 * So I created a separate internal containers for this feature. So now I have another
 * container to keep track of which adds to volume of code. I do want to call that a
 * failed experiment. There is just too much baked into the seershape evolve container.
 * I have a better idea to keep the shape graph lean by adding another 'field' to the
 * evolve record called 'active'. See id mapping in notes.
 * 
 * Trying to pull shapes out of multiple inputs and process, is challenging to id map later.
 * We need a way to keep the id and the shape more closely tied during processing. Question
 * is: do we keep just the id with the shape or should we carry the seershape ref with it?
 * We have the former here with Warehouse.
 * 
 * Trying to 'sew' edges into wires and possibly a faces, adds a lot of headaches.
 * This is just too much for this feature. We need to be able to be 'sew' together
 * edges into a wire in a separate feature.
 * 
 * We have to move the profile by 'offset' and that messes up the id mapping as the
 * shapes are no longer equal after being transformed. So we do an offset
 * mapping for that.
 * 
 * Warehouse is an attemp of making sense of all the id mapping. It so needs
 * boost::multi_index. I was thinking about moving Warehouse into tls::ShapeId,
 * but it is just a workaround for a larger problem with the id mapping and
 * I don't want to develop it as a solution. Leaving here for now.
 */

  struct Unit
  {
    occt::ShapeVector shapes; //!< first should be input shape. Last should in final result.
    std::vector<uuid> ids; //!< should end up with 2;
    int offset = -1; //!< offset inside a bigger shape.
    
    Unit(){} //used for dummies.
    Unit(const TopoDS_Shape& sIn, const uuid& idIn)
    {
      shapes.emplace_back(sIn);
      ids.emplace_back(idIn);
    }
    
    //a valid unit means the mapping has been fully processed.
    //maybe invalid in any intermediate stage.
    bool isValid() const
    {
      /* offset can be left at -1.
       * need at least one shape.
       * need at least one id.
       * we need a non-nil id for last when we have 2 ids
       */
      if (shapes.empty() || ids.empty())
        return false;
      if (ids.size() == 2 && ids.back().is_nil())
        return false;
      return true;
    }
  };
  
  struct WareHouse
  {
    std::vector<Unit> units;
    
    /*! searches on shape and returns found unit.
     * if shape is not found, a new unit is created
     * with a nil id added as first entry in the 
     * vector of ids. Use for new shapes.
     */
    Unit& getOrCreate(const TopoDS_Shape &shapeIn)
    {
      static const uuid nilId = gu::createNilId();
      return getOrCreate(shapeIn, nilId);
    }
    
    /*! searches on shape only and returns found unit.
     *  if shape is not found a new unit is created
     *  and the id is added as the first entry in the
     *  vector of ids. Use for existing shape with id.
     */
    Unit& getOrCreate(const TopoDS_Shape &shapeIn, const uuid &id)
    {
      for (auto &u : units)
      {
        for (const auto &s : u.shapes)
        {
          if (s.IsSame(shapeIn))
            return u;
        }
      }
      units.emplace_back(shapeIn, id);
      return units.back();
    }
    
    Unit& get(int offsetIn)
    {
      for (auto &u : units)
      {
        if (u.offset == offsetIn)
          return u;
      }
      static Unit dummy;
      return dummy;
    }
    
    Unit& get(const TopoDS_Shape &shapeIn)
    {
      for (auto &u : units)
      {
        for (const auto &s : u.shapes)
        {
          if (s.IsSame(shapeIn))
            return u;
        }
      }
      static Unit dummy;
      return dummy;
    }
  };
}

struct Feature::Stow
{
  Feature &feature;
  ann::SeerShape sShape;
  prm::Parameter extrusionType;
  prm::Parameter distance;
  prm::Parameter offset;
  prm::Parameter solid; //!< turns closed wires into faces before extrusion
  prm::Parameter reverse; //!< reversed extrusion direction
  prm::Parameter profilePicks;
  prm::Parameter axisPicks;
  prm::Parameter direction;
  prm::Observer directionObserver;
  osg::ref_ptr<lbr::PLabel> directionLabel;
  osg::ref_ptr<lbr::PLabel> solidLabel;
  osg::ref_ptr<lbr::PLabel> reverseLabel;
  osg::ref_ptr<lbr::IPGroup> distanceLabel;
  osg::ref_ptr<lbr::IPGroup> offsetLabel;
  
  std::map<uuid, uuid> originalMap; //map inputs to equivalents in the output.
  std::map<uuid, uuid> generatedMap; //map side shapes.
  std::map<uuid, uuid> lastMap; //map 'top' shapes.
  std::map<uuid, uuid> oWireMap; //map new face to outer wire. snuck solid to outer shell.
  std::map<std::set<uuid>, uuid> setMap; //map edges 'sewn' into wire, faces into shell.

  Stow(Feature &fIn)
  : feature(fIn)
  , sShape()
  , extrusionType(QObject::tr("Type"), 0, PrmTags::extrusionType)
  , distance(prm::Names::Distance, 10.0, prm::Tags::Distance)
  , offset(prm::Names::Offset, 0.0, prm::Tags::Offset)
  , solid(QObject::tr("Solid"), true, PrmTags::solid)
  , reverse(QObject::tr("Reverse"), false, PrmTags::reverse)
  , profilePicks(QObject::tr("Profile"), ftr::Picks(), PrmTags::profilePicks)
  , axisPicks(QObject::tr("Axis"), ftr::Picks(), PrmTags::axisPicks)
  , direction(prm::Names::Direction, osg::Vec3d(0.0, 0.0, 1.0), prm::Tags::Direction)
  , directionObserver(std::bind(&Feature::setModelDirty, &feature))
  , directionLabel(new lbr::PLabel(&direction))
  , solidLabel(new lbr::PLabel(&solid))
  , reverseLabel(new lbr::PLabel(&reverse))
  , distanceLabel(new lbr::IPGroup(&distance))
  , offsetLabel(new lbr::IPGroup(&offset))
  {
    auto wirePrm = [&](prm::Parameter &param)
    {
      param.connectValue(std::bind(&Feature::setModelDirty, &feature));
      feature.parameters.push_back(&param);
    };
    
    QStringList tStrings = //keep in sync with enum in header.
    {
      QObject::tr("Constant")
      , QObject::tr("Infer")
      , QObject::tr("Picks")
    };
    extrusionType.setEnumeration(tStrings);
    wirePrm(extrusionType);
    extrusionType.connectValue(std::bind(&Stow::prmActiveSync, this));
    
    wirePrm(distance);
    wirePrm(offset);
    wirePrm(solid);
    wirePrm(reverse);
    wirePrm(profilePicks);
    wirePrm(axisPicks);
    direction.connect(directionObserver);
    feature.parameters.push_back(&direction);
    
    feature.annexes.insert(std::make_pair(ann::Type::SeerShape, &sShape));
    
    feature.overlaySwitch->addChild(directionLabel.get());
    feature.overlaySwitch->addChild(solidLabel.get());
    feature.overlaySwitch->addChild(reverseLabel.get());
    auto setupLabel = [&](lbr::IPGroup *group)
    {
      group->setMatrixDims(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(1.0, 0.0, 0.0)));
      group->setMatrix(osg::Matrixd::identity());
      group->noAutoRotateDragger();
      feature.overlaySwitch->addChild(group);
    };
    setupLabel(distanceLabel.get());
    setupLabel(offsetLabel.get());
    distanceLabel->setRotationAxis(osg::Vec3d(0.0, 0.0, 1.0), osg::Vec3d(0.0, 1.0, 0.0));
    offsetLabel->setRotationAxis(osg::Vec3d(0.0, 0.0, 1.0), osg::Vec3d(0.0, -1.0, 0.0));
  }
  
  void prmActiveSync()
  {
    auto et = static_cast<ExtrusionType>(extrusionType.getInt());
    if (et == ExtrusionType::Constant)
      direction.setActive(true);
    else
      direction.setActive(false);
    if (et == ExtrusionType::Picks)
      axisPicks.setActive(true);
    else
      axisPicks.setActive(false);
  }
  
  void updateLabels(occt::BoundingBox &bb)
  {
    osg::Vec3d z = direction.getVector();
    osg::Vec3d vtc; //vector to cross.
    double dot = 1.0;
    auto check = [&](const osg::Vec3d &vIn)
    {
      double t = std::fabs(z * vIn);
      if (t < dot)
      {
        dot = t;
        vtc = vIn;
      }
    };
    check(osg::Vec3d(1.0, 0.0, 0.0));
    check(osg::Vec3d(0.0, 1.0, 0.0));
    check(osg::Vec3d(0.0, 0.0, 1.0));
    osg::Vec3d y = z ^ vtc; y.normalize();
    osg::Vec3d x = y ^ z; x.normalize();
    osg::Vec3d o = gu::toOsg(bb.getCenter());
    osg::Matrixd m
    (
      x.x(), x.y(), x.z(), 0.0,
      y.x(), y.y(), y.z(), 0.0,
      z.x(), z.y(), z.z(), 0.0,
      o.x(), o.y(), o.z(), 1.0
    );
    
    directionLabel->setMatrix(osg::Matrixd::translate(o + x * bb.getDiagonal() / 2.0));
    solidLabel->setMatrix(osg::Matrixd::translate(o - x * bb.getDiagonal() / 2.0));
    reverseLabel->setMatrix(osg::Matrixd::translate(o + y * bb.getDiagonal() / 2.0));
    distanceLabel->setMatrix(m);
    offsetLabel->setMatrix(m);
  }
};


Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  name = QObject::tr("Extrude");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
}

Feature::~Feature() = default;

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
    
    prm::ObserverBlocker directionBlock(stow->directionObserver);
    auto currentType = static_cast<ExtrusionType>(stow->extrusionType.getInt());
    
    //when using inferred I want any features z axis csys parameter to take precedence.
    std::optional<osg::Vec3d> inferredFromFeature; //even if we don't use it.
    std::optional<osg::Vec3d> inferredFromFace; //even if we don't use it.
    WareHouse wareHouse; //another temp id mapping.
    occt::ShapeVector profileShapes;
    occt::ShapeVector edges; //We try to 'sew' edges to wires.
    tls::Resolver resolver(pIn);
    for (const auto &p : stow->profilePicks.getPicks())
    {
      if (!resolver.resolve(p))
      {
        std::ostringstream s; s << "Warning: Profile pick resolution failed" << std::endl;
        lastUpdateLog += s.str();
        continue;
      }
      
      const ann::SeerShape *css = resolver.getSeerShape(); assert(css);
      occt::ShapeVector currentShapes;
      if (slc::isObjectType(resolver.getPick().selectionType))
      {
        currentShapes = resolver.getSeerShape()->useGetNonCompoundChildren();
        if (!inferredFromFeature)
        {
          auto csysPrms = resolver.getFeature()->getParameters(prm::Tags::CSys);
          if (!csysPrms.empty())
            inferredFromFeature = gu::getZVector(csysPrms.front()->getMatrix());
        }
      }
      else
        currentShapes = resolver.getShapes();
      
      for (const auto &cs : currentShapes)
      {
        auto wareHouseIn = [&]()
        {
          wareHouse.getOrCreate(cs, css->findId(cs));
          for (const auto &ts : occt::mapShapes(cs))
            wareHouse.getOrCreate(ts, css->findId(ts));
        };
        
        auto t= cs.ShapeType();
        if (t == TopAbs_SHELL || t == TopAbs_VERTEX)
        {
          profileShapes.push_back(cs);
          wareHouseIn();
        }
        else if (t == TopAbs_FACE)
        {
          if (!inferredFromFace)
          {
            auto gleaned = occt::gleanAxis(cs);
            if (gleaned.second)
              inferredFromFace = gu::toOsg(gleaned.first.Direction());
          }
          profileShapes.push_back(cs);
          wareHouseIn();
        }
        else if (cs.ShapeType() == TopAbs_EDGE)
        {
          edges.push_back(cs);
          wareHouseIn();
        }
        else if (t == TopAbs_WIRE)
        {
          wareHouseIn();
          if (stow->solid.getBool() && cs.Closed())
          {
            occt::WireVector wv{TopoDS::Wire(cs)};
            TopoDS_Face wireFace = occt::buildFace(wv);
            if (!wireFace.IsNull())
            {
              profileShapes.push_back(wireFace);
              
              auto wireId = resolver.getSeerShape()->findId(cs);
              auto result = stow->oWireMap.insert(std::make_pair(wireId, gu::createRandomId())).first;
              auto &u = wareHouse.getOrCreate(wireFace); //leave inId nil
              u.ids.emplace_back(result->second);
              if (!inferredFromFace)
              {
                auto gleaned = occt::gleanAxis(wireFace);
                if (gleaned.second)
                  inferredFromFace = gu::toOsg(gleaned.first.Direction());
              }
            }
            else
            {
              std::ostringstream s; s << "Warning: Couldn't build face from closed wire" << std::endl;
              lastUpdateLog += s.str();
              profileShapes.push_back(cs);
            }
          }
          else
          {
            profileShapes.push_back(cs);
          }
        }
      }
    }
    
    //try to 'sew' edges into wires. This only works for planar edges. I would like
    //to make it work for non-planar edges, but going to have to save that for later.
    //don't have to worry about edge ids as they have been processed.
    if (edges.size() == 1)
      profileShapes.push_back(edges.front());
    else if (!edges.empty())
    {
      TopoDS_Compound wiresOut;
      int result = BOPAlgo_Tools::EdgesToWires(static_cast<TopoDS_Compound>(occt::ShapeVectorCast(edges)), wiresOut);
      if (result != 0)
      {
        //didn't work, add edges back to shapes to extrude.
        std::copy(edges.begin(), edges.end(), std::back_inserter(profileShapes));
      }
      else
      {
        occt::ShapeVector wo = static_cast<occt::ShapeVector>(occt::ShapeVectorCast(wiresOut));
        for (auto &w : wo) //no const might set closed because EdgesToWires doesn't
        {
          if (BRep_Tool::IsClosed(w))
            w.Closed(true);
          occt::EdgeVector ev = static_cast<occt::EdgeVector>(occt::ShapeVectorCast(occt::mapShapes(w)));
          auto mapWire = [&]() -> uuid
          {
            std::set<uuid> edgeSet;
            for (const auto &e : ev)
            {
              const auto &eUnit = wareHouse.getOrCreate(e);
              assert(!eUnit.ids.empty() && !eUnit.ids.front().is_nil()); //edge should have been added.
              edgeSet.insert(eUnit.ids.front());
            }
            auto result = stow->setMap.insert(std::make_pair(edgeSet, gu::createRandomId())).first;
            auto &u = wareHouse.getOrCreate(w);
            u.ids.emplace_back(result->second);
            return u.ids.back();
          };
          auto cleanAndPushWire = [&]()
          {
            //it appears that we get only wires. So we have some 1 edge wires which extrudes to a shell.
            //so here we will detect that condition and avoid it.
            if (ev.size() == 1)
              profileShapes.push_back(ev.front());
            else
            {
              mapWire();
              profileShapes.push_back(w);
            }
          };
          assert(w.ShapeType() == TopAbs_WIRE);
          if (stow->solid.getBool() && w.Closed())
          {
            //we want to build a face.
            occt::WireVector wv{TopoDS::Wire(w)};
            TopoDS_Face wireFace = occt::buildFace(wv);
            if (!wireFace.IsNull())
            {
              auto wId = mapWire();
              auto &faceUnit = wareHouse.getOrCreate(wireFace);
              auto result = stow->oWireMap.insert(std::make_pair(wId, gu::createRandomId())).first;
              faceUnit.ids.emplace_back(result->second);
              profileShapes.push_back(wireFace);
            }
            else
              cleanAndPushWire();
          }
          else
            cleanAndPushWire();
        }
      }
    }
    
    if (profileShapes.empty())
      throw std::runtime_error("Couldn't resolve any profile shapes");
    
    if (currentType == ExtrusionType::Infer)
    {
      if (inferredFromFeature)
        stow->direction.setValue(*inferredFromFeature);
      else if (inferredFromFace)
        stow->direction.setValue(*inferredFromFace);
      else
      {
        //ok we didn't get a direction from any csys feature parameters or from faces
        //so try to get it from the profile geometry.
        auto shapeDir = inferDirection(profileShapes);
        if (shapeDir)
          stow->direction.setValue(*shapeDir);
        else
          throw std::runtime_error("Couldn't infer direction");
      }
    }
    else if (currentType == ExtrusionType::Picks)
    {
      const auto &axisPicks = stow->axisPicks.getPicks();
      if (axisPicks.size() == 1)
      {
        if (slc::isPointType(axisPicks.front().selectionType))
          throw std::runtime_error("Can't determine direction from 1 point");
        tls::Resolver aResolver(pIn);
        if (!aResolver.resolve(axisPicks.front()))
          throw std::runtime_error("Resolution failure for first axis pick");
        if (slc::isObjectType(axisPicks.front().selectionType))
        {
          auto directionPrms = resolver.getFeature()->getParameters(prm::Tags::Direction);
          auto csysPrms = aResolver.getFeature()->getParameters(prm::Tags::CSys);
          if (!directionPrms.empty())
            stow->direction.setValue(directionPrms.front()->getVector());
          else if(!csysPrms.empty())
            stow->direction.setValue(gu::getZVector(csysPrms.front()->getMatrix()));
        }
        else 
        {
          auto shapes = aResolver.getShapes();
          if (shapes.empty())
            throw std::runtime_error("No resolved shapes for 1 axis pick");
          if (shapes.size() > 1)
          {
            std::ostringstream s; s << "WARNING: More than 1 shape resolved for 1 axis pick." << std::endl;
            lastUpdateLog += s.str();
          }
          auto glean = occt::gleanAxis(shapes.front());
          if (glean.second)
            stow->direction.setValue(gu::toOsg(gp_Vec(glean.first.Direction())));
          else
            throw std::runtime_error("Couldn't glean direction for 1 axis pick");
        }
      }
      else if (axisPicks.size() == 2)
      {
        if (!slc::isPointType(axisPicks.front().selectionType) || !slc::isPointType(axisPicks.back().selectionType))
          throw std::runtime_error("Wrong type for 2 pick axis");
        tls::Resolver resolver(pIn);
        resolver.resolve(axisPicks.front());
        auto points0 = resolver.getPoints();
        resolver.resolve(axisPicks.back());
        auto points1 = resolver.getPoints();
        if (points0.empty() || points1.empty())
          throw std::runtime_error("Failed to resolve 2 axis pick points");
        if (points0.size() > 1 || points1.size() > 1)
        {
          std::ostringstream s; s << "WARNING: More than 1 shape resolved for 2 pick axis." << std::endl;
          lastUpdateLog += s.str();
        }
        osg::Vec3d tAxis = points0.front() - points1.front();
        tAxis.normalize();
        stow->direction.setValue(tAxis);
      }
      else
        throw std::runtime_error("wrong number of axis picks");
    }
   
    //make sure that distance > offset.
    if (stow->offset.getDouble() >= stow->distance.getDouble())
      throw std::runtime_error("offset needs to be less than distance");
    //force direction to be unit vector.
    osg::Vec3d td = stow->direction.getVector();
    td.normalize();
    if (td.length() < std::numeric_limits<float>::epsilon())
      throw std::runtime_error("direction is zero vector");
    
    occt::BoundingBox bb;
    bb.add(profileShapes);
    stow->updateLabels(bb);
    
    //put all shapes into a compound so we have 1 extrusion
    //this should make mapping a little easier.
    TopoDS_Compound stec = static_cast<TopoDS_Compound>(occt::ShapeVectorCast(profileShapes));
    
    //store offset ids for use after we move the shape.
    occt::ShapeVector previousMap = occt::mapShapes(stec);
    int offset = -1;
    for (const auto &s : previousMap)
    {
      offset++;
      if (s.ShapeType() == TopAbs_COMPOUND)
        continue; //ignore wrapping compound.
      auto &unit = wareHouse.get(s);
      if (!unit.isValid())
      {
        std::ostringstream s; s << "WARNING: Have profile geometry not in warehouse" << std::endl;
        lastUpdateLog += s.str();
        continue;
      }
      unit.offset = offset;
    }
    
    double lo = stow->offset.getDouble();
    double ld = stow->distance.getDouble();
    osg::Vec3d ldir = stow->direction.getVector();
    if (stow->reverse.getBool())
      ldir = -ldir;
    
    //move shape, if necessary, and extrude
    //moving of shape will mess up id mapping.
    if (lo != 0.0)
    {
      occt::moveShape(stec, gu::toOcc(ldir), lo);
      int offset = -1;
      for (const auto &s : occt::mapShapes(stec))
      {
        offset++;
        if (s.ShapeType() == TopAbs_COMPOUND)
          continue; //skip wrapping compound.
        auto &oUnit = wareHouse.get(offset);
        if (!oUnit.isValid())
        {
          std::ostringstream s; s << "WARNING: No unit for offset in warehouse" << std::endl;
          lastUpdateLog += s.str();
          continue;
        }
        oUnit.shapes.push_back(s);
      }
    }
    BRepPrimAPI_MakePrism extruder(stec, gu::toOcc(ldir * (ld - lo)));
    if (!extruder.IsDone())
      throw std::runtime_error("Extrusion operation failed");
    
    stow->sShape.setOCCTShape(extruder.Shape(), getId());
    
    for (const auto &u : wareHouse.units)
    {
      assert(u.isValid());
      const auto &mappingId = u.ids.back(); //always map from outId.
      
      if (!stow->sShape.hasShape(u.shapes.back()))
      {
        /* this is happens when we extrude a wire, absent its' face, as
         * the wire itself is missing from the results. So only warn if it is
         * not a wire.
         */
        if (u.shapes.back().ShapeType() != TopAbs_WIRE)
        {
          std::ostringstream s; s << "WARNING: No Shape in SeerShape for warehouse shape" << std::endl;
          lastUpdateLog += s.str();
        }
        continue;
      }
      stow->sShape.updateId(u.shapes.back(), mappingId);
      
      /* update ids for 'sides. notes for extruding one face
       * face to solid
       * edge to face
       * vertex to edge
       * no wires to shell!
       */
      const TopTools_ListOfShape &generated = extruder.Generated(u.shapes.back());
      if (!generated.IsEmpty() && stow->sShape.hasShape(generated.First()))
      {
        auto gmIt = stow->generatedMap.insert(std::make_pair(mappingId, gu::createRandomId())).first;
        stow->sShape.updateId(generated.First(), gmIt->second);
        stow->sShape.insertEvolve(gu::createNilId(), gmIt->second);
      }
      
      /* update ids for 'top'. notes for extruding one face
       * face to face
       * wire to wire
       * edge to edge
       * vertex to vertex
       */
      const TopoDS_Shape &last = extruder.LastShape(u.shapes.back());
      if (!last.IsNull() && stow->sShape.hasShape(last))
      {
        auto lsIt = stow->lastMap.insert(std::make_pair(mappingId, gu::createRandomId())).first;
        stow->sShape.updateId(last, lsIt->second);
        stow->sShape.insertEvolve(gu::createNilId(), lsIt->second);
      }
    }
    
    //generated maps a face to a solid but we get no shell mapping, so this:
    auto solids = stow->sShape.useGetChildrenOfType(stow->sShape.getRootOCCTShape(), TopAbs_SOLID);
    for (const auto &s : solids)
    {
      const auto &shell = BRepClass3d::OuterShell(TopoDS::Solid(s));
      if (!stow->sShape.findId(shell).is_nil())
        continue;
      auto it = stow->oWireMap.insert(std::make_pair(stow->sShape.findId(s), gu::createRandomId())).first;
      stow->sShape.updateId(shell, it->second);
      stow->sShape.insertEvolve(gu::createNilId(), it->second);
    }
    
    //ok when we extrude a wire and don't get a solid but a shell so this:
    auto shells = stow->sShape.useGetChildrenOfType(stow->sShape.getRootOCCTShape(), TopAbs_SHELL);
    for (const auto &s : shells)
    {
      if (!stow->sShape.findId(s).is_nil())
        continue;
      auto faces = stow->sShape.useGetChildrenOfType(s, TopAbs_FACE);
      decltype(stow->setMap)::key_type faceSet;
      for (const auto &f : faces)
      {
        const auto& fId = stow->sShape.findId(f);
        if (!fId.is_nil())
          faceSet.insert(stow->sShape.findId(f));
      }
      if (faceSet.empty())
        continue;
      auto it = stow->setMap.insert(std::make_pair(faceSet, gu::createRandomId())).first;
      stow->sShape.updateId(s, it->second);
      stow->sShape.insertEvolve(gu::createNilId(), it->second);
    }
    
    //this should assign ids to nil outerwires based upon parent face id.
    occt::ShapeVector ns = stow->sShape.getAllNilShapes();
    for (const auto &s : ns)
    {
      if (s.ShapeType() != TopAbs_WIRE)
        continue;
      occt::ShapeVector ps = stow->sShape.useGetParentsOfType(s, TopAbs_FACE);
      if (ps.empty())
        continue;
      uuid fId = stow->sShape.findId(ps.front());
      if (fId.is_nil())
        continue;
      
      auto it = stow->oWireMap.insert(std::make_pair(fId, gu::createRandomId())).first;
      stow->sShape.updateId(s, it->second);
      stow->sShape.insertEvolve(gu::createNilId(), it->second);
    }
    
    
//     stow->sShape.dumpShapeIdContainer(std::cout);
    
    stow->sShape.derivedMatch();
//     stow->sShape.uniqueTypeMatch(*tResolver.getSeerShape());
    stow->sShape.dumpNils("extrude feature");
    stow->sShape.dumpDuplicates("extrude feature");
    stow->sShape.ensureNoNils();
    stow->sShape.ensureNoDuplicates();
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in extrude update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in extrude update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in extrude update. " << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Feature::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::exrs::Extrude so
  (
    Base::serialOut(),
    stow->sShape.serialOut(),
    stow->extrusionType.serialOut(),
    stow->profilePicks.serialOut(),
    stow->direction.serialOut(),
    stow->directionLabel->serialOut(),
    stow->distance.serialOut(),
    stow->distanceLabel->serialOut(),
    stow->offset.serialOut(),
    stow->offsetLabel->serialOut(),
    stow->solid.serialOut(),
    stow->solidLabel->serialOut(),
    stow->reverse.serialOut(),
    stow->reverseLabel->serialOut()
  );
  
  if (stow->extrusionType.getInt() == static_cast<int>(ExtrusionType::Picks))
    so.axisPicks() = stow->axisPicks.serialOut();
  
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
  so.originalMap() = serializeMap(stow->originalMap);
  so.generatedMap() = serializeMap(stow->generatedMap);
  so.lastMap() = serializeMap(stow->lastMap);
  so.oWireMap() = serializeMap(stow->oWireMap);
  
  for (const auto &r : stow->setMap)
  {
    prj::srl::spt::DerivedRecord ro(gu::idToString(r.second));
    for (const auto &s : r.first)
      ro.idSet().push_back(gu::idToString(s));
    so.setMap().push_back(ro);
  }
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::exrs::extrude(stream, so, infoMap);
}

void Feature::serialRead(const prj::srl::exrs::Extrude &so)
{
  
  Base::serialIn(so.base());
  stow->sShape.serialIn(so.seerShape());
  stow->extrusionType.serialIn(so.extrusionType());
  stow->profilePicks.serialIn(so.profilePicks());
  stow->direction.serialIn(so.direction());
  stow->directionLabel->serialIn(so.directionLabel());
  stow->distance.serialIn(so.distance());
  stow->distanceLabel->serialIn(so.distanceLabel());
  stow->offset.serialIn(so.offset());
  stow->offsetLabel->serialIn(so.offsetLabel());
  stow->solid.serialIn(so.solid());
  stow->solidLabel->serialIn(so.solidLabel());
  stow->reverse.serialIn(so.reverse());
  stow->reverseLabel->serialIn(so.reverseLabel());
  
  if (so.axisPicks().present())
    stow->axisPicks.serialIn(so.axisPicks().get());
  
  auto serializeMap = [](const prj::srl::spt::SeerShape::EvolveContainerSequence &container) -> std::map<uuid, uuid>
  {
    std::map<uuid, uuid> out;
    for (const auto &r : container)
      out.insert(std::make_pair(gu::stringToId(r.idIn()), gu::stringToId(r.idOut())));
    return out;
  };
  stow->originalMap = serializeMap(so.originalMap());
  stow->generatedMap = serializeMap(so.generatedMap());
  stow->lastMap = serializeMap(so.lastMap());
  stow->oWireMap = serializeMap(so.oWireMap());
  
  for (const auto &p : so.setMap())
  {
    std::set<uuid> set;
    for (const auto &sids : p.idSet())
      set.insert(gu::stringToId(sids));
    stow->setMap.insert(std::make_pair(set, gu::stringToId(p.id())));
  }
}
