/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2022 Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef CMV_DRAGLIST_H
#define CMV_DRAGLIST_H

#include <memory>

#include <QWidget>

class QListView;

namespace cmv
{
  class DragList : public QWidget
  {
    Q_OBJECT
  public:
    DragList(QWidget*, const QStringList&, const QString&);
    ~DragList() override;
    
    void setList(const QStringList&);
    void setList(const std::vector<int>&);
    
    QStringList getStringList();
    std::vector<int> getIndexList();
    
    QListView* getListView();
    QListView* getProtoView();
  Q_SIGNALS:
    void added(int);
    void removed(int);
  public Q_SLOTS:
    void rowsInserted(const QModelIndex&, const QModelIndex&, const QVector<int>&);
    void rowsRemoved(const QModelIndex&, int, int);
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
  };
}

#endif //CMV_DRAGLIST_H
