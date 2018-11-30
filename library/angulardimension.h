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

#ifndef LBR_ANGULARDIMENSION_H
#define LBR_ANGULARDIMENSION_H


#include <boost/optional/optional.hpp>

#include <osg/ref_ptr>
#include <osg/MatrixTransform>
#include <osg/NodeCallback>

#include "library/rangemask.h"

namespace osg
{
  class Billboard;
  class Switch;
  class Geometry;
  class PositionAttitudeTransform;
  class Node;
  class Material;
  class AutoTransform;
}

namespace prm{class Parameter;}

namespace lbr
{
  /*! @class AngularDimensionCallback
   * 
   * @brief update callback for linear dimensions.
   * 
   */
  class AngularDimensionCallback : public osg::NodeCallback
  {
  public:
    AngularDimensionCallback() = delete;
    AngularDimensionCallback(const char*);
    AngularDimensionCallback(const std::string&);
    void setRangeMask1(const RangeMask&);
    void setRangeMask2(const RangeMask&);
    void setAngleDegrees(double);
    void setAngleRadians(double);
    void setTextLocation(const osg::Vec3d&);
    void setExtensionFactor(double);
    virtual void operator()(osg::Node*, osg::NodeVisitor*) override;
    
  protected:
    void init(); //!< ran on 1st run to cache pointers.
    void updateLayout();
    void updateExtensionLines(double);
    void placeArrowsInside(double);
    void placeArrowsOutside(double);
    void updateDimensionArcsOutside(double, double);
    void updateDimensionArcsInside(double, double, double);
    void updateDimensionArcInsideSolid(double, double);
    
    double angle = osg::PI_2; //!< angle in radians. defaults 90 degrees.
    double extensionFactor = 1.1; //!< 1.0 stops at textobject distance.
    osg::MatrixTransform *rootTransform = nullptr; //!< node that this has been attached to.
    osg::ref_ptr<osg::Switch> rootSwitch; //!< Parent to all nodes.
    osg::ref_ptr<osg::MatrixTransform> textObject; //!< Optional and not used by default.
    osg::ref_ptr<osg::Geometry> extensionLine1; //!< base extension line.
    osg::ref_ptr<osg::Geometry> extensionLine2; //!< rotating extension line.
    osg::ref_ptr<osg::PositionAttitudeTransform> dimensionArc1; //!< arc connected to base extension line.
    osg::ref_ptr<osg::PositionAttitudeTransform> dimensionArc2; //!< arc connected to rotating extension line.
    osg::ref_ptr<osg::AutoTransform> arrowTransform1; //!< transformation for 1st arrow.
    osg::ref_ptr<osg::AutoTransform> arrowTransform2; //!< transformation for 2nd arrow.
    osg::ref_ptr<osg::PositionAttitudeTransform> arrowShape; //!< Scales arrow to preferences.
    osg::ref_ptr<osg::Geometry> arrow; //!< arrow from library manager.
    RangeMask rangeMask1;
    RangeMask rangeMask2;
    bool dirty = true;
    boost::optional<bool> isValid;
    std::string textObjectName;
    osg::BoundingBox cachedTextBound;
  };
  
  osg::MatrixTransform* buildAngularDimension(osg::MatrixTransform*);
}

#endif // LBR_ANGULARDIMENSION_H
