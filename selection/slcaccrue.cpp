/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2019  Thomas S. Anderson blobfish.at.gmx.com
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

#include <QObject>

#include "selection/slcaccrue.h"

using namespace slc;

const Accrues Accrue::accrues =
{
  Accrue(Accrue::None, QObject::tr("None"))
  , Accrue(Accrue::Tangent, QObject::tr("Tangent"))
};

Accrue::Accrue(Type t, const QString &s)
: type(t)
, userString(s)
{}

Accrue::Accrue()
: type(accrues.front().type)
, userString(accrues.front().userString)
{}

Accrue::Accrue(Type t)
{
  for (const auto &a : accrues)
  {
    if (a.type == t)
    {
      type = a.type;
      userString = a.userString;
      break;
    }
  }
}

Accrue::Accrue(int index)
{
  assert(index >= 0);
  assert(index < static_cast<int>(accrues.size()));
  type = accrues.at(index).type;
  userString = accrues.at(index).userString;
}

Accrue::Accrue(const QString &s)
{
  for (const auto &a : accrues)
  {
    if (a.userString == s)
    {
      type = a.type;
      userString = a.userString;
      break;
    }
  }
}

Accrue::operator Type() const
{
  return type;
}

Accrue::operator int() const
{
  return static_cast<int>(type);
}

Accrue::operator const QString&() const
{
  return userString;
}
