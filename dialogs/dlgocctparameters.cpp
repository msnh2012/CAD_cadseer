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
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QScrollArea>

#include "mesh/mshparameters.h"
#include "dialogs/dlgocctparameters.h"

using namespace dlg;

OCCTParameters::OCCTParameters(QWidget *parent)
: QWidget(parent)
{
  msh::prm::OCCT prms;
  buildGui(prms);
}

void OCCTParameters::buildGui(const msh::prm::OCCT &prms)
{
  resetButton = new QPushButton(tr("Reset"), this);
  QHBoxLayout *resetLayout = new QHBoxLayout();
  resetLayout->addStretch();
  resetLayout->addWidget(resetButton);
  connect(resetButton, &QPushButton::clicked, this, &OCCTParameters::resetParameters);
  
  layout = new QGridLayout();
  
  auto addDouble = [&](const QString &name, double value, int row)
  {
    QLabel *label = new QLabel(name, this);
    QLineEdit *edit = new QLineEdit(this);
    edit->setText(QString::number(value));
    layout->addWidget(label, row, 0, Qt::AlignVCenter | Qt::AlignRight);
    layout->addWidget(edit, row, 1);
  };
  
  auto addBoolean = [&](const QString &name, bool value, int row)
  {
    QLabel *label = new QLabel(name, this);
    QComboBox *box = new QComboBox(this);
    box->addItem(tr("True"));
    box->addItem(tr("False"));
    if (value)
      box->setCurrentIndex(0);
    else
      box->setCurrentIndex(1);
    layout->addWidget(label, row, 0, Qt::AlignVCenter | Qt::AlignRight);
    layout->addWidget(box, row, 1);
  };
  
  addDouble(tr("Angle"), prms.Angle, 0);
  addDouble(tr("Deflection"), prms.Deflection, 1);
  addDouble(tr("Angle Interior"), prms.AngleInterior, 2);
  addDouble(tr("Deflection Interior"), prms.DeflectionInterior, 3);
  addDouble(tr("Minimum Size"), prms.MinSize, 4);
  addBoolean(tr("In Parallel"), prms.InParallel, 5);
  addBoolean(tr("Relative"), prms.Relative, 6);
  addBoolean(tr("Internal Vertices Mode"), prms.InternalVerticesMode, 7);
  addBoolean(tr("Control Surface Deflection"), prms.ControlSurfaceDeflection, 8);
  addBoolean(tr("Clean Model"), prms.CleanModel, 9);
  addBoolean(tr("Adjust Minimum Size"), prms.AdjustMinSize, 10);
  
  QVBoxLayout *mainLayout = new QVBoxLayout();
  mainLayout->addLayout(resetLayout);
  mainLayout->addLayout(layout);
  mainLayout->addStretch();
  
  QWidget *dummy = new QWidget(this);
  dummy->setLayout(mainLayout);
  
  scrollArea = new QScrollArea(this);
  scrollArea->setWidget(dummy);
  
  QVBoxLayout *wrapper = new QVBoxLayout();
  wrapper->addWidget(scrollArea);
  
  this->setLayout(wrapper);
}

void OCCTParameters::resetParameters()
{
  setOCCT(msh::prm::OCCT());
}

void OCCTParameters::setOCCT(const msh::prm::OCCT &prms)
{
  auto setDouble = [&](int row, double value)
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
  
  setDouble(0, prms.Angle);
  setDouble(1, prms.Deflection);
  setDouble(2, prms.AngleInterior);
  setDouble(3, prms.DeflectionInterior);
  setDouble(4, prms.MinSize);
  setBool(5, prms.InParallel);
  setBool(6, prms.Relative);
  setBool(7, prms.InternalVerticesMode);
  setBool(8, prms.ControlSurfaceDeflection);
  setBool(9, prms.CleanModel);
  setBool(10, prms.AdjustMinSize);
}

msh::prm::OCCT OCCTParameters::getParameters() const
{
  auto toDouble = [&](int row) -> double
  {
    QLineEdit *edit = dynamic_cast<QLineEdit*>(layout->itemAtPosition(row, 1)->widget());
    assert(edit);
    return edit->text().toDouble();
  };
  
  auto toBoolean = [&](int row) -> bool
  {
    QComboBox *box = dynamic_cast<QComboBox*>(layout->itemAtPosition(row, 1)->widget());
    assert(box);
    if (box->currentIndex() == 0)
      return true;
    return false;
  };
  
  msh::prm::OCCT out;
  out.Angle = toDouble(0);
  out.Deflection = toDouble(1);
  out.AngleInterior = toDouble(2);
  out.DeflectionInterior = toDouble(3);
  out.MinSize = toDouble(4);
  out.InParallel = toBoolean(5);
  out.Relative = toBoolean(6);
  out.InternalVerticesMode = toBoolean(7);
  out.ControlSurfaceDeflection = toBoolean(8);
  out.CleanModel = toBoolean(9);
  out.AdjustMinSize = toBoolean(10);
  
  return out;
}
