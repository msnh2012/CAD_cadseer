/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2020 Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef FTR_FILL_H
#define FTR_FILL_H

#include <osg/ref_ptr>

#include "feature/ftrpick.h"
#include "feature/ftrbase.h"

namespace ann{class SeerShape;}
namespace prj{namespace srl{namespace fls{class Fill;}}}
namespace lbr{class PLabel;}

namespace ftr
{
  namespace Fill
  {
    constexpr static const char *edgeTag = "edge";
    constexpr static const char *faceTag = "face";
    
    struct Entry
    {
      Entry();
      ~Entry();
      void createLabel(); //create label from parameter
      std::optional<Pick> facePick;
      std::optional<Pick> edgePick;
      bool boundary = true;
      
      std::shared_ptr<prm::Parameter> continuity; //!< parameter containing blend radius.
      osg::ref_ptr<lbr::PLabel> continuityLabel; //!< graphic icon
    };
    
    std::shared_ptr<prm::Parameter> buildContinuityParameter();
    
    class Feature : public Base
    {
    public:
      
      Feature();
      ~Feature() override;
      
      void updateModel(const UpdatePayload&) override;
      Type getType() const override {return Type::Fill;}
      const std::string& getTypeString() const override {return toString(Type::Fill);}
      const QIcon& getIcon() const override {return icon;}
      Descriptor getDescriptor() const override {return Descriptor::Create;}
      
      void serialWrite(const boost::filesystem::path&) override;
      void serialRead(const prj::srl::fls::Fill&);
      
      void clearEntries();
      void addEntry(const Entry&);
      const std::vector<Entry>& getEntries() const {return entries;}
      
    private:
      std::unique_ptr<ann::SeerShape> sShape;
      std::vector<Entry> entries;
      
      static QIcon icon;
    };
  }
}

#endif //FTR_FILL_H
