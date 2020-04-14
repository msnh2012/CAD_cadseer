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

#ifndef CMV_PARAMETERWIDGETS_H
#define CMV_PARAMETERWIDGETS_H

#include <memory>

#include <QWidget>

namespace prm{class Parameter;}

namespace cmv
{
  class ExpressionEdit : public QWidget
  {
    Q_OBJECT
  public:
    ExpressionEdit() = delete;
    ExpressionEdit(QWidget*, prm::Parameter*);
    ~ExpressionEdit() override;
  public Q_SLOTS:
    void goToolTipSlot(const QString&);
  private Q_SLOTS:
    void editingSlot(const QString &);
    void finishedEditingSlot();
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
  };
  
  class BoolCombo : public QWidget
  {
    Q_OBJECT
  public:
    BoolCombo() = delete;
    BoolCombo(QWidget*, prm::Parameter*);
    ~BoolCombo() override;
  private Q_SLOTS:
    void currentChangedSlot(int);
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
  };
  
  class EnumCombo : public QWidget
  {
    Q_OBJECT
  public:
    EnumCombo() = delete;
    EnumCombo(QWidget*, prm::Parameter*);
    ~EnumCombo() override;
  private Q_SLOTS:
    void currentChangedSlot(int);
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
  };
  
  class StringEdit : public QWidget
  {
    Q_OBJECT
  public:
    StringEdit() = delete;
    StringEdit(QWidget*, prm::Parameter*);
    ~StringEdit() override;
  private Q_SLOTS:
    void finishedEditingSlot();
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
  };
  
  class PathEdit : public QWidget
  {
    Q_OBJECT
  public:
    PathEdit() = delete;
    PathEdit(QWidget*, prm::Parameter*);
    ~PathEdit() override;
  private Q_SLOTS:
    void finishedEditingSlot();
    void browseSlot();
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
  };
  
  /*! @class ParameterWidget
   * @brief Self contained widget for parameter editing
   * 
   * Will layout multiple parameters in a grid
   */
  class ParameterWidget : public QWidget
  {
    Q_OBJECT
  public:
    ParameterWidget(QWidget*, const std::vector<prm::Parameter*>&);
    ~ParameterWidget();
    void enableWidget(prm::Parameter*);
    void disableWidget(prm::Parameter*);
    QWidget* getWidget(prm::Parameter*); // maybe nullptr
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
  };
}

#endif // CMV_PARAMETERWIDGETS_H
