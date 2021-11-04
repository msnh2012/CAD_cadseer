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

#ifndef CMV_SELECTION_H
#define CMV_SELECTION_H

#include <memory>

#include <QAbstractTableModel>
#include <QTableView>

namespace slc{struct Message; typedef std::vector<Message> Messages;}
namespace prm{class Parameter;}

namespace cmv
{
  namespace tbl{struct SelectionCue;}
  namespace edt
  {
    /**
    * @class Model. @brief Editor widget model for pick parameters
    * 
    * @details Think delegate. I tried like hell to get qt signal/slots
    * to mesh with node messages. The only way I succeeded was to
    * put node messages in with the view along side the signal slots.
    * So then I had to make the mode a friend class of the view
    * to access the model stow data. Kind of a hack.
    */
    class Model : public QAbstractTableModel
    {
      Q_OBJECT
    public:
      Model() = delete;
      Model(QObject*, prm::Parameter*, const tbl::SelectionCue&);
      Model(const Model&) = delete;
      Model(const Model&&) = delete;
      Model& operator=(const Model&) = delete;
      Model& operator=(const Model&&) = delete;
      ~Model() override;
      
      int rowCount(const QModelIndex&) const override;
      int columnCount(const QModelIndex&) const override;
      QVariant data(const QModelIndex&, int) const override;
      Qt::ItemFlags flags(const QModelIndex&) const override;
      bool setData(const QModelIndex&, const QVariant&, int = Qt::EditRole) override;
      
      //can't do in constructor. need to resolve in views after construction.
      void setMessages(const slc::Messages&);
      const slc::Messages& getMessages() const;
      const tbl::SelectionCue& getCue() const;
      
      //angle for accrue tangent doesn't exist in ftr::pick or slc::Message
      //It is a parameter of feature. Kind of a hack.
//       void setAccrue(bool);
    private:
      struct Stow;
      std::unique_ptr<Stow> stow;
      friend class View;
    };
  
    /**
    * @class View. @brief Editor widget view for pick parameters
    * 
    * @details Think delegate editor. When editors are removed, deleteLater
    * is used. This might cause problems for our selection message handlers.
    * They can be alive while the gui is hidden and before deletion. So any
    * selection handlers test for visibility before any processing. If that
    * doesn't work we can create a state member and set it 'dead' from
    * the delegate functions: commitSlcEditor, rejectSlcEditor
    */
    class View : public QTableView
    {
      Q_OBJECT
    public:
      View() = delete;
      View(QWidget*, Model*);
      View(const View&) = delete;
      View(const View&&) = delete;
      View& operator=(const View&) = delete;
      View& operator=(const View&&) = delete;
      ~View() override;
    protected:
      void mousePressEvent(QMouseEvent*) override;
      void mouseReleaseEvent(QMouseEvent*) override;
    private Q_SLOTS:
      void selectionHasChanged(const QItemSelection&, const QItemSelection&);
    private:
      struct Stow;
      std::unique_ptr<Stow> stow;
    };
    
    class FocusFilter : public QObject
    {
      Q_OBJECT
    public:
      FocusFilter(QObject *parent) : QObject(parent){}
    protected:
      bool eventFilter(QObject*, QEvent*) override;
    };
  }
}

#endif // CMV_SELECTION_H
