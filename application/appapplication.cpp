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

#include <iostream>
#include <memory>

#include <boost/filesystem/operations.hpp>
#include <boost/current_function.hpp>

#include <QTimer>
#include <QMessageBox>
#include <QSettings>

#include <global.hpp> //for git start and shutdown.

#include "application/appapplication.h"
#include "application/appmainwindow.h"
#include "viewer/vwrspaceballqevent.h"
#include "viewer/vwrwidget.h"
#include "project/prjmessage.h"
#include "project/prjgitmanager.h" //needed for unique_ptr destructor call.
#include "project/prjproject.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "message/msgnode.h"
#include "message/msgsift.h"
#include "application/appfactory.h"
#include "command/cmdmanager.h"
#include "lod/lodmanager.h"
#include "dialogs/dlgproject.h"
#include "dialogs/dlgabout.h"

#include <spnav.h>


using namespace app;

Application::Application(int &argc, char **argv) :
  QApplication(argc, argv),
  lodManager(new lod::Manager(arguments().at(0).toStdString()))
{
    qRegisterMetaType<msg::Message>("msg::Message");
    
    node = std::make_unique<msg::Node>();
    node->connect(msg::hub());
    sift = std::make_unique<msg::Sift>();
    sift->name = "app::Application";
    node->setHandler(std::bind(&msg::Sift::receive, sift.get(), std::placeholders::_1));
    setupDispatcher();
    
    //didn't have any luck using std::make_shared. weird behavior.
    std::unique_ptr<MainWindow> tempWindow(new MainWindow());
    mainWindow = std::move(tempWindow);
    
    std::unique_ptr<Factory> tempFactory(new Factory());
    factory = std::move(tempFactory);
    
    cmd::manager(); //just to construct the singleton and get it ready for messages.
    
    //some day pass the preferences file from user home directory to the manager constructor.
    prf::Manager &manager = prf::manager(); //this should cause preferences file to be read.
    if (!manager.isOk())
    {
      QMessageBox::critical(0, tr("CadSeer"), tr("Preferences failed to load"));
      QTimer::singleShot(0, this, SLOT(quit()));
      return;
    }
    
    setOrganizationName("blobfish");
    setOrganizationDomain("blobfish.org"); //doesn't exist.
    setApplicationName("cadseer");
    
    git2::start();
}

Application::~Application()
{
  git2::stop();
  spnav_close();
}

void Application::appStartSlot()
{
  //a project might be loaded before we launch the message queue.
  //if so then don't do the dialog.
  if (!project)
  {
    dlg::Project dialog(this->getMainWindow());
    if (dialog.exec() == QDialog::Accepted)
    {
      dlg::Project::Result r = dialog.getResult();
      if (r == dlg::Project::Result::Open || r == dlg::Project::Result::Recent)
      {
        openProject(dialog.getDirectory().string());
      }
      if (r == dlg::Project::Result::New)
      {
        createNewProject(dialog.getDirectory().string());
      }
    }
    else
    {
      this->quit();
    }
  }
  
  getMainWindow()->getViewer()->getGraphicsWidget()->setFocus();
}

void Application::quittingSlot()
{
    if (!spnav_close())// the not seems goofy.
    {
//        std::cout << "spaceball disconnected" << std::endl;
    }
    else
    {
//        std::cout << "couldn't disconnect spaceball" << std::endl;
    }
}

void Application::queuedMessage(const msg::Message &message)
{
  QMetaObject::invokeMethod(this, "messageSlot", Qt::QueuedConnection, Q_ARG(msg::Message, message));
}

void Application::messageSlot(const msg::Message &messageIn)
{
  //can't block, message might be open file or something we need to handle here.
  node->send(messageIn);
}

