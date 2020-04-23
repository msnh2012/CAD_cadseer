/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2019 Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef DLG_VIZFILTER_H
#define DLG_VIZFILTER_H

#include <QObject>
#include <QWidget>
#include <QEvent>

namespace dlg
{
  /*! @class VizFilter
   * @brief Convert visibility events into signals.
   * 
   * @details We need to keep the selection state in
   * sync with gui controls. We use the visibility
   * of the controls to indicate whether they should
   * be interacting with selection. This seems to be
   * working good. The only problem is we need to subclass
   * as there are no signals, only event handlers to be
   * overridden. That is where this comes in. This will
   * just forward the show and hide events onto signals.
   * 
   * @note show and hide events are 'informational only' events.
   * 
   */
  class VizFilter : public QObject
  {
    Q_OBJECT
  public:
    explicit VizFilter(QObject *parent) : QObject(parent){}
    
  protected:
    virtual bool eventFilter(QObject *obj, QEvent *event) override
    {
      if (event->type() == QEvent::Show)
        shown();
      else if (event->type() == QEvent::Hide)
        hidden();
      else if (event->type() == QEvent::EnabledChange)
      {
        QWidget *w = dynamic_cast<QWidget*>(obj);
        assert(w);
        if (w->isEnabled())
          enabled();
        else
          disabled();
      }
      return QObject::eventFilter(obj, event);
    }
    
  Q_SIGNALS:
    void shown();
    void hidden();
    void enabled();
    void disabled();
  };
}

#endif //DLG_VIZFILTER_H
