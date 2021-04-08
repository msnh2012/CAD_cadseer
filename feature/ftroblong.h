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

#ifndef FTR_OBLONG_H
#define FTR_OBLONG_H

#include <osg/ref_ptr>

#include "feature/ftrbase.h"

namespace lbr{class IPGroup;}
namespace prj{namespace srl{namespace obls{class Oblong;}}}
namespace ann{class CSysDragger; class SeerShape;}
namespace prm{struct Observer;}

namespace ftr
{
  class OblongBuilder;
  
  class Oblong : public Base
  {
  public:
    Oblong();
    virtual ~Oblong() override;
    
    void setLength(double);
    const std::shared_ptr<prm::Parameter>& getLength() const {return length;}
    void setWidth(double);
    const std::shared_ptr<prm::Parameter>& getWidth() const {return width;}
    void setHeight(double);
    const std::shared_ptr<prm::Parameter>& getHeight() const {return height;}
    void setCSys(const osg::Matrixd&);
    osg::Matrixd getCSys() const;
    
    virtual void updateModel(const UpdatePayload&) override;
    virtual Type getType() const override {return Type::Oblong;}
    virtual const std::string& getTypeString() const override {return toString(Type::Oblong);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
    virtual void serialWrite(const boost::filesystem::path&) override; //!< write xml file. not const, might reset a modified flag.
    void serialRead(const prj::srl::obls::Oblong &sOblong);
  protected:
    std::shared_ptr<prm::Parameter> length;
    std::shared_ptr<prm::Parameter> width;
    std::shared_ptr<prm::Parameter> height;
    std::unique_ptr<prm::Parameter> csys;
    
    std::unique_ptr<prm::Observer> prmObserver;
  
    std::unique_ptr<ann::CSysDragger> csysDragger;
    std::unique_ptr<ann::SeerShape> sShape;
    
    osg::ref_ptr<lbr::IPGroup> lengthIP;
    osg::ref_ptr<lbr::IPGroup> widthIP;
    osg::ref_ptr<lbr::IPGroup> heightIP;
    
    void initializeMaps();
    void updateResult(const OblongBuilder&);
    void setupIPGroup();
    void updateIPGroup();
    
  private:
    static QIcon icon;
  };
}

#endif // FTR_OBLONG_H
