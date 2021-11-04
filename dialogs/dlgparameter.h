/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2015  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef DLG_PARAMETER_H
#define DLG_PARAMETER_H

#include <memory>

#include <QDialog>

namespace boost{namespace uuids{struct uuid;}}

namespace prm{class Parameter;}

namespace dlg
{
  class Parameter : public QDialog
  {
    Q_OBJECT
  public:
    Parameter(prm::Parameter *parameterIn, const boost::uuids::uuid &idIn);
    ~Parameter() override;
  public Q_SLOTS:
    void modelChanged(const QModelIndex&, const QModelIndex&);
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
  };
}

#endif // DLG_PARAMETER_H
