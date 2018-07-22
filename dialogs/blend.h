/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef DLG_BLEND_H
#define DLG_BLEND_H

#include <memory>
#include <set>

#include <boost/uuid/uuid.hpp>

#include <QDialog>

#include <dialogs/expressionedit.h> //for expression delegate

class QCloseEvent;
class QListWidget;
class QTableWidget;
class QTableWidgetItem;
class QItemSelection;
class QComboBox;
class QTreeView;
class QStandardItemModel;
class QStackedWidget;

namespace ftr{class Base; class Blend; class Pick;}
namespace msg{class Message; class Observer;}
namespace slc{class Message;}

namespace dlg
{
  class VariableWidget;
  
  class Blend : public QDialog
  {
    Q_OBJECT
  public:
    Blend(QWidget*); //!< create blend dialog
    Blend(ftr::Blend*, QWidget*); //!< edit blend dialog
    virtual ~Blend() override;
    
  public Q_SLOTS:
    virtual void reject() override;
    virtual void accept() override;
    
  private:
    std::shared_ptr<ftr::Blend> blendSmart;
    ftr::Blend *blend = nullptr;
    const ftr::Base *blendParent = nullptr;
    std::unique_ptr<msg::Observer> observer;
    bool isAccepted = false;
    bool isEditDialog = false;
    bool overlayWasOn = false; //!< restore overlay state upon edit finish.
    std::vector<boost::uuids::uuid> leafChildren; //!< edit mode rewinds to blendParent. this restores state.
    
    void init();
    void buildGui();
    void addToSelection(const boost::uuids::uuid&); //!< parent feature shape id.
    void updateBlendFeature();
    
    void setupDispatcher();
    void selectionAdditionDispatched(const msg::Message&);
    
    void finishDialog();
    
  private:
    QComboBox *typeCombo;
    QComboBox *shapeCombo;
    QStackedWidget *stacked;
    QStandardItemModel *cModel;
    QTreeView *cView;
    VariableWidget *vWidget;
    
    void cEnsureOne();
    void cSelectFirstRadius();
    void cSelectionChanged(const QItemSelection &, const QItemSelection &);
    void cAddRadius(); //!< adds a new radius definiton.
    void cRemove(); //!< can remove both blends and picks.
    void cLink(QModelIndex, const QString&);
    void cUnlink(); //removes link between parameter and expression.
    void vSelectFirstContour();
    void vContourSelectionChanged(int);
    void vContourRemove();
    void vConstraintSelectionChanged(QTableWidgetItem *current, QTableWidgetItem *previous);
    void vConstraintRemove();
    void vHighlightContour(int);
    void vHighlightConstraint(int);
    void vUpdate3dSelection();
    void vLink(QTableWidgetItem*, const QString&);
    void vUnlink();
    
  private Q_SLOTS:
    void typeComboChangedSlot(int); //ugly connect syntax if not a 'Q_SLOT'
  };
  
  //! a filter to do drag and drop for constant tree view.
  class CDropFilter : public QObject
  {
    Q_OBJECT
  public:
    explicit CDropFilter(QObject *parent) : QObject(parent){}
    
  protected:
    virtual bool eventFilter(QObject *obj, QEvent *event) override;
    
  Q_SIGNALS:
    void requestLinkSignal(QModelIndex, const QString&);
  };
  
  //! a filter to do drag and drop for variable table.
  class VDropFilter : public QObject
  {
    Q_OBJECT
  public:
    explicit VDropFilter(QObject *parent) : QObject(parent){}
    
  protected:
    virtual bool eventFilter(QObject *obj, QEvent *event) override;
    
  Q_SIGNALS:
    void requestLinkSignal(QTableWidgetItem*, const QString&);
  };
  
  //! @brief Delegate for constant tree.
  class CDelegate : public ExpressionDelegate
  {
    Q_OBJECT
  public:
    explicit CDelegate(QObject *parent);
    //! Fill editor with text to edit.
    virtual void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    //! Set the model data or re-edit upon failure.
    virtual void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
  };
  
  //! @brief Delegate for constant tree.
  class VDelegate : public ExpressionDelegate
  {
    Q_OBJECT
  public:
    //! Parent must be the table view.
    explicit VDelegate(QObject *parent);
    //! Fill editor with text to edit.
    virtual void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    //! Set the model data or re-edit upon failure.
    virtual void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
  };
}

#endif // DLG_BLEND_H
