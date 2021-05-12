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

#ifndef FTR_INERT_H
#define FTR_INERT_H

#include "feature/ftrbase.h"

namespace prj{namespace srl{namespace ints{class Inert;}}}

namespace ftr
{
  namespace Inert
  {
    class Feature : public Base
    {
    public:
      Feature() = delete;
      Feature(const TopoDS_Shape&);
      Feature(const TopoDS_Shape&, const osg::Matrixd&);
      ~Feature() override;
      void updateModel(const UpdatePayload&) override;
      Type getType() const override {return Type::Inert;}
      const std::string& getTypeString() const override {return toString(Type::Inert);}
      const QIcon& getIcon() const override {return icon;}
      Descriptor getDescriptor() const override {return Descriptor::Create;}
      void serialWrite(const boost::filesystem::path&) override;
      void serialRead(const prj::srl::ints::Inert &sBox);
    private:
      static QIcon icon;
      struct Stow;
      std::unique_ptr<Stow> stow;
    };
  }
}

#endif // FTR_INERT_H
