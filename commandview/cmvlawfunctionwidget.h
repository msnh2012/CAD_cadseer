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

#ifndef CMV_LAWFUNCTIONWIDGET_H
#define CMV_LAWFUNCTIONWIDGET_H

#include <memory>

#include <QWidget>
#include <QDialog>

namespace lwf{struct Cue;}

namespace cmv
{
  /*! @class LawFunctionWidget
   * @brief Widget for manipulating law functions
   * 
   * @details
   * To use construct and then setCue.
   * osg visual representation works in 2 parts
   * The first part is the construction of the subgraph.
   * The second part is the updating of the subgraph.
   * The second, updating part can only handle parameter modifications.
   * When parameters are created or destroyed we need to generate
   * a new subgraph. This is purpose of 'structural change'
   */
  class LawFunctionWidget : public QWidget
  {
    Q_OBJECT
  public:
    LawFunctionWidget(QWidget*);
    ~LawFunctionWidget() override;
    void setCue(const lwf::Cue&);
    const lwf::Cue& getCue();
    void updateGui();
    bool isInterpolate(double);
    void addInternalPoint(double = 0);
    void removeInternalPoint(double = 0);
//     void smooth();
    void appendConstant();
    void appendLinear();
    void appendInterpolate();
    void removeLaw(double);
  Q_SIGNALS:
    void lawIsDirty();
    void valueChanged();
  public Q_SLOTS:
    void selectionChangedSlot();
    void comboChangedSlot(int);
    void pChangedSlot();
    void vChangedSlot();
    void dChangedSlot();
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
    
    void buildGui();
    void changed(const QString&, int);
  };
}

#endif // CMV_LAWFUNCTIONWIDGET_H
