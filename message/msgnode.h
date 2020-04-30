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

#ifndef MSG_NODE_H
#define MSG_NODE_H

#include <memory>
#include <vector>

#include "message/msgmessage.h"

namespace boost{namespace signals2{class shared_connection_block;}}

namespace msg
{
  /*! @struct Node 
   * @brief for message connections
   * 
   * @details Can be used as mesh or partial mesh network.
   * Where nodes are equal and connected as needed.
   * https://en.wikipedia.org/wiki/Mesh_networking
   * 
   * Can be used as a star network. Where one and only one node
   * is a hub and all others node connect to the hub only.
   * A node is NOT a hub by default.
   * https://en.wikipedia.org/wiki/Star_network
   * 
   * In any configuration, there are no safe guards for recursive
   * signals. @see sendBlocked @see createBlocker
   * 
   * This wrapper struct is meant to replace both the current
   * and observer class with a pimpl idiom for compile performance.
   */
  struct Node
  {
  public:
    Node();
    Node(Handler);
    ~Node();
    Node(const Node&) = delete; //no copy
    Node& operator=(const Node&) = delete; //no copy
    Node(Node &&);
    Node& operator=(Node &&);
    
    void setHandler(Handler);
    void send(const Message&) const;
    void sendBlocked(const Message&) const;
    std::vector<boost::signals2::shared_connection_block> createBlocker() const;
    void setHub(bool vIn = true){hub = vIn;}
    
    void connect(Node &other);
    
  private:
    void receive(const Message&) const;
    
    Handler handler; //!< callback function to pass the message out.
    struct Stow; //!< forward declare for private data
    std::unique_ptr<Stow> stow; //!< private data
    bool hub = false; //!< true means node sends any received messages.
  };
  
  //! Singleton message hub.
  Node& hub();
}

#endif // MSG_NODE_H
