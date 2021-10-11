/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2019 Thomas S. Anderson blobfish.at.gmx.com
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

#include <boost/optional.hpp>

#include <Law_Composite.hxx>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsEllipseItem>
#include <QGraphicsSceneMouseEvent>
#include <QLineF>
#include <QDialogButtonBox>
#include <QTimer>
#include <QMenu>

#include <osg/Vec3d>

#include "tools/idtools.h"
#include "tools/tlsstring.h"
#include "law/lwfcue.h"
#include "dialogs/dlgwidgetgeometry.h"
#include "dialogs/dlgenterfilter.h"
#include "commandview/cmvlawfunctionwidget.h"

static const int QTKEY = 0;
using boost::uuids::uuid;

namespace cmv
{
  class LawScene : public QGraphicsScene
  {
  public:
    LawScene(QObject *parent) : QGraphicsScene(parent) {}
    
    uuid getParameterSelectionId()
    {
      if (selectedParameterItem)
        return getParameterId(selectedParameterItem);
      return gu::createNilId();
    }
    
    void resetSelection()
    {
      selectedParameterItem = nullptr;
      selectionChanged();
    }
    
    //parameters
    double xmin = 0.0;
    double xmax = 1.0;
    double ymin = 0.0;
    double ymax = 1.0;
  protected:
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent) override
    {
      QGraphicsScene::mouseReleaseEvent(mouseEvent);
      
      if (mouseEvent->button() == Qt::LeftButton)
      {
        auto hits = items(mouseEvent->scenePos());
        for (auto *i : hits)
        {
          uuid tempId = getParameterId(i);
          if (tempId.is_nil())
            continue;
          
          if (selectedParameterItem)
          {
            selectedParameterItem->setSelected(false);
            unhighlightParameter(selectedParameterItem);
          }
          QGraphicsEllipseItem *eItem = dynamic_cast<QGraphicsEllipseItem*>(i);
          assert(eItem);
          selectedParameterItem = eItem;
          highlightParameter(selectedParameterItem);
          selectionChanged();
          break;
        }
      }
      
      if (mouseEvent->button() == Qt::RightButton)
      {
        scenePoint = mouseEvent->scenePos();
        LawFunctionWidget *lfw = dynamic_cast<LawFunctionWidget*>(this->parent());
        assert(lfw);
        
        QMenu menu;
        if (lfw->getCue().type == lwf::Type::composite)
        {
          menu.addAction(tr("Append Constant"), this, &LawScene::appendConstant);
          menu.addAction(tr("Append Linear"), this, &LawScene::appendLinear);
          menu.addAction(tr("Append Interpolate"), this, &LawScene::appendInterpolate);
          menu.addAction(tr("Remove Law"), this, &LawScene::removeLaw);
        }
        if (lfw->isInterpolate(scenePoint.x()))
        {
          menu.addAction(tr("Add Internal Point"), this, &LawScene::addInternalParameter);
          menu.addAction(tr("Remove Internal Point"), this, &LawScene::removeInternalParameter);
        }
        if (!menu.isEmpty())
          menu.exec(mouseEvent->screenPos());
        
//         const lwf::Cue &cue = lfw->getCue();
//         if (cue.type == lwf::Type::composite)
//         {
//           QMenu menu;
//           menu.addAction(tr("Smooth"), this, &LawScene::smooth);
//           menu.exec(mouseEvent->screenPos());
//         }
      }
    }
    
  private:
    QGraphicsEllipseItem *selectedParameterItem = nullptr;
    QPointF scenePoint;
    
    uuid getParameterId(QGraphicsItem *item)
    {
      QVariant v = item->data(QTKEY);
      if (v.isNull())
        return gu::createNilId();
      return gu::stringToId(v.toString().toStdString());
    }
    
    void highlightParameter(QGraphicsEllipseItem *item)
    {
      QBrush pointBrush(Qt::yellow);
      item->setBrush(pointBrush);
    }
    
