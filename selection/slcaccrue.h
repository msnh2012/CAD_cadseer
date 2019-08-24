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

#ifndef SLC_ACCRUE_H
#define SLC_ACCRUE_H

#include <string>
#include <vector>

#include <QString>

namespace slc
{
  struct Accrue;
  typedef std::vector<Accrue> Accrues;
  
  struct Accrue
  {
  public:
    enum Type //add entries to static array initializer list in source file
    {
      None = 0
      , Tangent
    };
    
    Accrue(); //!< uses static array
    Accrue(Type); //!< uses static array
    explicit Accrue(int); //!< uses static array
    explicit Accrue(const QString&); //!< uses static array
    
    //!conversion operators
    operator Type() const;
    explicit operator int() const;
    explicit operator const QString&() const;
    
    double angle = 0.0; //for tangent
    
    static const Accrues accrues;
  private:
    Type type;
    QString userString;
    
    Accrue(Type, const QString&); //!< only used to construct static array
  };
  
}

#endif //SLC_ACCRUE_H
