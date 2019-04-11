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
// #include "annex/seershape.h"
// #include "library/plabel.h"
// #include "library/ipgroup.h"
// #include "parameter/parameter.h"
#include "tools/occtools.h"
// #include "tools/featuretools.h"
// #include "tools/idtools.h"
// #include "feature/shapecheck.h"
#include "feature/updatepayload.h"
// #include "feature/inputtype.h"
// #include "project/serial/xsdcxxoutput/feature%CLASSNAMELOWERCASE%.h"
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
  
//   annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
}

%CLASSNAME%::~%CLASSNAME%(){}

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
//     std::vector<const Base*> tfs = pIn.getFeatures(InputType::target);
//     if (tfs.size() != 1)
//       throw std::runtime_error("wrong number of parents");
//     if (!tfs.front()->hasAnnex(ann::Type::SeerShape))
//       throw std::runtime_error("parent doesn't have seer shape.");
//     const ann::SeerShape &tss = tfs.front()->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
//     if (tss.isNull())
//       throw std::runtime_error("target seer shape is null");
    
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
//   prj::srl::Feature%CLASSNAME% so
//   (
// 
//   );
//   
//   xml_schema::NamespaceInfomap infoMap;
//   std::ofstream stream(buildFilePathName(dIn).string());
//   prj::srl::%CLASSNAMELOWERCASE%(stream, so, infoMap);
}

// void %CLASSNAME%::serialRead(const prj::srl::Feature%CLASSNAME% &so)
// {
//   Base::serialIn(so.featureBase());
//   picks = ftr::serialIn(so.picks());
//   direction->serialIn(so.direction());
//   directionLabel->serialIn(so.directionLabel());
//   distanceLabel->serialIn(so.distanceLabel());
//   
//   directionLabel->valueHasChanged();
//   directionLabel->constantHasChanged();
//   distanceLabel->valueHasChanged();
//   distanceLabel->constantHasChanged();
// }