    void unhighlightParameter(QGraphicsEllipseItem *item)
    {
      QBrush pointBrush(Qt::blue);
      item->setBrush(pointBrush);
    }
    
    void addInternalParameter()
    {
      LawFunctionWidget *lfw = dynamic_cast<LawFunctionWidget*>(this->parent());
      assert(lfw);
      lfw->addInternalPoint(scenePoint.x());
    }
    
    void removeInternalParameter()
    {
      LawFunctionWidget *lfw = dynamic_cast<LawFunctionWidget*>(this->parent());
      assert(lfw);
      lfw->removeInternalPoint(scenePoint.x());
    }
    
    void appendConstant()
    {
      LawFunctionWidget *lfw = dynamic_cast<LawFunctionWidget*>(this->parent());
      assert(lfw);
      lfw->appendConstant();
    }
    
    void appendLinear()
    {
      LawFunctionWidget *lfw = dynamic_cast<LawFunctionWidget*>(this->parent());
      assert(lfw);
      lfw->appendLinear();
    }
    
    void appendInterpolate()
    {
      LawFunctionWidget *lfw = dynamic_cast<LawFunctionWidget*>(this->parent());
      assert(lfw);
      lfw->appendInterpolate();
    }
    
    void removeLaw()
    {
      LawFunctionWidget *lfw = dynamic_cast<LawFunctionWidget*>(this->parent());
      assert(lfw);
      lfw->removeLaw(scenePoint.x());
    }
    
//     void smooth()
//     {
//       LawFunctionWidget *lfw = dynamic_cast<LawFunctionWidget*>(this->parent());
//       assert(lfw);
//       lfw->smooth();
//     }
  };
  
  class LawView : public QGraphicsView
  {
  public:
    LawView(QGraphicsScene *scene, QWidget *parent) : QGraphicsView(scene, parent)
    {
      setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
      setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      connect
      (
        &resizeTimer,
        &QTimer::timeout,
        [&]()
        {
          LawFunctionWidget *lfw = dynamic_cast<LawFunctionWidget*>(this->parentWidget());
          assert(lfw);
          lfw->updateGui();
          resizeTimer.stop();
        }
      );
    }
    void fitToData()
    {
      LawScene *ls = dynamic_cast<LawScene*>(scene());
      assert(ls);
      double padding = std::max(ls->xmax - ls->xmin, ls->ymax - ls->ymin) * 0.05;
      fitInView(ls->xmin - padding, ls->ymin - padding, ls->xmax + padding, ls->ymax + padding, Qt::KeepAspectRatio);
    }
  protected:
    void resizeEvent (QResizeEvent * event) override
    {
      QGraphicsView::resizeEvent(event);
      fitToData();
      resizeTimer.start(1000);
    }
  private:
    QTimer resizeTimer;
  };
  
  struct LawFunctionWidget::Stow
  {
    lwf::Cue cue;
    QVBoxLayout *mainLayout;
    QComboBox *typeCombo;
    QLabel *pLabel;
    QLineEdit *pEdit;
    QLabel *vLabel;
    QLineEdit *vEdit;
    QLabel *dLabel;
    QLineEdit *dEdit;
    LawScene *graphicsScene;
    LawView *graphicsView;
    
    prm::Parameter* findParameter(const uuid &testId)
    {
      prm::Parameters prms = cue.getParameters();
      for (auto *p : prms)
      {
        if (p->getId() == testId)
          return p;
      }
      return nullptr;
    }
  };
}

using namespace cmv;

LawFunctionWidget::LawFunctionWidget(QWidget *parent)
: QWidget(parent)
, stow(new Stow())
{
  buildGui();
}

LawFunctionWidget::~LawFunctionWidget() {}

void LawFunctionWidget::setCue(const lwf::Cue &cueIn)
{
  stow->cue = cueIn;
  
  //we don't want to emit a signal that will over write the type being set.
  QSignalBlocker contactBlocker(stow->typeCombo);
  stow->typeCombo->setCurrentIndex(static_cast<int>(cueIn.type) - 1); //there is a 'none' type for enum.
  
  updateGui();
}

