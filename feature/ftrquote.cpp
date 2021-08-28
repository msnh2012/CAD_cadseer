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

#include <boost/filesystem.hpp>

#include <osg/Switch>

#include "subprojects/libzippp/src/libzippp.h"

#include "globalutilities.h"
#include "tools/occtools.h"
#include "application/appapplication.h"
#include "project/prjproject.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "tools/idtools.h"
#include "libreoffice/lboodshack.h"
#include "feature/ftrstrip.h"
#include "feature/ftrdieset.h"
#include "annex/annseershape.h"
#include "project/serial/generated/prjsrlqtsquote.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinputtype.h"
#include "parameter/prmconstants.h"
#include "parameter/prmparameter.h"
#include "tools/featuretools.h"
#include "feature/ftrquote.h"

using namespace ftr::Quote;
using boost::filesystem::path;
QIcon Feature::icon = QIcon(":/resources/images/constructionQuote.svg");

namespace
{
  path getTemplatePath()
  {
    return path(prf::manager().rootPtr->features().quote().get().templateSheet());
  }
  path getOutputPath()
  {
    return path(app::instance()->getProject()->getSaveDirectory()) /= "Quote.ods";
  }
}

struct Feature::Stow
{
  Feature &feature;
  prm::Parameter stripPick{QObject::tr("Strip"), ftr::Picks(), PrmTags::stripPick};
  prm::Parameter diesetPick{QObject::tr("Die Set"), ftr::Picks(), PrmTags::diesetPick};
  prm::Parameter tFile{QObject::tr("Template File"), getTemplatePath(), prm::PathType::Read, PrmTags::tFile};
  prm::Parameter oFile{QObject::tr("Output File"), getOutputPath(), prm::PathType::Write, PrmTags::oFile};
  prm::Parameter pFile{QObject::tr("Picture File"), path(), prm::PathType::Read, PrmTags::pFile};
  prm::Parameter quoteNumber{QObject::tr("Quote Number"), 1, PrmTags::quoteNumber};
  prm::Parameter customerName{QObject::tr("Customer Name"), std::string("customer"), PrmTags::customerName};
  prm::Parameter customerId{QObject::tr("Customer Id"), 1, PrmTags::customerId};
  prm::Parameter partName{QObject::tr("Part Name"), std::string("part"), PrmTags::partName};
  prm::Parameter partNumber{QObject::tr("Part Number"), std::string("number"), PrmTags::partNumber};
  prm::Parameter partSetup{QObject::tr("Part Setup"), 0, PrmTags::partSetup};
  prm::Parameter partRevision{QObject::tr("Part Revision"), std::string("revision"), PrmTags::partRevision};
  prm::Parameter materialType{QObject::tr("Material Type"), std::string("material"), PrmTags::materialType};
  prm::Parameter materialThickness{QObject::tr("Material Thickness"), 1.0, PrmTags::materialThickness};
  prm::Parameter processType{QObject::tr("Process"), 0, PrmTags::processType};
  prm::Parameter annualVolume{QObject::tr("Volume"), 1, PrmTags::annualVolume};
  
  osg::ref_ptr<lbr::PLabel> tFileLabel{new lbr::PLabel(&tFile)};
  osg::ref_ptr<lbr::PLabel> oFileLabel{new lbr::PLabel(&oFile)};
  osg::ref_ptr<lbr::PLabel> pFileLabel{new lbr::PLabel(&pFile)};
  
  Stow() = delete;
  Stow(Feature &fIn)
  : feature(fIn)
  {
    auto processPrm = [&](prm::Parameter &prmIn)
    {
      prmIn.connectValue(std::bind(&Feature::setModelDirty, &feature));
      feature.parameters.push_back(&prmIn);
    };
    
    processPrm(stripPick);
    processPrm(diesetPick);
    processPrm(tFile);
    processPrm(oFile);
    processPrm(pFile);
    processPrm(quoteNumber);
    processPrm(customerName);
    processPrm(customerId);
    processPrm(partName);
    processPrm(partNumber);
    processPrm(partSetup);
    processPrm(partRevision);
    processPrm(materialType);
    processPrm(materialThickness);
    processPrm(processType);
    processPrm(annualVolume);
    
    QStringList pSetup =
    {
      QObject::tr("One Out")
      , QObject::tr("Two Out")
      , QObject::tr("Symmetrical Opposite")
      , QObject::tr("Other")
    };
    partSetup.setEnumeration(pSetup);
    QStringList pType =
    {
      QObject::tr("Prog")
      , QObject::tr("Prog Partial")
      , QObject::tr("Mechanical Transfer")
      , QObject::tr("Hand Transfer")
      , QObject::tr("Other")
    };
    processType.setEnumeration(pType);
    
    feature.overlaySwitch->addChild(tFileLabel.get());
    feature.overlaySwitch->addChild(oFileLabel.get());
    feature.overlaySwitch->addChild(pFileLabel.get());
  }
};

