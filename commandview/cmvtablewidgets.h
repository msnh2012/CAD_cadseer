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

#ifndef CMV_TBL_WIDGETS_H
#define CMV_TBL_WIDGETS_H

#include <memory>

#include <QWidget>


namespace prm{class Parameter;}

namespace cmv
{
  namespace trf {class Signal;}
  
  namespace tbl
  {
    QWidget* buildWidget(QWidget*, prm::Parameter*);
  
    //see note in source about event filter
    class Base : public QWidget
    {
      Q_OBJECT
    public:
      Base() = delete;
      Base(QWidget*);
      
      bool eventFilter(QObject*, QEvent*) override;
    };
  
    class ExpressionEdit : public Base
    {
      Q_OBJECT
    public:
      ExpressionEdit() = delete;
      ExpressionEdit(QWidget*, prm::Parameter*);
      ~ExpressionEdit() override;
      
      QString text() const;
    private:
      struct Stow;
      std::unique_ptr<Stow> stow;
      
      void editing(const QString&);
      void finishedEditing();
    };
    
    class PathEdit : public Base
    {
      Q_OBJECT
    public:
      PathEdit() = delete;
      PathEdit(QWidget*, const prm::Parameter*);
      ~PathEdit() override;
      
      QString text() const;
    private Q_SLOTS:
      void finishedEditingSlot();
      void browseSlot();
    private:
      struct Stow;
      std::unique_ptr<Stow> stow;
    };
  }
}

#endif // CMV_TBL_WIDGETS_H
