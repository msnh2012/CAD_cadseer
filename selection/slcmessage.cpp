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

#include <algorithm>

#include "tools/idtools.h"
#include "selection/slcmessage.h"

using namespace slc;

Message::Message(Mask mask)
: selectionMask(mask)
, featureId(gu::createNilId())
, shapeId(gu::createNilId())
{}

Message::Message(const boost::uuids::uuid &fIdIn)
: featureId(fIdIn)
, shapeId(gu::createNilId())
{}

bool slc::has(const slc::Messages& messagesIn, const slc::Message& messageIn)
{
  auto it = std::find(messagesIn.begin(), messagesIn.end(), messageIn);
  return it != messagesIn.end();
}

void slc::add(slc::Messages& messagesIn, const slc::Message& messageIn)
{
  auto it = std::find(messagesIn.begin(), messagesIn.end(), messageIn);
  if (it == messagesIn.end())
    messagesIn.push_back(messageIn);
}

int slc::remove(slc::Messages& messagesIn, const slc::Message& messageIn)
{
  int out = -1;
  auto it = std::find(messagesIn.begin(), messagesIn.end(), messageIn);
  if (it != messagesIn.end())
  {
    out = std::distance(messagesIn.begin(), it);
    messagesIn.erase(it);
  }
  return out;
}

std::vector<Messages> slc::split(const Messages &msgsIn)
{
  std::vector<Messages> out;
  auto findInOut = [&](const Message &mIn) -> std::vector<Messages>::iterator
  {
    for (auto it = out.begin(); it != out.end(); ++it)
    {
      //there is always one. because we don't add an empty vector
      if (it->front().featureId == mIn.featureId)
        return it;
    }
    return out.end();
  };
  
  for (const auto &msg : msgsIn)
  {
    auto it = findInOut(msg);
    if (it == out.end())
      out.push_back({msg});
    else
      it->push_back(msg);
  }
  
  return out;
}
