/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2018  Thomas S. Anderson blobfish.at.gmx.com
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

#include <boost/signals2.hpp>

#include "node.h"

namespace msg
{
  struct Node::Stow
  {
    std::vector<boost::signals2::scoped_connection> connections;
    boost::signals2::signal<void (const msg::Message &)> signal;
  };
}

using namespace msg;

Node::Node() : stow(std::make_unique<Stow>()) {}

Node::Node(Handler hIn) : handler(hIn), stow(std::make_unique<Stow>()) {}

Node::~Node() = default;

Node::Node(Node &&) = default;

Node& Node::operator=(Node &&) = default;

void Node::setHandler(Handler hIn)
{
  handler = hIn;
}

void Node::send(const Message &mIn) const
{
  stow->signal(mIn);
}

void Node::sendBlocked(const Message &mIn) const
{
  std::vector<boost::signals2::shared_connection_block> blockers;
  for (const auto &c : stow->connections)
    blockers.emplace_back(c);
  stow->signal(mIn);
}

std::vector<boost::signals2::shared_connection_block> Node::createBlocker() const
{
  std::vector<boost::signals2::shared_connection_block> blockers;
  for (const auto &c : stow->connections)
    blockers.emplace_back(c);
  return blockers;
}

void Node::connect(Node &other)
{
  other.stow->connections.push_back(this->stow->signal.connect(std::bind(&Node::receive, &other, std::placeholders::_1)));
  this->stow->connections.push_back(other.stow->signal.connect(std::bind(&Node::receive, this, std::placeholders::_1)));
}

void Node::receive(const Message &mIn) const
{
  if (handler)
    handler(mIn); //handle before hub send?
  if (hub)
    stow->signal(mIn);
}

Node& msg::hub()
{
  static Node n;
  n.setHub(true);
  return n;
}
