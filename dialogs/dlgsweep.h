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

#ifndef DLG_SWEEP_H
#define DLG_SWEEP_H

#include <memory>

#include <boost/uuid/uuid.hpp>

#include <QDialog>

class QComboBox;
class QButtonGroup;
class QListWidget;
class QLabel;
class QAbstractButton;
class QCheckBox;

namespace ftr{class Sweep;}
namespace dlg{class SelectionButton;}
namespace msg{struct Node;}

namespace dlg
{
  /**
  * @todo write docs
  */
  class Sweep : public QDialog
  {
    Q_OBJECT
  public:
    Sweep() = delete;
    Sweep(ftr::Sweep*, QWidget*, bool = false);
    ~Sweep() override;
    
  public Q_SLOTS:
    virtual void reject() override;
    virtual void accept() override;
    
  private:
    std::unique_ptr<msg::Node> node;
    ftr::Sweep *sweep = nullptr;
    
    struct Stow;
    std::unique_ptr<Stow> stow;
    
    bool isAccepted = false;
    bool isEditDialog = false;
    
    void init();
    void loadFeatureData();
    void buildGui();
    void finishDialog();
    
  private Q_SLOTS:
    void advanceSlot(); //!< move to next button in selection group.
    void comboSlot(int); //!< selection changed in combobox
    void buttonToggled(QAbstractButton *, bool);
    void profileSelectionDirtySlot();
  };
}

#endif // DLG_SWEEP_H
