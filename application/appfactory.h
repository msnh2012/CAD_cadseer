/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2015  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef APP_FACTORY_H
#define APP_FACTORY_H

#include <functional>
#include <map>
#include <memory>

#include "selection/slccontainer.h"

namespace prj{class Project;}
namespace msg{class Message; struct Node; struct Sift;}

namespace app
{
  class Factory
  {
  public:
    Factory();
    
  private:
    prj::Project *project = nullptr;
    slc::Containers containers;
    
    std::unique_ptr<msg::Node> node;
    std::unique_ptr<msg::Sift> sift;
    void setupDispatcher();
    void triggerUpdate(); //!< just convenience.
    void newProjectDispatched(const msg::Message&);
    void selectionAdditionDispatched(const msg::Message&);
    void selectionSubtractionDispatched(const msg::Message&);
    void openProjectDispatched(const msg::Message&);
    void closeProjectDispatched(const msg::Message&);
    void debugDumpDispatched(const msg::Message&);
    void debugShapeTrackUpDispatched(const msg::Message&);
    void debugShapeGraphDispatched(const msg::Message&);
    void debugShapeTrackDownDispatched(const msg::Message&);
    void featureModelDirtyDispatched(const msg::Message&);
    
    void messageStressTestDispatched(const msg::Message&); //testing
    void osgToDotTestDispatched(const msg::Message&); //testing
    void bopalgoTestDispatched(const msg::Message&); //testing
  };
}

#endif // APP_FACTORY_H
