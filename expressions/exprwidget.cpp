/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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
#include <sstream>
#include <functional>

#include <QTabWidget>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QToolBar>
#include <QInputDialog>
#include <QMessageBox>
#include <QTextEdit>
#include <QTextStream>
#include <QDesktopServices>

#include "tools/idtools.h"
#include "application/appapplication.h"
#include "application/appmessage.h"
#include "dialogs/dlgsplitterdecorated.h"
#include "message/msgnode.h"
#include "message/msgsift.h"
#include "expressions/exprtablemodel.h"
#include "expressions/exprtableview.h"
#include "expressions/exprmanager.h"
#include "expressions/exprwidget.h"
#include "project/prjproject.h"

namespace
{
  const std::string& getScalarExamples()
  {
    static const std::string out
    (
      "example1 = 1.0\n"
      "example2 = 2.\n"
      "example3 = .3\n"
      "example4 = 0.4\n"
      "example5 = 5\n"
      "example6 = PI\n"
      "example7 = E\n"
      "exampleTan = 10 * (2 + tan(0.52359878)) - 4\n"
      "exampleSin = 8 + 3 - sin(0.75) * 4 / 2.1 + 3\n"
      "exampleCos = cos(0.75) * ((8 / 3.547) + 1)\n"
      "exampleAsin = asin(0.5)\n"
      "exampleAcos = acos(0.5)\n"
      "exampleAtan = atan(1)\n"
      "exampleCombo = exampleSin + exampleCos + (example1 - example2)\n"
      "examplePow = pow(2, 3)\n"
      "exampleAtan2 = atan2(1, 1)\n"
      "exampleAbs = 10 * abs(-1.2)+ 2\n"
      "exampleMin = min(1.4, 4.8)\n"
      "exampleMax = max(1.4, 4.8)\n"
      "exampleFloor1 = floor(1.374, 0.125)\n"
      "exampleFloor2 = floor(1.375, 0.125)\n"
      "exampleFloor3 = floor(1.376, 0.125)\n"
      "exampleCeil1 = ceil(1.374, 0.125)\n"
      "exampleCeil2 = ceil(1.375, 0.125)\n"
      "exampleCeil3 = ceil(1.376, 0.125)\n"
      "exampleRound1 = round(1.0624, 0.125)\n"
      "exampleRound2 = round(1.0625, 0.125)\n"
      "exampleRound3 = round(1.0626, 0.125)\n"
      "exampleRadtodeg = radtodeg(3.14159)\n"
      "exampleDegtorad = degtorad(180)\n"
      "exampleLog = log(2.0)\n"
      "exampleExp = exp(2.0)\n"
      "exampleSqrt = sqrt(25.0)\n"
      "exampleHypot = hypot(3,4)\n"
      "exampleIf1 = if(3 > 3) then(1.0) else(0.0)\n"
      "exampleIf2 = if(3 < 3) then(1.0) else(0.0)\n"
      "exampleIf3 = if(3 >= 3) then(1.0) else(0.0)\n"
      "exampleIf4 = if(3 <= 3) then(1.0) else(0.0)\n"
      "exampleIf5 = if(3 == 3) then(1.0) else(0.0)\n"
      "exampleIf6 = if(3 != 3) then(1.0) else(0.0)\n"
      "exampleIf7 = if(sin(degtorad(44)) < sin(degtorad(46))) then(example1) else(example2)\n"
    );
    return out;
  }
  
