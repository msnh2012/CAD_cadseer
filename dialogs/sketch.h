/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2018  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef DLG_SKETCH_H
#define DLG_SKETCH_H

#include <memory>

#include <QDialog>

#include <osg/ref_ptr>

class QTabWidget;

namespace prm{class Parameter;}
namespace ftr{class Sketch;}
namespace skt{class Selection;}
namespace msg{class Message; struct Node; struct Sift;}

namespace dlg
{
  class ExpressionEdit;
  
  class EditPage : public QWidget
  {
    Q_OBJECT
  public:
    EditPage(QWidget*);
    virtual ~EditPage() override;
  Q_SIGNALS:
    void shown();
    void hidden();
  protected:
    virtual void showEvent(QShowEvent *) override;
    virtual void hideEvent(QHideEvent *) override;
  };
  
  class PositionPage : public QWidget
  {
    Q_OBJECT
  public:
    PositionPage(QWidget*);
    virtual ~PositionPage() override;
  Q_SIGNALS:
    void shown();
    void hidden();
  protected:
    virtual void showEvent(QShowEvent *) override;
    virtual void hideEvent(QHideEvent *) override;
  };
  
  /**
  * @brief Dialog class to interact with sketch
  */
  class Sketch : public QDialog
  {
    Q_OBJECT
  public:
    Sketch() = delete;
    Sketch(ftr::Sketch*, QWidget*);
    virtual ~Sketch() override;
  public Q_SLOTS:
    virtual void reject() override;
    virtual void accept() override;
  protected:
    ftr::Sketch *sketch;
    osg::ref_ptr<skt::Selection> selection;
    bool isAccepted = false;
    bool wasVisible3d = false;
    bool wasVisibleOverlay = false;
    std::unique_ptr<msg::Node> node;
    std::unique_ptr<msg::Sift> sift;
    void setupDispatcher();
    void selectParameterDispatched(const msg::Message &);
    
    QTabWidget *tabWidget = nullptr;
    ExpressionEdit *pEdit = nullptr;
    prm::Parameter *parameter = nullptr; //!< currently selected parameter or nullptr
    
    void buildGui();
    QWidget* buildPositionPage();
    QWidget* buildEditPage();
    void initGui();
    void finishDialog();
    
    void sketchSelectionGo();
    void sketchSelectionStop();
    void selectionGo();
    void selectionStop();
    void draggerShow();
    void draggerHide();
    
    void addPoint();
    void addLine();
    void addArc();
    void addCircle();
    void addCoincident();
    void addHorizontal();
    void addVertical();
    void addTangent();
    void addDistance();
    void addDiameter();
    void addAngle();
    void addSymmetric();
    void addParallel();
    void addPerpendicular();
    void addEqual();
    void addEqualAngle();
    void addMidpoint();
    void addWhereDragged();
    void remove();
    void cancel();
    void toggleConstruction();
    
    void setEditLinked();
    void setEditUnlinked();
    
  private Q_SLOTS:
    void requestParameterLinkSlot(const QString &);
    void requestParameterUnlinkSlot();
    void updateParameterSlot();
    void textEditedParameterSlot(const QString &);
  };
}

#endif // DLG_SKETCH_H
