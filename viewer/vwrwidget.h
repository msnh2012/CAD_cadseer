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

#ifndef VWR_WIDGET_H
#define VWR_WIDGET_H

#include <memory>

#include <QWidget>

#include <osg/ref_ptr>
#include <osgViewer/CompositeViewer>
#include "osgQOpenGL/osgQOpenGLWidget"

#include "selection/slccontainer.h"

class QTextStream;
namespace vwr{class SpaceballManipulator;}
namespace slc{class EventHandler; class OverlayHandler;}
namespace lbr{class CSysDragger; class CSysCallBack;}
namespace msg{class Message; struct Node; struct Sift;}

namespace vwr
{
  class Widget : public osgQOpenGLWidget
  {
    Q_OBJECT
  public:
    Widget(QWidget* parent = nullptr);
    ~Widget() override;
    
    slc::EventHandler* getSelectionEventHandler();
    const osg::Matrixd& getCurrentSystem() const;
    void setCurrentSystem(const osg::Matrixd &mIn);
    const osg::Matrixd& getViewSystem() const;
    QTextStream& getInfo(QTextStream &stream) const;
    double getDiagonalLength() const;
    void screenCapture(const std::string &, const std::string &);
  protected:
    bool event(QEvent *event) override;
    void keyPressEvent(QKeyEvent*) override;
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
  };

}
#endif // VWR_WIDGET_H
