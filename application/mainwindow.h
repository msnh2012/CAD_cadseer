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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <osg/Node>

#ifndef Q_MOC_RUN
#include <boost/signals2.hpp>
#include <message/message.h>
#endif

class ViewerWidget;
namespace slc{class Manager;}

namespace dag{class View; class Model;}

namespace Ui {
class MainWindow;
}

class TopoDS_Face;
class TopoDS_Shape;

namespace app
{
class IncrementWidgetAction;
class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    ViewerWidget* getViewer(){return viewWidget;}
    slc::Manager* getSelectionManager(){return selectionManager;}
    
    void messageInSlot(const msg::Message &);
    typedef boost::signals2::signal<void (const msg::Message &)> MessageOutSignal;
    boost::signals2::connection connectMessageOut(const MessageOutSignal::slot_type &subscriber)
    {
      return messageOutSignal.connect(subscriber);
    }

private:
    void setupSelectionToolbar();
    Ui::MainWindow *ui;
    ViewerWidget* viewWidget;
    dag::Model *dagModel;
    dag::View *dagView;
    slc::Manager *selectionManager;
    IncrementWidgetAction *incrementWidget;
    
    MessageOutSignal messageOutSignal;
    msg::MessageDispatcher dispatcher;
    void setupDispatcher();
    void preferencesChanged(const msg::Message&);
    
private Q_SLOTS:
    void incrementChangedSlot();
};
}

#endif // MAINWINDOW_H
