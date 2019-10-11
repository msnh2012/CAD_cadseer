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

#ifndef DLG_REMOVE_H
#define DLG_REMOVE_H

#include <memory>

#include <QDialog>

namespace dlg
{
  /**
  * @todo write docs
  */
  class Remove : public QDialog
  {
    Q_OBJECT
  public:
    Remove(QWidget*);
    ~Remove() override;
  public Q_SLOTS:
    void reject() override;
    void accept() override;
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
    
    void init();
    void buildGui();
  };
}

#endif // DLG_REMOVE_H
