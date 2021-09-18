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

#ifndef FTR_STRIP_H
#define FTR_STRIP_H

#include "feature/ftrbase.h"

namespace prj{namespace srl{namespace stps{class Strip;}}}
namespace ftr
{
  namespace Strip
  {
    //try to only add or break file format. Update anonymous map in source file.
    enum Station
    {
      AerialCam
      , Blank
      , Cam
      , Coin
      , Cutoff
      , Draw
      , Flange
      , Form
      , Idle
      , Pierce
      , Restrike
      , Tip
      , Trim
    };
    using Stations = std::vector<Station>;
    QString getStationString(Station);
    QStringList getAllStationStrings();
    Stations getAllStations();
    
    namespace PrmTags
    {
      inline constexpr std::string_view part = "part";
      inline constexpr std::string_view blank = "blank";
      inline constexpr std::string_view nest = "nest";
      inline constexpr std::string_view feedDirection = "feedDirection";
      inline constexpr std::string_view pitch = "pitch";
      inline constexpr std::string_view width = "width";
      inline constexpr std::string_view widthOffset = "widthOffset";
      inline constexpr std::string_view gap = "gap";
      inline constexpr std::string_view autoCalc = "autoCalc";
      inline constexpr std::string_view stripHeight = "stripHeight";
    }
    class Feature : public Base
    {
    public:
      Feature();
      ~Feature() override;
      
      void updateModel(const UpdatePayload&) override;
      Type getType() const override {return Type::Strip;}
      const std::string& getTypeString() const override {return toString(Type::Strip);}
      const QIcon& getIcon() const override {return icon;}
      Descriptor getDescriptor() const override {return Descriptor::Create;}
      void serialWrite(const boost::filesystem::path&) override;
      void serialRead(const prj::srl::stps::Strip&);
      
      const Stations& getStations() const;
      void setStations(const Stations&);
    private:
      static QIcon icon;
      struct Stow;
      std::unique_ptr<Stow> stow;
    };
  }
}

#endif // FTR_STRIP_H
