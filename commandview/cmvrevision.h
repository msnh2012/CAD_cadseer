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

#ifndef CMV_REVISION_H
#define CMV_REVISION_H

#include <memory>

#include "commandview/cmvbase.h"

class QListWidget;
class QTabWidget;
namespace cmd{class Revision;}
namespace dlg{class CommitWidget; class TagWidget;}

namespace cmv
{
  class UndoPage : public QWidget
  {
    Q_OBJECT
  public:
    UndoPage(QWidget*);
    virtual ~UndoPage() override;
    
  protected:
    struct Data;
    std::unique_ptr<Data> data;
    QListWidget *commitList;
    dlg::CommitWidget *commitWidget;
    
    void buildGui();
    void fillInCommitList();
    
    void commitRowChangedSlot(int);
    void resetActionSlot();
  };
  
  class AdvancedPage : public QWidget
  {
    Q_OBJECT
  public:
    AdvancedPage(QWidget*);
    virtual ~AdvancedPage() override;
    
  protected:
    struct Data;
    std::unique_ptr<Data> data;
    QListWidget *tagList;
    dlg::TagWidget *tagWidget;
    QAction *createTagAction;
    QAction *checkoutTagAction;
    QAction *destroyTagAction;
    
    void init();
    void buildGui();
    void fillInTagList();
    void setCurrentHead();
    
    void tagRowChangedSlot(int);
    void createTagSlot();
    void destroyTagSlot();
    void checkoutTagSlot();
  };
  
  class Revision : public Base
  {
    Q_OBJECT
  public:
    Revision(cmd::Revision*);
    ~Revision() override;
    
  private:
    struct Stow;
    std::unique_ptr<Stow> stow;
  };
}

#endif // CMV_REVISION_H
