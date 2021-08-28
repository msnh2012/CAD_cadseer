/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef FTR_QUOTE_H
#define FTR_QUOTE_H

#include "feature/ftrbase.h"

namespace prj{namespace srl{namespace qts{class Quote;}}}

namespace ftr
{
  namespace Quote
  {
    namespace InputTags
    {
      inline constexpr std::string_view stripPick = "stripPick";
      inline constexpr std::string_view diesetPick = "diesetPick";
    }
    namespace PrmTags
    {
      inline constexpr std::string_view stripPick = "stripPick";
      inline constexpr std::string_view diesetPick = "diesetPick";
      inline constexpr std::string_view tFile = "tFile";
      inline constexpr std::string_view oFile = "oFile";
      inline constexpr std::string_view pFile = "pFile";
      
      inline constexpr std::string_view quoteNumber = "quoteNumber";
      inline constexpr std::string_view customerName = "customerName";
      inline constexpr std::string_view customerId = "customerId";
      inline constexpr std::string_view partName = "partName";
      inline constexpr std::string_view partNumber = "partNumber";
      inline constexpr std::string_view partSetup = "partSetup";
      inline constexpr std::string_view partRevision = "partRevision";
      inline constexpr std::string_view materialType = "materialType";
      inline constexpr std::string_view materialThickness = "materialThickness";
      inline constexpr std::string_view processType = "processType";
      inline constexpr std::string_view annualVolume = "annualVolume";
    }
    class Feature : public Base
    {
    public:
      Feature();
      ~Feature() override;
      
      void updateModel(const UpdatePayload&) override;
      Type getType() const override {return Type::Quote;}
      const std::string& getTypeString() const override {return toString(Type::Quote);}
      const QIcon& getIcon() const override {return icon;}
      Descriptor getDescriptor() const override {return Descriptor::Create;}
      void serialWrite(const boost::filesystem::path&) override;
      void serialRead(const prj::srl::qts::Quote&);
      
    private:
      static QIcon icon;
      struct Stow;
      std::unique_ptr<Stow> stow;
    };
  }
}

#endif // FTR_QUOTE_H