const lwf::Cue& LawFunctionWidget::getCue()
{
  return stow->cue;
}

void LawFunctionWidget::selectionChangedSlot()
{
  auto disableWidgets = [&]()
  {
    stow->pEdit->setDisabled(true);
    stow->vEdit->setDisabled(true);
    stow->dEdit->setDisabled(true);
  };
  
  uuid psId = stow->graphicsScene->getParameterSelectionId();
  if (psId.is_nil())
  {
    disableWidgets();
    return;
  }
  else
  {
    prm::Parameter *p = stow->findParameter(psId);
    if (p)
    {
      osg::Vec3d pv = p->getVector();
      stow->pEdit->setEnabled(true);
      stow->vEdit->setEnabled(true);
      stow->dEdit->setEnabled(true);
      stow->pEdit->setText(QString::fromStdString(tls::prettyDouble(pv.x())));
      stow->vEdit->setText(QString::fromStdString(tls::prettyDouble(pv.y())));
      stow->dEdit->setText(QString::fromStdString(tls::prettyDouble(pv.z())));
    }
    else
      disableWidgets();
  }
}

void LawFunctionWidget::comboChangedSlot(int index)
{
  if (index == 0)
    stow->cue.setConstant(1.0);
  else if (index == 1)
    stow->cue.setLinear(1.0, 2.0);
  else if (index == 2)
    stow->cue.setInterpolate(1.0, 2.0);
  else if (index == 3)
  {
    stow->cue.setComposite(1.0);
    stow->cue.appendConstant(.25);
    stow->cue.appendInterpolate(.75, 1.5);
    stow->cue.appendLinear(1.0, 2.0);
    stow->cue.smooth();
  }
  
  updateGui();
  lawIsDirty();
}

void LawFunctionWidget::pChangedSlot()
{
  changed(stow->pEdit->text(), 0);
  valueChanged();
}

void LawFunctionWidget::vChangedSlot()
{
  if (stow->cue.type == lwf::Type::constant)
  {
    //for constant we need to change both parameters.
    //so just call set on cue.
    double nv = stow->vEdit->text().toDouble();
    stow->cue.setConstant(nv);
    updateGui();
  }
  else
    changed(stow->vEdit->text(), 1);
  
  valueChanged();
}

void LawFunctionWidget::dChangedSlot()
{
  changed(stow->dEdit->text(), 2);
  valueChanged();
}

void LawFunctionWidget::changed(const QString &textIn, int index)
{
  assert(index >= 0 && index < 3);
  assert(stow->cue.boundaries.size() >= 2);
  double nv = textIn.toDouble();
  uuid pId = stow->graphicsScene->getParameterSelectionId();
  if (pId.is_nil())
    return;
  prm::Parameter *parameter = stow->findParameter(pId);
  assert(parameter);
  osg::Vec3d value = parameter->getVector();
  value[index] = nv;
  parameter->setValue(value);
  
  //if we changed the value of a composite with a constant, update the other parameter
  if (stow->cue.type == lwf::Type::composite && index == 1)
  {
    assert((stow->cue.datas.size() + 1) == stow->cue.boundaries.size());
    //find index of boundary parameter
    boost::optional<std::size_t> bi = boost::make_optional(false, std::size_t());
    std::size_t count = 0;
    for (const auto &b : stow->cue.boundaries)
    {
      if (b.getId() == pId)
      {
        bi = count;
        break;
      }
      count++;
    }
    if (bi)
    {
      auto match = [&](std::size_t bi)
      {
        osg::Vec3d tv = stow->cue.boundaries.at(bi).getVector();
        tv.y() = value.y();
        stow->cue.boundaries.at(bi).setValue(tv);
      };
      if (bi.get() != 0)
      {
        if (stow->cue.datas.at(bi.get() - 1).subType == lwf::Type::constant)
          match(bi.get() - 1);
      }
      else if (stow->cue.periodic)
        match(stow->cue.boundaries.size() - 1);
      if (bi.get() != stow->cue.boundaries.size() - 1)
      {
        if (stow->cue.datas.at(bi.get()).subType == lwf::Type::constant)
          match(bi.get() + 1);
      }
      else if (stow->cue.periodic)
        match(0);
    }
  }
  
  stow->cue.smooth();
  
  updateGui();
}

