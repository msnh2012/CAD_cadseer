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

#include "command/cmdbase.h"

namespace slc{class Container;}

namespace cmd
{
  /**
  * @brief command for datum plane
  */
  class DatumPlane : public Base
  {
  public:
    DatumPlane();
    virtual ~DatumPlane() override;
    
    virtual std::string getCommandName() override{return "DatumPlane";}
    virtual std::string getStatusMessage() override;
    virtual void activate() override;
    virtual void deactivate() override;
  private:
    void go();
    bool attemptOffset(const slc::Container&);
    bool attemptCenter(const std::vector<slc::Container>&);
    bool attemptAxisAngle(const std::vector<slc::Container>&);
    bool attemptAverage3P(const std::vector<slc::Container>&);
    bool attemptThrough3P(const std::vector<slc::Container>&);
  };
}

#endif // CMD_DATUMPLANE_H
