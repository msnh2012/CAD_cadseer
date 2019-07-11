/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2019 Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef LWF_CUE_H
#define LWF_CUE_H

#include <cassert>
#include <vector>

#include <boost/uuid/nil_generator.hpp>

#include <Law_Function.hxx>

#include <parameter/prmparameter.h>

namespace prj{namespace srl{namespace lwf{class Cue; class Data;}}}

/*! @namespace lwf
 * @brief Law Function Namespace
 * 
 */
namespace lwf
{
  /*! @enum Type Enumeration for the type of law function */
  enum class Type
  {
    none
    , constant
    , linear
    , interpolate
    , composite
  };
  
  /*! Note this is a vector of value type parameters.
   * Where as prm::Parameters is a vector of parameter pointers
   */
  typedef std::vector<prm::Parameter> Parameters;
  
  /*! @struct Data
   * @brief value type data for one law.
   * 
   * @details this is separate as a composite law can have many laws.
   * 
   */
  struct Data
  {
    Data(){}
    Data(Type tIn): subType(tIn){}
    Data(const Parameters &psIn): internalParameters(psIn){}
    Data(Type tIn, const Parameters &psIn): subType(tIn), internalParameters(psIn){}
    Type subType = Type::none; //sub-law type. only for composite.
    Parameters internalParameters; //only for interpolate and derivative is ignored.
    prj::srl::lwf::Data serialOut() const;
    void serialIn(const prj::srl::lwf::Data&);
  };
  
  /*! @struct Cue
   * @brief Value type for law function definition.
   * 
   * @details Parameter are of the type osg::Vec3d!
   * When building composite, append calls must be done in parametric order.
   * boundaries and datas are parallel vectors with boundaries.size() == datas.size() + 1.
   */
  struct Cue
  {
    Cue();
    Cue(const prj::srl::lwf::Cue&);
    Type type = Type::none; //overall type.
    Parameters boundaries; //size() = 2 except for composite laws.
    std::vector<Data> datas; //for composite laws.
    bool periodic = false;
    
    //! set to a constant law [0.0, 1.0] with value
    void setConstant(double vIn);
    
    //! set to a constant law with points.
    void setConstant(const prm::Parameter &begin, const prm::Parameter &end);
    
    //! set to a linear law [0.0, 1.0] with values
    void setLinear(double firstValue, double lastValue);
    
    //! set to a linear law with points
    void setLinear(const prm::Parameter &begin, const prm::Parameter &end);
    
    //! set to an interpolate law [0.0, 1.0] with values and derivatives
    void setInterpolate
    (
      double firstValue
      , double lastValue
      , double firstDerivative = 0.0 // 's' law
      , double lastDerivative = 0.0 // 's' law
    );
    
    //! set to an interpolate law with points
    void setInterpolate(const prm::Parameter &begin, const prm::Parameter &end);
    
    //! add internal points to interpolate law
    void setInterpolateParameters(const Parameters &psIn);
    
    /*! add internal point to interpolate law
     * 
     * @param index The index of law for composite.
     * @note reconfigures all internal points.
     */
    void addInternalParameter(double prm = 0);
    
    /*! remove internal point from interpolate law
     * 
     * @param index The index of law for composite.
     * @note reconfigures all internal points.
     */
    void removeInternalParameter(double prm = 0);
    
    //! set to a composite law at parameter 0.0 with first value
    void setComposite(double firstValue);
    
    //! set to a composite law beginning with point
    void setComposite(const prm::Parameter &pIn);
    
    //! append constant law to composite law
    void appendConstant(double endParameter);
    
    //! append linear law to composite law
    void appendLinear(double endParameter, double endValue);
    
    //! append linear law to composite law
    void appendLinear(const prm::Parameter &endParameter);
    
    //! append interpolate law to composite law
    void appendInterpolate(double endParameter, double endValue);
    
    //! append interpolate law to composite law
    void appendInterpolate(const prm::Parameter &endParameter);
    
    /*! append interpolate law to composite law.
     * 
     * @param endpoint parameter must be greater than last boundary point parameter.
     * @param internalParametersIn must be in parametric order.
     * parameters must < than endParameter parameter.
     * parameters must be > previous boundary point parameter.
     */
    void appendInterpolate(const prm::Parameter &endParameter, const Parameters &internalParametersIn);
    
    /*! remove sub law from composite law
     * @param index position of law to remove.
     */
    void remove(int index);
    
    /*! @brief smooth composite.
     * 
     * @details makes c1 where possible.
     */
    void smooth();
    
    /*! @brief make constant start and end values equal.
     * 
     * @details change parameters from the overlay geometry just dirties
     * feature and updates. So if we change one end of the constant the
     * overlay graph is wrong. This sets the end equal to the front.
     * user will just have to change first parameter of the constant law.
     */
    void alignConstant();
    
    /*! @brief search composite law and find the index of the sub law containing parameter.
     * 
     * @param prm parameter value
     * @return index of sub law or -1.
     */
    int compositeIndex(double prm);
    
    /*! @brief Shrink the last parameter of a composite.
     * 
     * @return parameter before squeeze.
     * @details This prepares the cue for a new law.
     */
    double squeezeBack();
    
    /*! @brief get parameters contained in cue
     * 
     * @return vector of parameter pointers.
     * @details parameters are of the type osg::Vec3d
     */
    std::vector<const prm::Parameter*> getParameters() const;
    std::vector<prm::Parameter*> getParameters();
    
    opencascade::handle<Law_Function> buildLawFunction() const;
    
    prj::srl::lwf::Cue serialOut() const;
    void serialIn(const prj::srl::lwf::Cue&);
  };
  
  /*! @brief build points along law curve for visual
    * 
    * @param law is source of points.
    * @param steps is the number of steps between first and last parameter.
    * @return vector of points along curve.
    */
  std::vector<osg::Vec3d> discreteLaw(opencascade::handle<Law_Function> &law, int steps);
  
}

#endif //LWF_CUE_H
