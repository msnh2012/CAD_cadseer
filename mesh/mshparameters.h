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

#ifndef MSH_PARAMETERS_H
#define MSH_PARAMETERS_H

#include <limits>
#include <boost/filesystem/path.hpp>

#include <IMeshTools_Parameters.hxx>

namespace nglib{class Ng_Meshing_Parameters;}
namespace prj
{
  namespace srl
  {
    namespace mshs
    {
      class ParametersOCCT;
      class ParametersNetgen;
      class ParametersGMSH;
    }
  }
}

namespace msh
{
  namespace prm
  {
    /**
    * @struct OCCT
    * @brief Parameters for generating mesh using occt
    */
    struct OCCT : public IMeshTools_Parameters
    {
      OCCT();
      void serialIn(const prj::srl::mshs::ParametersOCCT&);
      prj::srl::mshs::ParametersOCCT serialOut() const;
    };
    
    /**
    * @struct Netgen
    * @brief Parameters for generating mesh using netgen
    * @details values are defaulted to values from nglib.cpp.
    * The netgen api is a little inconvenient because it is structured
    * around the reading of a file from a given filename. So we are
    * adding a filename that will be used as a temporary file to write
    * the shape and then let netgen read it.
    */
    struct Netgen
    {
      bool useLocalH = true;
      double maxH = 1000.0;
      double minH = 0.0;
      
      double fineness = 0.5;
      double grading = 0.3;

      double elementsPerEdge = 2.0;
      double elementsPerCurve = 2.0;

      bool closeEdgeEnable = false;
      double closeEdgeFactor = 2.0;

      bool minEdgeLenEnable = false;
      double minEdgeLen = 1e-4;

      bool secondOrder = false;
      bool quadDominated = false;

      boost::filesystem::path filePath;

      bool optSurfMeshEnable = true;
      bool optVolMeshEnable = true;

      int optSteps2d = 3;
      int optSteps3d = 3;

      bool invertTets = false;
      bool invertTrigs = false;

      bool checkOverlap = true;
      bool checkOverlappingBoundary = true;
      
#ifdef NETGEN_PRESENT
      nglib::Ng_Meshing_Parameters convert() const;
#endif
      void resetValues();
      void ensureValidValues();
      void serialIn(const prj::srl::mshs::ParametersNetgen&);
      prj::srl::mshs::ParametersNetgen serialOut() const;
    };
    
    /**
    * @struct GMSH
    * @brief Parameters for generating mesh using gmsh
    */
    struct GMSH
    {
      struct Option
      {
      public:
        struct Enumerate;
        
        Option(const std::string&, const std::string&, double);
        Option(const std::string&, const std::string&, double, std::unique_ptr<Enumerate>);
        Option(Option&&);
        ~Option();
        std::string getKey() const {return key;}
        std::string getDescription() const {return description;}
        void setValue(double);
        void setToDefault();
        bool isDefault() const;
        double getValue() const {return value;}
        bool isEnumerated() const {return static_cast<bool>(enumerate);}
        void setValue(const std::string&);
        std::vector<std::string> getEnumerateStrings() const;
      private:
        std::string key;
        std::string description;
        double value;
        double defaultValue;
        std::unique_ptr<Enumerate> enumerate;
      };
      GMSH();
      std::vector<Option> options;
      void setOption(const std::string&, double);
      void setOption(const std::string&, const std::string&);
      bool refine = false;
      boost::filesystem::path filePath;
      
      void serialIn(const prj::srl::mshs::ParametersGMSH&);
      prj::srl::mshs::ParametersGMSH serialOut() const;
    };
  }
}

#endif // MSH_PARAMETERS_H
