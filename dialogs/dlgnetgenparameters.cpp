/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2020 Thomas S. Anderson blobfish.at.gmx.com
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

#include <QSettings>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QScrollArea>

#include "mesh/mshparameters.h"
#include "dialogs/dlgnetgenparameters.h"

using namespace dlg;

struct NetgenParameters::Stow
{
  QGridLayout *layout = nullptr;
  QPushButton *resetButton = nullptr;
  QScrollArea *scrollArea = nullptr;
  
  Stow(){}
  
  void buildGui(QWidget *parent)
  {
    resetButton = new QPushButton(tr("Reset"), parent);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(resetButton);
    
    layout = new QGridLayout();
    
    auto addDouble = [&](const QString &name, double value, int row)
    {
      QLabel *label = new QLabel(name, parent);
      QLineEdit *edit = new QLineEdit(parent);
      edit->setText(QString::number(value));
      layout->addWidget(label, row, 0, Qt::AlignVCenter | Qt::AlignRight);
      layout->addWidget(edit, row, 1);
    };
    
    auto addInt = [&](const QString &name, int value, int row)
    {
      QLabel *label = new QLabel(name, parent);
      QLineEdit *edit = new QLineEdit(parent);
      edit->setText(QString::number(value));
      layout->addWidget(label, row, 0, Qt::AlignVCenter | Qt::AlignRight);
      layout->addWidget(edit, row, 1);
    };

    auto addBoolean = [&](const QString &name, bool value, int row)
    {
      QLabel *label = new QLabel(name, parent);
      QComboBox *box = new QComboBox(parent);
      box->addItem(tr("True"));
      box->addItem(tr("False"));
      if (value)
        box->setCurrentIndex(0);
      else
        box->setCurrentIndex(1);
      layout->addWidget(label, row, 0, Qt::AlignVCenter | Qt::AlignRight);
      layout->addWidget(box, row, 1);
    };
    
    msh::prm::Netgen prms;
    addBoolean(tr("Use Local"), prms.useLocalH, 0);
    addDouble(tr("Maximum Height"), prms.maxH, 1);
    addDouble(tr("Minimum Height"), prms.minH, 2);
    addDouble(tr("Fineness (0.0, 1.0)"), prms.fineness, 3);
    addDouble(tr("Grading (0.1, 1.0)"), prms.grading, 4);
    addDouble(tr("Elements Per Edge (0.2, 5.0)"), prms.elementsPerEdge, 5);
    addDouble(tr("Elements Per Curve (0.2, 5.0)"), prms.elementsPerCurve, 6);
    addBoolean(tr("Close Edge Enable"), prms.closeEdgeEnable, 7);
    addDouble(tr("Close Edge Factor (0.2, 8.0)"), prms.closeEdgeFactor, 8);
    addBoolean(tr("Minimum Edge Length Enable"), prms.minEdgeLenEnable, 9);
    addDouble(tr("Minimum Edge Length (0.2, 5.0)"), prms.minEdgeLen, 10);
    addBoolean(tr("Second Order"), prms.secondOrder, 11);
    addBoolean(tr("Quad Dominated"), prms.quadDominated, 12);
    addBoolean(tr("Optimum Surf Mesh Enable"), prms.optSurfMeshEnable, 13);
    addBoolean(tr("Optimum Volume Mesh Enable"), prms.optVolMeshEnable, 14);
    addInt(tr("2d Optimize Steps"), prms.optSteps2d, 15);
    addInt(tr("3d Optimize Steps"), prms.optSteps3d, 16);
    addBoolean(tr("Invert Volume Elements"), prms.invertTets, 17);
    addBoolean(tr("Invert Surface Elements"), prms.invertTrigs, 18);
    addBoolean(tr("Check Overlap"), prms.checkOverlap, 19);
    addBoolean(tr("Check Boundary Overlap"), prms.checkOverlappingBoundary, 20);
    
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addLayout(buttonLayout);
    mainLayout->addLayout(layout);
    mainLayout->addStretch();
    
    QWidget *dummy = new QWidget(parent);
    dummy->setLayout(mainLayout);
    
    scrollArea = new QScrollArea(parent);
    scrollArea->setWidget(dummy);
    
    QVBoxLayout *wrapper = new QVBoxLayout();
    wrapper->addWidget(scrollArea);
    
    parent->setLayout(wrapper);
  }
  
