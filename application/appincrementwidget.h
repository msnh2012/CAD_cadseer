/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef APP_INCREMENTWIDGET_H
#define APP_INCREMENTWIDGET_H

#include <memory>

#include <QWidget>
#include <QWidgetAction>

class QLineEdit;

namespace dlg{class ExpressionEdit;}

namespace app
{
  class IncrementWidget : public QWidget
  {
    Q_OBJECT
  public:
    IncrementWidget(QWidget*, const QString&, double&);
    ~IncrementWidget();
    void update(); //called when prefs changed externally. like through pref dialog.
  private Q_SLOTS:
    void textEditedSlot(const QString&);
    void returnPressedSlot();
    void editingFinishedSlot();
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
    
    void buildGui();
  };
  
  //! a filter to select text when editlines are picked.
  class HighlightOnFocusFilter : public QObject
  {
    Q_OBJECT
  public:
    explicit HighlightOnFocusFilter(QObject *parent) : QObject(parent){}
    
  protected:
    virtual bool eventFilter(QObject *obj, QEvent *event) override;
  };
}

#endif // APP_INCREMENTWIDGET_H
