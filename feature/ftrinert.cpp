/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2015  Thomas S. Anderson blobfish.at.gmx.com
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

#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <gp_Ax3.hxx>
#include <gp_Trsf.hxx>
#include <TopLoc_Location.hxx>

#include "project/serial/generated/prjsrlintsinert.h"
#include "globalutilities.h"
#include "tools/idtools.h"
#include "tools/occtools.h"
#include "library/lbrcsysdragger.h"
#include "annex/annseershape.h"
#include "annex/anncsysdragger.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "feature/ftrprimitive.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinert.h"

using namespace ftr::Inert;
using namespace boost::uuids;

QIcon Feature::icon;

struct Feature::Stow
{
  Feature &feature;
  Primitive primitive;
  osg::Matrixd cachedMatrix; //detect csys change
  
  Stow() = delete;
  Stow(Feature &fIn)
  : feature(fIn)
  , primitive(Primitive::Input{fIn, fIn.parameters, fIn.annexes})
  {
  }
};

Feature::Feature(const TopoDS_Shape &shapeIn)
// : Feature(shapeIn, gu::toOsg(shapeIn.Location().Transformation()))
: Feature(shapeIn, osg::Matrixd::identity())
{}

Feature::Feature(const TopoDS_Shape &shapeIn, const osg::Matrixd &mIn)
: Base()
, stow(std::make_unique<Stow>(*this))
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionInert.svg");
  
  name = QObject::tr("Inert");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  stow->primitive.sShape.setOCCTShape(shapeIn, getId());
  stow->primitive.sShape.ensureNoNils();
  
  stow->primitive.csys.setValue(mIn);
  stow->primitive.csysDragger.draggerUpdate(); //set dragger to parameter.
  stow->cachedMatrix = mIn;
}

Feature::~Feature() = default;

void Feature::updateModel(const UpdatePayload&)
{
  try
  {
    setFailure();
    lastUpdateLog.clear();
    
    //store a map of offset to id for restoration.
    std::vector<uuid> oldIds;
    occt::ShapeVector shapes = occt::mapShapes(stow->primitive.sShape.getRootOCCTShape());
    for (const auto &s : shapes)
    {
      if (stow->primitive.sShape.hasShape(s))
      {
        oldIds.push_back(stow->primitive.sShape.findId(s));
      }
      else
      {
        std::cerr << "WARNING: shape is not in shapeId container in Feature::updateModel" << std::endl;
        oldIds.push_back(gu::createNilId()); //place holder will be skipped again.
      }
    }
    uuid oldRootId = stow->primitive.sShape.getRootShapeId();
    
    const osg::Matrixd &prmMatrix = stow->primitive.csys.getMatrix();
    osg::Matrixd diff = osg::Matrixd::inverse(stow->cachedMatrix) * prmMatrix;
    
    TopoDS_Shape tempShape(stow->primitive.sShape.getRootOCCTShape());
    osg::Matrixd shapeMatrix = gu::toOsg(tempShape.Location().Transformation());
    osg::Matrixd freshMatrix = shapeMatrix * diff;
    //I have no idea why I have to 'inverse' here. I don't have to inverse when I pull the matrix out of the shape.
    gp_Ax3 tempAx3(gu::toOcc(osg::Matrixd::inverse(freshMatrix)));
    gp_Trsf tempTrsf;
    tempTrsf.SetTransformation(tempAx3);
    TopLoc_Location freshLocation(tempTrsf);
    tempShape.Location(freshLocation);
    stow->primitive.sShape.setOCCTShape(tempShape, getId());
    stow->primitive.sShape.updateId(stow->primitive.sShape.getRootOCCTShape(), oldRootId);
    stow->primitive.sShape.setRootShapeId(oldRootId);
    
    shapes = occt::mapShapes(stow->primitive.sShape.getRootOCCTShape());
    std::size_t count = 0;
    for (const auto &s : shapes)
    {
      if (stow->primitive.sShape.hasShape(s))
      {
        stow->primitive.sShape.updateId(s, oldIds.at(count));
      }
      else
      {
        std::cerr << "WARNING: shape is not in moved shapeId container in Feature::updateModel" << std::endl;
      }
      count++;
    }
    
    stow->primitive.sShape.dumpNils("inert");
    stow->primitive.sShape.dumpDuplicates("inert");
    
    stow->primitive.sShape.ensureNoNils();
    stow->primitive.sShape.ensureNoDuplicates();
    
    mainTransform->setMatrix(osg::Matrixd::identity());
    stow->cachedMatrix = stow->primitive.csys.getMatrix();
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in inert update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in inert update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in inert update." << std::endl;
    lastUpdateLog += s.str();
  }
  
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Feature::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::ints::Inert inertOut
  (
    Base::serialOut(),
    stow->primitive.sShape.serialOut(),
    stow->primitive.csys.serialOut(),
    stow->primitive.csysDragger.serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::ints::inert(stream, inertOut, infoMap);
}

void Feature::serialRead(const prj::srl::ints::Inert& inert)
{
  Base::serialIn(inert.base());
  stow->primitive.sShape.serialIn(inert.seerShape());
  stow->primitive.csys.serialIn(inert.csys());
  stow->primitive.csysDragger.serialIn(inert.csysDragger());
  stow->cachedMatrix = stow->primitive.csys.getMatrix();
}
