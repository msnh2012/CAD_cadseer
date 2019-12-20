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

#ifndef FTR_INPUTTYPE_H
#define FTR_INPUTTYPE_H

#include <vector>
#include <string>
#include <initializer_list>

namespace ftr
{
  //! information describing connection between features
  class InputType
  {
  public:
    //@{
    //! Convenient string constants that may apply to more than one feature.
    constexpr static const char *target = "Target";
    constexpr static const char *tool = "Tool";
    constexpr static const char *create = "Create";
    constexpr static const char *linkCSys = "LinkCSys";
    //@}
    
    //@{
    //! Constructors
    InputType(){}
    InputType(std::initializer_list<std::string> listIn);
    InputType(const InputType &other);
    //@}
    
    void add(const std::string &tagIn);
    void add(std::initializer_list<std::string> listIn);
    void remove(const std::string &tagIn);
    void remove(std::initializer_list<std::string> listIn);
    bool has(const std::string &tagIn) const;
    bool isEmpty(){return tags.empty();}
    InputType& operator +=(const InputType &other);
    
    const std::vector<std::string>& getTags() const {return tags;}
    static std::string createIndexedTag(const std::string&, int);
    
  private:
    std::vector<std::string> tags;
  };
}

#endif //FTR_INPUTTYPE_H