bool LawFunctionWidget::isInterpolate(double prm)
{
  if (stow->cue.type == lwf::Type::interpolate)
    return true;
  if (stow->cue.type == lwf::Type::composite)
  {
    int index = stow->cue.compositeIndex(prm);
    if (index == -1)
      return false;
    if (stow->cue.datas.at(index).subType == lwf::Type::interpolate)
      return true;
  }
  
  return false;
}

void LawFunctionWidget::addInternalPoint(double prm)
{
  if (stow->cue.type == lwf::Type::interpolate || stow->cue.type == lwf::Type::composite)
  {
    stow->cue.addInternalParameter(prm);
    updateGui();
    lawIsDirty();
  }
}

void LawFunctionWidget::removeInternalPoint(double prm)
{
  if (stow->cue.type == lwf::Type::interpolate || stow->cue.type == lwf::Type::composite)
  {
    stow->cue.removeInternalParameter(prm);
    updateGui();
    lawIsDirty();
  }
}

// void LawFunctionWidget::smooth()
// {
//   if (stow->cue.type == lwf::Type::composite)
//   {
//     stow->cue.smooth();
//     updateGui();
//   }
// }

void LawFunctionWidget::appendConstant()
{
  double end = stow->cue.squeezeBack();
  stow->cue.appendConstant(end);
  stow->cue.smooth();
  updateGui();
  lawIsDirty();
}

void LawFunctionWidget::appendLinear()
{
  double end = stow->cue.squeezeBack();
  double value = stow->cue.boundaries.back().getVector().y();
  value += 0.1;
  stow->cue.appendLinear(end, value);
  stow->cue.smooth();
  updateGui();
  lawIsDirty();
}

void LawFunctionWidget::appendInterpolate()
{
  double end = stow->cue.squeezeBack();
  double value = stow->cue.boundaries.back().getVector().y();
  value += 0.1;
  stow->cue.appendInterpolate(end, value);
  stow->cue.smooth();
  updateGui();
  lawIsDirty();
}

void LawFunctionWidget::removeLaw(double p)
{
  //don't remove the last law. User will have to add another law
  //then can remove the current one.
  if (stow->cue.boundaries.size() < 3)
    return;
  
  int index = stow->cue.compositeIndex(p);
  if (index == -1)
    return;
  stow->cue.remove(index);
  stow->cue.smooth();
  updateGui();
  lawIsDirty();
}

