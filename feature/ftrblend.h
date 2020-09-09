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

#ifndef FTR_BLEND_H
#define FTR_BLEND_H

#include <memory>

#include <osg/ref_ptr>

#include "library/lbrplabel.h"
#include "feature/ftrpick.h"
#include "feature/ftrbase.h"

class ChFi3d_FilBuilder;
class TopoDS_Edge;

namespace prj{namespace srl{namespace blns{class Blend;}}}
namespace ann{class SeerShape;}
namespace ftr
{
  class ShapeHistory;
  
  namespace Blend
  {
    enum class BlendType
    {
      Constant = 0
      , Variable
    };
    
    enum class Shape
    {
      Rational,
      QuasiAngular,
      Polynomial
    };
    
    struct Constant
    {
      Picks picks; //!< vector of picked objects
      std::shared_ptr<prm::Parameter> radius; //!< parameter containing blend radius.
      osg::ref_ptr<lbr::PLabel> label; //!< graphic icon
    };

    struct VariableEntry
    {
      Pick pick; //!< edge or vertex.
      std::shared_ptr<prm::Parameter> position; //!< parameter along edge 0 to 1. ignored if vertex. maybe invalid
      std::shared_ptr<prm::Parameter> radius; //!< value of blend.
      osg::ref_ptr<lbr::PLabel> label; //!< graphic icon
      osg::ref_ptr<lbr::PLabel> positionLabel; //!< graphic icon
    };

    /*! @struct Variable
     * @brief Data for a variable blend
     * 
     * @details We have to combine the picks/edges/contours
     * and the parameters. Think about a box where 3 adjacent
     * edges are picked and we share a vertex on all 3 edges.
     */
    struct Variable
    {
      Picks picks; //!< pick object.
      std::vector<VariableEntry> entries;
    };
      
    std::shared_ptr<prm::Parameter> buildRadiusParameter();
    std::shared_ptr<prm::Parameter> buildPositionParameter();
    std::vector<boost::uuids::uuid> getSpineEnds(const ann::SeerShape&, const boost::uuids::uuid&);
    
    class Feature : public Base
    {
      public:
        
        Feature();
        ~Feature() override;
        
        void setBlendType(BlendType); //!< will call clearBlends to reset data.
        BlendType getBlendType() const {return blendType;}
        
        void setShape(Shape sIn);
        Shape getShape() const {return filletShape;}
        
        void clearBlends(); //remove all blend definitions
        
        void addConstantBlend(const Constant&); //assert on blend type.
        void removeConstantBlend(int); //assert on blend type.
        void setConstantPicks(int, const Picks&); //assert on blend type.
        const std::vector<Constant>& getConstantBlends() const; //assert on blend type.
        
        void addOrCreateVariableBlend(const Pick&); //assert on blend type.
        void addVariableEntry(const VariableEntry&);
        const std::optional<Variable>& getVariableBlend() const; //!< notice optional return.
        
        void updateModel(const UpdatePayload&) override;
        Type getType() const override {return Type::Blend;}
        const std::string& getTypeString() const override {return toString(Type::Blend);}
        const QIcon& getIcon() const override {return icon;}
        Descriptor getDescriptor() const override {return Descriptor::Alter;}
        void serialWrite(const boost::filesystem::path&) override; //!< write xml file. not const, might reset a modified flag.
        void serialRead(const prj::srl::blns::Blend&); //!<initializes this from serial. not virtual, type already known.
      
      private:
        Shape filletShape = Shape::Rational;
        BlendType blendType = BlendType::Constant;
        
        std::vector<Constant> constantBlends;
        std::optional<Variable> variableBlend; 
        
        /*! used to map the edges that are blended away to the face generated.
        * used to map new generated face to outer wire.
        */ 
        std::map<boost::uuids::uuid, boost::uuids::uuid> shapeMap; //!< map edges or vertices to faces
        std::unique_ptr<ann::SeerShape> sShape;
        
        void match(ChFi3d_FilBuilder&, const ann::SeerShape &);
        
        /*! now that we are 'resolving' picks we need to update the shapemap to ensure
        * consistant id output of generated faces.
        */
        void updateShapeMap(const boost::uuids::uuid&, const ShapeHistory &);
        void ensureNoFaceNils();
        void dumpInfo(ChFi3d_FilBuilder&, const ann::SeerShape&);
        
        //needed for serial in.
        void addConstantBlendQuiet(const Constant&);
        void wireEntry(VariableEntry&);
        
        static QIcon icon;
    };
  }
}

#endif // FTR_BLEND_H
