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

#ifndef DLG_BASE_H
#define DLG_BASE_H

#include <memory>
#include <boost/uuid/uuid.hpp>

#include <QDialog>

namespace ftr{class Base;}
namespace msg{struct Node; struct Sift;}
namespace prj{class Project;}

namespace dlg
{
  class Base : public QDialog
  {
    Q_OBJECT
  public:
    Base(ftr::Base*, bool = false);
    Base(ftr::Base*, QWidget*, bool = false);
    ~Base() override;
  protected:
    prj::Project *project = nullptr;
    bool isEditDialog = false;
    bool isAccepted = false;
    std::vector<boost::uuids::uuid> leafChildren; //!< used for rolling back and forward during edits.
    std::unique_ptr<msg::Node> node;
    std::unique_ptr<msg::Sift> sift;
    
    void queueUpdate();
    void commandDone();
  };
}

#endif // DLG_BASE_H