void LawFunctionWidget::buildGui()
{
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  
  stow->typeCombo = new QComboBox(this);
  stow->typeCombo->addItem(tr("Constant"));
  stow->typeCombo->addItem(tr("Linear"));
  stow->typeCombo->addItem(tr("Interpolate"));
  stow->typeCombo->addItem(tr("Composite"));
  QVBoxLayout *typeLayout = new QVBoxLayout();
  typeLayout->addWidget(stow->typeCombo);
  typeLayout->addStretch();
  connect(stow->typeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(comboChangedSlot(int)));
  
  dlg::EnterFilter *ef = new dlg::EnterFilter(this);
  ef->setShouldClearFocus(true);
  stow->pLabel = new QLabel(tr("Parameter"), this);
  stow->pEdit = new QLineEdit(this);
  stow->pEdit->setDisabled(true);
  stow->pEdit->installEventFilter(ef);
  connect(stow->pEdit, &QLineEdit::editingFinished, this, &LawFunctionWidget::pChangedSlot);
  stow->vLabel = new QLabel(tr("Value"), this);
  stow->vEdit = new QLineEdit(this);
  stow->vEdit->setDisabled(true);
  stow->vEdit->installEventFilter(ef);
  connect(stow->vEdit, &QLineEdit::editingFinished, this, &LawFunctionWidget::vChangedSlot);
  stow->dLabel = new QLabel(tr("Derivative"), this);
  stow->dEdit = new QLineEdit(this);
  stow->dEdit->setDisabled(true);
  stow->dEdit->installEventFilter(ef);
  connect(stow->dEdit, &QLineEdit::editingFinished, this, &LawFunctionWidget::dChangedSlot);
  QGridLayout *pvdLayout = new QGridLayout();
  pvdLayout->addWidget(stow->pLabel, 0, 0, Qt::AlignVCenter | Qt::AlignRight);
  pvdLayout->addWidget(stow->pEdit, 0, 1, Qt::AlignVCenter | Qt::AlignLeft);
  pvdLayout->addWidget(stow->vLabel, 1, 0, Qt::AlignVCenter | Qt::AlignRight);
  pvdLayout->addWidget(stow->vEdit, 1, 1, Qt::AlignVCenter | Qt::AlignLeft);
  pvdLayout->addWidget(stow->dLabel, 2, 0, Qt::AlignVCenter | Qt::AlignRight);
  pvdLayout->addWidget(stow->dEdit, 2, 1, Qt::AlignVCenter | Qt::AlignLeft);
  
  QHBoxLayout *topLayout = new QHBoxLayout();
  topLayout->addLayout(typeLayout);
  topLayout->addStretch();
  topLayout->addLayout(pvdLayout);
  
  stow->graphicsScene = new LawScene(this);
  stow->graphicsView = new LawView(stow->graphicsScene, this);
  stow->graphicsView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  
  connect(stow->graphicsScene, &LawScene::selectionChanged, this, &LawFunctionWidget::selectionChangedSlot);
  
  stow->mainLayout = new QVBoxLayout();
  stow->mainLayout->setContentsMargins(0, 0, 0, 0); //makes this widget imperceptible
  stow->mainLayout->addLayout(topLayout);
  stow->mainLayout->addWidget(stow->graphicsView, 255);
//   stow->mainLayout->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
  this->setLayout(stow->mainLayout);
  
  //put actions for adding and removing to composite in context menu.
}

