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

#include <QObject> //for translate

#include "tools/tlsnameindexer.h"

using namespace tls;

NameIndexer::NameIndexer(int boundIn)
{
  mag = magnitude(boundIn);
}

NameIndexer::NameIndexer(std::size_t boundIn)
: NameIndexer(static_cast<int>(boundIn))
{
  
}

/*! @brief string suffix
 * 
 * @param autoIncrement is true increments the current index by 1
 * @return suffix padded with zeros made from current index
 */
std::string NameIndexer::buildSuffix(bool autoIncrement)
{
  std::string prefix;
  int currentMag = magnitude(current);
  for (int index = 0; index < (mag - currentMag); ++index)
    prefix += QObject::tr("0").toStdString();
  prefix += std::to_string(current);
  
  if (autoIncrement)
    current++;
  
  return prefix;
}

/*! @brief increments the current index by 1
 */
void NameIndexer::bump()
{
  current++;
}

void NameIndexer::setCurrent(int indexIn)
{
  current = indexIn;
}

/*! @brief get the order of magnitude of a number
 * 
 * 0-9 = 0
 * 10-99 = 1
 * 100-999 = 2
 * 1000-9999 = 3
 * etc...
 */
int NameIndexer::magnitude(int boundIn)
{
  int oom = 0; //order of magnitude.
  double numer = static_cast<double>(boundIn);
  double denom = 10.0;
  int maxIterations = 1000000;
  while ((numer / denom) >= 1.0 && oom < maxIterations)
  {
    oom++;
    denom *= 10.0;
  }
  return oom;
}

int NameIndexer::magnitude(std::size_t boundIn)
{
  return magnitude(static_cast<int>(boundIn));
}
