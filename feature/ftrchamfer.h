/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef FTR_CHAMFER_H
#define FTR_CHAMFER_H

#include <map>

#include "library/lbrplabel.h"
#include "feature/ftrpick.h"
#include "feature/ftrbase.h"

class BRepFilletAPI_MakeChamfer;
namespace prj{namespace srl{namespace chms{class Chamfer; class Entry;}}}
namespace ann{class SeerShape;}

namespace ftr
{
  class Chamfer : public Base
  {
  public:
    constexpr static const char *edge = "edge";
    constexpr static const char *face = "face";
    enum class Style
    {
      Symmetric = 0
      , TwoDistances
      , DistanceAngle
    };
    
    enum class Mode
    {
      Classic = 0
      , Throat
      , ThroatPenetration
    };
    
    struct Cue
    {
      /* symmetric uses only 1 parameter and 1 label and edgePicks.
       * TwoDistances and DistanceAngle: edgePicks.size() == facePicks.size()
       */
      struct Entry
      {
        Style style = Style::Symmetric;
        std::shared_ptr<prm::Parameter> parameter1;
        std::shared_ptr<prm::Parameter> parameter2;
        osg::ref_ptr<lbr::PLabel> label1;
        osg::ref_ptr<lbr::PLabel> label2;
        Picks edgePicks;
        Picks facePicks;
        
        Entry() = default;
        Entry(const Entry&) = default;
        Entry(const Entry&, bool); //makes new parameters with same ids.
        Entry(const prj::srl::chms::Entry&);
        
        prj::srl::chms::Entry serialOut() const;
        void serialIn(const prj::srl::chms::Entry&);
        
        static Entry buildDefaultSymmetric();
        static Entry buildDefaultTwoDistances();
        static Entry buildDefaultDistanceAngle();
      };
      
      /* Mode::Classic can have: Style::Symmetric, Style::TwoDistances, Style::DistanceAngle
       * Mode::Throat can only have: Style::Symmetric
       * Mode::ThroatPenetration can only have: Style::TwoDistances 
       */
      Mode mode = Mode::Classic;
      std::vector<Entry> entries;
    };
    
    Chamfer();
    virtual ~Chamfer() override;
    virtual void updateModel(const UpdatePayload&) override;
    virtual Type getType() const override {return Type::Chamfer;}
    virtual const std::string& getTypeString() const override {return toString(Type::Chamfer);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Alter;}
    virtual void serialWrite(const boost::filesystem::path&) override;
    void serialRead(const prj::srl::chms::Chamfer&);
    
    void setCue(const Cue&, bool = true); //boolean to set model dirty
    const Cue& getCue(){return cue;};
  private:
    void generatedMatch(BRepFilletAPI_MakeChamfer&, const ann::SeerShape &);
    
    /*! now that we are 'resolving' picks we need to update the shapemap to ensure
     * consistent id output of generated faces. duplicate function in blend.
     */
    void updateShapeMap(const boost::uuids::uuid&, const ShapeHistory &);
    Cue cue;
    std::map<boost::uuids::uuid, boost::uuids::uuid> shapeMap; //!< map edges or vertices to faces
    std::unique_ptr<ann::SeerShape> sShape;
    
    static QIcon icon;
  };
}

#endif // CHAMFER_H
