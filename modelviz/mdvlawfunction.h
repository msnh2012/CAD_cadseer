/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2019 Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef MDV_LAWFUNCTION_H
#define MDV_LAWFUNCTION_H

#include <osg/NodeCallback>

namespace osg{class Switch;}

namespace lwf{struct Cue;}
namespace lbr{class PLabel;}

namespace mdv
{
  osg::Switch* generate(lwf::Cue&);
  
  class LawCallback : public osg::NodeCallback
  {
  public:
    LawCallback() = delete;
    LawCallback(lwf::Cue&);
    ~LawCallback();
    void setDirty(){dirty = true;}
    void setClean(){dirty = false;}
    void setScale(double);
    double getScale(){return scale;}
    osg::Matrixd getMatrix(osg::Node*);
    void setMatrix(const osg::Matrixd&, osg::Node*);
    
    void operator()(osg::Node*, osg::NodeVisitor*) override;
  private:
    bool dirty = true;
    double scale = 1.0;
    lwf::Cue &cue;
    void update(osg::Node*);

  };
}

#endif // MDV_LAWFUNCTION_H
