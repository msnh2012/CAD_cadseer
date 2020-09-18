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
// #include "parameter/prmparameter.h"
#include "tools/occtools.h"
// #include "tools/featuretools.h"
// #include "tools/idtools.h"
// #include "feature/ftrshapecheck.h"
#include "feature/ftrupdatepayload.h"
// #include "feature/ftrinputtype.h"
// #include "project/serial/generated/prjsrl_FIX_%CLASSNAMELOWERCASE%.h"
#include "feature/ftr%CLASSNAMELOWERCASE%.h"

using namespace ftr;
using boost::uuids::uuid;

QIcon %CLASSNAME%::icon;

%CLASSNAME%::%CLASSNAME%():
Base()
// , sShape(new ann::SeerShape())
// , direction(new prm::Parameter(prm::Names::Direction, osg::Vec3d(0.0, 0.0, 1.0)))
// , directionLabel(new lbr::PLabel(direction.get()))
// , distanceLabel(new lbr::IPGroup(distance.get()))
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionBase.svg"); //fix me
  
  name = QObject::tr("%CLASSNAME%");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
//   direction->connectValue(std::bind(&%CLASSNAME%::setModelDirty, this));
//   parameters.push_back(direction.get());
  
//   distance->connectValue(std::bind(&%CLASSNAME%::setModelDirty, this));
//   parameters.push_back(distance.get());
  
//   distanceLabel->showName = true;
//   distanceLabel->valueHasChanged();
//   distanceLabel->constantHasChanged();
//   overlaySwitch->addChild(distanceLabel.get());
  
//   annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
}

%CLASSNAME%::~%CLASSNAME%() = default;

// void %CLASSNAME%::setPicks(const Picks &pIn)
// {
//   picks = pIn;
//   setModelDirty();
// }

void %CLASSNAME%::updateModel(const UpdatePayload &/*pIn*/)
{
  setFailure();
  lastUpdateLog.clear();
//   sShape->reset();
  try
  {
//     auto getFeature = [&](const std::string& tagIn) -> const Base&
//     {
//       std::vector<const Base*> tfs = pIn.getFeatures(tagIn);
//       if (tfs.size() != 1)
//         throw std::runtime_error("wrong number of parents");
//       return *(tfs.front());
//     };
//     
//     auto getSeerShape = [&](const Base &bfIn) -> const ann::SeerShape&
//     {
//       if (!bfIn.hasAnnex(ann::Type::SeerShape))
//         throw std::runtime_error("parent doesn't have seer shape.");
//       const ann::SeerShape &tss = bfIn.getAnnex<ann::SeerShape>();
//       if (tss.isNull())
//         throw std::runtime_error("target seer shape is null");
//       return tss;
//     };
//     
//     const Base &tbf0 = getFeature(pickZero);
//     const ann::SeerShape &tss0 = getSeerShape(tbf0);
//     
//     const Base &tbf1 = getFeature(pickOne);
//     const ann::SeerShape &tss1 = getSeerShape(tbf1);
//     
//     tls::Resolver pr(pIn);
//     if (!pr.resolve(picks.front()))
//       throw std::runtime_error("invalid pick resolution");

    
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

void %CLASSNAME%::serialWrite(const boost::filesystem::path &/*dIn*/)
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

// void %CLASSNAME%::serialRead(const prj::srl::FIXME::%CLASSNAME% &so)
// {
//   Base::serialIn(so.base());
//   sShape->serialIn(so.seerShape());
//   pick.serialIn(so.pick());
//   for (const auto &p : so.picks())
//     picks.emplace_back(p);
//   direction->serialIn(so.direction());
//   directionLabel->serialIn(so.directionLabel());
//   distanceLabel->serialIn(so.distanceLabel());
//   
//   directionLabel->valueHasChanged();
//   directionLabel->constantHasChanged();
//   distanceLabel->valueHasChanged();
//   distanceLabel->constantHasChanged();
// }
