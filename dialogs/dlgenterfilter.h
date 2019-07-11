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

#ifndef DLG_ENTERFILTER_H
#define DLG_ENTERFILTER_H

#include <QObject>

namespace dlg
{
  /*! @class EnterFilter
   * @brief absorb enter key presses.
   * 
   * @details Sorry to rant but this is bullshit.
   * When typing in an edit line inside a dialog, an
   * enter key stroke should not close the dialog....ever.
   * I don't care what any user interface standards says.
   * When the editline is being edited and has keyboard
   * focus, the enter key should simply finish the editing,
   * then another enter can close the dialog
   * as nothing is being edited.
   * 
   */
  class EnterFilter : public QObject
  {
    Q_OBJECT
  public:
    explicit EnterFilter(QObject *parent) : QObject(parent){}
    void setShouldClearFocus(bool nv){shouldClearFocus = nv;}
    
  protected:
    virtual bool eventFilter(QObject *obj, QEvent *event) override;
    
  private:
    bool shouldClearFocus = false;
    
  Q_SIGNALS:
    void enterPressed();
  };
}

#endif // DLG_ENTERFILTER_H
