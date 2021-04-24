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

#include <QEvent>
#include <QKeyEvent>

#include "application/appapplication.h"
#include "commandview/cmvparameterbase.h"

using namespace cmv;



/* Hack for delegate: With my custom widget, editing via a delegate would not
 * finish when focus was lost. Apparently the delegate expects behavior from its'
 * editor widget, not its' children, that is not documented. The delegate assigns an event filter
 * on the created editor. This override forwards events from children to 'this',
 * the custom widget used as delegate editor, So the delegate can respond accordingly.
 * The appropriate children will need to have this installed as an event filter.
 * 
 * https://stackoverflow.com/questions/12145522/why-pressing-of-tab-key-emits-only-qeventshortcutoverride-event
 * https://forum.qt.io/topic/3778/item-delegate-editor-focus-problem/9
 */
bool ParameterBase::eventFilter(QObject *watched, QEvent *event)
{
  if(event->type() == QEvent::KeyPress)
  {
    QKeyEvent* keyEvent = static_cast<QKeyEvent *>(event);
    if(keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab)
    {
      QApplication::postEvent(this, new QKeyEvent(keyEvent->type(), keyEvent->key(), keyEvent->modifiers()));
      // Filter this event because the editor will be closed anyway
      return true;
    }
  }
  else if(event->type() == QEvent::FocusOut)
  {
    QFocusEvent* focusEvent = static_cast<QFocusEvent *>(event);
    QApplication::postEvent(this, new QFocusEvent(focusEvent->type(), focusEvent->reason()));

    // Don't filter because focus can be changed internally in editor
    return false;
  }

  return QWidget::eventFilter(watched, event);
}