Feature::Feature()
: Base()
, stow(std::make_unique<Stow>(*this))
{
  name = QObject::tr("Quote");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
}

Feature::~Feature() = default;

void Feature::updateModel(const UpdatePayload &payloadIn)
{
  setFailure();
  lastUpdateLog.clear();
  try
  {
    tls::Resolver resolver(payloadIn);
    
    const auto &sPicks = stow->stripPick.getPicks();
    if (sPicks.empty() || !resolver.resolve(sPicks.front()))
      throw std::runtime_error("Invalid strip input");
    const Strip *sf = dynamic_cast<const Strip*>(resolver.getFeature());
    if(!sf)
      throw std::runtime_error("can not cast to strip feature");
    
    const auto &dPicks = stow->diesetPick.getPicks();
    if (dPicks.empty() || !resolver.resolve(dPicks.front()))
      throw std::runtime_error("Invalid dieset input");
    const DieSet *dsf = dynamic_cast<const DieSet*>(resolver.getFeature());
    if (!dsf)
      throw std::runtime_error("can not cast to dieset feature");
    
    //place labels
    const ann::SeerShape &dss = dsf->getAnnex<ann::SeerShape>(); //part seer shape.
    if (dss.isNull())
      throw std::runtime_error("die set seer shape is null");
    const TopoDS_Shape &ds = dss.getRootOCCTShape(); //part shape.
    occt::BoundingBox sbbox(ds); //blank bounding box.
    osg::Vec3d tLoc = gu::toOsg(sbbox.getCorners().at(0));
    stow->tFileLabel->setMatrix(osg::Matrixd::translate(tLoc));
    osg::Vec3d pLoc = gu::toOsg(sbbox.getCorners().at(2));
    stow->pFileLabel->setMatrix(osg::Matrixd::translate(pLoc));
    osg::Vec3d oLoc = gu::toOsg(sbbox.getCorners().at(3));
    stow->oFileLabel->setMatrix(osg::Matrixd::translate(oLoc));
    
    if (!boost::filesystem::exists(stow->tFile.getPath()))
      throw std::runtime_error("template file doesn't exist");
    /* we change the entries in a specific sheet and these values are linked into
    * a customer designed sheet. The links are not updated when we open the sheet in calc by default.
    * calc menu: /tools/options/libreoffice calc/formula/Recalculation on file load
    * set that to always.
    */
    boost::filesystem::copy_file
    (
      stow->tFile.getPath(),
      stow->oFile.getPath(),
      boost::filesystem::copy_option::overwrite_if_exists
    );
    
    libzippp::ZipArchive zip(stow->oFile.getPath().string());
    zip.open(libzippp::ZipArchive::WRITE);
    for(const auto &e : zip.getEntries())
    {
      if (e.getName() == "content.xml")
      {
        std::string contents = e.readAsText();
        boost::filesystem::path temp(boost::filesystem::temp_directory_path());
        temp /= "content.xml";
        
        lbo::Map map = 
        {
          (std::make_pair(std::make_pair(0, 0), std::to_string(stow->quoteNumber.getInt()))),
          (std::make_pair(std::make_pair(1, 0), stow->customerName.getString())),
          (std::make_pair(std::make_pair(2, 0), std::to_string(stow->customerId.getInt()))),
          (std::make_pair(std::make_pair(3, 0), stow->partName.getString())),
          (std::make_pair(std::make_pair(4, 0), stow->partNumber.getString())),
          (std::make_pair(std::make_pair(5, 0), stow->partSetup.getEnumerationString().toStdString())),
          (std::make_pair(std::make_pair(6, 0), stow->partRevision.getString())),
          (std::make_pair(std::make_pair(7, 0), stow->materialType.getString())),
          (std::make_pair(std::make_pair(8, 0), std::to_string(stow->materialThickness.getDouble()))),
          (std::make_pair(std::make_pair(9, 0), stow->processType.getEnumerationString().toStdString())),
          (std::make_pair(std::make_pair(10, 0), std::to_string(stow->annualVolume.getInt()))),
          (std::make_pair(std::make_pair(11, 0), std::to_string(sf->stations.size()))),
          (std::make_pair(std::make_pair(12, 0), std::to_string(sf->getPitch()))),
          (std::make_pair(std::make_pair(13, 0), std::to_string(sf->getWidth()))),
          (std::make_pair(std::make_pair(14, 0), std::to_string(sf->getPitch() * sf->getWidth()))),
          (std::make_pair(std::make_pair(15, 0), std::to_string(dsf->getLength()))),
          (std::make_pair(std::make_pair(16, 0), std::to_string(dsf->getWidth()))),
          (std::make_pair(std::make_pair(17, 0), std::to_string(sf->getHeight() + 25.0))),
          (std::make_pair(std::make_pair(18, 0), std::to_string(sf->getHeight() + 25.0)))
        };
        int index = 19;
        for (const auto &s : sf->stations)
        {
          map.insert(std::make_pair(std::make_pair(index, 0), s.toStdString()));
          index++;
        }
        lbo::replace(contents, map);
        
        std::fstream stream;
        stream.open(temp.string(), std::ios::out);
        stream << contents;
        stream.close();
        
        zip.addFile(e.getName(), temp.string());
        break; //might invalidate, so get out.
      }
    }
    
    //doing a separate loop to ensure that we don't invalidate with addFile.
    for(const auto &e : zip.getEntries())
    {
  //     std::cout << e.getName() << std::endl;
      
      boost::filesystem::path cPath = e.getName();
      
  //     std::cout << "parent path: " << cPath.parent_path()
  //     << "      extension: " << cPath.extension() << std::endl;
      
      if (cPath.parent_path() == "Pictures" && cPath.extension() == ".png")
      {
        if (boost::filesystem::exists(stow->pFile.getPath()))
          zip.addFile(e.getName(), stow->pFile.getPath().string());
      }
    }
    
    zip.close();
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in Quote update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in Quote update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in Quote update. " << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}


/*
 < xs:element name="base" type="spt:Base"/>        *
 <xs:element name="stripPick" type="spt:Parameter"/>
 <xs:element name="diesetPick" type="spt:Parameter"/>
 <xs:element name="tFile" type="spt:Parameter"/>
 <xs:element name="oFile" type="spt:Parameter"/>
 <xs:element name="pFile" type="spt:Parameter"/>
 <xs:element name="quoteNumber" type="spt:Parameter"/>
 <xs:element name="customerName" type="spt:Parameter"/>
 <xs:element name="customerId" type="spt:Parameter"/>
 <xs:element name="partName" type="spt:Parameter"/>
 <xs:element name="partNumber" type="spt:Parameter"/>
 <xs:element name="partSetup" type="spt:Parameter"/>
 <xs:element name="partRevision" type="spt:Parameter"/>
 <xs:element name="materialType" type="spt:Parameter"/>
 <xs:element name="materialThickness" type="spt:Parameter"/>
 <xs:element name="processType" type="spt:Parameter"/>
 <xs:element name="annualVolume" type="spt:Parameter"/>
 <xs:element name="tFileLabel" type="spt:PLabel"/>
 <xs:element name="oFileLabel" type="spt:PLabel"/>
 <xs:element name="pFileLabel" type="spt:PLabel"/>
*/


void Feature::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::qts::Quote qo
  (
    Base::serialOut()
    , stow->stripPick.serialOut()
    , stow->diesetPick.serialOut()
    , stow->tFile.serialOut()
    , stow->oFile.serialOut()
    , stow->pFile.serialOut()
    , stow->quoteNumber.serialOut()
    , stow->customerName.serialOut()
    , stow->customerId.serialOut()
    , stow->partName.serialOut()
    , stow->partNumber.serialOut()
    , stow->partSetup.serialOut()
    , stow->partRevision.serialOut()
    , stow->materialType.serialOut()
    , stow->materialThickness.serialOut()
    , stow->processType.serialOut()
    , stow->annualVolume.serialOut()
    , stow->tFileLabel->serialOut()
    , stow->oFileLabel->serialOut()
    , stow->pFileLabel->serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::qts::quote(stream, qo, infoMap);
}

void Feature::serialRead(const prj::srl::qts::Quote &qIn)
{
  Base::serialIn(qIn.base());
  stow->stripPick.serialIn(qIn.stripPick());
  stow->diesetPick.serialIn(qIn.diesetPick());
  stow->tFile.serialIn(qIn.tFile());
  stow->oFile.serialIn(qIn.oFile());
  stow->pFile.serialIn(qIn.pFile());
  stow->quoteNumber.serialIn(qIn.quoteNumber());
  stow->customerName.serialIn(qIn.customerName());
  stow->customerId.serialIn(qIn.customerId());
  stow->partName.serialIn(qIn.partName());
  stow->partNumber.serialIn(qIn.partNumber());
  stow->partSetup.serialIn(qIn.partSetup());
  stow->partRevision.serialIn(qIn.partRevision());
  stow->materialType.serialIn(qIn.materialType());
  stow->materialThickness.serialIn(qIn.materialThickness());
  stow->processType.serialIn(qIn.processType());
  stow->annualVolume.serialIn(qIn.annualVolume());
  stow->tFileLabel->serialIn(qIn.tFileLabel());
  stow->oFileLabel->serialIn(qIn.oFileLabel());
  stow->pFileLabel->serialIn(qIn.pFileLabel());
}