  const std::string& getVectorExamples()
  {
    static const std::string out
    (
      "vDoubles = [0.25, 0.5, 0.75]\n"
      "vDoubles2=[ 0.25 , 0.5 , 0.75 ]\n"
      "vScalarNames = [example2, example2, example2]\n"
      "vName = vDoubles\n"
      "vConstantX = X\n"
      "vConstantY = Y\n"
      "vConstantZ = Z\n"
      "vConstantZero = VZERO\n"
      "vScalarMath = [0.25+.125, 3/2, 6*.75]\n"
      "sDecayX = vdecay(X, [1.0, 2.0, 3.0])\n"
      "sDecayY = vdecay(Y, [1.0, 2.0, 3.0])\n"
      "sDecayZ = vdecay(Z, [1.0, 2.0, 3.0])\n"
      "sLength = vlength([0.25, 0.25, 0.25])\n"
      "sDot = vdot([0.25, 0.25, 0.25], [0.5, 0.5, 0.5])\n"
      "vNormalize = vnormalize([0.25, 0.25, 0.25])\n"
      "vCross = vcross(vnormalize([1.0, 1.0, 0.0]), vnormalize([-1.0, 1.0, 0.0]))\n"
      "sLengthSin = sin(vlength([0.25, 0.25, 0.25]))\n"
      "sDotSin = sin(vdot(vnormalize([0.25, 0.25, 0.25]), vnormalize([0.25, -0.25, 0.25])))\n"
      "vAddition = [0.25, 0.25, 0.25] + [0.5, 0.5, 0.5]\n"
      "vMultiplication = [0.25, 0.25, 0.25] * 0.5\n"
      "vDivision = [0.25, 0.25, 0.25] / 0.5\n"
      "vSubtraction = [0.25, 0.25, 0.25] - [0.5, 0.5, 0.5]\n"
      "vParenth1 = ([0.25, 0.25, 0.25])\n"
      "vOp1 = [0.25, 0.25, 0.25] + [0.5, 0.5, 0.5] * 2.0\n"
      "vOp2 = ([0.25, 0.25, 0.25] + [0.5, 0.5, 0.5]) * 5.0\n"
      "vOp3 = [0.25, 0.25, 0.25] + ([0.5, 2.0 * 0.25, 0.5] * 5.0)\n"
    );
    return out;
  }
  
  const std::string& getRotationExamples()
  {
    static const std::string out
    (
      "rConstant = RZERO\n"
      "rScalars4 = [0.25, 0.25, 0.25, .35]\n"
      "rNames4 = [example2, example2, example2, example2]\n"
      "rAxisAngle = [[0.25, 0.25, 0.25], .35]\n"
      "rVectorFromTo = [[1.0, 0.0, 0.0], [0.0, 1.0, 1.0]]\n"
      "vRDecayAxis = rdecayaxis([0.25, 0.25, 0.25, .35])\n"
      "sRDecayAngle = rdecayangle([0.25, 0.25, 0.25, .35])\n"
      "vRDecayAxisVectorX = rdecayaxis(X, [0.25, 0.25, 0.25, .35])\n"
      "vRDecayAxisVectorY = rdecayaxis(Y, [0.25, 0.25, 0.25, .35])\n"
      "vRDecayAxisVectorZ = rdecayaxis(Z, [0.25, 0.25, 0.25, .35])\n"
      "rBasePairsXY = [XY, [0.0, 1.0, 0.0], [0.0, 0.0, 1.0]]\n"
      "rBasePairsXZ = [XZ, [0.0, 1.0, 0.0], [1.0, 0.0, 0.0]]\n"
      "rBasePairsYZ = [YZ, [0.0, 0.0, 1.0], [1.0, 0.0, 0.0]]\n"
      "rInverse = rinverse([0.25, 0.25, 0.25, .35])\n"
      "rMultiply = [0.25, 0.25, 0.25, .35] * [0.25, 0.25, 0.25, .35]\n"
      "rDivide = [0.25, 0.25, 0.25, .35] / [0.5, 0.5, 0.5, 2.0]\n"
      "rParenth = [0.25, 0.25, 0.25, .35] * ([0.5, 0.5, 0.5, .5] / [0.1, 0.1, 0.1, .5])\n"
      "vRotatedVector = rrotatevector([[0.0, 0.0, 1.0], PI / 2.0], vnormalize([1.0, 1.0, 1.0]))\n"
    );
    return out;
  }
  
