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
#include <osg/Point>
#include <osgUtil/LineSegmentIntersector>

#include "globalutilities.h"
#include "library/lbrplabel.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "tools/occtools.h"
#include "tools/featuretools.h"
#include "tools/tlsosgtools.h"
#include "selection/slcvisitors.h"
#include "feature/ftrupdatepayload.h"
#include "modelviz/mdvnodemaskdefs.h"
#include "project/serial/generated/prjsrlundundercut.h"
#include "feature/ftrundercut.h"

using boost::uuids::uuid;
using namespace ftr::UnderCut;
QIcon Feature::icon = QIcon(":/resources/images/constructionUnderCut.svg");

struct Feature::Stow
{
  Feature &feature;
  prm::Parameter sourcePick{QObject::tr("Source"), ftr::Picks(), PrmTags::sourcePick};
  prm::Parameter directionType{QObject::tr("Type"), 0, PrmTags::directionType};
  prm::Parameter directionPicks{QObject::tr("Direction Picks"), ftr::Picks(), PrmTags::directionPicks};
  prm::Parameter direction{QObject::tr("Direction"), osg::Vec3d(), PrmTags::direction};
  prm::Parameter subdivision{QObject::tr("Subdivision"), 5, PrmTags::subdivision};
  osg::ref_ptr<lbr::PLabel> directionTypeLabel{new lbr::PLabel(&directionType)};
  osg::ref_ptr<lbr::PLabel> directionLabel{new lbr::PLabel(&direction)};
  osg::ref_ptr<lbr::PLabel> subdivisionLabel{new lbr::PLabel(&subdivision)};
  prm::Observer blockObserver{std::bind(&Feature::setModelDirty, &feature)};
//   osg::ref_ptr<osg::Geometry> pointGeometry;
  osg::ref_ptr<osg::Geometry> intersectionGeometry{new osg::Geometry()};
  osg::ref_ptr<osg::Vec3Array> intersectionPoints{new osg::Vec3Array()};
  osg::ref_ptr<osg::DrawArrays> drawArray{new osg::DrawArrays(GL_POINTS)};
  
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
    
