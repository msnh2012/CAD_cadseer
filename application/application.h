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

#ifndef APP_APPLICATION_H
#define APP_APPLICATION_H

#include <memory>

#include <QApplication>
#include <boost/filesystem/path.hpp>

#include <osg/Node> //for viewer message.

class QSettings;

namespace prf{class Manager;}
namespace prj{class Project;}
namespace msg{class Message; struct Node; struct Sift;}
namespace lod{class Manager;}

namespace app
{
class MainWindow;
class Factory;

class Application : public QApplication
{
    Q_OBJECT
public:
    explicit Application(int &argc, char **argv);
    ~Application();
    bool notify(QObject * receiver, QEvent * e) override;
    void initializeSpaceball();
    prj::Project* getProject(){return project.get();}
    MainWindow* getMainWindow(){return mainWindow.get();}
    boost::filesystem::path getApplicationDirectory();
    QSettings& getUserSettings();
    void queuedMessage(const msg::Message&); //queue message into qt event loop
    
public Q_SLOTS:
    void quittingSlot();
    void appStartSlot();
    void messageSlot(const msg::Message&); //for queued connection
    void spaceballPollSlot();
private:
    std::unique_ptr<MainWindow> mainWindow;
    std::unique_ptr<prj::Project> project;
    std::unique_ptr<Factory> factory;
    std::unique_ptr<lod::Manager> lodManager;
    bool spaceballPresent = false;
    
    void createNewProject(const std::string &);
    void openProject(const std::string &);
    void closeProject();
    void updateTitle();
    
    std::unique_ptr<msg::Node> node;
    std::unique_ptr<msg::Sift> sift;
    void setupDispatcher();
    void newProjectRequestDispatched(const msg::Message &);
    void openProjectRequestDispatched(const msg::Message &);
    void closeProjectRequestDispatched(const msg::Message &);
    void ProjectDialogRequestDispatched(const msg::Message &);
    void AboutDialogRequestDispatched(const msg::Message &);
};

static Application* instance(){return static_cast<Application*>(qApp);}

struct WaitCursor
{
  WaitCursor();
  ~WaitCursor();
};
}

#endif // APP_APPLICATION_H
