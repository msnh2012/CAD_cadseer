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

#ifndef CMD_BASE_H
#define CMD_BASE_H

#include <string>
#include <memory>

#include <boost/optional/optional.hpp>

#include <QObject> //for string translation.

#include "selection/slcmessage.h" //for derived classes

namespace app{class Application; class MainWindow;}
namespace prj{class Project;}
namespace slc{class Manager; class EventHandler;}
namespace msg{struct Message; struct Node; struct Sift;}
namespace vwr{class Widget;}
namespace cmv{class Base;}

namespace cmd
{
  /*! @brief base class for commands.
   * 
   */
  class Base
  {
  public:
    Base();
    Base(const std::string&);
    virtual ~Base();
    virtual std::string getCommandName() = 0;
    virtual std::string getStatusMessage() = 0;
    virtual void activate() = 0;
    virtual void deactivate() = 0;
    
    bool getShouldUpdate(){return shouldUpdate;}
    
  protected:
    std::unique_ptr<msg::Node> node;
    std::unique_ptr<msg::Sift> sift;
    void splitterDispatched(const msg::Message&);
    
    void sendDone();
    
    app::Application *application;
    app::MainWindow *mainWindow;
    prj::Project *project;
    slc::Manager *selectionManager;
    slc::EventHandler *eventHandler;
    vwr::Widget *viewer;
    
    std::unique_ptr<cmv::Base> viewBase; //set in derived class
    
    bool isActive = false;
    bool shouldUpdate = true; //not real useful anymore.
    
    //using optional to force derived class to set a meaningful value in constructors
    boost::optional<bool> isEdit;
    boost::optional<bool> isFirstRun;
  };
  
  typedef std::shared_ptr<Base> BasePtr;
}

#endif // CMD_BASE_H