bool Application::notify(QObject* receiver, QEvent* e)
{
  try
  {
    bool outDefault = QApplication::notify(receiver, e);
    if (e->type() == vwr::MotionEvent::Type)
    {
      vwr::MotionEvent *motionEvent = dynamic_cast<vwr::MotionEvent*>(e);
      if
      (
        (!motionEvent) ||
        (motionEvent->isHandled())
      )
        return true;
        
      //make a new event and post to parent.
      vwr::MotionEvent *newEvent = new vwr::MotionEvent(*motionEvent);
      QObject *theParent = receiver->parent();
      if (!theParent || theParent == mainWindow.get())
        postEvent(mainWindow->getViewer()->getGraphicsWidget(), newEvent);
      else
        postEvent(theParent, newEvent);
      return true;
    }
    else if(e->type() == vwr::ButtonEvent::Type)
    {
      vwr::ButtonEvent *buttonEvent = dynamic_cast<vwr::ButtonEvent*>(e);
      if
      (
        (!buttonEvent) ||
        (buttonEvent->isHandled())
      )
        return true;
        
      //make a new event and post to parent.
      vwr::ButtonEvent *newEvent = new vwr::ButtonEvent(*buttonEvent);
      QObject *theParent = receiver->parent();
      if (!theParent || theParent == mainWindow.get())
        postEvent(mainWindow->getViewer()->getGraphicsWidget(), newEvent);
      else
        postEvent(theParent, newEvent);
      return true;
    }
    else
      return outDefault;
  }
  catch(...)
  {
    std::cerr << "unhandled exception in Application::notify" << std::endl;
  }
  return false;
}

void Application::initializeSpaceball()
{
    if (!mainWindow)
        return;

    vwr::registerEvents();

    if (spnav_open() == -1)
    {
      std::cout << "No spaceball found" << std::endl;
    }
    else
    {
      std::cout << "Spaceball found" << std::endl;
      spaceballPresent = true;
      QTimer *spaceballTimer = new QTimer(this);
      spaceballTimer->setInterval(1000/30); //30 times a second
      connect(spaceballTimer, &QTimer::timeout, this, &Application::spaceballPollSlot);
      spaceballTimer->start();
    }
}

void Application::spaceballPollSlot()
{
  spnav_event navEvent;
  if (!spnav_poll_event(&navEvent))
    return;
  
  QWidget *currentWidget = qApp->focusWidget();
  if (!currentWidget)
    return;

  if (navEvent.type == SPNAV_EVENT_MOTION)
  {
    vwr::MotionEvent *qEvent = new vwr::MotionEvent();
    qEvent->setTranslations(navEvent.motion.x, navEvent.motion.y, navEvent.motion.z);
    qEvent->setRotations(navEvent.motion.rx, navEvent.motion.ry, navEvent.motion.rz);
    this->postEvent(currentWidget, qEvent);
    return;
  }

  if (navEvent.type == SPNAV_EVENT_BUTTON)
  {
    vwr::ButtonEvent *qEvent = new vwr::ButtonEvent();
    qEvent->setButtonNumber(navEvent.button.bnum);
    if (navEvent.button.press == 1)
      qEvent->setButtonStatus(vwr::BUTTON_PRESSED);
    else
      qEvent->setButtonStatus(vwr::BUTTON_RELEASED);
    this->postEvent(currentWidget, qEvent);
    return;
  }
}

boost::filesystem::path Application::getApplicationDirectory()
{
  //if windows wants hidden, somebody else will have to do it.
  boost::filesystem::path path = boost::filesystem::path(getenv("HOME"));
  path /= ".CadSeer";
  if (!boost::filesystem::exists(path))
    if (!boost::filesystem::create_directory(path))
      std::cout << "ERROR: couldn't create application directory in home directory" << std::endl;
  
  return path;
}

QSettings& Application::getUserSettings()
{
  static QSettings *out = nullptr;
  if (!out)
  {
    boost::filesystem::path path = getApplicationDirectory();
    path /= "QSettings.ini";
    out = new QSettings(QString::fromStdString(path.string()), QSettings::IniFormat, this);
  }
  return *out;
}

void Application::createNewProject(const std::string &directoryIn)
{
  msg::Message preMessage;
  preMessage.mask = msg::Response | msg::Pre | msg::New | msg::Project;
  node->send(preMessage);
  
  //directoryIn has been verified to exist before this call.
  std::unique_ptr<prj::Project> tempProject(new prj::Project());
  project = std::move(tempProject);
  project->setSaveDirectory(directoryIn);
  project->initializeNew();
  
  updateTitle();
  
  msg::Message postMessage;
  postMessage.mask = msg::Response | msg::Post | msg::New | msg::Project;
  node->send(postMessage);
}

