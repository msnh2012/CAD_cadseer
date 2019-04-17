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

#ifndef LBR_DIAMETERDIMENSION_H
#define LBR_DIAMETERDIMENSION_H

#include <boost/optional/optional_fwd.hpp>

#include <osg/NodeCallback>

#include "library/lbrrangemask.h"

namespace osg
{
  class MatrixTransform;
  class Switch;
  class Geometry;
  class AutoTransform;
  class PositionAttitudeTransform;
}

namespace lbr
{
  /*! @class DiameterDimensionCallback
   * 
   * @brief update callback for linear dimensions.
   * 
   */
  class DiameterDimensionCallback : public osg::NodeCallback
  {
  public:
    DiameterDimensionCallback() = delete;
    DiameterDimensionCallback(const char*);
    DiameterDimensionCallback(const std::string&);
    void setRangeMask(const RangeMask&);
    void setDiameter(double);
    void setTextLocation(const osg::Vec3d&);
    virtual void operator()(osg::Node*, osg::NodeVisitor*) override;
    
  protected:
    void init(); //!< ran on 1st run to cache pointers.
    void updateLayout();
    
    osg::MatrixTransform *rootTransform = nullptr;
    osg::Switch *rootSwitch = nullptr;
    osg::MatrixTransform *textObject = nullptr;
    osg::PositionAttitudeTransform *extensionArcTransform = nullptr; //so we can rotate extension arc
    osg::Geometry *extensionArc = nullptr;
    osg::Geometry *dimensionLine = nullptr;
    osg::AutoTransform *arrowTransform = nullptr;
    RangeMask rangeMask;
    bool dirty = true;
    boost::optional<bool> isValid;
    std::string textObjectName;
    osg::BoundingBox cachedTextBound;
    double diameter = 0.0;
    
  };
  
  osg::MatrixTransform* buildDiameterDimension(osg::MatrixTransform*);
}

#endif // LBR_DIAMETERDIMENSION_H