    prmActiveSync();
    buildIntersectionGeometry();
  }
  
  void buildIntersectionGeometry()
  {
    osg::Vec4Array *colors = new osg::Vec4Array();
    colors->push_back(osg::Vec4(1.0, 1.0, 1.0, 1.0));
    intersectionGeometry->setColorArray(colors);
    intersectionGeometry->setColorBinding(osg::Geometry::BIND_OVERALL);
    intersectionGeometry->setDataVariance(osg::Object::DYNAMIC);
    intersectionGeometry->setVertexArray(intersectionPoints);
    intersectionGeometry->addPrimitiveSet(drawArray);
    intersectionGeometry->getOrCreateStateSet()->setAttribute(new osg::Point(5.0));
    feature.mainTransform->addChild(intersectionGeometry);
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
  
  /* build a matrix outside on sphere
   * 
   */
  std::optional<osg::Matrixd> buildMatrix(const osg::BoundingSphere &sphereIn)
  {
    if (!sphereIn.valid() || (sphereIn.radius() < std::numeric_limits<float>::epsilon()))
      return std::nullopt;
    
    //in any case the direction/intersection vector is the x-axis of matrix.
    //because matrixFromAxis favors x axis when passing in 2 vectors
    auto build = [&](const osg::Vec3d &yAxis) -> std::optional<osg::Matrixd>
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
    
    auto m = build(osg::Vec3d(0.0, 0.0, 1.0));
    if (!m)
    {
      m = build(osg::Vec3d(0.0, 1.0, 0.0));
      if (!m)
        return m;
    }
    
    /* we had to make the x direction the projection direction to make sure that direction
     * doesn't get changed by 'matrixFromAxes'. so now let us make matrix like we expect
     * where the zaxis is the projection direction.
     */
    std::array<std::optional<osg::Vec3d>, 4> attempt =
    {
      gu::getYVector(*m)
      , std::nullopt
      , direction.getVector()
      , sphereIn.center() + direction.getVector() * -sphereIn.radius()
    };
    return tls::matrixFromAxes(attempt);
  }
  
  std::vector<osg::Vec3d> buildGrid(const osg::Matrixd &matrixIn, double gridLength)
  {
    std::vector<osg::Vec3d> out;
    
    osg::Vec3d gridX = gu::getXVector(matrixIn);
    osg::Vec3d gridY = gu::getYVector(matrixIn);
    if (gridLength < std::numeric_limits<float>::epsilon())
      return out;
    int projectionCount = std::pow(2, (subdivision.getInt()));
    double projectionDistance = gridLength / projectionCount;
    osg::Vec3d rowStart = matrixIn.getTrans(); //start at sphere center.
    rowStart += (-gridX * gridLength / 2.0) + (-gridY * gridLength / 2.0); //move to lower left
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
    stow->intersectionPoints->clear();
    stow->intersectionGeometry->dirtyDisplayList();
    stow->intersectionGeometry->dirtyBound();
    
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
    
    auto *inputTransform = sourceResolver.getFeature()->getMainTransform(); //accept isn't const
    const auto &inputBound = inputTransform->getBound();
    auto gridMatrix = stow->buildMatrix(inputBound);
    if (!gridMatrix)
      throw std::runtime_error("couldn't build grid matrix");
    
    /* when projecting on the end of something long and thin, our rectangular grid
     * is much larger than necessary so excess of wasted grid points. So we will collect
     * and project all osg vertices onto base plane and get a bounding sphere from that.
     * This seemed to slow it down considerably.
     */
    slc::ChildMaskVisitor cmVisitor(mdv::face);
    inputTransform->accept(cmVisitor);
    if (!cmVisitor.out)
      throw std::runtime_error("Couldn't get input child geometry");
    auto *childGeometry = dynamic_cast<osg::Geometry*>(cmVisitor.out);
    assert(childGeometry);
    auto *childVertices = dynamic_cast<osg::Vec3Array*>(childGeometry->getVertexArray());
    assert(childVertices);
    osg::BoundingSphere projectedBound;
    osg::Plane plane(gu::getZVector(*gridMatrix), gridMatrix->getTrans());
    for (const auto &v : *childVertices)
    {
      double distance = plane.distance(v);
      osg::Vec3d projectedPoint = v + (stow->direction.getVector() * -distance);
      projectedBound.expandBy(projectedPoint);
//       stow->intersectionPoints->push_back(projectedPoint); //temp for debug
    }
    if (!projectedBound.valid() || (projectedBound.radius() < std::numeric_limits<float>::epsilon()))
      throw std::runtime_error("Couldn't create projected bound");
    gridMatrix->setTrans(projectedBound.center()); //move matrix origin to center of projected sphere.
    auto gridPoints = stow->buildGrid(*gridMatrix, projectedBound.radius() * 2.0);
    std::ostringstream s; s << getTypeString() << " grid points: " << gridPoints.size() << std::endl;
    lastUpdateLog += s.str();
    if (gridPoints.empty())
      throw std::runtime_error("Couldn't build grid points");
    
    for (const auto &startPoint : gridPoints)
    {
      auto endPoint = startPoint + stow->direction.getVector() * inputBound.radius() * 2.0;
      
//       stow->intersectionPoints->push_back(startPoint); //temp for debug
//       stow->intersectionPoints->push_back(endPoint); //temp for debug
      
      //visitor keeps a ref_ptr of intersector so have to 'new' it.
      auto *intersector = new osgUtil::LineSegmentIntersector(startPoint, endPoint);
      osgUtil::IntersectionVisitor visitor(intersector);
      visitor.setTraversalMask(~mdv::edge);
      inputTransform->accept(visitor);
      const auto &intersections = intersector->getIntersections();
      if (intersections.empty())
        continue;
      stow->intersectionPoints->push_back(intersections.begin()->getWorldIntersectPoint());
    }
    
    stow->drawArray->set(GL_POINTS, 0, stow->intersectionPoints->size());
    
    //set label locations
    stow->directionTypeLabel->setMatrix(osg::Matrixd::translate(inputBound.center()));
    stow->directionLabel->setMatrix(osg::Matrixd::translate(gridPoints.front()));
    stow->subdivisionLabel->setMatrix(osg::Matrixd::translate(projectedBound.center()));
    
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
  prj::srl::und::UnderCut so
  (
    Base::serialOut()
    , stow->sourcePick.serialOut()
    , stow->directionType.serialOut()
    , stow->directionPicks.serialOut()
    , stow->direction.serialOut()
    , stow->subdivision.serialOut()
    , stow->directionTypeLabel->serialOut()
    , stow->directionLabel->serialOut()
    , stow->subdivisionLabel->serialOut()
  );
  for (const auto &p : *stow->intersectionPoints)
    so.intersectionPoints().push_back(prj::srl::spt::Vec3d(p.x(), p.y(), p.z()));
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::und::undercut(stream, so, infoMap);
}

void Feature::serialRead(const prj::srl::und::UnderCut &so)
{
  Base::serialIn(so.base());
  stow->sourcePick.serialIn(so.sourcePick());
  stow->directionType.serialIn(so.directionType());
  stow->directionPicks.serialIn(so.directionPicks());
  stow->direction.serialIn(so.direction());
  stow->subdivision.serialIn(so.subdivision());
  stow->directionTypeLabel->serialIn(so.directionTypeLabel());
  stow->directionLabel->serialIn(so.directionLabel());
  stow->subdivisionLabel->serialIn(so.subdivisionLabel());
  for (const auto &v : so.intersectionPoints())
    stow->intersectionPoints->push_back(osg::Vec3d(v.x(), v.y(), v.z()));
  stow->drawArray->set(GL_POINTS, 0, stow->intersectionPoints->size());
  stow->intersectionGeometry->dirtyDisplayList();
  stow->intersectionGeometry->dirtyBound();
}
