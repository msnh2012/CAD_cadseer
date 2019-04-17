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

#ifndef LBR_SKETCHLINEARDIMENSION_H
#define LBR_SKETCHLINEARDIMENSION_H

#include <boost/optional/optional_fwd.hpp>

#include <osg/ref_ptr>
#include <osg/NodeCallback>

#include "library/lbrrangemask.h"

namespace osg
{
  class MatrixTransform;
  class Geometry;
  class Switch;
  class AutoTransform;
}

namespace lbr
{
  /*! @class LinearDimensionCallback
   * 
   * @brief update callback for linear dimensions.
   * 
   */
  class LinearDimensionCallback : public osg::NodeCallback
  {
  public:
    LinearDimensionCallback() = default;
    void setTextObjectName(const char*);
    void setTextLocation(const osg::Vec3d&);
    void setDistance(double);
    void setRangeMask1(const RangeMask&);
    void setRangeMask2(const RangeMask&);
    void setExtensionFactor(double);
    double getExtensionFactor() const;
    virtual void operator()(osg::Node*, osg::NodeVisitor*) override;
    
  protected:
    void init(); //!< ran on 1st run to cache pointers.
    void updateLayout();
    void updateExtensionLines(const osg::Vec3d&);
    void placeArrowsInside();
    void placeArrowsOutside();
    void updateDimensionLinesInside();
    void updateDimensionLinesOutside();
    void updateDimensionLineInsideSolid();
    void updateDimLine(osg::Geometry*, const osg::Vec3d&, const osg::Vec3d&);
    
    boost::optional<bool> isValid;
    bool dirty = true;
    double distance = 1.0;
    double extensionFactor = 1.1; //!< 1.0 stops at textobject distance.
    const char *textObjectName = nullptr;
    osg::BoundingBox cachedTextBound;
    osg::MatrixTransform *rootTransform = nullptr;
    osg::Switch *rootSwitch = nullptr;
    osg::MatrixTransform *textObject = nullptr;
    osg::Geometry *extensionLine1 = nullptr;
    osg::Geometry *extensionLine2 = nullptr;
    osg::Geometry *dimensionLine1 = nullptr;
    osg::Geometry *dimensionLine2 = nullptr;
    osg::AutoTransform *arrowTransform1 = nullptr;
    osg::AutoTransform *arrowTransform2 = nullptr;
    RangeMask rangeMask1;
    RangeMask rangeMask2;
  };
  
  osg::MatrixTransform* buildLinearDimension(osg::Node*);
}

#endif // LBR_SKETCHLINEARDIMENSION_H
