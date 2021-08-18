/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2021 Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef CMV_TABLE_H
#define CMV_TABLE_H

#include <memory>

#include <QAbstractTableModel>
#include <QTableView>

namespace prm{class Parameter; typedef std::vector<Parameter*> Parameters;}
namespace slc{struct Message; typedef std::vector<Message> Messages;}
namespace ftr{class Base;}

namespace cmv
{
  namespace tbl
  {
    struct SelectionCue;
    
    /**
    * @class Model @brief Model to show feature parameters.
    * 
    * @details
    * Any selection messages will be resolved from ftr::Picks
    * parameters. Don't use setMessages. That is only for 
    * the delegate to update messages from the selection widget.
    * 
    * Our individual widgets were setup to change the parameter value
    * and emit the signal 'prmValueChanged' to parent widgets so they
    * would know that the gui element changed the value. This was
    * primarily so command view could trigger updates. That is obsolete
    * with a table. There we just trigger data changed from the model
    * and that can trigger command view actions. We are however still changing
    * the parameter values in the widget and this might be confusing
    * to user who changes a value and then hits escape to reject the
    * change, but the value changes anyway. Going with the table 'flow', 
    * will require us to rework all the individual widgets once again!
    * I will never move this software forward if I continually rewrite
    * gui interfaces!
    * 
    */
    class Model : public QAbstractTableModel
    {
      Q_OBJECT
    public:
      Model() = delete;
      Model(QObject*, ftr::Base*); //uses all the parameters
      Model(QObject*, ftr::Base*, const prm::Parameters&); //uses subset of parameters passed in.
      Model(const Model&) = delete;
      Model(const Model&&) = delete;
      Model& operator=(const Model&) = delete;
      Model& operator=(const Model&&) = delete;
      ~Model() override;
      
      int rowCount(const QModelIndex&) const override;
      int columnCount(const QModelIndex&) const override;
      QVariant data(const QModelIndex&, int) const override;
      Qt::ItemFlags flags(const QModelIndex&) const override;
      bool mySetData(const QModelIndex&, const QWidget*);
      void mySetData(prm::Parameter*, const slc::Messages&);
      
      prm::Parameter* getParameter(const QModelIndex&) const;
      QModelIndex addParameter(prm::Parameter*);
      void removeParameter(const QModelIndex&);
      void removeParameter(const prm::Parameter*);
      
      void setMessages(prm::Parameter*, const slc::Messages&);
      const slc::Messages& getMessages(const QModelIndex&) const;
      const slc::Messages& getMessages(const prm::Parameter*) const;
      
      void setCue(prm::Parameter*, const SelectionCue&);
      void setCue(const QModelIndex&, const SelectionCue&);
      const SelectionCue& getCue(const prm::Parameter*) const;
      
      QStringList mimeTypes() const override;
      
      //setDelegate for parameter???? things like blend dialog.
      //maybe have a delegate of View type. Think nested or recursive
      //persistent editor for nested/recursive?
    private:
      struct Stow;
      std::unique_ptr<Stow> stow;
    };
    
    /**
    * @class View. @brief view to show feature parameters.
    * 
    * @details I have to put an eventFilter on the persistent
    * editors or dataChanged signals will be emitted when
    * I go into 3d view.
    */
    class View : public QTableView
    {
      Q_OBJECT
    public:
      View() = delete;
      View(QWidget*, Model*, bool = false);
      View(const View&) = delete;
      View(const View&&) = delete;
      View& operator=(const View&) = delete;
      View& operator=(const View&&) = delete;
      ~View() override;
      
      void updateHideInactive();
      void closePersistent(bool = true);
    Q_SIGNALS:
      void openingPersistent(); //signal emitted before
      void closingPersistent(); //signal emitted after
    protected:
      void mouseReleaseEvent(QMouseEvent*) override;
      void mouseDoubleClickEvent(QMouseEvent*) override;
      bool eventFilter(QObject*, QEvent*) override;
      void dragMoveEvent(QDragMoveEvent *event) override;
      void dropEvent(QDropEvent *event) override;
    private Q_SLOTS:
      void selectionHasChanged(const QItemSelection&, const QItemSelection&);
    private:
      bool hideInactive = false;
    };
  }
}

#endif // CMV_TABLE_H
