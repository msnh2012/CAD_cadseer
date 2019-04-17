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

#ifndef MSG_VARIANT_H
#define MSG_VARIANT_H

#include <boost/variant/variant.hpp>

#include "project/prjmessage.h"
#include "selection/slcmessage.h"
#include "application/appmessage.h"
#include "viewer/vwrmessage.h"
#include "feature/ftrmessage.h"
#include "lod/lodmessage.h"

namespace msg
{
  typedef boost::variant
  <
    prj::Message,
    slc::Message,
    app::Message,
    vwr::Message,
    ftr::Message,
    lod::Message
  > Variant;
  
  struct Stow
  {
    Variant variant;
    
    Stow(const prj::Message &mIn) : variant(mIn){}
    Stow(const slc::Message &mIn) : variant(mIn){}
    Stow(const app::Message &mIn) : variant(mIn){}
    Stow(const vwr::Message &mIn) : variant(mIn){}
    Stow(const ftr::Message &mIn) : variant(mIn){}
    Stow(const lod::Message &mIn) : variant(mIn){}
  };
}

#endif // MSG_VARIANT_H
