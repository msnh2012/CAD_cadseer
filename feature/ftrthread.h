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


#ifndef FTR_THREAD_H
#define FTR_THREAD_H

#include "feature/ftrbase.h"

namespace prj{namespace srl{namespace thds{class Thread;}}}

namespace ftr
{
  namespace Thread
  {
    namespace Tags
    {
      inline constexpr std::string_view Internal = "Internal";
      inline constexpr std::string_view Fake = "Fake";
      inline constexpr std::string_view LeftHanded = "LeftHanded";
    }
    
    /**
    * @brief For creating screw threads.
    * 
    * Fake set true builds a simulated thread and
    * ignores the internal parameter setting. Fake set to false
    * allows Real threads to be internal or external.
    * Note a 'male' body is always built. for internal threads,
    * resulting body will need to be a tool for a subtraction.
    */
    class Feature : public Base
    {
    public:
      Feature();
      ~Feature() override;
      
      void updateModel(const UpdatePayload&) override;
      Type getType() const override {return Type::Thread;}
      const std::string& getTypeString() const override {return toString(Type::Thread);}
      const QIcon& getIcon() const override {return icon;}
      Descriptor getDescriptor() const override {return Descriptor::Create;}
      
      void serialWrite(const boost::filesystem::path&) override;
      void serialRead(const prj::srl::thds::Thread&);
    private:
      static QIcon icon;
      struct Stow;
      std::unique_ptr<Stow> stow;
    };
  }
}

#endif // FTR_THREAD_H
