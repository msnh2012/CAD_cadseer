/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2022 Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef LWF_POINT_H
#define LWF_POINT_H

#include "feature/ftrpick.h"
#include "parameter/prmparameter.h"
#include "library/lbrplabel.h"
#include "library/lbrplabelgrid.h"

namespace lwf
{
  namespace Tags
  {
    inline constexpr std::string_view parameter = "parameter";
    inline constexpr std::string_view value = "value";
    inline constexpr std::string_view slope = "slope";
    inline constexpr std::string_view pick = "pick";
  }

  /*! @struct Node
   *  @brief A general parameter and label for graph.
   *  @details A general parameter and label for graph.
   */
  struct Node
  {
    bool isNew = true;
    prm::Parameter parameter;
    osg::ref_ptr<osg::Transform> osgNode;
    
    Node(const QString &NameIn, double dIn, std::string_view tagIn)
    : parameter(NameIn, dIn, tagIn)
    {}
    
    Node(const QString &NameIn, ftr::Picks psIn, std::string_view tagIn)
    : parameter(NameIn, psIn, tagIn)
    {}
    
    Node(const prj::srl::spt::Parameter &pIn)
    : parameter(pIn)
    {}
    
    static Node buildParameter(double dIn){return Node(QObject::tr("Parameter"), dIn, Tags::parameter);}
    static Node buildValue(double dIn){return Node(QObject::tr("Value"), dIn, Tags::value);}
    static Node buildSlope(double dIn){return Node(QObject::tr("Slope"), dIn, Tags::slope);}
    static Node buildPick(const ftr::Pick &pIn){return Node(QObject::tr("Pick"), {pIn}, Tags::pick);}
  };
}

#endif //LWF_POINT_H
