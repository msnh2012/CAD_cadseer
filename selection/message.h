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

#ifndef SELECTIONMESSAGE_H
#define SELECTIONMESSAGE_H

#include <boost/uuid/uuid.hpp>

#include <osg/Vec3d>

#include <selection/definitions.h>

namespace slc
{
  struct Message
  {
    Message();
    slc::Type type;
    boost::uuids::uuid featureId;
    boost::uuids::uuid shapeId;
    osg::Vec3d pointLocation;
  };
}

#endif // SELECTIONMESSAGE_H