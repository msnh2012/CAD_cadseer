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

#include <tools/idtools.h>
#include <selection/message.h>

slc::Message::Message()
{
  type = slc::Type::None;
  featureType = ftr::Type::Base; //like empty
  featureId = gu::createNilId();
  shapeId = gu::createNilId();
  selectionMask = 0;
}

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

void slc::remove(slc::Messages& messagesIn, const slc::Message& messageIn)
{
  auto it = std::find(messagesIn.begin(), messagesIn.end(), messageIn);
  if (it != messagesIn.end())
    messagesIn.erase(it);
}
