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

#ifndef CMV_EXPRESSIONBASE_H
#define CMV_EXPRESSIONBASE_H

#include <memory>

#include "commandview/cmvparameterbase.h"

class QLineEdit;

namespace prm{class Parameter;}

namespace cmv
{
  namespace trf {class Signal;}
  
  /*! @class BaseEdit
   * @brief Editing for math formulas and traffic signal.
   * @details derived classes should construct lineEdit
   * and traffic signal then call buildGui.
   */
  class BaseEdit : public ParameterBase
  {
    Q_OBJECT
  public:
    BaseEdit() = delete;
    explicit BaseEdit(QWidget *p) : ParameterBase(p){};
    void buildGui();
    void goToolTip(const QString&);
  protected:
    QLineEdit *lineEdit = nullptr; //construct in derived.
    trf::Signal *trafficSignal = nullptr; //construct in derived.
  };
  
  class ParameterEdit : public BaseEdit
  {
    Q_OBJECT
  public:
    ParameterEdit() = delete;
    ParameterEdit(QWidget*, prm::Parameter*);
    ~ParameterEdit() override;
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
    
    void editing(const QString&);
    void finishedEditing();
  };
  
  class ExpressionEdit : public BaseEdit
  {
    Q_OBJECT
  public:
    ExpressionEdit() = delete;
    explicit ExpressionEdit(QWidget*);
    ~ExpressionEdit() override;
    
    void parseSucceeded(const QString&);
    void parseFailed(const QString&);
    void setText(const QString&);
    QString getText();
    void setTrafficGreen();
    void setTrafficYellow();
    void setTrafficRed();
  Q_SIGNALS:
    void editing(const QString&);
  };
}

#endif // CMV_EXPRESSIONBASE_H
