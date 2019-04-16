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

#ifndef APP_MAINWINDOW_H
#define APP_MAINWINDOW_H

#include <memory>

#include <QMainWindow>

#include <osg/Node>

class QCloseEvent;

namespace vwr{class Widget;}
namespace slc{class Manager;}
namespace msg{class Message; struct Node; struct Sift;}
namespace dag{class View; class Model;}
namespace expr{class Widget;}

namespace Ui {
class MainWindow;
}

class TopoDS_Face;
class TopoDS_Shape;

namespace app
{
class IncrementWidgetAction;
class InfoDialog;
class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    vwr::Widget* getViewer(){return viewWidget;}
    slc::Manager* getSelectionManager(){return selectionManager;}
    
protected:
  virtual void closeEvent (QCloseEvent*) override;

private:
    void setupSelectionToolbar();
    Ui::MainWindow *ui;
    vwr::Widget* viewWidget;
    dag::Model *dagModel;
    dag::View *dagView;
    expr::Widget *expressionWidget;
    slc::Manager *selectionManager;
    IncrementWidgetAction *incrementWidget;
    InfoDialog*infoDialog;
    
    std::unique_ptr<msg::Node> node;
    std::unique_ptr<msg::Sift> sift;
    void setupDispatcher();
    void preferencesChanged(const msg::Message&);
    void infoTextDispatched(const msg::Message&);
};
}

#endif // APP_MAINWINDOW_H