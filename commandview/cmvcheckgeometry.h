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

#ifndef CMV_CHECKGEOMETRY_H
#define CMV_CHECKGEOMETRY_H

#include <memory>
#include <stack>
#include <set>

#ifndef Q_MOC_RUN
#include <boost/uuid/uuid.hpp>
#endif

#include <QWidget>

#include <TopAbs_ShapeEnum.hxx>

#include <osg/observer_ptr>
#include <osg/BoundingSphere>

#include "commandview/cmvbase.h"

class QTabWidget;
class QTreeWidget;
class QTreeWidgetItem;
class QTableWidget;
class QCloseEvent;
class QHideEvent;
class QTextEdit;

class BRepCheck_Analyzer;
class TopoDS_Shape;

namespace osg{class PositionAttitudeTransform;}

namespace ftr{class Base;}
namespace ann{class SeerShape;}

namespace cmv
{
  class CheckPageBase : public QWidget
  {
    Q_OBJECT
  public:
    CheckPageBase(const ftr::Base&, QWidget*);
    ~CheckPageBase() override;
  protected:
    const ftr::Base &feature;
    const ann::SeerShape &seerShape;
    osg::observer_ptr<osg::PositionAttitudeTransform> boundingSphere;
    osg::BoundingSphered minBoundingSphere;
  };
  
  class BasicCheckPage : public CheckPageBase
  {
    Q_OBJECT
  public:
    BasicCheckPage(const ftr::Base&, QWidget*);
    ~BasicCheckPage() override;
    void go();
  protected:
    void hideEvent(QHideEvent *) override;
  private:
    QTreeWidget *treeWidget = nullptr;
    std::stack<QTreeWidgetItem*> itemStack; //!< used during shape iteration.
    std::set<boost::uuids::uuid> checkedIds;
    
    void buildGui();
    void recursiveCheck(const BRepCheck_Analyzer &, const TopoDS_Shape &);
    void checkSub(const BRepCheck_Analyzer &shapeCheck, const TopoDS_Shape &shape,
                                            TopAbs_ShapeEnum subType);
  private Q_SLOTS:
    void selectionChangedSlot();
  Q_SIGNALS:
    void basicCheckPassed();
    void basicCheckFailed();
  };
  
  class BOPCheckPage : public CheckPageBase
  {
    Q_OBJECT
  public:
    BOPCheckPage(const ftr::Base&, QWidget*);
    ~BOPCheckPage() override;
  public Q_SLOTS:
    void basicCheckFailedSlot();
    void basicCheckPassedSlot();
    void goSlot();
  protected:
    void hideEvent(QHideEvent *) override;
  private:
    QTableWidget *tableWidget = nullptr;
    
  private Q_SLOTS:
    void selectionChangedSlot();
  };
  
  class ToleranceCheckPage : public CheckPageBase
  {
    Q_OBJECT
  public:
    ToleranceCheckPage(const ftr::Base&, QWidget*);
    ~ToleranceCheckPage() override;
    void go();
  protected:
    void hideEvent(QHideEvent *) override;
  private:
    QTableWidget *tableWidget;
    void buildGui();
    
  private Q_SLOTS:
    void selectionChangedSlot();
  };
  
  class ShapesPage : public CheckPageBase
  {
    Q_OBJECT
  public:
    ShapesPage(const ftr::Base&, QWidget*);
    void go();
  private:
    QTextEdit *textEdit;
    QTableWidget *boundaryTable;
    void buildGui();
    std::vector<std::vector<boost::uuids::uuid>> boundaries;
  protected:
    void hideEvent(QHideEvent *) override;
  private Q_SLOTS:
    void boundaryItemChangedSlot();
  };
  
  
  
  
  
  
  
  
  /**
  * @todo write docs
  */
  class CheckGeometry : public Base
  {
    Q_OBJECT
  public:
    CheckGeometry(const ftr::Base&);
    ~CheckGeometry() override;
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
  };
}

#endif // CMV_CHECKGEOMETRY_H
