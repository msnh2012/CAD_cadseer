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

#ifndef MSG_MESSAGE_H
#define MSG_MESSAGE_H

#include <bitset>
#include <functional>
#include <unordered_map>

#include <boost/variant.hpp>

#include <project/message.h>
#include <selection/message.h>
#include <application/message.h>
#include <viewer/message.h>

namespace msg
{
  //! Mask is for a key in a function dispatch message handler. We might need a validation routine.
  // no pre and post on requests only on response.
  typedef std::bitset<128> Mask;
  static const Mask Request(Mask().		set(0));//!< message classification
  static const Mask Response(Mask().		set(1));//!< message classification
  static const Mask Pre(Mask().			set(2));//!< message response timing. think "about to". Data still valid
  static const Mask Post(Mask().		set(3));//!< message response timing. think "done". Data invalid.
  static const Mask Preselection(Mask().	set(4));//!< selection classification
  static const Mask Selection(Mask().		set(5));//!< selection classification
  static const Mask Addition(Mask().		set(6));//!< selection action
  static const Mask Subtraction(Mask().		set(7));//!< selection action
  static const Mask Clear(Mask().		set(8));//!< selection action
  static const Mask SetCurrentLeaf(Mask().	set(9));//!< project action
  static const Mask RemoveFeature(Mask().	set(10));//!< project action & command
  static const Mask AddFeature(Mask().		set(11));//!< project action
  static const Mask Update(Mask().		set(12));//!< project action. request only for all: model, visual etc..
  static const Mask ForceUpdate(Mask().		set(13));//!< project action. request only. marks dirty to force updates
  static const Mask UpdateModel(Mask().		set(14));//!< project action
  static const Mask UpdateVisual(Mask().	set(15));//!< project action
  static const Mask AddConnection(Mask().	set(16));//!< project action
  static const Mask RemoveConnection(Mask().	set(17));//!< project action
  static const Mask NewProject(Mask().		set(18));//!< application action
  static const Mask Construct(Mask().		set(19));//!< factory action.
  static const Mask ViewTop(Mask().		set(20));//!< command
  static const Mask ViewFront(Mask().		set(21));//!< command
  static const Mask ViewRight(Mask().		set(22));//!< command
  static const Mask ViewFit(Mask().		set(23));//!< command
  static const Mask ViewFill(Mask().		set(24));//!< command
  static const Mask ViewLine(Mask().		set(25));//!< command
  static const Mask Box(Mask().			set(26));//!< command
  static const Mask Sphere(Mask().		set(27));//!< command
  static const Mask Cone(Mask().		set(28));//!< command
  static const Mask Cylinder(Mask().		set(29));//!< command
  static const Mask Blend(Mask().		set(30));//!< command
  static const Mask Union(Mask().		set(31));//!< command
  static const Mask Subtract(Mask().		set(32));//!< command
  static const Mask Intersect(Mask().		set(33));//!< command
  static const Mask ImportOCC(Mask().		set(34));//!< command
  static const Mask ExportOCC(Mask().		set(35));//!< command
  static const Mask ExportOSG(Mask().		set(36));//!< command
  static const Mask Preferences(Mask().		set(37));//!< command
  static const Mask Remove(Mask().		set(38));//!< command
  static const Mask StatusText(Mask().		set(39));//!< display text for info cam
  
  typedef boost::variant<prj::Message, slc::Message, app::Message, vwr::Message> Payload;
  
  struct Message
  {
    Message(){}
    Message(const Mask &maskIn) : mask(maskIn){}
    Mask mask = 0;
    Payload payload;
  };
  
  typedef std::function< void (const Message&) > MessageHandler;
  typedef std::unordered_map<Mask, MessageHandler> MessageDispatcher;
}



#endif // MSG_MESSAGE_H
