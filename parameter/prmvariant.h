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


#ifndef PRM_VARIANT_H
#define PRM_VARIANT_H

#include <string>
#include <variant>

#include <boost/filesystem/path.hpp>

#include <osg/Vec3d>
#include <osg/Quat>
#include <osg/Matrixd>

#include "feature/ftrpick.h"

namespace prm
{
  using Variant = std::variant
  <
    double,
    int,
    bool,
    std::string,
    boost::filesystem::path,
    osg::Vec3d,
    osg::Quat,
    osg::Matrixd,
    ftr::Picks
  >;
  
  struct Stow
  {
    Variant variant;
    
    Stow(double i) : variant(i){}
    Stow(int i) : variant(i){}
    Stow(bool i) : variant(i){}
    Stow(const std::string &i) : variant(i){}
    Stow(const boost::filesystem::path &i) : variant(i){}
    Stow(const osg::Vec3d &i) : variant(i){}
    Stow(const osg::Quat &i) : variant(i){}
    Stow(const osg::Matrixd &i) : variant(i){}
    Stow(const ftr::Picks &i) : variant(i){}
    Stow(const Variant &i) : variant(i){}
  };
}

#endif // PRM_VARIANT_H
