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

#ifndef DLG_SELECTIONWIDGET_H
#define DLG_SELECTIONWIDGET_H

#include <memory>

#include <QWidget>
#include <QIcon>

#include "selection/slcdefinitions.h"
#include "selection/slcaccrue.h"

class QAbstractButton;

namespace slc{struct Message; typedef std::vector<Message> Messages;}


namespace dlg
{
  class SelectionButton;
  
  /*! @struct SelectionWidgetCue
   * @brief describes selection entry.
   */
  struct SelectionWidgetCue
  {
    QString name = "Dummy";
    bool singleSelection = false;
    bool showAccrueColumn = true;
    slc::Accrue::Type accrueDefault = slc::Accrue::None;
    slc::Mask mask = slc::None;
    QString statusPrompt = "Make Selection";
    QIcon icon; //a default icon will be used if null.
  };
  
  class SelectionWidget : public QWidget
  {
    Q_OBJECT
  public:
    SelectionWidget(QWidget*, const std::vector<SelectionWidgetCue>&);
    ~SelectionWidget() override;
    int getButtonCount() const;
    SelectionButton* getButton(int) const;
    const slc::Messages& getMessages(int) const;
    void activate(int); //!< Send timed click
    void setAngle(double); //!< tangent accrue for all buttons and all messages.
  Q_SIGNALS:
    void accrueChanged();
  private Q_SLOTS:
    void advance();
    void buttonToggled(QAbstractButton *, bool);
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
    
    void buildGui(const std::vector<SelectionWidgetCue>&);
  };
}

#endif //DLG_SELECTIONWIDGET_H
