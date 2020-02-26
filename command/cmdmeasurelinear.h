/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef CMD_MEASURELINEAR_H
#define CMD_MEASURELINEAR_H

#include "selection/slcdefinitions.h"
#include "command/cmdbase.h"

namespace cmd
{
  class MeasureLinear :  public Base
  {
  public:
    MeasureLinear();
    virtual ~MeasureLinear() override;
    
    std::string getCommandName() override{return "Measure Linear";}
    std::string getStatusMessage() override;
    void activate() override;
    void deactivate() override;
  };
}

#endif // CMD_MEASURELINEAR_H
