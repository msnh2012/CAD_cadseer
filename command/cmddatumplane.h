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

#ifndef CMD_DATUMPLANE_H
#define CMD_DATUMPLANE_H

#include "command/cmdleafmanager.h"
#include "command/cmdbase.h"

namespace slc{class Container;}

namespace ftr{class DatumPlane;}

namespace cmd
{
  /**
  * @brief command for datum plane
  */
  class DatumPlane : public Base
  {
  public:
    ftr::DatumPlane *feature = nullptr;
    
    DatumPlane();
    DatumPlane(ftr::Base*);
    ~DatumPlane() override;
    
    virtual std::string getCommandName() override{return "DatumPlane";}
    virtual std::string getStatusMessage() override;
    virtual void activate() override;
    virtual void deactivate() override;
    
    void setToConstant();
    void setToPlanarOffset(const std::vector<slc::Message>&);
    void setToPlanarCenter(const std::vector<slc::Message>&);
    void setToAxisAngle(const std::vector<slc::Message>&);
    void setToAverage3Plane(const std::vector<slc::Message>&);
    void setToThrough3Points(const std::vector<slc::Message>&);
    void localUpdate();
  private:
    bool firstRun = true;
    cmd::LeafManager leafManager;
    void go();
    bool isPlanarFace(const slc::Container&);
    bool isAxis(const slc::Container&);
    bool attemptOffset(const slc::Container&);
    bool attemptCenter(const std::vector<slc::Container>&);
    bool attemptAxisAngle(const std::vector<slc::Container>&);
    bool attemptAverage3P(const std::vector<slc::Container>&);
    bool attemptThrough3P(const std::vector<slc::Container>&);
  };
}

#endif // CMD_DATUMPLANE_H
