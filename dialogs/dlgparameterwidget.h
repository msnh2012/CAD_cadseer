/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2019  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef DLG_PARAMETERWIDGET_H
#define DLG_PARAMETERWIDGET_H

#include <memory>

#include <QWidget>

class QLabel;

namespace prm{class Parameter;}

namespace dlg
{
  class ExpressionEdit;
  
  /*! @class ParameterContainer
   * @brief Manages a parameter.
   * 
   * This is not a widget but will construct widgets
   * to be used some where else. This bundles up
   * expression edit with parameter functionality.
   */
  class ParameterContainer : public QObject
  {
    Q_OBJECT
  public:
    ParameterContainer(QWidget*, prm::Parameter*);
    ~ParameterContainer();
    QLabel* getLabel();
    ExpressionEdit* getEditor();
    void syncLink(); 
    
  private Q_SLOTS:
    void requestParameterLinkSlot(const QString &);
    void requestParameterUnlinkSlot();
    void updateParameterSlot();
    void textEditedParameterSlot(const QString &);
    
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
    void setEditLinked();
    void setEditUnlinked();
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
    void syncLinks(); //!< call to commit link changes.
  private:
    void buildGui();
    std::vector<prm::Parameter*> parameters;
    std::vector<ParameterContainer*> containers;
  };
}


#endif //DLG_PARAMETERWIDGET_H
