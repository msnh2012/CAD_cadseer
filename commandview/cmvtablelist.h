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

#ifndef CMV_TABLELIST_H
#define CMV_TABLELIST_H

#include <memory>

#include <QWidget>
class QListWidget;
class QStackedWidget;

namespace ftr{class Base;}
namespace prm{class Parameter; typedef std::vector<Parameter*> Parameters;}
namespace slc{struct Message; typedef std::vector<Message> Messages;}

namespace cmv
{
  namespace tbl{class View; class Model;}
  /**
  * @class TableList
  * @brief Widget to contain a list of strings associated to a prm::Table
  * @details Need feature for table to resolve picks.
  */
  class TableList : public QWidget
  {
    Q_OBJECT
  public:
    TableList(QWidget*, ftr::Base*);
    ~TableList() override;
    
    int count() const;
    int add(const QString&, prm::Parameters&);
    int add(const QString&, prm::Parameters&, int);
    void remove(int);
    int remove(); //removes selected and returns that index or -1 if no selection.
    void clear();
    void updateHideInactive();
    void restoreSettings(const QString &nameIn); //for splitter location
    void setSelected(int=-1); //select item at index. -1 = last item.
    void setSelectedDelayed(int=-1); //queue select item
    
    QListWidget* getListWidget() const;
    QStackedWidget* getStackedWidget() const;
    tbl::Model* getPrmModel(int) const; //null return if invalid position
    tbl::View* getPrmView(int) const; //null return if invalid position
    slc::Messages getMessages(int) const; //all selection messages of pick parameters
    int getSelectedIndex() const; // -1 if no selection
    tbl::Model* getSelectedModel() const; //null return if no selection
    tbl::View* getSelectedView() const; //null return if no selection
    slc::Messages getSelectedMessages() const;
    
    void closePersistent(bool = true);
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
  };
}

#endif // CMV_TABLELIST_H
