/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) %YEAR% Thomas S. Anderson blobfish.at.gmx.com
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

#include "globalutilities.h"
// #include "annex/annseershape.h"
// #include "library/lbrplabel.h"
// #include "library/lbripgroup.h"
// #include "parameter/prmconstants.h"
// #include "parameter/prmparameter.h"
#include "tools/occtools.h"
// #include "tools/featuretools.h"
// #include "tools/idtools.h"
// #include "feature/ftrshapecheck.h"
#include "feature/ftrupdatepayload.h"
// #include "feature/ftrinputtype.h"
// #include "project/serial/generated/prjsrl_FIX_%CLASSNAMELOWERCASE%.h"
#include "feature/ftr%CLASSNAMELOWERCASE%.h"

using boost::uuids::uuid;
using namespace ftr::%CLASSNAME%;
QIcon Feature::icon = QIcon(":/resources/images/constructionBase.svg"); //fix me

struct Feature::Stow
{
  Feature &feature;
  ann::SeerShape sShape;
  prm::Parameter parameter;
  prm::Observer observer;
  osg::ref_ptr<lbr::PLabel> label;
  osg::ref_ptr<lbr::IPGroup> iLabel;
  
  Stow(Feature &fIn)
  : feature(fIn)
  , sShape()
  , parameter(prm::Names::parameter, 1.0, prm::Tags::parameter)
  , observer(std::bind(&Feature::setModelDirty, &feature))
  , label(new lbr::PLabel(&parameter))
  , iLabel(new lbr::IPGroup(&parameter))
  {
    QStringList tStrings =
    {
      QObject::tr("Constant")
      , QObject::tr("Infer")
      , QObject::tr("Picks")
    };
    parameter.setEnumeration(tStrings);
    parameter.connectValue(std::bind(&Feature::setModelDirty, &feature));
    parameter.connectValue(std::bind(&Stow::prmActiveSync, this));
    parameter.connect(observer);
    feature.parameters.push_back(&parametr);
    
    feature.annexes.insert(std::make_pair(ann::Type::SeerShape, &sShape));
    
    feature.overlaySwitch->addChild(label.get());
    feature.overlaySwitch->addChild(iLabel.get());
  }
  
  void prmActiveSync()
  {

  }
};

Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  name = QObject::tr("%CLASSNAME%");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
}

Feature::~Feature() = default;

void Feature::updateModel(const UpdatePayload &/*pIn*/)
{
  setFailure();
  lastUpdateLog.clear();
//   sShape->reset();
  try
  {
    //setup new failure state.
//     sShape->setOCCTShape(tss.getRootOCCTShape());
//     sShape->shapeMatch(tss);
//     sShape->uniqueTypeMatch(tss);
//     sShape->outerWireMatch(tss);
//     sShape->derivedMatch();
//     sShape->ensureNoNils(); //just in case
//     sShape->ensureNoDuplicates(); //just in case
    
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
//     tls::Resolver pr(pIn);
//     if (!pr.resolve(picks.front()))
//       throw std::runtime_error("invalid pick resolution");
    
    //update goes here.
    
//     ShapeCheck check(out);
//     if (!check.isValid())
//       throw std::runtime_error("shapeCheck failed");
    
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

void Feature::serialWrite(const boost::filesystem::path &/*dIn*/)
{
//   prj::srl::FIX::%CLASSNAME% so
//   (
//     Base::serialOut()
//     , sShape->serialOut()
//     , pick.serialOut()
//     , direction->serialOut()
//     , directionLabel->serialOut()
//   );
//   for (const auto &p : picks)
//     so.picks().push_back(p);
//   
//   xml_schema::NamespaceInfomap infoMap;
//   std::ofstream stream(buildFilePathName(dIn).string());
//   prj::srl::FIX::%CLASSNAMELOWERCASE%(stream, so, infoMap);
}

// void Feature::serialRead(const prj::srl::FIXME::%CLASSNAME% &so)
// {
//   Base::serialIn(so.base());
//   sShape->serialIn(so.seerShape());
//   pick.serialIn(so.pick());
//   for (const auto &p : so.picks())
//     picks.emplace_back(p);
//   direction->serialIn(so.direction());
//   directionLabel->serialIn(so.directionLabel());
//   distanceLabel->serialIn(so.distanceLabel());
// }
