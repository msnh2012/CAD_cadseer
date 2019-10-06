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

#ifndef TLS_NAMEINDEXER_H
#define TLS_NAMEINDEXER_H

#include <string>

namespace tls
{
  class NameIndexer
  {
  public:
    NameIndexer() = delete;
    explicit NameIndexer(int);
    explicit NameIndexer(std::size_t);
    
    std::string buildSuffix(bool = true);
    void bump();
    void setCurrent(int);
    
    static int magnitude(int);
    static int magnitude(std::size_t);
  private:
    int mag = 0;
    int current = 0;
  };
}

#endif // TLS_NAMEINDEXER_H