  void setParameters(const msh::prm::Netgen &prms)
  {
    auto setDouble = [&](int row, double value)
    {
      QLineEdit *edit = dynamic_cast<QLineEdit*>(layout->itemAtPosition(row, 1)->widget());
      assert(edit);
      edit->setText(QString::number(value));
    };
    
    auto setInt = [&](int row, int value)
    {
      QLineEdit *edit = dynamic_cast<QLineEdit*>(layout->itemAtPosition(row, 1)->widget());
      assert(edit);
      edit->setText(QString::number(value));
    };
    
    auto setBool = [&](int row, bool value)
    {
      QComboBox *box = dynamic_cast<QComboBox*>(layout->itemAtPosition(row, 1)->widget());
      assert(box);
      if (value)
        box->setCurrentIndex(0);
      else
        box->setCurrentIndex(1);
    };
  
    setBool(0, prms.useLocalH);
    setDouble(1, prms.maxH);
    setDouble(2, prms.minH);
    setDouble(3, prms.fineness);
    setDouble(4, prms.grading);
    setDouble(5, prms.elementsPerEdge);
    setDouble(6, prms.elementsPerCurve);
    setBool(7, prms.closeEdgeEnable);
    setDouble(8, prms.closeEdgeFactor);
    setBool(9, prms.minEdgeLenEnable);
    setDouble(10, prms.minEdgeLen);
    setBool(11, prms.secondOrder);
    setBool(12, prms.quadDominated);
    setBool(13, prms.optSurfMeshEnable);
    setBool(14, prms.optVolMeshEnable);
    setInt(15, prms.optSteps2d);
    setInt(16, prms.optSteps3d);
    setBool(17, prms.invertTets);
    setBool(18, prms.invertTrigs);
    setBool(19, prms.checkOverlap);
    setBool(20, prms.checkOverlappingBoundary);
  }
  
  msh::prm::Netgen getParameters() const
  {
    auto getDouble = [&](int row) -> double
    {
      QLineEdit *edit = dynamic_cast<QLineEdit*>(layout->itemAtPosition(row, 1)->widget());
      assert(edit);
      return edit->text().toDouble();
    };
    
    auto getInt = [&](int row) -> int
    {
      QLineEdit *edit = dynamic_cast<QLineEdit*>(layout->itemAtPosition(row, 1)->widget());
      assert(edit);
      return edit->text().toInt();
    };
    
    auto getBool = [&](int row) -> bool
    {
      QComboBox *box = dynamic_cast<QComboBox*>(layout->itemAtPosition(row, 1)->widget());
      assert(box);
      if (box->currentIndex() == 0)
        return true;
      return false;
    };
    
    msh::prm::Netgen out;
    
    out.useLocalH = getBool(0);
    out.maxH = getDouble(1);
    out.minH = getDouble(2);
    out.fineness = getDouble(3);
    out.grading = getDouble(4);
    out.elementsPerEdge = getDouble(5);
    out.elementsPerCurve = getDouble(6);
    out.closeEdgeEnable = getBool(7);
    out.closeEdgeFactor = getDouble(8);
    out.minEdgeLenEnable = getBool(9);
    out.minEdgeLen = getDouble(10);
    out.secondOrder = getBool(11);
    out.quadDominated = getBool(12);
    out.optSurfMeshEnable = getBool(13);
    out.optVolMeshEnable = getBool(14);
    out.optSteps2d = getInt(15);
    out.optSteps3d = getInt(16);
    out.invertTets = getBool(17);
    out.invertTrigs = getBool(18);
    out.checkOverlap = getBool(19);
    out.checkOverlappingBoundary = getBool(20);
    
    out.ensureValidValues();
    return out;
  }
};

NetgenParameters::NetgenParameters(QWidget *pIn)
: QWidget(pIn)
, stow(new Stow())
{
  stow->buildGui(this);
  connect(stow->resetButton, &QPushButton::clicked, this, &NetgenParameters::resetParameters);
}

NetgenParameters::~NetgenParameters() = default;

void NetgenParameters::setParameters(const msh::prm::Netgen &pIn)
{
  stow->setParameters(pIn);
}

msh::prm::Netgen NetgenParameters::getParameters() const
{
  return stow->getParameters();
}

void NetgenParameters::resetParameters()
{
  stow->setParameters(msh::prm::Netgen());
}
