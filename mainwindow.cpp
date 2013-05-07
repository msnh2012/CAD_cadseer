#include <iostream>

#include <QHBoxLayout>
#include <QFileDialog>
#include <QTimer>

#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepMesh.hxx>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "viewerwidget.h"
#include "./modelviz/graph.h"
#include "selectionmanager.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    viewWidget = new ViewerWidget(osgViewer::ViewerBase::SingleThreaded);
    viewWidget->setGeometry( 100, 100, 800, 600 );
    QHBoxLayout *aLayout = new QHBoxLayout();
    aLayout->addWidget(viewWidget);
    ui->centralwidget->setLayout(aLayout);

    selectionManager = new SelectionManager(this);
    connect(ui->actionSelectFaces, SIGNAL(toggled(bool)), selectionManager, SLOT(toggledFaces(bool)));
    connect(ui->actionSelectEdges, SIGNAL(toggled(bool)), selectionManager, SLOT(toggledEdges(bool)));
    connect(ui->actionSelectVertices, SIGNAL(toggled(bool)), selectionManager, SLOT(toggledVertices(bool)));
    connect(selectionManager, SIGNAL(guiFacesEnabled(bool)), ui->actionSelectFaces, SLOT(setEnabled(bool)));
    connect(selectionManager, SIGNAL(guiEdgesEnabled(bool)), ui->actionSelectEdges, SLOT(setEnabled(bool)));
    connect(selectionManager, SIGNAL(guiVerticesEnabled(bool)), ui->actionSelectVertices, SLOT(setEnabled(bool)));
    connect(selectionManager, SIGNAL(guiFacesSelectable(bool)), ui->actionSelectFaces, SLOT(setChecked(bool)));
    connect(selectionManager, SIGNAL(guiEdgesSelectable(bool)), ui->actionSelectEdges, SLOT(setChecked(bool)));
    connect(selectionManager, SIGNAL(guiVerticesSelectable(bool)), ui->actionSelectVertices, SLOT(setChecked(bool)));
    connect(selectionManager, SIGNAL(setSelectionMask(int)), viewWidget, SLOT(setSelectionMask(int)));

    SelectionState state;
    state.facesEnabled = true;
    state.edgesEnabled = true;
    state.verticesEnabled = true;
    state.facesSelectable = true;
    state.edgesSelectable = true;
    state.verticesSelectable = true;
    selectionManager->setState(state);

    connect(ui->actionAppendBrep, SIGNAL(triggered()), this, SLOT(readBrepSlot()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::addNode(osg::Node *node)
{
    viewWidget->addNode(node);
}

void MainWindow::readBrepSlot()
{
    QString fileName = QFileDialog::getOpenFileName(ui->centralwidget, tr("Open File"), "/home", tr("brep (*.brep *.brp)"));
    if (fileName.isEmpty())
        return;

    TopoDS_Shape base;
    BRep_Builder junk;
    std::fstream file(fileName.toStdString().c_str());
    BRepTools::Read(base, file, junk);

    BRepTools::Clean(base);
    ModelViz::Build builder(base);
    if (builder.go(0.25, 0.5))
        addNode(builder.getViz().get());
//    builder.go(0.5, 1.0);
//    builder.go(1.0, 2.0);
}
