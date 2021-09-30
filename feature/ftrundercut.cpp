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

#include <boost/filesystem/path.hpp>

#include <osg/Switch>
#include <osg/Geometry>

#include "globalutilities.h"
// #include "annex/annseershape.h"
#include "library/lbrplabel.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "tools/occtools.h"
#include "tools/featuretools.h"
#include "tools/tlsosgtools.h"
// #include "tools/idtools.h"
// #include "feature/ftrshapecheck.h"
#include "feature/ftrupdatepayload.h"
// #include "feature/ftrinputtype.h"
// #include "project/serial/generated/prjsrl_FIX_undercut.h"
#include "feature/ftrundercut.h"

using boost::uuids::uuid;
using namespace ftr::UnderCut;
QIcon Feature::icon = QIcon(":/resources/images/constructionBase.svg"); //fix me

struct Feature::Stow
{
  Feature &feature;
  prm::Parameter sourcePick{QObject::tr("Source"), ftr::Picks(), PrmTags::sourcePick};
  prm::Parameter directionType{QObject::tr("Type"), 0, PrmTags::directionType};
  prm::Parameter directionPicks{QObject::tr("Direction Picks"), ftr::Picks(), PrmTags::directionPicks};
  prm::Parameter direction{QObject::tr("Direction"), osg::Vec3d(), PrmTags::direction};
  prm::Parameter subdivision{QObject::tr("Subdivision"), 1, PrmTags::subdivision};
  osg::ref_ptr<lbr::PLabel> directionTypeLabel{new lbr::PLabel(&directionType)};
  osg::ref_ptr<lbr::PLabel> directionLabel{new lbr::PLabel(&direction)};
  osg::ref_ptr<lbr::PLabel> subdivisionLabel{new lbr::PLabel(&subdivision)};
  prm::Observer blockObserver{std::bind(&Feature::setModelDirty, &feature)};
  
  Stow(Feature &fIn)
  : feature(fIn)
  {
    sourcePick.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&sourcePick);
    
    QStringList tStrings =
    {
      QObject::tr("Picks")
      , QObject::tr("Constant")
    };
    directionType.setEnumeration(tStrings);
    directionType.connectValue(std::bind(&Feature::setModelDirty, &feature));
    directionType.connectValue(std::bind(&Stow::prmActiveSync, this));
    feature.parameters.push_back(&directionType);
    directionTypeLabel->refresh();
    
    directionPicks.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&directionPicks);
    
    prm::Boundary lower(1.0, prm::Boundary::End::Closed);
    prm::Boundary upper(10.0, prm::Boundary::End::Closed);
    prm::Interval interval(lower, upper);
    prm::Constraint subConstraint;
    subConstraint.intervals.push_back(interval);
    subdivision.setConstraint(subConstraint);
    subdivision.connectValue(std::bind(&Feature::setModelDirty, &feature));
    feature.parameters.push_back(&subdivision);
    
    direction.connect(blockObserver);
    feature.parameters.push_back(&direction);
    
    feature.overlaySwitch->addChild(directionTypeLabel.get());
    feature.overlaySwitch->addChild(directionLabel.get());
    feature.overlaySwitch->addChild(subdivisionLabel.get());
  }
  
  void prmActiveSync()
  {
    switch (directionType.getInt())
    {
      case 0:
      {
        directionPicks.setActive(true);
        direction.setActive(false);
        break;
      }
      case 1:
      {
        directionPicks.setActive(false);
        direction.setActive(true);
        break;
      }
      default:
      {
        assert(0); //unrecognized type
        break;
      }
    }
  }
  
  std::vector<osg::Vec3d> buildGrid(const osg::BoundingSphere &sphereIn)
  {
    std::vector<osg::Vec3d> out;
    
    if (!sphereIn.valid() || (sphereIn.radius() < std::numeric_limits<float>::epsilon()))
      return out;
    
    //in any case the direction/intersection vector is the x-axis of matrix.
    auto buildMatrix = [&](const osg::Vec3d &yAxis) -> std::optional<osg::Matrixd>
    {
      std::array<std::optional<osg::Vec3d>, 4> attempt0 =
      {
        direction.getVector()
        , yAxis
        , std::nullopt
        , sphereIn.center()
      };
      return tls::matrixFromAxes(attempt0);
    };
    
    auto m = buildMatrix(osg::Vec3d(0.0, 0.0, 1.0));
    if (!m)
    {
      m = buildMatrix(osg::Vec3d(0.0, 1.0, 0.0));
      if (!m)
        return out;
    }
    
    osg::Vec3d gridX = gu::getYVector(*m);
    osg::Vec3d gridY = gu::getZVector(*m);
    double gridLength = M_SQRT1_2 * sphereIn.radius() * 2.0;
    if (gridLength < std::numeric_limits<float>::epsilon())
      return out;
    int projectionCount = std::pow(2, (subdivision.getInt()));
    double projectionDistance = gridLength / std::pow(2, projectionCount);
    osg::Vec3d rowStart = sphereIn.center() + (-gridX * gridLength / 2.0) + (-gridY * gridLength / 2.0); //lower left
    for (int row = 0; row <= projectionCount; ++row) // <= because we start at zero
    {
      for (int column = 0; column <= projectionCount; ++column) // <= because we start at zero.
        out.push_back(rowStart + gridX * (column * projectionDistance));
      rowStart += gridY * projectionDistance;
    }
    
    return out;
  }
};

Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  name = QObject::tr("UnderCut");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
}

