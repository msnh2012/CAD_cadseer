/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#include <QShowEvent>
#include <QHideEvent>
#include <QTimer>

#include <message/node.h>
#include <message/sift.h>
#include <selection/message.h>
#include <selection/eventhandler.h>
#include <dialogs/selectionbutton.h>

using namespace dlg;

SelectionButton::SelectionButton(QWidget *parent) : SelectionButton(QString(), parent)
{
}

SelectionButton::SelectionButton(const QString &text, QWidget *parent) : SelectionButton(QIcon(), text, parent)
{
}

SelectionButton::SelectionButton(const QIcon &icon, const QString &text, QWidget *parent) : QPushButton(icon, text, parent)
{
  node = std::make_unique<msg::Node>();
  node->connect(msg::hub());
  sift = std::make_unique<msg::Sift>();
  sift->name = "dlg::SelectionButton";
  node->setHandler(std::bind(&msg::Sift::receive, sift.get(), std::placeholders::_1));
    
  setCheckable(true);
  mask = slc::None;
  
  setupDispatcher();
  
  connect(this, &QPushButton::toggled, this, &SelectionButton::toggledSlot);
}

SelectionButton::~SelectionButton()
{

}

void SelectionButton::setupDispatcher()
{
  sift->insert
  (
    {
      std::make_pair
      (
        msg::Response | msg::Post | msg::Selection | msg::Add
        , std::bind(&SelectionButton::selectionAdditionDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Pre | msg::Selection | msg::Remove
        , std::bind(&SelectionButton::selectionSubtractionDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Selection | msg::SetMask
        , std::bind(&SelectionButton::selectionMaskDispatched, this, std::placeholders::_1)
      )
    }
  );
}

/* I think it will be easier to check state in the message handlers than trying
 * to block boost signals in qt events.
 */
void SelectionButton::selectionAdditionDispatched(const msg::Message &mIn)
{
  if (!isVisible() || !isChecked())
    return;
  
  messages.push_back(mIn.getSLC());
  
  dirty();
  
  if (isSingleSelection)
    QTimer::singleShot(0, this, SIGNAL(advance()));
}

void SelectionButton::selectionSubtractionDispatched(const msg::Message &mIn)
{
  if (!isVisible() || !isChecked())
    return;
  
  const slc::Message& sm = mIn.getSLC();
  if (slc::has(messages, sm))
  {
    slc::remove(messages, sm);
    dirty();
  }
}

void SelectionButton::setMessages(const slc::Messages &msIn)
{
  messages = msIn;
  dirty();
}

void SelectionButton::setMessages(const slc::Message &mIn)
{
  messages.clear();
  addMessage(mIn);
  dirty();
}

void SelectionButton::addMessage(const slc::Message &mIn)
{
  slc::add(messages, mIn);
  dirty();
}

void SelectionButton::addMessages(const slc::Messages &msIn)
{
  std::copy(msIn.begin(), msIn.end(), std::back_inserter(messages));
  dirty();
}

void SelectionButton::syncToSelection()
{
  if (mask != slc::None)
    node->sendBlocked(msg::buildSelectionMask(mask));
  
  for (const auto &m : messages)
  {
    msg::Message mm
    (
      msg::Request | msg::Selection | msg::Add
      , m
    );
    node->sendBlocked(mm);
  }
  
  if (!statusPrompt.isEmpty())
    node->sendBlocked(msg::buildStatusMessage(statusPrompt.toStdString()));
}

void SelectionButton::showEvent(QShowEvent *)
{
  if (!isChecked())
    return;
  
  syncToSelection();
}

void SelectionButton::hideEvent(QHideEvent *)
{
  if (!isChecked())
    return;
  
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

void SelectionButton::toggledSlot(bool cState)
{
  if (cState)
    syncToSelection();
  else
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

void SelectionButton::selectionMaskDispatched(const msg::Message &messageIn)
{
  if (!isVisible() || !isChecked())
    return;
  
  mask = messageIn.getSLC().selectionMask;
}