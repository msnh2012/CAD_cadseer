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
  namespace trf
  {
    /*! @class Signal 
   * @brief Traffic signal
   * @details This is a pixmapped qlabel for displaying
   * results of expression parsing. If the parameter is null
   * then any expression linking capabilities will be hidden.
   * When the parameter is valid, the label will accept drops
   * form expressions and will have a context menu with an
   * 'unlink' action to unlink.
   */
    class Signal : public QLabel
    {
      Q_OBJECT
    public:
      Signal() = delete;
      explicit Signal(QWidget*, prm::Parameter* = nullptr);
      ~Signal() override;
      void setTrafficRed();
      void setTrafficYellow();
      void setTrafficGreen();
      void setTrafficEmpty(); //hides the signal.
      void setTrafficLink(); //assert on parameter ! null
    Q_SIGNALS:
      void prmValueChanged();
      void prmConstantChanged();
    protected:
      void dragEnterEvent(QDragEnterEvent*) override;
      void dropEvent(QDropEvent*) override;
    private:
      struct Stow;
      std::unique_ptr<Stow> stow;
      
      void unlink();
    };
  }
}

#endif //CMV_TRAFFICSIGNAL_H
