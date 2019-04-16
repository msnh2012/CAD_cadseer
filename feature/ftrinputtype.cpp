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

#include <globalutilities.h>
#include <feature/inputtype.h>

using namespace ftr;

InputType::InputType(std::initializer_list<std::string> listIn) : tags(listIn)
{
  gu::uniquefy(tags);
}

InputType::InputType(const InputType &other) : tags(other.tags)
{
  gu::uniquefy(tags); //should be redundant.
}

bool InputType::has(const std::string &tagIn) const
{
  auto it = std::find(tags.begin(), tags.end(), tagIn);
  return it != tags.end();
}

void InputType::add(const std::string &tagIn)
{
  tags.push_back(tagIn);
  gu::uniquefy(tags);
}

void InputType::add(std::initializer_list<std::string> listIn)
{
  for (const std::string &s : listIn)
    tags.push_back(s);
  gu::uniquefy(tags);
}

void InputType::remove(const std::string &tagIn)
{
  std::vector<std::string> t(1, tagIn);
  std::vector<std::string> temp;
  std::set_difference(tags.begin(), tags.end(), t.begin(), t.end(), std::back_inserter(temp));
  tags = temp;
}

void InputType::remove(std::initializer_list<std::string> listIn)
{
  std::vector<std::string> temp;
  std::set_difference(tags.begin(), tags.end(), listIn.begin(), listIn.end(), std::back_inserter(temp));
  tags = temp;
}

InputType& InputType::operator +=(const InputType &other)
{
  for (const auto &t : other.tags)
    tags.push_back(t);
  gu::uniquefy(tags);
  return *this;
}
