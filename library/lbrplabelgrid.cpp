/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2022 Thomas S. Anderson blobfish.at.gmx.com
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

#include <cassert>

#include <osg/ComputeBoundsVisitor>

#include "parameter/prmparameter.h"
#include "library/lbrplabel.h"
#include "library/lbrplabelgrid.h"

using namespace lbr;

struct PLabelGridCallback::Stow
{
  bool dirtyLayout = true;
  bool dirtyParameters = true;
  int columns = 1;
  prm::Observer observer{std::bind(&Stow::valueHasChanged, this)};
  
  void valueHasChanged(){dirtyLayout = true;}
  
  void bindParameters(osg::AutoTransform *trsf)
  {
    if (!dirtyParameters) return;
    observer = prm::Observer(std::bind(&Stow::valueHasChanged, this));
    
    for (unsigned int index = 0; index < trsf->getNumChildren(); ++index)
    {
      auto *l = dynamic_cast<lbr::PLabel*>(trsf->getChild(index));
      if (!l) continue;
      l->getParameter()->connect(observer);
    }
  }
  
  void layout(osg::AutoTransform *trsf)
  {
    if (!dirtyLayout) return;
    if (trsf->getNumChildren() < 1)
    {
      dirtyLayout = false;
      return;
    }
    columns = std::max(1, columns); //at least 1 column.
    
    //loop through and get max width/height and
    //derive grid position.
    int cc = 0; //current column
    int cr = 0; //current row
    int columnCount = 0; //store the actual number of columns.
    int rowCount = 0; //store the actual number of rows.
    osg::BoundingBox::value_type maxWidth = 0.0, maxHeight = 0.0;
    std::vector<Position> positions;
    for (unsigned int index = 0; index < trsf->getNumChildren(); ++index)
    {
      auto *l = dynamic_cast<osg::MatrixTransform*>(trsf->getChild(index));
      assert(l); if (!l) continue;
      
      columnCount = std::max(columnCount, cc);
      rowCount = std::max(rowCount, cr);
      
      osg::ComputeBoundsVisitor bv;
      l->accept(bv);
      const osg::BoundingBox &bb = bv.getBoundingBox();
      maxWidth = std::max(maxWidth, (bb.xMax() - bb.xMin()));
      maxHeight = std::max(maxHeight, (bb.yMax() - bb.yMin()));
      
      positions.push_back({l, cr, cc});
      cc++;
      if (cc >= columns)
      {
        cr++;
        cc = 0;
      }
    }
    
    //adjust for 0 based.
    columnCount++;
    rowCount++;
    
    //setup grid projections
    auto widthPadding = maxWidth * 0.1;
    auto heightPadding = maxHeight * 0.1;
    osg::Vec3d rowProjection(maxWidth + widthPadding, 0.0, 0.0);
    osg::Vec3d columnProjection(0.0, -maxHeight - heightPadding, 0.0);
    
    //setup overall projection, so grid is centered on origin.
    //tricky part here is the the text is anchored at center.
    osg::Vec3d xProjection(-1.0, 0.0, 0.0);
    xProjection *= (columnCount * maxWidth + (columnCount - 1) * widthPadding) / 2.0 - maxWidth / 2.0;
    osg::Vec3d yProjection(0.0, 1.0, 0.0);
    yProjection *= (rowCount * maxHeight + (rowCount - 1) * heightPadding) / 2.0 - maxHeight / 2.0;
    osg::Vec3d overallProject = xProjection + yProjection;
    
    //set position of children
    for (auto &p : positions)
    {
      osg::Vec3d localX = rowProjection * p.column;
      osg::Vec3d localY = columnProjection * p.row;
      p.label->setMatrix(osg::Matrix::translate(localX + localY + overallProject));
    }
    
    dirtyLayout = false;
  }
private:
  struct Position
  {
    osg::MatrixTransform *label;
    int row;
    int column;
  };
};

PLabelGridCallback::PLabelGridCallback()
: stow(std::make_unique<Stow>())
{}

PLabelGridCallback::~PLabelGridCallback() = default;

void PLabelGridCallback::setDirtyLayout()
{
  stow->dirtyLayout = true;
}

void PLabelGridCallback::setDirtyParameters()
{
  stow->dirtyParameters = true;
  stow->dirtyLayout = true;
}

void PLabelGridCallback::setColumns(int count)
{
  stow->columns = std::max(1, count);
  stow->dirtyLayout = true;
}

bool PLabelGridCallback::run(osg::Object* object, osg::Object* data)
{
  osg::AutoTransform *trsf = dynamic_cast<osg::AutoTransform*>(object); assert(trsf);
  stow->bindParameters(trsf);
  stow->layout(trsf);
  
  return traverse(object, data);
}

osg::AutoTransform* lbr::buildGrid(int columns)
{
  osg::ref_ptr<osg::AutoTransform> out = new osg::AutoTransform();
  out->setName("lbr::PLabelGrid::AutoTransform");
  out->setAutoRotateMode(osg::AutoTransform::ROTATE_TO_SCREEN);
  out->setAutoScaleToScreen(true);
  
  PLabelGridCallback *cb = new PLabelGridCallback();
  cb->stow->columns = columns;
  
  out->setUpdateCallback(cb);
  return out.release();
}
