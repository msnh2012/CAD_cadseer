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

#include <cassert>

#include <QWidget>
#include <QKeyEvent>

#include "dialogs/dlgenterfilter.h"

using namespace dlg;

bool EnterFilter::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::KeyPress)
  {
    QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
    if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)
    {
      QWidget *widget = qobject_cast<QWidget*>(obj);
      assert(widget);
      if (widget->hasFocus())
      {
        Q_EMIT enterPressed();
        if (shouldClearFocus)
        {
          widget->clearFocus();
          QWidget *next = widget->nextInFocusChain();
          if (next)
            next->setFocus();
        }
        return true; // mark the event as handled
      }
    }
  }
  return QObject::eventFilter(obj, event);
}
