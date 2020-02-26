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

#include <QScrollArea>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "message/msgnode.h"
#include "message/msgsift.h"
#include "message/msgmessage.h"
#include "commandview/cmvbase.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvpane.h"

namespace cmv
{
  struct Pane::Stow
  {
    QScrollArea *scrollArea;
    QPushButton *doneButton;
    
    msg::Node node;
    msg::Sift sift;
    
    Stow()
    {
      node.connect(msg::hub());
      sift.name = "vwr::Widget";
      node.setHandler(std::bind(&msg::Sift::receive, &sift, std::placeholders::_1));
      setupDispatcher();
    }
    
    void addView(const msg::Message &mIn)
    {
      assert(mIn.isCMV());
      if (!mIn.isCMV())
        return;
      const cmv::Message &cm = mIn.getCMV();
      assert(cm.widget);
      scrollArea->setWidget(cm.widget);
      cm.widget->show();
    }
    
    void removeView(const msg::Message&)
    {
      scrollArea->takeWidget();
    }
    
    void setupDispatcher()
    {
      sift.insert
      (
        {
          std::make_pair
          (
            msg::Request | msg::Command | msg::View | msg::Show
            , std::bind(&Stow::addView, this, std::placeholders::_1)
          )
          , std::make_pair
          (
            msg::Request | msg::Command | msg::View | msg::Hide
            , std::bind(&Stow::removeView, this, std::placeholders::_1)
          )
        }
      );
    }
  };
}

using namespace cmv;

Pane::Pane()
: stow(std::make_unique<Stow>())
{
  buildGui();
}

Pane::~Pane()
{
  /* commands should delete their views, so
   * we remove the current view, if it exists.
   * I would rather leak one view at application
   * close, than double free and crash.
   */
  if (stow->scrollArea->widget())
    stow->scrollArea->takeWidget();
}

void Pane::buildGui()
{
  QVBoxLayout *mainLayout = new QVBoxLayout();
  mainLayout->setContentsMargins(0, 0, 0, 0);
  
  stow->scrollArea = new QScrollArea(this);
  stow->scrollArea->setWidgetResizable(true);
  mainLayout->addWidget(stow->scrollArea);
  
  stow->doneButton = new QPushButton(tr("Done"), this);
  QHBoxLayout *doneLayout = new QHBoxLayout();
  doneLayout->addStretch();
  doneLayout->addWidget(stow->doneButton);
  doneLayout->addStretch();
  mainLayout->addLayout(doneLayout);
  connect(stow->doneButton, &QPushButton::clicked, this, &Pane::doneSlot);
  
  this->setLayout(mainLayout);
}

void Pane::doneSlot()
{
  stow->node.send(msg::Message(msg::Request | msg::Command | msg::Done));
}