  const std::string& getCSysExamples()
  {
    static const std::string out
    (
      "cVectors = [[1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0], [10.0, 10.0, 10.0]]\n"
      "cNames4 = [vConstantX, vConstantY, vConstantZ, vDoubles]\n"
      "cRotationOrigin = [RZERO, [1.0, 2.0, 3.0]]\n"
      "cAbsolute2 = [RZERO, VZERO]\n"
      "cAbsolute1 = CZERO\n"
      "cInverse = cinverse([RZERO, [5.0, 5.0, 5.0]])\n"
      "vMapVector01 = cmap([RZERO, [10.0, 10.0, 10.0]], [RZERO, [5.0, 5.0, 5.0]], [0.25, 0.25, 0.25])\n"
      "vMapVector02 = cmap([RZERO, [10.0, 10.0, 10.0]], [RZERO, [5.0, 5.0, 5.0]], [0.25, 0.25, 0.25] + [0.125, 0.125, 0.125])\n"
      "cMultiplication = [RZERO, [5.0, 5.0, 5.0]] * [RZERO, [10.0, 10.0, 10.0]]\n"
      "cDivision = [RZERO, [5.0, 5.0, 5.0]] / [RZERO, [10.0, 10.0, 10.0]]\n"
      "cParenth = [RZERO, [5.0, 5.0, 5.0]] / ([RZERO, [10.0, 10.0, 10.0]] * [RZERO, [15.0, 15.0, 15.0]])\n"
      "rCDecayRotation = cdecayrotation([RZERO, [5.0, 5.0, 5.0]])\n"
      "vCDecayOrigin = cdecayorigin([RZERO, [5.0, 5.0, 5.0]])\n"
    );
    return out;
  }
  
  QString buildExamples()
  {
    static const QString qout(QString::fromStdString(getScalarExamples() + '\n' + getVectorExamples() + '\n' + getRotationExamples() + '\n' + getCSysExamples()));
    return qout;
  }
}

using namespace expr;

/* I tried like hell to get the tableview and header to cooperate
 * in making the center column expand and contract when the widget
 * was resized. What a fucking shit show!
 * 'tableViewAll->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);'
 * gets you 90% of the way there. The only problem with this is the user can't resize
 * the last section. WTF. Tried override resizeEvent on widget and then manually
 * adjusting the section sizes, but something else was trying to adjust the sizes
 * and columns would bounce around. Tried subclassing QHeaderView, but it doesn't
 * get resized by itself. After many hours of fucking around and reading qt's source
 * I will just use 'setStretchLastSection' = true on horizontal header and make the user
 * have to resize the last column. I have saved the section sizes so they can be synced
 * across the views, but the user still has to play a game with the last section
 * and the scrollbar.
 */

Widget::Widget(QWidget* parent):
  QWidget(parent)
{
  node = std::make_unique<msg::Node>();
  node->connect(msg::hub());
  sift = std::make_unique<msg::Sift>();
  sift->name = "expr::Widget";
  node->setHandler(std::bind(&msg::Sift::receive, sift.get(), std::placeholders::_1));
  setupDispatcher();
    
  setupGui();
}

Widget::~Widget() = default;

