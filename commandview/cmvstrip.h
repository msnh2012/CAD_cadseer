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

#ifndef CMV_STRIP_H
#define CMV_STRIP_H

#include <memory>

#include <QLabel>

#include "commandview/cmvbase.h"

class QMimeData;
namespace cmd{class Strip;}

namespace cmv
{
  /*! This is a simple QLabel with a picture
   * to absorb drags.
   */
  class TrashCan : public QLabel
  {
    Q_OBJECT
  public:
    TrashCan() = delete;
    explicit TrashCan(QWidget*);
  Q_SIGNALS:
    void droppedIndex(int);
  protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
  private:
    bool isDataOk(const QMimeData*);
    int stationIndex;
  };
  
  /**
  * @todo write docs
  */
  class Strip : public Base
  {
    Q_OBJECT
  public:
    Strip(cmd::Strip*);
    ~Strip() override;
  public Q_SLOTS:
    void modelChanged(const QModelIndex&, const QModelIndex&);
    void trashed(int);
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
  };
}

#endif // CMV_STRIP_H
