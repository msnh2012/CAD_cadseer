/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2022 Thomas S. Anderson blobfish.at.gmx.com
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

#include <BRepBuilderAPI_GTransform.hxx>
#include <gp_Ax3.hxx>

#include <osg/Switch>

#include "globalutilities.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "tools/occtools.h"
#include "tools/featuretools.h"
#include "library/lbrcsysdragger.h"
#include "feature/ftrshapecheck.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrprimitive.h"
#include "project/serial/generated/prjsrlmttsmutate.h"
#include "feature/ftrmutate.h"

using boost::uuids::uuid;
using namespace ftr::Mutate;
QIcon Feature::icon = QIcon(":/resources/images/constructionMutate.svg");

struct Feature::Stow
{
  Feature &feature;
  Primitive primitive;
  prm::Parameter scale{prm::Names::Scale, osg::Vec3d(2.0, 1.0, 1.0), prm::Tags::Scale};
  prm::Parameter shapePicks{QObject::tr("Shape Picks"), ftr::Picks(), PrmTags::shape};
  
  osg::ref_ptr<lbr::PLabel> scaleLabel{new lbr::PLabel(&scale)};
  
  Stow(Feature &fIn)
  : feature(fIn)
  , primitive(Primitive::Input{fIn, fIn.parameters, fIn.annexes})
  {
    scale.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&scale);
    
    shapePicks.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&shapePicks);
    
    feature.overlaySwitch->addChild(scaleLabel.get());
    
    //we don't want the dragger moving the main transform.
    //therefore have no concept of linking dragger to feature.
    primitive.csysDragger.dragger->unlinkToMatrix(feature.getMainTransform());
    primitive.csysDragger.dragger->hide(lbr::CSysDragger::SwitchIndexes::LinkIcon);
  }
};

Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  name = QObject::tr("Mutate");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
}

Feature::~Feature() = default;

void Feature::updateModel(const UpdatePayload &pIn)
{
  setFailure();
  lastUpdateLog.clear();
  stow->primitive.sShape.reset();
  try
  {
    tls::Resolver resolver(pIn);
    const auto &ps = stow->shapePicks.getPicks();
    if (ps.empty())
      throw std::runtime_error("No input picks");
    //setup new failure state.
    if (!resolver.resolve(ps.front()))
      throw std::runtime_error("Failed to resolve input pick");
    const auto &tss = *resolver.getSeerShape();
    auto shapes = resolver.getShapes();
    if (shapes.empty() || shapes.front().IsNull())
      throw std::runtime_error("Invalid input shape");
    
    auto setFailureState = [&]()
    {
      stow->primitive.sShape.setOCCTShape(shapes.front(), getId());
      stow->primitive.sShape.shapeMatch(tss);
      stow->primitive.sShape.uniqueTypeMatch(tss);
      stow->primitive.sShape.outerWireMatch(tss);
      stow->primitive.sShape.derivedMatch();
      stow->primitive.sShape.ensureNoNils(); //just in case
      stow->primitive.sShape.ensureNoDuplicates(); //just in case
    };
    
    auto bail = [&](std::string_view m)
    {
      setFailureState();
      throw std::runtime_error(m.data());
    };
    
    if (isSkipped())
    {
      setSuccess();
      bail("feature is skipped");
    }
    
    //set csys if linked. nothing for constant.
    if (static_cast<Primitive::CSysType>(stow->primitive.csysType.getInt()) == Primitive::Linked)
    {
      const auto &picks = stow->primitive.csysLinked.getPicks();
      if (picks.empty())
        bail("No picks for csys link");
      tls::Resolver resolver(pIn);
      if (!resolver.resolve(picks.front()))
        bail("Couldn't resolve csys link pick");
      auto csysPrms = resolver.getFeature()->getParameters(prm::Tags::CSys);
      if (csysPrms.empty())
        bail("csys link feature has no csys parameter");
      prm::ObserverBlocker(stow->primitive.csysObserver);
      stow->primitive.csys.setValue(csysPrms.front()->getMatrix());
      stow->primitive.csysDragger.draggerUpdate();
    }
    
    //man occt sucks! Make more classes to represent transformations and make
    //sure none of them are compatible to any other.
    osg::Matrixd temp = stow->primitive.csys.getMatrix();
    osg::Matrixd m = osg::Matrixd::inverse(temp) * osg::Matrixd::scale(stow->scale.getVector()) * temp;
    
    //note: gp_GTrsf doesn't support editing 4th row.
    gp_GTrsf t;
    t.SetValue(1, 1, m(0, 0));
    t.SetValue(2, 1, m(0, 1));
    t.SetValue(3, 1, m(0, 2));
//     t.SetValue(4, 1, m(0, 3));
    
    t.SetValue(1, 2, m(1, 0));
    t.SetValue(2, 2, m(1, 1));
    t.SetValue(3, 2, m(1, 2));
//     t.SetValue(4, 2, m(1, 3));
    
    t.SetValue(1, 3, m(2, 0));
    t.SetValue(2, 3, m(2, 1));
    t.SetValue(3, 3, m(2, 2));
//     t.SetValue(4, 3, m(2, 3));
    
    t.SetValue(1, 4, m(3, 0));
    t.SetValue(2, 4, m(3, 1));
    t.SetValue(3, 4, m(3, 2));
//     t.SetValue(4, 4, m(3, 3));
    
    BRepBuilderAPI_GTransform mutater(shapes.front(), t, Standard_False);
    if (!mutater.IsDone())
      bail("BRepBuilderAPI_GTransform is not done");
    
    auto mutatedShape = occt::compoundWrap(mutater.Shape());
    ShapeCheck check(mutatedShape);
    if (!check.isValid())
      bail("shapeCheck failed");
    
    //modifiedMatch does all we need.
    stow->primitive.sShape.setOCCTShape(mutatedShape, getId());
    stow->primitive.sShape.modifiedMatch(mutater, tss);
//     stow->primitive.sShape.dumpNils("mutate feature");
//     stow->primitive.sShape.dumpDuplicates("mutate feature");
    stow->primitive.sShape.ensureNoNils();
    stow->primitive.sShape.ensureNoDuplicates();
    stow->primitive.sShape.ensureEvolve();
    
    occt::BoundingBox bb(mutatedShape);
    stow->scaleLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(bb.getCenter())));
    
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
  prj::srl::mtts::Mutate so
  (
    Base::serialOut()
    , stow->scale.serialOut()
    , stow->shapePicks.serialOut()
    , stow->primitive.csysType.serialOut()
    , stow->primitive.csys.serialOut()
    , stow->primitive.csysLinked.serialOut()
    , stow->primitive.sShape.serialOut()
    , stow->scaleLabel->serialOut()
    , stow->primitive.csysDragger.serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::mtts::mutate(stream, so, infoMap);
}

void Feature::serialRead(const prj::srl::mtts::Mutate &so)
{
  Base::serialIn(so.base());
  stow->scale.serialIn(so.scale());
  stow->shapePicks.serialIn(so.shapePicks());
  stow->primitive.csysType.serialIn(so.csysType());
  stow->primitive.csys.serialIn(so.csys());
  stow->primitive.csysLinked.serialIn(so.csysLinked());
  stow->primitive.sShape.serialIn(so.seerShape());
  stow->scaleLabel->serialIn(so.scaleLabel());
  stow->primitive.csysDragger.serialIn(so.csysDragger());
}