void Widget::setupGui()
{
  QVBoxLayout *mainLayout = new QVBoxLayout();
  mainLayout->setContentsMargins(0, 0, 0, 0);
  this->setLayout(mainLayout);
  
  toolbar = new QToolBar(this);
  toolbar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  toolbar->addAction(QIcon(":/resources/images/debugExpressionGraph.svg"), tr("Write Graph"), this, SLOT(writeOutGraphSlot()));
  toolbar->addAction(QIcon(":/resources/images/viewExpressionExamples.svg"), tr("Toggle View Examples"), this, SLOT(goExamplesTabSlot()));
  toolbar->addAction(QIcon(":/resources/images/loadExpressionExamples.svg"), tr("Load Examples"), this, SLOT(fillInTestManagerSlot()));
  toolbar->addAction(tr("Dump Links"), this, SLOT(dumpLinksSlot()));
  toolbar->setContentsMargins(0, 0, 0, 0);
  
  tabWidget = new QTabWidget(this);
  
  dlg::SplitterDecorated *splitter = new dlg::SplitterDecorated(this);
  splitter->setOrientation(Qt::Vertical);
  splitter->addWidget(toolbar);
  splitter->addWidget(tabWidget);
  splitter->setCollapsible(1, false);
  splitter->setSizes(QList<int>({16, 200}));
  splitter->restoreSettings("ExpressionsSplitter");
  mainLayout->addWidget(splitter);
}

void Widget::writeOutGraphSlot()
{
  boost::filesystem::path p = app::instance()->getApplicationDirectory() / "expressionGraph.dot";
  eManager->writeOutGraph(p.string());
  QDesktopServices::openUrl(QUrl(QString::fromStdString(p.string())));
}

void Widget::groupRenamedSlot(QWidget *tab, const QString &newName)
{
  int index = tabWidget->indexOf(tab);
  assert(index >= 0); //no such widget
  tabWidget->setTabText(index, newName);
}

void Widget::addGroupSlot()
{
  bool ok;
  QString newName = QInputDialog::getText(this, tr("Enter Group Name"), tr("Name:"), QLineEdit::Normal, "Group Name", &ok);
  if (!ok || newName.isEmpty())
    return;
  if (eManager->hasGroup(newName.toStdString()))
  {
    QMessageBox::critical(this, tr("Error:"), tr("Name Already Exists"));
    return;
  }
  
  int groupId;
  groupId = eManager->addGroup(newName.toStdString());
  
  addGroupView(groupId, newName);
}

void Widget::removeGroupSlot(QWidget *tab)
{
  int tabIndex = tabWidget->indexOf(tab);
  assert(tabIndex >= 0); //no such widget
  tabWidget->removeTab(tabIndex);
  tab->deleteLater();
}

void Widget::addGroupView(int idIn, const QString &name)
{
  TableViewGroup *userView = new TableViewGroup(tableViewAll);
  userView->setDragEnabled(true);
  GroupProxyModel *userProxyModel = new GroupProxyModel(*eManager, idIn, userView);
  userProxyModel->setSourceModel(mainTable);
  userView->setModel(userProxyModel);
  tabWidget->addTab(userView, name);
  connect(mainTable, &TableModel::groupChangedSignal, userProxyModel, &GroupProxyModel::groupChangedSlot);
  connect(userView, SIGNAL(groupRenamedSignal(QWidget*,QString)), this, SLOT(groupRenamedSlot(QWidget*,QString)));
  connect(userView, SIGNAL(groupRemovedSignal(QWidget*)), this, SLOT(removeGroupSlot(QWidget*)));
}

void Widget::fillInTestManagerSlot()
{
  auto go = [&](std::function<const std::string&()> f, int groupId, std::ostream &errorStream) -> std::string
  {
    std::istringstream stream(f());
    auto results = mainTable->importExpressions(stream, groupId);
    
    std::ostringstream out;
    for (const auto &r : results)
    {
      if (!r.isAllGood())
        errorStream << r.input << " Failed with: " << r.getError() << std::endl;
    }
    return out.str();
  };
  
  std::ostringstream errors;
  if (!eManager->hasGroup("ScalarExamples"))
  {
    int gId = eManager->addGroup("ScalarExamples");
    go(getScalarExamples, gId, errors);
    addGroupView(gId, "ScalarExamples");
  }
  if (!eManager->hasGroup("VectorExamples"))
  {
    int gId = eManager->addGroup("VectorExamples");
    go(getVectorExamples, gId, errors);
    addGroupView(gId, "VectorExamples");
  }
  if (!eManager->hasGroup("RotationExamples"))
  {
    int gId = eManager->addGroup("RotationExamples");
    go(getRotationExamples, gId, errors);
    addGroupView(gId, "RotationExamples");
  }
  if (!eManager->hasGroup("CSysExamples"))
  {
    int gId = eManager->addGroup("CSysExamples");
    go(getCSysExamples, gId, errors);
    addGroupView(gId, "CSysExamples");
  }
  
  std::string errorMessage = errors.str();
  if (!errorMessage.empty())
  {
    app::Message am;
    am.infoMessage = QString::fromStdString(errorMessage);
    node->sendBlocked(msg::Message(msg::Request | msg::Info | msg::Text, am));
  }
}

