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

#ifndef CMV_TRAFFICSIGNAL_H
#define CMV_TRAFFICSIGNAL_H

#include <memory>

#include <QLabel>

namespace boost{namespace uuids{struct uuid;}}
class QDragEnterEvent;
class QDropEvent;

namespace prm{class Parameter;}

namespace cmv
{
  /*! @class TrafficSignal 
   * @brief Widget for user interaction with parameter links
   * @details User can drag and drop an expression on to widget to link.
   * right click and select 'unlink' to unlik.
   */
  class TrafficSignal : public QLabel
  {
    Q_OBJECT
  public:
    TrafficSignal() = delete;
    TrafficSignal(QWidget*, prm::Parameter*);
    ~TrafficSignal() override;
  public Q_SLOTS:
    void setTrafficRedSlot();
    void setTrafficYellowSlot();
    void setTrafficGreenSlot();
    void setTrafficLinkSlot();
  protected:
    void dragEnterEvent(QDragEnterEvent*) override;
    void dropEvent(QDropEvent*) override;
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
    
    void unlink();
  };
}

#endif //CMV_TRAFFICSIGNAL_H
