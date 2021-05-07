/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2021 Thomas S. Anderson blobfish.at.gmx.com
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


#include <osg/Switch>

#include "library/lbrplabel.h"
#include "library/lbripgroup.h"
#include "library/lbrcsysdragger.h"
// #include "project/serial/generated/prjsrl_FIX_primitive.h"
#include "feature/ftrprimitive.h"

using namespace ftr;


Primitive::Primitive(const Input &iIn)
: input(iIn)
, csysObserver(std::bind(&Base::setModelDirty, &input.feature))
, syncObserver(std::bind(&Primitive::prmActiveSync, this))
, csysDragger(&input.feature, &csys)
{
  QStringList tStrings = //keep in sync with enum in header.
  {
    QObject::tr("Constant")
    , QObject::tr("Linked")
  };
  csysType.setEnumeration(tStrings);
  csysType.connect(syncObserver);
  csysType.connectValue(std::bind(&Base::setModelDirty, &input.feature));
  input.prms.push_back(&csysType);
  
  csys.connect(csysObserver);
  input.prms.push_back(&csys);
  
  csysLinked.connectValue(std::bind(&Base::setModelDirty, &input.feature));
  csysLinked.setActive(false); //constant by default
  input.prms.push_back(&csysLinked);
  
  input.annexes.insert(std::make_pair(ann::Type::SeerShape, &sShape));
  input.annexes.insert(std::make_pair(ann::Type::CSysDragger, &csysDragger));
  
  input.feature.getOverlaySwitch()->addChild(csysDragger.dragger);
}

void Primitive::prmActiveSync()
{
  //shouldn't need to block anything
  switch (static_cast<CSysType>(csysType.getInt()))
  {
    case Constant:{
      csys.setActive(true);
      csysLinked.setActive(false);
      break;}
    case Linked:{
      csys.setActive(false);
      csysLinked.setActive(true);
      break;}
  }
}

lbr::IPGroup* Primitive::common(prm::Parameter *prmIn)
{
  prmIn->setConstraint(prm::Constraint::buildNonZeroPositive());
  input.prms.push_back(prmIn);
  prmIn->connectValue(std::bind(&Base::setModelDirty, &input.feature));
  
  auto out = new lbr::IPGroup(prmIn);
  csysDragger.dragger->linkToMatrix(out);
  input.feature.getOverlaySwitch()->addChild(out);
  
  return out;
}

prm::Parameter* Primitive::addLength(double value)
{
  length = prm::Parameter(prm::Names::Length, value, prm::Tags::Length);
  lengthIP = common(&*length);
  lengthIP->noAutoRotateDragger();
  return &*length;
}

prm::Parameter* Primitive::addWidth(double value)
{
  width = prm::Parameter(prm::Names::Width, value, prm::Tags::Width);
  widthIP = common(&*width);
  widthIP->noAutoRotateDragger();
  return &*width;
}

prm::Parameter* Primitive::addHeight(double value)
{
  height = prm::Parameter(prm::Names::Height, value, prm::Tags::Height);
  heightIP = common(&*height);
  heightIP->noAutoRotateDragger();
  return &*height;
}

prm::Parameter* Primitive::addRadius(double value)
{
  radius = prm::Parameter(prm::Names::Radius, value, prm::Tags::Radius);
  radiusIP = common(&*radius);
  return &*radius;
}

prm::Parameter* Primitive::addRadius1(double value)
{
  radius1 = prm::Parameter(prm::Names::Radius1, value, prm::Tags::Radius1);
  radius1IP = common(&*radius1);
  return &*radius1;
}

prm::Parameter* Primitive::addRadius2(double value)
{
  radius2 = prm::Parameter(prm::Names::Radius2, value, prm::Tags::Radius2);
  radius2IP = common(&*radius2);
  return &*radius2;
}

void Primitive::IPsToCsys()
{
  if (lengthIP)
    lengthIP->setMatrix(csys.getMatrix());
  if (widthIP)
    widthIP->setMatrix(csys.getMatrix());
  if (heightIP)
    heightIP->setMatrix(csys.getMatrix());
  if (radiusIP)
    radiusIP->setMatrix(csys.getMatrix());
  if (radius1IP)
    radius1IP->setMatrix(csys.getMatrix());
  if (radius2IP)
    radius2IP->setMatrix(csys.getMatrix());
}


/*
void Primitive::serialWrite(const boost::filesystem::path&)
{
  prj::srl::FIX::Primitive so
  (
    Base::serialOut()
    , sShape->serialOut()
    , pick.serialOut()
    , direction->serialOut()
    , directionLabel->serialOut()
  );
  for (const auto &p : picks)
    so.picks().push_back(p);
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::FIX::primitive(stream, so, infoMap);
}

void Primitive::serialRead(const prj::srl::FIXME::Primitive &so)
{
  Base::serialIn(so.base());
  sShape->serialIn(so.seerShape());
  pick.serialIn(so.pick());
  for (const auto &p : so.picks())
    picks.emplace_back(p);
  direction->serialIn(so.direction());
  directionLabel->serialIn(so.directionLabel());
  distanceLabel->serialIn(so.distanceLabel());
}
*/