Feature::~Feature() = default;

void Feature::updateModel(const UpdatePayload &pIn)
{
  setFailure();
  lastUpdateLog.clear();
  try
  {
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    const auto &sPicks = stow->sourcePick.getPicks();
    if (sPicks.empty())
      throw std::runtime_error("No source inputs");
    tls::Resolver sourceResolver(pIn);
    if (!sourceResolver.resolve(sPicks.front()))
      throw std::runtime_error("invalid pick resolution");
    
    if (stow->directionType.getInt() == 0) //picks
    {
      prm::ObserverBlocker blocker(stow->blockObserver);
      const auto &dp = stow->directionPicks.getPicks();
      if (dp.empty())
        throw std::runtime_error("No picks for direction");
      tls::Resolver directionResolver(pIn);
      std::vector<osg::Vec3d> points;
      for (const auto &p : dp)
      {
        if (!directionResolver.resolve(p))
          throw std::runtime_error("Can't resolve direction pick");
        if (slc::isObjectType(p.selectionType))
        {
          prm::Parameters params = directionResolver.getFeature()->getParameters(prm::Tags::Direction);
          if (params.empty())
            throw std::runtime_error("No direction parameter from direction input");
          else
          {
            stow->direction.setValue(params.front()->getVector());
            break;
          }
          //TODO look for csys parameter and take z-axis
        }
        else if (slc::isPointType(p.selectionType))
        {
          auto localPoints = directionResolver.getPoints();
          points.insert(points.end(), localPoints.begin(), localPoints.end());
        }
      }
      if (!points.empty())
      {
        if (points.size() != 2)
          throw std::runtime_error("Incorrect number of points for direction");
        osg::Vec3d temp = points.front() - points.back();
        temp.normalize();
        stow->direction.setValue(temp);
      }
    }
    
    const auto *inputTransform = sourceResolver.getFeature()->getMainTransform();
    const auto &inputBound = inputTransform->getBound();
    auto gridPoints = stow->buildGrid(inputBound);
    
    //temp just to test out grid construction.
    osg::Vec3Array *points = new osg::Vec3Array(gridPoints.begin(), gridPoints.end());
    osg::Vec4Array *colors = new osg::Vec4Array(); colors->push_back(osg::Vec4(1.0, 1.0, 1.0, 1.0));
    osg::Geometry *geometry = new osg::Geometry();
    geometry->setVertexArray(points);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, points->size()));
    mainTransform->addChild(geometry);
    
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
//   prj::srl::FIX::UnderCut so
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
//   prj::srl::FIX::undercut(stream, so, infoMap);
}

// void Feature::serialRead(const prj::srl::FIXME::UnderCut &so)
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
