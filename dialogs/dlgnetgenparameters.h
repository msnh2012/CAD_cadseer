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

#ifndef DLG_NETGENPARAMETERS_H
#define DLG_NETGENPARAMETERS_H

#include <memory>

#include <QWidget>

namespace msh{namespace prm{struct Netgen;}}

namespace dlg
{
  /**
  * @todo write docs
  */
  class NetgenParameters : public QWidget
  {
    Q_OBJECT
  public:
    NetgenParameters(QWidget*);
    ~NetgenParameters() override;
    
    void setParameters(const msh::prm::Netgen&);
    msh::prm::Netgen getParameters() const;
    
  Q_SIGNALS:
    void dirty();
    
  public Q_SLOTS:
    void resetParameters();
    void comboChanged(int);
    
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
  };
}

#endif // DLG_NETGENPARAMETERS_H
