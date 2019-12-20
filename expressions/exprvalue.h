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

#include <cassert>

#include <boost/variant/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/get.hpp>

#include <osg/Matrixd>
#include <osg/io_utils>

#ifndef EXPR_VALUE_H
#define EXPR_VALUE_H

namespace expr
{
  /*! @brief ValueTypes. 
   * 
   * represent what kind of output the nodes have.
   */
  enum class ValueType
  {
    Variant = 0,  //!< might be any.
    Scalar,       //!< double value
    Vector,       //!< osg::Vec3d
    Quat,         //!< osg::Quat
    CSys          //!< osg::matrixd
  };
  
  //!* an object capable of holding all expression outputs.
  typedef boost::variant
  <
    double,
    osg::Vec3d,    //!< vector
    osg::Quat,     //!< orientation
    osg::Matrixd   //!< coordinate system
  > Value;
  
  class DoubleVisitor : public boost::static_visitor<double>
  {
  public:
    double operator()(const double &dIn) const {return dIn;}
    double operator()(const osg::Vec3d&) const {assert(0); return 0.0;}
    double operator()(const osg::Quat&) const {assert(0); return 0.0;}
    double operator()(const osg::Matrixd&) const {assert(0); return 0.0;}
  };
  
  class VectorVisitor : public boost::static_visitor<osg::Vec3d>
  {
  public:
    osg::Vec3d operator()(const double&) const {assert(0); return osg::Vec3d();}
    osg::Vec3d operator()(const osg::Vec3d &vIn) const {return vIn;}
    osg::Vec3d operator()(const osg::Quat&) const {assert(0); return osg::Vec3d();}
    osg::Vec3d operator()(const osg::Matrixd&) const {assert(0); return osg::Vec3d();}
  };
  
  class QuatVisitor : public boost::static_visitor<osg::Quat>
  {
  public:
    osg::Quat operator()(const double&) const {assert(0); return osg::Quat();}
    osg::Quat operator()(const osg::Vec3d&) const {assert(0); return osg::Quat();}
    osg::Quat operator()(const osg::Quat &qIn) const {return qIn;}
    osg::Quat operator()(const osg::Matrixd&) const {assert(0); return osg::Quat();}
  };
  
  class MatrixVisitor : public boost::static_visitor<osg::Matrixd>
  {
  public:
    osg::Matrixd operator()(const double&) const {assert(0); return osg::Matrixd();}
    osg::Matrixd operator()(const osg::Vec3d&) const {assert(0); return osg::Matrixd();}
    osg::Matrixd operator()(const osg::Quat&) const {assert(0); return osg::Matrixd();}
    osg::Matrixd operator()(const osg::Matrixd &mIn) const {return mIn;}
  };
  
  class ValueStreamVisitor : public boost::static_visitor<>
  {
  public:
    ValueStreamVisitor(std::ostream &streamIn): stream(streamIn){}
    std::ostream &stream;
    
    void operator()(const double &dIn) const
    {
      stream << dIn;
    }
    
    void operator()(const osg::Vec3d &vIn) const
    {
      stream << vIn;
    }
    
    void operator()(const osg::Quat &qIn) const
    {
      stream << qIn;
    }
    
    void operator()(const osg::Matrixd &mIn) const
    {
      stream << mIn;
    }
  };
}

inline std::ostream& operator<<(std::ostream &stream, const expr::Value &valueIn)
{
  expr::ValueStreamVisitor visitor(stream);
  boost::apply_visitor(visitor, valueIn);
  return stream;
}

#endif // EXPR_VALUE_H