void LawFunctionWidget::updateGui()
{
  //WTF. Qt y axis is inverted.
  stow->graphicsScene->resetSelection();
  stow->graphicsScene->clear();
  
  qreal pointFactor = std::max(stow->graphicsScene->width(), stow->graphicsScene->height());
  
  auto buildPoint = [&]() -> QGraphicsEllipseItem*
  {
    float ps = 0.03 * pointFactor; //point size
    QPen pointPen;
    pointPen.setWidthF(0.001);
    QBrush pointBrush(Qt::blue);
    return stow->graphicsScene->addEllipse(-ps/2.0f, -ps/2.0f, ps, ps, pointPen, pointBrush);
  };
  
  //use size of view to calculate steps for bspline laws. 
  auto calcDiscreteSteps = [&]() -> int
  {
    QSize viewSize = stow->graphicsView->size();
    int maxSize = std::max(viewSize.width(), viewSize.height());
    int steps = static_cast<int>(static_cast<double>(maxSize) / 10.0);
    steps = std::max(steps, 5); //min of 5 steps
    return steps;
  };
  
  //set scene rect to parameter ranges with some padding
  //build points.
  prm::Parameters prms = stow->cue.getParameters();
  double xmin = 0.0;
  double xmax = 1.0;
  double ymin = 0.0;
  double ymax = 1.0;
  for (auto *p : prms) //have to establish range first.
  {
    osg::Vec3d t = p->getVector();
    xmin = std::min(xmin, t.x());
    xmax = std::max(xmax, t.x());
    ymin = std::min(ymin, t.y());
    ymax = std::max(ymax, t.y());
  }
  stow->graphicsScene->xmin = xmin;
  stow->graphicsScene->xmax = xmax;
  stow->graphicsScene->ymin = ymin;
  stow->graphicsScene->ymax = ymax;
  
  auto buildParameterPoint = [&] (const prm::Parameter &pIn)
  {
    osg::Vec3d t = pIn.getVector();
    QGraphicsEllipseItem *point = buildPoint();
    point->setPos(t.x(), ymax - t.y());
    point->setData(QTKEY, QString::fromStdString(gu::idToString(pIn.getId())));
  };
  
  for (auto *p : prms)
    buildParameterPoint(*p);
  
  QPen axisPen;
  axisPen.setWidthF(0.001f);
  axisPen.setStyle(Qt::DotLine);
  QLineF xAxis(xmin, ymax, xmax, ymax);
  QLineF yAxis(0.0, ymin, 0.0, ymax);
  stow->graphicsScene->addLine(xAxis, axisPen);
  stow->graphicsScene->addLine(yAxis, axisPen);
  
  auto drawLine = [&] (const osg::Vec3d &s, const osg::Vec3d &f)
  {
    QPen pen;
    pen.setColor(Qt::red);
    pen.setWidthF(0.01f);
    auto *line = stow->graphicsScene->addLine(s.x(), ymax - s.y(), f.x(), ymax - f.y(), pen);
    line->setZValue(-0.1);
  };
  
  auto drawInterpolate = [&](opencascade::handle<Law_Function> law)
  {
    QPen pen;
    pen.setColor(Qt::red);
    pen.setWidthF(0.01f);
    
    auto points = lwf::discreteLaw(law, calcDiscreteSteps());
    //goofy construction to eliminate compiler warnings in release mode.
    //https://www.boost.org/doc/libs/1_70_0/libs/optional/doc/html/boost_optional/tutorial/gotchas/false_positive_with__wmaybe_uninitialized.html
    boost::optional<osg::Vec3d> lp = boost::make_optional(false, osg::Vec3d()); // last point
    for (const auto &p : points)
    {
      //skip first point
      if (lp)
      {
        auto *line = stow->graphicsScene->addLine(lp.get().x(), ymax - lp.get().y(), p.x(), ymax - p.y(), pen);
        line->setZValue(-0.1);
      }
      lp = p;
    }
  };
  
  //build curves
  lwf::Type lft = stow->cue.type;
  if (lft == lwf::Type::constant || lft == lwf::Type::linear)
  {
    osg::Vec3d f = prms.front()->getVector();
    osg::Vec3d l = prms.back()->getVector();
    drawLine(f, l);
  }
  else if (lft == lwf::Type::interpolate)
  {
    //note internal points should already be added.
    drawInterpolate(stow->cue.buildLawFunction());
  }
  else if (lft == lwf::Type::composite)
  {
    opencascade::handle<Law_Composite> compLaw = dynamic_cast<Law_Composite*>(stow->cue.buildLawFunction().get());
    assert(!compLaw.IsNull());
    Law_Laws &laws = compLaw->ChangeLaws();
    auto itLaws = laws.begin();
    if ((stow->cue.datas.size() + 1) == stow->cue.boundaries.size())
    {
      for (std::size_t index = 0; index < stow->cue.datas.size(); ++index)
      {
        const lwf::Data &data = stow->cue.datas.at(index);
        if (data.subType == lwf::Type::constant || data.subType == lwf::Type::linear)
        {
          osg::Vec3d f = stow->cue.boundaries.at(index).getVector();
          osg::Vec3d l = stow->cue.boundaries.at(index + 1).getVector();
          drawLine(f, l);
        }
        else if (data.subType == lwf::Type::interpolate)
        {
          drawInterpolate(*itLaws);
        }
        itLaws++;
      }
    }
    else
      std::cout << "ERROR: Inconsistent composite" << std::endl;
  }
  stow->graphicsScene->update();
  stow->graphicsView->fitToData();
}