void Widget::dumpLinksSlot()
{
  std::ostringstream stream;
  stream << std::endl << "expression links: " << std::endl;
  eManager->dumpLinks(stream);
  
  app::Message am;
  am.infoMessage = QString::fromStdString(stream.str());
  node->sendBlocked(msg::Message(msg::Request | msg::Info | msg::Text, am));
}

void Widget::goExamplesTabSlot()
{
  static bool added = false;
  static QTextEdit *examplesTab = nullptr;
  if (!examplesTab)
  {
    examplesTab = new QTextEdit(this);
    examplesTab->setReadOnly(true);
    examplesTab->setLineWrapMode(QTextEdit::NoWrap);
    examplesTab->setText(buildExamples());
    examplesTab->setObjectName("examples");
  }
  
  if (!added)
  {
    tabWidget->insertTab(2, examplesTab, tr("Examples"));
    tabWidget->setCurrentIndex(2);
    added = true;
  }
  else
  {
    int index = tabWidget->indexOf(examplesTab);
    assert(!(index < 0));
    tabWidget->removeTab(index);
    added = false;
  }
}

void Widget::setupDispatcher()
{
  sift->insert
  (
    {
      std::make_pair
      (
        msg::Response | msg::Pre | msg::Close | msg::Project
        , std::bind(&Widget::closeProjectDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Post | msg::Open | msg::Project
        , std::bind(&Widget::openNewProjectDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Post | msg::New | msg::Project
        , std::bind(&Widget::openNewProjectDispatched, this, std::placeholders::_1)
      )
    }
  );
}

void Widget::closeProjectDispatched(const msg::Message&)
{
  
  tabWidget->clear();
  tableViewAll->deleteLater(); //should delete all child models and views.
  tableViewAll = nullptr;
  mainTable = nullptr; //was a child of tabwidget, so should be deleted.
  eManager = nullptr; //Manager owned by project. should be deleted there.
}

void Widget::openNewProjectDispatched(const msg::Message&)
{
  assert(!eManager);
  eManager = &(static_cast<app::Application *>(qApp)->getProject()->getManager());
  assert(eManager);
  
  assert(!tableViewAll);
  tableViewAll = new TableViewAll(tabWidget);
  tableViewAll->setDragEnabled(true);
  
  assert(!mainTable);
  mainTable = new TableModel(*eManager, tableViewAll);
  
  AllProxyModel *allModel = new AllProxyModel(tableViewAll);
  allModel->setSourceModel(mainTable);
  tableViewAll->setModel(allModel);
  tabWidget->addTab(tableViewAll, tr("All"));
  connect(tableViewAll, SIGNAL(addGroupSignal()), this, SLOT(addGroupSlot()));
  
  TableViewSelection *selectionView = new TableViewSelection(tableViewAll);
  SelectionProxyModel *selectionModel = new SelectionProxyModel(*eManager, selectionView);
  selectionModel->setSourceModel(mainTable);
  selectionView->setModel(selectionModel);
  tabWidget->addTab(selectionView, tr("Selection"));
  
  for (const auto &gId : eManager->getGroupIds())
  {
    auto oName = eManager->getGroupName(gId);
    assert(oName);
    addGroupView(gId, QString::fromStdString(*oName));
  }
}