void Application::openProject(const std::string &directoryIn)
{
  msg::Message preMessage;
  preMessage.mask = msg::Response | msg::Pre | msg::Open | msg::Project;
  node->send(preMessage);
  
  assert(!project);
  //directoryIn has been verified to exist before this call.
  std::unique_ptr<prj::Project> tempProject(new prj::Project());
  project = std::move(tempProject);
  project->setSaveDirectory(directoryIn);
  project->open();
  
  updateTitle();
  
  msg::Message postMessage;
  postMessage.mask = msg::Response | msg::Post | msg::Open | msg::Project;
  node->send(postMessage);
  
  node->sendBlocked(msg::Message(msg::Request | msg::Project | msg::Update | msg::Visual));
}

void Application::closeProject()
{
  //something here for modified project.
  
  msg::Message preMessage;
  preMessage.mask = msg::Response | msg::Pre | msg::Close | msg::Project;
  node->send(preMessage);
 
  if (project)
  {
    //test modified. qmessagebox to save. save if applicable.
    project.reset();
  }
  
  msg::Message postMessage;
  postMessage.mask = msg::Response | msg::Post | msg::Close | msg::Project;
  node->send(postMessage);
}

void Application::updateTitle()
{
  if (!project)
    return;
  boost::filesystem::path path = project->getSaveDirectory();
  QString title = tr("CadSeer --") + QString::fromStdString(path.rbegin()->string()) + "--";
  mainWindow->setWindowTitle(title);
}

void Application::setupDispatcher()
{
  msg::Mask mask;
  
  
  sift->insert
  (
    {
      std::make_pair
      (
        msg::Request | msg::New | msg::Project
        , std::bind(&Application::newProjectRequestDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Open | msg::Project
        , std::bind(&Application::openProjectRequestDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Close | msg::Project
        , std::bind(&Application::closeProjectRequestDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Project | msg::Dialog
        , std::bind(&Application::ProjectDialogRequestDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::About | msg::Dialog
        , std::bind(&Application::AboutDialogRequestDispatched, this, std::placeholders::_1)
      )
    }
  );
}

void Application::ProjectDialogRequestDispatched(const msg::Message&)
{
  dlg::Project dialog(this->getMainWindow());
  if (dialog.exec() == QDialog::Accepted)
  {
    dlg::Project::Result r = dialog.getResult();
    if (r == dlg::Project::Result::Open || r == dlg::Project::Result::Recent)
    {
      closeProject();
      openProject(dialog.getDirectory().string());
    }
    if (r == dlg::Project::Result::New)
    {
      closeProject();
      createNewProject(dialog.getDirectory().string());
    }
  }
}

void Application::newProjectRequestDispatched(const msg::Message &messageIn)
{
  prj::Message pMessage = messageIn.getPRJ();
  
  boost::filesystem::path path = pMessage.directory;
  if (!boost::filesystem::create_directory(path))
  {
    QMessageBox::critical(this->getMainWindow(), tr("Failed building directory: "), QString::fromStdString(path.string()));
    return;
  }
  
  if(project)
    closeProject();
  
  createNewProject(path.string());
}

void Application::openProjectRequestDispatched(const msg::Message &messageIn)
{
  prj::Message pMessage = messageIn.getPRJ();
  
  boost::filesystem::path path = pMessage.directory;
  if (!boost::filesystem::exists(path))
  {
    QMessageBox::critical(this->getMainWindow(), tr("Failed finding directory: "), QString::fromStdString(path.string()));
    return;
  }
  
  if(project)
    closeProject();
  
  openProject(path.string());
}

void Application::closeProjectRequestDispatched(const msg::Message&)
{
  if (project)
    closeProject();
}

void Application::AboutDialogRequestDispatched(const msg::Message &)
{
  std::unique_ptr<dlg::About> dialog(new dlg::About(mainWindow.get()));
  dialog->exec();
}

Application* app::instance()
{
  return static_cast<Application*>(qApp);
}

WaitCursor::WaitCursor()
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
}

WaitCursor::~WaitCursor()
{
  QApplication::restoreOverrideCursor();
}
