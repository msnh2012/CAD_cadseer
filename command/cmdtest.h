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

#ifndef CMD_TEST_H
#define CMD_TEST_H

#include "command/cmdbase.h"

namespace cmd
{
  /**
  * @todo write docs
  */
  class Test : public Base
  {
  public:
    Test();
    ~Test() override;
    
    std::string getCommandName() override{return "Test";}
    std::string getStatusMessage() override;
    void activate() override;
    void deactivate() override;
  private:
    void go();
    void messageStressTestDispatched();
    void osgToDotTestDispatched();
    void bopalgoTestDispatched();
  };
}
#endif // CMD_TEST_H